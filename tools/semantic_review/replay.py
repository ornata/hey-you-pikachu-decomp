"""Replay explicitly bounded, ROM-authenticated MIPS CPU slices.

This backend is not an R4300 emulator. Device accesses, interrupts and external
calls are unsupported. Full-system cross-validation is required for claims.
"""

from __future__ import annotations

import hashlib
from pathlib import Path


def replay(capture: dict, rom: Path | None = None, *, synthetic: bool = False) -> dict:
    """Execute a capture and return its completion classification and observed writes."""
    import unicorn as u
    from unicorn import mips_const as m

    result = {
        "status": "unsupported",
        "reason": "",
        "instructions": 0,
        "writes": [],
        "registers": {},
        "memory": [],
        "backend": f"unicorn-{u.__version__}",
        "limitations": [
            "No interrupts, MMIO, or full-system timing",
            "R4000 model is not R4300",
        ],
    }
    cpu = None
    registers = {}
    try:
        if not isinstance(capture, dict):
            raise ValueError("capture must be an object")
        if capture.get("schema_version") != 1 or capture.get("byte_order") != "big":
            raise ValueError("requires schema version 1 and big-endian bytes")
        if not synthetic:
            if rom is None:
                raise ValueError("original ROM required for byte authentication")
            original = Path(rom).read_bytes()
            if hashlib.sha256(original).hexdigest() != capture.get("rom_sha256"):
                raise ValueError("ROM hash mismatch")
            code = capture.get("authenticated_code", [])
            if not code:
                raise ValueError("missing executable ROM provenance")
            for segment in code:
                if not segment.get("overlay") or segment["overlay"].lower() in (
                    "unknown",
                    "unresolved",
                ):
                    raise ValueError("executable overlay identity required")
                raw = original[
                    segment["rom_offset"] : segment["rom_offset"] + segment["size"]
                ]
                if (
                    len(raw) != segment["size"]
                    or hashlib.sha256(raw).hexdigest() != segment["sha256"]
                ):
                    raise ValueError("executable ROM hash mismatch")

        limit = capture.get("max_instructions", 10000)
        if not isinstance(limit, int) or not 0 < limit <= 1000000:
            raise ValueError("instruction budget must be in 1..1000000")
        cpu = u.Uc(u.UC_ARCH_MIPS, u.UC_MODE_MIPS64 | u.UC_MODE_BIG_ENDIAN)
        cpu.ctl_set_cpu_model(m.UC_CPU_MIPS64_R4000)
        cpu.ctl_set_tlb_mode(u.UC_TLB_VIRTUAL)
        physical_ranges = []
        translated_regions = []
        for region in capture["mappings"]:
            address, size = region["address"], region["size"]
            if address % 4096 or size <= 0 or size % 4096:
                raise ValueError("mappings must use aligned 4096-byte pages")
            physical = (
                address & 0x1FFFFFFF if 0x80000000 <= address < 0xC0000000 else address
            )
            if not (
                0 <= physical
                and physical + size <= 8 * 1024 * 1024
                or 0x04000000 <= physical
                and physical + size <= 0x04002000
            ):
                raise ValueError("MMIO or non-RDRAM mapping unsupported")
            if any(
                physical < hi and lo < physical + size for lo, hi in physical_ranges
            ):
                raise ValueError("overlapping physical/KSEG aliases unsupported")
            physical_ranges.append((physical, physical + size))
            permissions = region.get("permissions", "rw")
            if not permissions or any(char not in "rwx" for char in permissions):
                raise ValueError("invalid mapping permissions")
            flags = sum(
                flag
                for char, flag in [
                    ("r", u.UC_PROT_READ),
                    ("w", u.UC_PROT_WRITE),
                    ("x", u.UC_PROT_EXEC),
                ]
                if char in permissions
            )
            cpu.mem_map(address, size, flags)
            translated_regions.append((address, physical, size, flags))
            data = bytes.fromhex(region.get("data_hex", ""))
            if len(data) > size:
                raise ValueError("mapping data exceeds size")
            cpu.mem_write(address, data)
        if not synthetic:
            for segment in code:
                if (
                    bytes(cpu.mem_read(segment["address"], segment["size"]))
                    != original[
                        segment["rom_offset"] : segment["rom_offset"] + segment["size"]
                    ]
                ):
                    raise ValueError("captured executable differs from original ROM")
        registers = {f"gpr_{i}": getattr(m, f"UC_MIPS_REG_{i}") for i in range(32)}
        registers.update(hi=m.UC_MIPS_REG_HI, lo=m.UC_MIPS_REG_LO)
        for name, value in capture.get("registers", {}).items():
            if name not in registers:
                raise ValueError(f"unsupported register state: {name}")
            cpu.reg_write(registers[name], value)

        def canonical(address):
            if address >> 32 == 0xFFFFFFFF:
                return address & 0xFFFFFFFF
            return address

        def tlb_fill(uc, address, access, entry, _):
            # Virtual-TLB translation keeps cached, uncached and sign-extended
            # addresses on one backing store; it does not simulate the N64 TLB.
            virtual = canonical(address)
            physical = (
                virtual & 0x1FFFFFFF if 0x80000000 <= virtual < 0xC0000000 else virtual
            )
            for mapped, base, size, flags in translated_regions:
                if base <= physical < base + size:
                    entry.paddr = (mapped + physical - base) & ~4095
                    entry.perms = flags
                    return True
            return False

        cpu.hook_add(u.UC_HOOK_TLB_FILL, tlb_fill)
        stop = canonical(capture["stop_pc"])
        start = capture["start_pc"]
        allowed = [
            (s["address"], s["address"] + s["size"])
            for s in capture.get("authenticated_code", [])
        ]
        external = {entry["address"] for entry in capture.get("external_calls", [])}
        reached_stop = False

        def on_code(uc, address, size, _):
            nonlocal reached_stop
            address = canonical(address)
            result["last_pc"] = address
            if address == stop:
                reached_stop = True
                uc.emu_stop()
                return
            if address in external:
                result["reason"] = f"external call at {address:#x}"
                uc.emu_stop()
                return
            if not synthetic and not any(
                lo <= address and address + size <= hi for lo, hi in allowed
            ):
                result["reason"] = (
                    f"execution outside authenticated code at {address:#x}"
                )
                uc.emu_stop()
                return
            if result["instructions"] >= limit:
                result["reason"] = "instruction budget exhausted"
                uc.emu_stop()
                return
            instruction = int.from_bytes(uc.mem_read(address, 4), "big")
            opcode = instruction >> 26
            if opcode in (
                0x10,
                0x11,
                0x12,
                0x13,
                0x31,
                0x32,
                0x35,
                0x36,
                0x39,
                0x3A,
                0x3D,
                0x3E,
            ):
                result["reason"] = (
                    f"coprocessor instruction unsupported at {address:#x}"
                )
                uc.emu_stop()
                return
            result["instructions"] += 1

        def backing_address(address):
            virtual = canonical(address)
            physical = (
                virtual & 0x1FFFFFFF if 0x80000000 <= virtual < 0xC0000000 else virtual
            )
            for mapped, base, size, _ in translated_regions:
                if base <= physical < base + size:
                    return mapped + physical - base
            return virtual

        def on_write(uc, access, address, size, value, _):
            address = backing_address(address)
            result["writes"].append({"address": address, "size": size, "value": value})
            if not synthetic and any(
                address < hi and lo < address + size for lo, hi in allowed
            ):
                result["reason"] = "write to authenticated executable code unsupported"
                uc.emu_stop()

        cpu.hook_add(u.UC_HOOK_CODE, on_code)
        cpu.hook_add(u.UC_HOOK_MEM_WRITE, on_write)
        cpu.emu_start(start, 0, count=limit + 1)
        result["registers"] = {
            name: cpu.reg_read(reg) for name, reg in registers.items()
        }
        result["pc"] = cpu.reg_read(m.UC_MIPS_REG_PC)
        for region in capture.get("observations", capture.get("observe_memory", [])):
            result["memory"].append(
                {
                    "address": region["address"],
                    "data_hex": bytes(
                        cpu.mem_read(region["address"], region["size"])
                    ).hex(),
                }
            )
        if not result["reason"]:
            if reached_stop:
                result.update(status="completed", reason="reached explicit stop PC")
            else:
                result["reason"] = "instruction budget exhausted"
    except (ValueError, KeyError, TypeError, u.UcError) as error:
        result["reason"] = str(error)
        if cpu is not None:
            result["pc"] = cpu.reg_read(m.UC_MIPS_REG_PC)
            result["registers"] = {
                name: cpu.reg_read(reg) for name, reg in registers.items()
            }
    return result
