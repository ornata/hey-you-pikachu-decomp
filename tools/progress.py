#!/usr/bin/env python3
"""Report a lexical game-source census, separately from ROM verification.

Only referenced GLOBAL_ASM files and configured standalone assembly units count.
Unresolved files or function sizes stay explicit; stale generated files do not count.
Run with the locked environment: uv run --locked python tools/progress.py [--json].
"""

import argparse
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
TOKENS = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*', re.S)
CDEF = re.compile(r"^[ \t]*[A-Za-z_][\w\s*]*?\b(\w+)\s*\([^;{}]*\)\s*\{", re.M)
KRDEF = re.compile(
    r"^[ \t]*[A-Za-z_][\w\s*]*?\b(\w+)\s*\([\w\s,]*\)\s*(?:[\w\s*\[\],]+;\s*)+\{", re.M
)
PRAGMA = re.compile(r'^\s*#\s*pragma\s+GLOBAL_ASM\s*\(\s*"([^"]+)"\s*\)', re.M)
LABEL = re.compile(r"^\s*(?:glabel|nonmatching)\s+(\w+)", re.M)
SDK_DIRS = {"audio", "gu", "os", "libc", "libm", "mgu"}


def strip_comments(text):
    """Mask comments without joining adjacent tokens or changing line positions."""
    return TOKENS.sub(
        lambda m: (
            re.sub(r"[^\n]", " ", m[0]) if m[0].startswith(("/*", "//")) else m[0]
        ),
        text,
    )


def assembly_references(text):
    """Read active-looking pragma lines without interpreting quoted source text."""
    clean = strip_comments(text)
    literals = [(m.start(), m.end()) for m in TOKENS.finditer(clean)]
    return [
        m[1]
        for m in PRAGMA.finditer(clean)
        if not any(a <= clean.index("#", m.start(), m.end()) < b for a, b in literals)
    ]


def definitions(text):
    """List top-level C definitions, excluding literals and preprocessor bodies."""
    code = TOKENS.sub(lambda m: re.sub(r"[^\n]", " ", m[0]), text)
    code = re.sub(
        r"^[ \t]*#(?:[^\n]*\\\n)*[^\n]*",
        lambda m: re.sub(r"[^\n]", " ", m[0]),
        code,
        flags=re.M,
    )
    depths, depth = [], 0
    for char in code:
        depths.append(depth)
        depth += (char == "{") - (char == "}")
    return [
        m[1]
        for pattern in (CDEF, KRDEF)
        for m in pattern.finditer(code)
        if depths[m.start()] == 0
    ]


def configured_assembly(root):
    """Return standalone assembly paths declared by the splitter configuration."""
    import yaml

    try:
        config = yaml.safe_load((root / "config/pikachu.yaml").read_text())
    except yaml.YAMLError as error:
        raise ValueError(f"invalid splitter configuration: {error}") from error
    if not isinstance(config, dict) or not isinstance(config.get("segments"), list):
        raise ValueError("splitter configuration requires a segments list")
    paths = set()

    def visit(node):
        if isinstance(node, dict):
            if node.get("type") in ("asm", "hasm"):
                if not node.get("name"):
                    raise ValueError(
                        "standalone assembly configuration requires a name"
                    )
                paths.add("build/asm/" + node["name"] + ".s")
            for value in node.values():
                visit(value)
        elif isinstance(node, list):
            if (
                len(node) >= 2
                and isinstance(node[1], str)
                and node[1] in ("asm", "hasm")
            ):
                if len(node) < 3 or not isinstance(node[2], str) or not node[2]:
                    raise ValueError(
                        "standalone assembly configuration requires a name"
                    )
                paths.add("build/asm/" + node[2] + ".s")
            else:
                for value in node:
                    visit(value)

    visit(config.get("segments", []))
    return sorted(paths)


def linked_sizes(root):
    """Read function sizes keyed by name; ambiguous overlay names remain unknown."""
    path = root / "build/pikachu.elf"
    if not path.exists():
        return {}, None
    try:
        from .compare_objects import Elf
    except ImportError:
        from compare_objects import Elf
    elf = Elf(path)
    sizes = {}
    for index, section in enumerate(elf.sections):
        if section[1] == 2:
            for name, address, size, info, _, shndx in elf.symbols(index):
                if shndx and info & 15 == 2 and size:
                    sizes.setdefault(name, set()).add((address, size, shndx))
    return {
        name: next(iter(values))[1]
        for name, values in sizes.items()
        if len(values) == 1
    }, path.stat().st_mtime_ns


