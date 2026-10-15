#!/usr/bin/env python3
"""Run advisory Clang readability analysis without changing matching sources.

Make supplies the compilation commands; an explicit adapter maps them to Clang.
Reports and proposed edits stay under build/clang-tidy. Parse failures never
become successful lint coverage, and SDK/generated-file replacements are rejected.
"""

from __future__ import annotations
import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import yaml

ROOT = Path(__file__).resolve().parents[1]
SDK_DIRS = {"audio", "gu", "os", "libc", "libm", "mgu"}
LEGACY_CONVERSIONS = {"src/game/action_script_commands.c", "src/game/texture_cache.c"}
KNOWN_UNSUPPORTED = {
    "src/game/save_progress.c": "progressReadFieldValue retains a dead K&R argument omitted by progressGetValue; see docs/INTERFACE_CONTRACTS.json and docs/TYPE_RECOVERY_NOTES.json. Do not alter the ABI for Clang."
}
BACKEND_FLAGS = {"-c", "-non_shared", "-G0", "-Xcpluscomm", "-Xfullwarn"}


def owned_files(root: Path) -> set[Path]:
    """Return tracked game C/header files, excluding manifest-owned SDK inputs."""
    manifest = json.loads((root / "docs/LOCAL_SDK_FILES.json").read_text())
    sdk = {entry["path"] for entry in manifest["files"]}
    tracked = (
        subprocess.check_output(
            ["git", "-C", str(root), "ls-files", "-z", "--", "src", "include"]
        )
        .decode()
        .split("\0")
    )
    resolved_sdk = {(root / path).resolve() for path in sdk}
    result = set()
    for path in tracked:
        if (
            not path
            or Path(path).parts[0] not in ("src", "include")
            or Path(path).suffix not in (".c", ".h")
        ):
            continue
        if path.startswith("src/") and Path(path).parts[1] in SDK_DIRS:
            continue
        candidate = (root / path).resolve()
        if candidate.is_relative_to(root) and candidate not in resolved_sdk:
            result.add(candidate)
    return result


def adapt_flags(flags: list[str], source: str) -> list[str]:
    """Translate known IDO options, failing explicitly on unfamiliar flags."""
    result = [
        "-target",
        "mips-unknown-linux-gnu",
        "-mabi=32",
        "-std=gnu89",
        "-fno-pic",
        "-fno-pie",
    ]
    isa = None
    index = 0
    while index < len(flags):
        flag = flags[index]
        if flag == "-o":
            index += 2
            continue
        if flag == source:
            index += 1
            continue
        if flag in ("-I", "-D", "-U"):
            if index + 1 >= len(flags):
                raise ValueError(f"missing operand for {flag}")
            result.extend(flags[index : index + 2])
            index += 2
            continue
        if flag.startswith(("-I", "-D", "-U")) or flag in (
            "-nostdinc",
            "-O0",
            "-O1",
            "-O2",
            "-O3",
        ):
            result.append(flag)
        elif flag in ("-mips1", "-mips2"):
            isa = flag.removeprefix("-mips")
        elif flag == "-signed":
            result.append("-fsigned-char")
        elif flag == "-32" or flag in BACKEND_FLAGS or flag.startswith("-Wab,"):
            pass
        else:
            raise ValueError(f"unmapped IDO flag: {flag}")
        index += 1
    if isa is None:
        raise ValueError(f"missing per-unit ISA for {source}")
    result.append("-march=mips" + isa)
    if source in LEGACY_CONVERSIONS:
        result.append("-Wno-error=int-conversion")
    return result


def commands_from_make(text: str, root: Path, owned: set[Path]) -> list[dict]:
    """Read asm-processor commands, including the accepted binasm wrapper."""
    entries = {}
    for line in text.splitlines():
        if "tools/asm-processor/build.py" not in line:
            continue
        words = shlex.split(line)
        if "--" not in words or not words[-1].endswith(".c"):
            raise ValueError("unrecognized asm-processor command")
        source = words[-1]
        path = (root / source).resolve()
        if path not in owned:
            raise ValueError(f"compilation command outside game ownership: {source}")
        separator = max(i for i, value in enumerate(words) if value == "--")
        arguments = [
            "clang",
            *adapt_flags(words[separator + 1 :], source),
            "-fsyntax-only",
            str(path),
        ]
        entry = dict(directory=str(root), file=str(path), arguments=arguments)
        if source in entries and entries[source] != entry:
            raise ValueError(f"conflicting compilation commands: {source}")
        entries[source] = entry
    expected = {p for p in owned if p.suffix == ".c" and p.is_relative_to(root / "src")}
    actual = {Path(entry["file"]) for entry in entries.values()}
    if actual != expected:
        raise ValueError(
            f"incomplete compilation inventory: missing {sorted(str(p.relative_to(root)) for p in expected-actual)}"
        )
    return [entries[key] for key in sorted(entries)]


