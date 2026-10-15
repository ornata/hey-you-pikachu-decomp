"""Instruction-level probes with independently stated expected register values.

These hand-encoded programs establish local backend behavior only. They must be
cross-checked against an N64 interpreter before supporting game-level claims.
"""

from pathlib import Path
from .replay import replay


def make_capture(words: list[int], registers: dict | None = None) -> dict:
    """Construct a KSEG0 CPU experiment usable by both execution engines."""
    return {
        "schema_version": 1,
        "byte_order": "big",
        "start_pc": 0x80001000,
        "stop_pc": 0x80001000 + len(words) * 4,
        "max_instructions": 100,
        "registers": registers or {},
        "mappings": [
            {
                "address": 0x80001000,
                "size": 4096,
                "permissions": "rwx",
                "data_hex": b"".join(w.to_bytes(4, "big") for w in words).hex(),
            }
        ],
    }


def probe(
    name: str, words: list[int], expected: dict, registers: dict | None = None
) -> dict:
    """Run one synthetic big-endian program against explicit expected results."""
    result = replay(make_capture(words, registers), synthetic=True)
    mismatches = {
        key: {"expected": value, "actual": result["registers"].get(key)}
        for key, value in expected.items()
        if result["registers"].get(key) != value
    }
    return {
        "name": name,
        "status": "supported"
        if result["status"] == "completed" and not mismatches
        else "unsupported",
        "mismatches": mismatches,
        "execution": result,
    }


def probe_cases():
    """Return hand-encoded programs and independently stated expected values."""
    return [
        ("signed-addiu", [0x2402FFFF], {"gpr_2": 0xFFFFFFFFFFFFFFFF}),
        ("64-bit-shift", [0x64020001, 0x0002103C], {"gpr_2": 1 << 32}),
        # taken branch runs its delay slot and skips one instruction
        (
            "branch-delay",
            [0x10000002, 0x24020007, 0x24020009, 0x00000000],
            {"gpr_2": 7},
        ),
        # false branch-likely annuls its delay slot
        (
            "branch-likely",
            [0x54000001, 0x24020007, 0x24030009],
            {"gpr_2": 0, "gpr_3": 9},
        ),
        (
            "multiply-hilo",
            [0x24020003, 0x24030007, 0x00430018, 0x00002012],
            {"gpr_4": 21, "hi": 0, "lo": 21},
        ),
        (
            "big-endian-load",
            [0x3C028000, 0x34421000, 0x80430000, 0x90440000, 0x8C450000],
            {"gpr_3": 0x3C, "gpr_4": 0x3C, "gpr_5": 0x3C028000},
        ),
    ]


def run_probes() -> dict:
    """Exercise delay slots, width, signedness, HI/LO and big-endian memory."""
    results = [probe(*case) for case in probe_cases()]
    return {
        "backend": "unicorn",
        "cross_validated": False,
        "status": "supported"
        if all(r["status"] == "supported" for r in results)
        else "unsupported",
        "limitations": [
            "Synthetic probes are not N64 interpreter cross-validation",
            "FPU, MMU, interrupts and devices excluded",
        ],
        "probes": results,
    }


def run_cross_probes(core: Path, rom: Path, output: Path) -> dict:
    """Compare independent expectations and both engines for each instruction class."""
    from .emulator.mupen import replay_slice

    results = []
    for name, words, expected in probe_cases():
        snapshot = make_capture(words)
        unicorn = replay(snapshot, synthetic=True)
        interpreter = replay_slice(snapshot, Path(rom), Path(core), Path(output) / name)
        mismatches = {}
        for key, value in expected.items():
            actual = {
                "unicorn": unicorn.get("registers", {}).get(key),
                "interpreter": interpreter.get("registers", {}).get(key),
            }
            if any(found != value for found in actual.values()):
                mismatches[key] = {"expected": value, **actual}
        supported = (
            unicorn["status"] == interpreter["status"] == "completed" and not mismatches
        )
        results.append(
            {
                "name": name,
                "status": "supported" if supported else "unsupported",
                "mismatches": mismatches,
                "unicorn": unicorn,
                "interpreter": interpreter,
            }
        )
    return {
        "status": "supported"
        if all(r["status"] == "supported" for r in results)
        else "unsupported",
        "cross_validated": all(r["status"] == "supported" for r in results),
        "probes": results,
        "limitations": [
            "Only enumerated integer instruction classes cross-validated",
            "FPU, MMU, interrupts and devices excluded",
        ],
    }