def inventory(root=ROOT):
    """Collect source definitions, assembly functions, data carriers and missing evidence."""
    root = Path(root)
    sources, references = [], {}
    for source in sorted((root / "src").rglob("*.c")):
        if source.relative_to(root / "src").parts[0] in SDK_DIRS:
            continue
        text = source.read_text()
        sources.extend(
            dict(source=str(source.relative_to(root)), name=name)
            for name in definitions(text)
        )
        for relative in assembly_references(text):
            references.setdefault(relative, []).append(source)
    sizes, linked_time = linked_sizes(root)
    fallbacks, standalone, data, missing = [], [], [], []
    paths = [(p, "fallback") for p in sorted(references)] + [
        (p, "standalone") for p in configured_assembly(root) if p not in references
    ]
    for relative, category in paths:
        path = root / relative
        sdk = category == "standalone" and Path(relative).parts[2] in SDK_DIRS
        if not path.exists():
            missing.append(dict(path=relative, category="sdk" if sdk else category))
            continue
        names = list(dict.fromkeys(LABEL.findall(strip_comments(path.read_text()))))
        if not names:
            if category == "fallback":
                data.append(relative)
            continue
        fresh = (
            linked_time is not None
            and path.stat().st_mtime_ns <= linked_time
            and all(
                s.stat().st_mtime_ns <= linked_time
                for s in references.get(relative, [])
            )
        )
        for name in names:
            record = dict(
                path=relative, name=name, bytes=sizes.get(name) if fresh else None
            )
            if category == "fallback":
                fallbacks.append(record)
            else:
                standalone.append(dict(record, sdk=sdk))
    unknown = sum(f["bytes"] is None for f in fallbacks)
    complete = not missing
    total = len(sources) + len(fallbacks)
    return dict(
        schema_version=2,
        metric="lexical source census; not ROM or semantic verification",
        counting_model="funcs_total counts C definitions plus referenced fallback function labels; standalone game and SDK assembly are separate",
        c_definitions=len(sources),
        funcs_matched=len(sources),
        funcs_total=(
            total if not any(m["category"] == "fallback" for m in missing) else None
        ),
        funcs_unmatched=len(fallbacks),
        asm_bytes_remaining=(
            sum(f["bytes"] for f in fallbacks)
            if not unknown and not any(m["category"] == "fallback" for m in missing)
            else None
        ),
        asm_functions_unknown_size=unknown,
        byte_size_evidence="linked ELF function sizes; unknown if missing, stale or ambiguous",
        funcs_standalone_game=sum(not f["sdk"] for f in standalone),
        funcs_excluded_sdk=sum(f["sdk"] for f in standalone),
        inventory_complete=complete,
        missing_assembly=missing,
        c_functions=sources,
        assembly_fallbacks=fallbacks,
        standalone_assembly=standalone,
        data_only_assembly=data,
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", action="store_true")
    parser.add_argument(
        "--strict",
        action="store_true",
        help="fail if inventory or fallback byte sizes are unresolved",
    )
    args = parser.parse_args()
    try:
        result = inventory()
    except (OSError, ValueError, ImportError) as error:
        parser.exit(2, f"progress: error: {error}\n")
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        matched, total = result["funcs_matched"], result["funcs_total"]
        percent = (
            min(99.99, 100 * matched / total)
            if total and matched < total
            else 100 if total else None
        )
        print(
            f"C definitions: {matched}/{total if total is not None else 'unknown'}"
            + (f" ({percent:.2f}%)" if percent is not None else "")
        )
        remaining = result["asm_bytes_remaining"]
        print(
            f"Game assembly fallbacks: {result['funcs_unmatched']}; bytes: {remaining if remaining is not None else 'unknown'}"
        )
        print(
            f"Standalone game assembly: {result['funcs_standalone_game']}; SDK assembly: {result['funcs_excluded_sdk']} (excluded)"
        )
        if not result["inventory_complete"]:
            print(
                f"Incomplete inventory: {len(result['missing_assembly'])} assembly files missing; run make split."
            )
        print(
            "Source census only; use make for ROM verification and make source-quality for recovery placeholders."
        )
    return int(
        args.strict
        and (not result["inventory_complete"] or result["asm_bytes_remaining"] is None)
    )


if __name__ == "__main__":
    raise SystemExit(main())