def generate_database(
    root: Path, output: Path, make: str
) -> tuple[list[dict], set[Path]]:
    """Capture Make's dry-run command lines and create the analysis database."""
    owned = owned_files(root)
    targets = [
        "build/" + str(p.relative_to(root).with_suffix(".o"))
        for p in sorted(owned)
        if p.suffix == ".c" and p.is_relative_to(root / "src")
    ]
    process = subprocess.run(
        [make, "--no-print-directory", "-n", "-B", *targets],
        cwd=root,
        capture_output=True,
        text=True,
    )
    (output / "make-commands.log").write_text(process.stdout + process.stderr)
    if process.returncode:
        raise RuntimeError("Make command discovery failed; see make-commands.log")
    entries = commands_from_make(process.stdout, root, owned)
    (output / "compile_commands.json").write_text(json.dumps(entries, indent=2) + "\n")
    return entries, owned


def collect_diagnostics(
    document: dict, root: Path, owned: set[Path]
) -> tuple[list[dict], list[dict], int]:
    """Separate owned readability findings from compiler errors and SDK noise."""
    findings, errors, excluded = [], [], 0
    for diagnostic in document.get("Diagnostics", []):
        name = diagnostic["DiagnosticName"]
        message = diagnostic["DiagnosticMessage"]
        path = (root / message.get("FilePath", "")).resolve()
        if diagnostic.get("Level") in ("Error", "Fatal"):
            errors.append(
                dict(
                    check=name,
                    message=message["Message"],
                    path=str(path),
                    notes=[note["Message"] for note in diagnostic.get("Notes", [])],
                )
            )
        if not name.startswith("readability-"):
            continue
        if path not in owned:
            excluded += 1
            continue
        replacements = []
        for part in [message, *diagnostic.get("Notes", [])]:
            replacements.extend(part.get("Replacements", []))
        findings.append(
            dict(
                check=name,
                path=str(path.relative_to(root)),
                offset=message.get("FileOffset", 0),
                message=message["Message"],
                replacements=replacements,
            )
        )
    return findings, errors, excluded


def validated_replacements(
    findings: list[dict], root: Path, owned: set[Path], hashes: dict[str, str]
) -> dict[Path, list[dict]]:
    """Validate explicitly selected proposed edits; this function writes nothing."""
    grouped = {}
    for finding in findings:
        for edit in finding["replacements"]:
            path = (root / edit["FilePath"]).resolve()
            if path not in owned:
                raise ValueError(f"replacement outside game ownership: {path}")
            relative = str(path.relative_to(root))
            if hashlib.sha256(path.read_bytes()).hexdigest() != hashes.get(relative):
                raise ValueError(f"stale replacement: {relative}")
            offset, size = edit["Offset"], edit["Length"]
            if offset < 0 or size < 0 or offset + size > path.stat().st_size:
                raise ValueError(f"replacement outside file bounds: {relative}")
            grouped.setdefault(path, {})[(offset, size, edit["ReplacementText"])] = edit
    result = {}
    for path, unique in grouped.items():
        edits = [unique[key] for key in sorted(unique)]
        for left, right in zip(edits, edits[1:]):
            if (
                left["Offset"] == right["Offset"]
                or left["Offset"] + left["Length"] > right["Offset"]
            ):
                raise ValueError(f"conflicting replacements: {path}")
        result[path] = edits
    return result


def parse_status(source: str, exit_code: int, errors: list[dict]) -> str:
    """Recognize only the documented arity failure, never unrelated parse errors."""
    if exit_code == 0 and not errors:
        return "complete"
    if (
        source in KNOWN_UNSUPPORTED
        and len(errors) == 1
        and errors[0]["message"]
        == "too few arguments to function call, expected 2, have 1"
        and any("progressReadFieldValue" in note for note in errors[0].get("notes", []))
    ):
        return "unsupported"
    return "parse-failed"


