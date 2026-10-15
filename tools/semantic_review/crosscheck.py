"""Cross-check predeclared retail-instruction experiments in the N64 interpreter."""

from __future__ import annotations
from collections import Counter
from pathlib import Path
from . import cases, subsystems
from .emulator.mupen import replay_slice, Unsupported
from .replay import replay


def evaluate(case, observed):
    if observed.get("status") != "completed":
        return None
    regions = observed.get("memory", observed.get("mappings", []))

    def read(address, size):
        for region in regions:
            raw = bytes.fromhex(region["data_hex"])
            start = region["address"]
            if start <= address and address + size <= start + len(raw):
                return raw[address - start : address - start + size]
        return None

    correct = all(
        read(address, len(expected)) == expected
        for address, expected in case["observations"]
    )
    wanted = case.get("expected_return")
    if wanted is not None:
        correct &= observed["registers"]["gpr_2"] & 0xFFFFFFFF == wanted & 0xFFFFFFFF
    return bool(correct)


def run(rom: Path, elf: Path, core: Path, output: Path, selected=(), repeats=3):
    """Require repeated interpreter agreement with expected returns and byte ranges."""
    rows = []
    for module in (cases, subsystems):
        for case in module.experiments():
            if selected and case["id"] not in selected:
                continue
            snapshot = (
                cases.capture_case(case, elf, rom)
                if module is cases
                else subsystems.capture(case, elf, rom)
            )
            unicorn = replay(snapshot, rom)
            runs = []
            for repetition in range(repeats):
                try:
                    observed = replay_slice(
                        snapshot, rom, core, output / case["id"] / str(repetition)
                    )
                    agrees = evaluate(case, observed)
                    runs.append(
                        {
                            "status": observed["status"],
                            "expected_observations_agree": agrees,
                            "reason": observed.get("reason"),
                            "registers": observed.get("registers"),
                            "checkpoint_count": len(observed.get("checkpoint_pcs", [])),
                        }
                    )
                except (Unsupported, ValueError, OSError) as error:
                    runs.append(
                        {
                            "status": "unsupported",
                            "reason": str(error),
                            "expected_observations_agree": None,
                        }
                    )
                    break
            verdicts = [item["expected_observations_agree"] for item in runs]
            status = (
                "contradicted"
                if False in verdicts
                else (
                    "supported"
                    if len(runs) == repeats and all(value is True for value in verdicts)
                    else "unsupported"
                )
            )
            rows.append(
                {
                    "id": case["id"],
                    "status": status,
                    "interpreter_runs": runs,
                    "scope": case.get("scope", "function entry"),
                    "unicorn_status": unicorn["status"],
                    "unicorn_expected_agree": evaluate(case, unicorn),
                    "limitations": [
                        *case.get("limitations", []),
                        "Synthetic entry state, not proof of gameplay reachability or device fidelity.",
                        "Agreement covers declared byte ranges and specified return value only.",
                    ],
                }
            )
    if selected and set(selected) - {row["id"] for row in rows}:
        raise ValueError("unknown selected case")
    return {
        "schema_version": 1,
        "engine": "mupen64plus-pure-interpreter",
        "repeats": repeats,
        "summary": dict(Counter(row["status"] for row in rows)),
        "experiments": rows,
    }
