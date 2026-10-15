"""Repeat one retail voice-command scenario and its no-speech control."""

from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys
from .voice_verdict import assess


def normalize(runtime: dict, expected: dict) -> dict:
    """Select the declared observation window without discarding the raw capture."""
    selected = []
    for raw in runtime["events"]:
        if not expected["window_start"] <= raw["poll"] <= expected["poll_bound"]:
            continue
        stage = raw["stage"]
        if stage not in (
            "prepared",
            "launch",
            "messenger",
            "posted",
            "message_read",
            "branch",
            "action",
        ):
            continue
        event = dict(raw)
        event["hit"] = event.pop("dispatch")
        if stage == "posted":
            event["accepted"] = event["return_value"]
        if stage == "branch":
            event["fallthrough"] = event["operand_offset"] + 5
        selected.append(event)
    voices = [e for e in runtime["events"] if e["stage"] == "voice_state"]
    return dict(
        status=runtime["status"],
        pikachu_actor=voices[-1]["pikachu"] if voices else None,
        valid_recognition_results=sum(
            e["event"] == "recognition_result" and e["b"] > 0
            for e in runtime["device_events"]
        ),
        events=selected,
    )


def run(
    rom: Path,
    core: Path,
    source: Path,
    rsp: Path,
    gfx: Path,
    output: Path,
    scenario: dict,
) -> dict:
    """Run three fresh-save commands and three controls through the original ROM."""
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    for label, response in (("command", scenario["response"]), ("control", None)):
        for index in range(3):
            arguments = dict(
                rom=str(rom),
                core=str(core),
                source=str(source),
                rsp=str(rsp),
                output=str(output / f"{label}-{index}"),
                seconds=scenario["max_seconds"],
                response=response,
                gfx=str(gfx),
                poll_bound=scenario["expected"]["poll_bound"],
            )
            # Mupen retains native debugger/plugin globals across CoreShutdown.
            # An OS process is the independent repetition boundary.
            subprocess.run(
                [
                    sys.executable,
                    "-c",
                    "import json,sys; from tools.semantic_review.emulator.voice_runtime import run; run(**json.loads(sys.argv[1]))",
                    json.dumps(arguments),
                ],
                check=True,
            )
    return assess_output(output, scenario)


def assess_output(output: Path, scenario: dict) -> dict:
    """Reassess retained raw observations with an explicitly identified scenario."""
    captures, controls, provenance = [], [], []
    for label, observations in (("command", captures), ("control", controls)):
        for index in range(3):
            directory = Path(output) / f"{label}-{index}"
            runtime = json.loads((directory / "runtime.json").read_text())
            normalized = normalize(runtime, scenario["expected"])
            (directory / "normalized.json").write_text(
                json.dumps(normalized, indent=2) + "\n"
            )
            observations.append(normalized)
            provenance.append(
                dict(
                    run=f"{label}-{index}",
                    rom_sha256=runtime["rom_sha256"],
                    core_sha256=runtime["core_sha256"],
                    plugin_sha256=runtime["plugin_sha256"],
                    raw_sha256=hashlib.sha256(
                        (directory / "runtime.json").read_bytes()
                    ).hexdigest(),
                )
            )
    result = assess(captures, controls, scenario["expected"])
    result.update(
        scenario=scenario["name"],
        expected=scenario["expected"],
        scenario_sha256=hashlib.sha256(
            json.dumps(scenario, sort_keys=True).encode()
        ).hexdigest(),
        provenance=provenance,
    )
    result["limitations"] += scenario.get("limitations", [])
    (Path(output) / "scenario.json").write_text(json.dumps(scenario, indent=2) + "\n")
    (Path(output) / "integration.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "core", "source", "rsp", "gfx"):
        parser.add_argument("--" + name, type=Path)
    parser.add_argument(
        "--output", type=Path, default=Path("build/semantic-review/voice-command")
    )
    parser.add_argument(
        "--scenario",
        type=Path,
        default=Path("tests/semantic_review/voice-command.json"),
    )
    parser.add_argument(
        "--assess-only",
        action="store_true",
        help="Reassess existing raw captures without running the emulator",
    )
    args = parser.parse_args()
    scenario = json.loads(args.scenario.read_text())
    if not args.assess_only and any(
        getattr(args, name) is None for name in ("rom", "core", "source", "rsp", "gfx")
    ):
        parser.error(
            "--rom, --core, --source, --rsp and --gfx are required for capture"
        )
    result = (
        assess_output(args.output, scenario)
        if args.assess_only
        else run(
            args.rom,
            args.core,
            args.source,
            args.rsp,
            args.gfx,
            args.output,
            scenario,
        )
    )
    print(json.dumps(result, indent=2))
    raise SystemExit(
        0
        if result["status"] == "supported"
        else 1 if result["status"] == "contradicted" else 2
    )