def scan(
    root: Path,
    output: Path,
    entries: list[dict],
    owned: set[Path],
    executable: str,
    jobs: int,
    checks: str | None,
) -> dict:
    """Run read-only analysis and retain per-unit diagnostics with coverage status."""
    version = subprocess.check_output([executable, "--version"], text=True).strip()
    config = root / "config/clang-tidy.yaml"
    hashes = {
        str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
        for p in sorted(owned)
    }
    # Main-file findings are always reported; owned headers need an explicit filter.
    header_filter = (
        "^("
        + "|".join(
            re.escape(name)
            for p in sorted(owned)
            if p.suffix == ".h"
            for name in (str(p), str(p.relative_to(root)))
        )
        + ")$"
    )

    def unit(entry):
        source = Path(entry["file"])
        relative = str(source.relative_to(root))
        stem = relative.replace("/", "__")
        fixes = output / (stem + ".yaml")
        fixes.unlink(missing_ok=True)
        command = [
            executable,
            str(source),
            "-p",
            str(output),
            "--config-file=" + str(config),
            "--header-filter=" + header_filter,
            "--export-fixes=" + str(fixes),
            "--quiet",
        ]
        if checks:
            command.append("--checks=" + checks)
        process = subprocess.run(command, cwd=root, capture_output=True, text=True)
        (output / (stem + ".log")).write_text(process.stdout + process.stderr)
        document = yaml.safe_load(fixes.read_text()) or {} if fixes.exists() else {}
        findings, errors, excluded = collect_diagnostics(document, root, owned)
        status = parse_status(relative, process.returncode, errors)
        return dict(
            source=relative,
            status=status,
            reason=KNOWN_UNSUPPORTED.get(relative) if status == "unsupported" else None,
            exit_code=process.returncode,
            compiler_errors=errors,
            excluded_sdk_findings=excluded,
            findings=findings,
        )

    with ThreadPoolExecutor(max_workers=jobs) as pool:
        units = list(pool.map(unit, entries))
    counts = Counter(f["check"] for u in units for f in u["findings"])
    unique = {
        (f["check"], f["path"], f["offset"], f["message"]): f
        for u in units
        for f in u["findings"]
    }
    unique_counts = Counter(f["check"] for f in unique.values())
    result = dict(
        schema_version=1,
        tool_version=version,
        config_sha256=hashlib.sha256(config.read_bytes()).hexdigest(),
        source_hashes=hashes,
        coverage=dict(
            total=len(units), complete=sum(u["status"] == "complete" for u in units)
        ),
        counts=dict(sorted(counts.items())),
        unique_counts=dict(sorted(unique_counts.items())),
        units=units,
        source_files_modified=False,
    )
    (output / "report.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--files", nargs="*", default=[])
    parser.add_argument("--output", type=Path, default=ROOT / "build/clang-tidy")
    parser.add_argument("--clang-tidy", default="clang-tidy")
    parser.add_argument("--make", default="make")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument(
        "--checks", help="override enabled checks for a focused read-only pass"
    )
    parser.add_argument("--generate-only", action="store_true")
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / "build/clang-tidy") or args.jobs < 1:
        parser.error("output must be under build/clang-tidy and jobs must be positive")
    output.mkdir(parents=True, exist_ok=True)
    try:
        entries, owned = generate_database(ROOT, output, args.make)
        if args.files:
            selected = {(ROOT / f).resolve() for f in args.files}
            if not selected <= {Path(e["file"]) for e in entries}:
                raise ValueError("FILES must select game C translation units")
            entries = [e for e in entries if Path(e["file"]) in selected]
        if args.generate_only:
            print(f"Generated {len(entries)} analysis commands in {output}")
            return 0
        if not shutil.which(args.clang_tidy):
            raise RuntimeError(
                "clang-tidy unavailable; enter nix develop .#lint --command bash"
            )
        result = scan(
            ROOT, output, entries, owned, args.clang_tidy, args.jobs, args.checks
        )
    except (
        OSError,
        ValueError,
        RuntimeError,
        subprocess.CalledProcessError,
        yaml.YAMLError,
    ) as error:
        print(f"clang-tidy: {error}", file=sys.stderr)
        return 2
    print(
        f"Clang readability: {result['coverage']['complete']}/{result['coverage']['total']} units parsed; {sum(result['counts'].values())} advisory findings. Report: {output / 'report.json'}"
    )
    for unit in result["units"]:
        if unit["status"] != "complete":
            print(
                f"  {unit['source']}: {unit['status']}; {unit['reason'] or 'see compiler_errors in report'}"
            )
    return 0 if result["coverage"]["complete"] == result["coverage"]["total"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
