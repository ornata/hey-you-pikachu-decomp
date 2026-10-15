"""Bounded cross-subsystem claims grounded in original MIPS instructions.

Inputs are synthetic. Passing does not prove ownership, gameplay reachability,
resource lifetime, hardware fidelity, or R4300 equivalence.
"""

from pathlib import Path
import struct

from tools.semantic_review.cases import RAM, capture_case, word
from tools.semantic_review.replay import replay


def experiments():
    """Declare expected observations before executing the retail instructions."""
    cases = []

    def add(name, symbol, args, patches, observations, result=None, globals=()):
        cases.append(
            dict(
                id=name,
                symbol=symbol,
                args=args,
                patches=patches,
                observations=observations,
                expected_return=result,
                global_patches=globals,
            )
        )

    # Ghidra 80008DE8: lhu/sh +36; child +2c; sibling +28; mask a1 to16bits.
    for symbol, mask, initial, wanted in [
        ("objectSetHierarchyFlags", 0x12348001, 0x2200, 0xA201),
        ("objectClearHierarchyFlags", 0x12348001, 0xFFFF, 0x7FFE),
    ]:
        patches = []
        observations = []
        for index in range(3):
            data = bytearray(b"\xa5" * 0x60)
            data[0x28:0x30] = word(RAM + 0x100 if index == 0 else 0) + word(
                RAM + 0x200 if index == 0 else 0
            )
            data[0x36:0x38] = struct.pack(">H", initial)
            expected = bytearray(data)
            expected[0x36:0x38] = struct.pack(">H", wanted)
            patches.append((RAM + index * 0x100, bytes(data)))
            observations.append((RAM + index * 0x100, bytes(expected)))
        add(symbol + ".tree-width", symbol, [RAM, mask], patches, observations)
        add(symbol + ".null", symbol, [0, mask], [], [])
    # ROM801144EC: signed lb state+6, shared4/5 rejection, separate holder masks.
    for state, flags, selector, wanted in [
        (4, 0, 0, 0),
        (5, 0, 1, 0),
        (2, 0, 1, 0),
        (3, 0x40000000, 7, 1),
        (1, 0, 0, 0),
        (1, 0x40000000, 0, 1),
        (0, 0x80, 0, 0),
        (0, 0x40, 1, 0),
        (0, 0x04000000, 1, 0),
        (0, 0x80, 1, 1),
        (0, 0x40, 0, 1),
        (0xFF, 0, 1, 1),
    ]:
        obj = bytearray(b"\xa5" * 0x60)
        obj[0x54:0x58] = word(RAM + 0x100)
        data = bytearray(b"\xa5" * 0x20)
        data[:4] = word(flags)
        data[6] = state
        patches = [(RAM, bytes(obj)), (RAM + 0x100, bytes(data))]
        add(
            f"item.pickup.{state}.{flags:x}.{selector}",
            "itemCanBePickedUp",
            [RAM, selector],
            patches,
            patches,
            wanted,
        )
    # ROM801145A0 requires both bit8 and sign bit, not either alone.
    for flags, wanted in [
        (0, 0),
        (0x100, 0),
        (0x80000000, 0),
        (0x80000100, 1),
        (0xFFFFFFFF, 1),
    ]:
        obj = bytearray(0x60)
        obj[0x54:0x58] = word(RAM + 0x100)
        patches = [(RAM, bytes(obj)), (RAM + 0x100, word(flags))]
        add(
            f"item.cooperation.{flags:x}",
            "itemAllowsCooperation",
            [RAM],
            patches,
            patches,
            wanted,
        )
    # Ghidra8001FA6C: lw at fixed global, no normalization or writes.
    for value in [0, 1, 0xFFFFFFFF, 0x80000000]:
        add(
            f"audio.output.{value:x}",
            "audioGetOutputMode",
            [],
            [],
            [(0x80088EE8, word(value))],
            value,
            [(0x80088EE8, word(value))],
        )
    # ROM80117B88: current config+70 pointer, index*8, signed word count.
    for index, count in [(0, 7), (1, 0), (2, 0xFFFFFFFF)]:
        config = bytearray(0x80)
        config[0x70:0x74] = word(RAM + 0x100)
        table = b"".join(word(v) + word(0xCAFEBABE) for v in [7, 0, 0xFFFFFFFF])
        patches = [(RAM, bytes(config)), (RAM + 0x100, table)]
        add(
            f"scene.path-count.{index}",
            "areaGetPathPointCount",
            [index],
            patches,
            patches,
            count,
            [(0x80133DE4, word(RAM))],
        )
    # Ghidra8003A058 copies twelve words from retail constant; guards constrain extent.
    identity = b"".join(
        word(v) for v in [0x3F800000, 0, 0, 0, 0x3F800000, 0, 0, 0, 0x3F800000, 0, 0, 0]
    )
    add(
        "render.identity.write-extent",
        "matrix34Identity",
        [RAM + 4],
        [(RAM, b"\xa5" * 56)],
        [(RAM, b"\xa5" * 4 + identity + b"\xa5" * 4)],
    )
    return cases


