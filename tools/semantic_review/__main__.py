"""Capture N64 execution, replay bounded slices, and check semantic claims."""

from __future__ import annotations

import argparse
import hashlib
import subprocess
from collections import Counter
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "build/semantic-review"


def save(path, result):
    path = Path(path).resolve()
    if not path.is_relative_to(OUTPUT.resolve()):
        raise ValueError("generated reports must be under build/semantic-review/")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(result, indent=2) + "\n")
    return path


def summarize(results):
    counts = Counter(row["status"] for row in results)
    return {
        name: counts[name]
        for name in ("supported", "contradicted", "inconclusive", "unsupported")
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    for name in ("check", "replay", "capture", "isa", "crosscheck"):
        command = commands.add_parser(name)
        command.add_argument("--rom", type=Path, default=ROOT / "rom/baserom.us.z64")
        command.add_argument("--output", type=Path, default=OUTPUT / f"{name}.json")
        if name in ("isa", "check"):
            command.add_argument("--core", type=Path)
        if name == "replay":
            command.add_argument("snapshot", type=Path)
        if name in ("check", "crosscheck"):
            command.add_argument("--elf", type=Path, default=ROOT / "build/pikachu.elf")
        if name == "crosscheck":
            command.add_argument("--core", type=Path, required=True)
            command.add_argument("--case", action="append", default=[])
        if name == "capture":
            command.add_argument("scenario", type=Path)
            command.add_argument("--core", type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        if not args.output.resolve().is_relative_to(OUTPUT.resolve()):
            raise ValueError("generated reports must be under build/semantic-review/")
        if args.command == "isa":
            from .isa import run_probes, run_cross_probes

            result = (
                run_cross_probes(args.core, args.rom, args.output.parent / "isa-runs")
                if args.core
                else run_probes()
            )
            code = 0 if result["status"] == "supported" else 2
        elif args.command == "replay":
            from .replay import replay

            result = replay(json.loads(args.snapshot.read_text()), args.rom)
            code = 0 if result["status"] == "completed" else 2
        elif args.command == "crosscheck":
            from .crosscheck import run

            result = run(
                args.rom,
                args.elf,
                args.core,
                args.output.parent / "crosscheck-runs",
                args.case,
            )
            code = (
                1
                if result["summary"].get("contradicted")
                else 2
                if result["summary"].get("unsupported")
                else 0
            )
        elif args.command == "capture":
            from .capture import capture

            result = capture(
                json.loads(args.scenario.read_text()),
                args.output.parent,
                args.core,
                args.rom,
            )
            code = 0 if result.get("status") == "completed" else 2
        else:
            from .cases import run_cases
            from .isa import run_probes, run_cross_probes

            probes = (
                run_cross_probes(args.core, args.rom, args.output.parent / "isa-runs")
                if args.core
                else run_probes()
            )
            results = run_cases(args.rom, args.elf)
            from .subsystems import run_cases as run_subsystems

            results += run_subsystems(args.rom, args.elf)
            interpreter = None
            if args.core:
                from .crosscheck import run

                interpreter = run(
                    args.rom,
                    args.elf,
                    args.core,
                    args.output.parent / "crosscheck-runs",
                )
                verdicts = {row["id"]: row for row in interpreter["experiments"]}
                for row in results:
                    if row["id"] in verdicts:
                        row["unicorn_status"] = row["status"]
                        row["interpreter_status"] = verdicts[row["id"]]["status"]
                        row["status"] = verdicts[row["id"]]["status"]
                        row["evidence"] += (
                            " Three original-instruction N64 interpreter runs checked the same predeclared observations."
                        )
            pending = [row for row in results if row.get("status") == "inconclusive"]
            contracts = json.loads(
                (ROOT / "tests/semantic_review/contracts.json").read_text()
            )
            result = {
                "schema_version": 1,
                "scope": "Risk-based bounded semantic review; not whole-project certification",
                "isa": probes,
                "interpreter": interpreter,
                "experiments": results,
                "outstanding": pending,
                "static_contracts": contracts,
                "summary": summarize(results),
                "cross_engine_validation_required": not probes.get(
                    "cross_validated", False
                ),
            }
            code = 1 if any(row["status"] == "contradicted" for row in results) else 0
            if any(row["status"] == "unsupported" for row in results) and not code:
                code = 2
        result["provenance"] = {
            "git_revision": subprocess.run(
                ["git", "rev-parse", "HEAD"], cwd=ROOT, capture_output=True, text=True
            ).stdout.strip(),
            "rom_sha256": hashlib.sha256(args.rom.read_bytes()).hexdigest()
            if args.rom.exists()
            else None,
            "scenario": str(args.scenario) if hasattr(args, "scenario") else None,
            "worktree_dirty": bool(
                subprocess.run(
                    ["git", "status", "--porcelain"],
                    cwd=ROOT,
                    capture_output=True,
                    text=True,
                ).stdout
            ),
            "review_sources_sha256": hashlib.sha256(
                b"".join(
                    str(path.relative_to(ROOT)).encode() + b"\0" + path.read_bytes()
                    for path in sorted((ROOT / "tools/semantic_review").rglob("*"))
                    if path.suffix in (".py", ".c")
                )
            ).hexdigest(),
        }
        path = save(args.output, result)
        print(
            f"{args.command}: {result.get('summary', result.get('status', 'recorded'))}; {path}"
        )
        return code
    except (OSError, ValueError, KeyError, ImportError, RuntimeError) as error:
        print(f"semantic review unavailable: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