def capture(case, elf, rom):
    """Add explicitly synthetic global state without changing executable bytes."""
    result = capture_case(case, elf, rom)
    for address, data in case["global_patches"]:
        for region in result["mappings"]:
            start = region["address"]
            if start <= address and address + len(data) <= start + region["size"]:
                raw = bytearray.fromhex(region["data_hex"])
                if len(raw) < address - start + len(data):
                    raw.extend(b"\0" * (address - start + len(data) - len(raw)))
                raw[address - start : address - start + len(data)] = data
                region["data_hex"] = raw.hex()
                # Exclude synthetic data from executable authenticated ranges.
                import hashlib

                original = Path(rom).read_bytes()
                segments = []
                for segment in result["authenticated_code"]:
                    lo = segment["address"]
                    hi = lo + segment["size"]
                    ranges = (
                        [(lo, address), (address + len(data), hi)]
                        if lo <= address < hi
                        else [(lo, hi)]
                    )
                    for begin, end in ranges:
                        if end <= begin:
                            continue
                        offset = segment["rom_offset"] + begin - lo
                        segments.append(
                            dict(
                                overlay=segment["overlay"],
                                address=begin,
                                size=end - begin,
                                rom_offset=offset,
                                sha256=hashlib.sha256(
                                    original[offset : offset + end - begin]
                                ).hexdigest(),
                            )
                        )
                result["authenticated_code"] = segments
                break
        else:
            start = address & ~4095
            raw = bytearray(4096)
            raw[address - start : address - start + len(data)] = data
            result["mappings"].append(
                dict(address=start, size=4096, permissions="rw", data_hex=raw.hex())
            )
    return result


def run_cases(rom_path, elf_path):
    """Return observed evidence with emulator-only limitations kept explicit."""
    results = []
    for case in experiments():
        observed = replay(capture(case, elf_path, rom_path), Path(rom_path))
        status = "unsupported"
        reason = observed["reason"]
        if observed["status"] == "completed":
            memory = {
                x["address"]: bytes.fromhex(x["data_hex"])
                for x in observed.get("memory", [])
            }
            correct = all(memory.get(a) == b for a, b in case["observations"])
            if case["expected_return"] is not None:
                correct &= (
                    observed["registers"]["gpr_2"] & 0xFFFFFFFF
                    == case["expected_return"] & 0xFFFFFFFF
                )
            status = "supported" if correct else "contradicted"
            reason = (
                "Predeclared observations agree."
                if correct
                else "Predeclared observations differ."
            )
        results.append(
            dict(
                id=case["id"],
                subsystem=case["id"].split(".")[0],
                claim=case["symbol"] + " bounded return/write footprint",
                evidence="Predeclared from original MIPS disassembly; ROM-authenticated execution.",
                status=status,
                reason=reason,
                instructions=observed["instructions"],
                observations=[
                    dict(address=a, expected_hex=b.hex())
                    for a, b in case["observations"]
                ],
                limitations=observed["limitations"]
                + ["Synthetic state; not a gameplay or ownership-lifetime proof."],
            )
        )
    return results
