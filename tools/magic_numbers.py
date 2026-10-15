#!/usr/bin/env python3
"""Group Clang numeric-literal findings by source domain without guessing meanings."""

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import sys

from tools.source_quality import code_only

ROOT = Path(__file__).resolve().parents[1]
DEFINITION = re.compile(r"^\w[^\n;{}]*\b(\w+)\([^;{}]*\)\s*\{", re.M)
NUMBER = re.compile(
    r"(?:0[xX][0-9A-Fa-f]+|(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)[uUlLfF]*"
)
POLICIES = {
    "macro_expansion_candidate": "Diagnostic is attributed to a macro invocation rather than a source numeric token; inspect argument spellings and keep SDK macro implementations unchanged.",
    "verification_literal": "Retain ABI size/offset assertions; these numbers are independent checks, not recovered gameplay constants.",
    "definition_literal": "Retain the numeric definition of an existing named domain; inspect its consumers instead.",
    "data_literal": "Retain table/resource data unless a specific semantic domain has independent evidence. Do not name every table entry.",
    "array_bound_candidate": "Establish the allocation, table extent and ownership before naming a capacity; a bracket alone is not evidence.",
    "selector_candidate": "Reconcile the producer and dispatch consumers; reuse an existing enum when its domain agrees.",
    "mask_candidate": "Establish access width, polarity and independent bit behavior before naming flags or mask combinations.",
    "numeric_candidate": "Determine units, thresholds, sentinels or resource identity from producers, consumers and scoped runtime observations.",
}


def subsystem(path):
    """Assign a navigation group, not an inferred semantic interpretation."""
    name = Path(path).stem
    if "voice" in name or name == "microphone":
        return "voice"
    if "script" in name:
        return "script-vm"
    for prefix, group in [
        ("item", "items"),
        ("player", "player"),
        ("actor", "actors"),
        ("object", "objects"),
        ("scene", "scenes"),
        ("area", "scenes"),
        ("terrain", "terrain"),
        ("audio", "audio"),
        ("sound", "audio"),
    ]:
        if name.startswith(prefix):
            return group
    return "scenes" if path.startswith("src/scenes/") else name


def regions(code):
    """Locate function and declaration scopes in masked C for lexical triage."""
    functions = []
    for match in DEFINITION.finditer(code):
        end, depth = match.end(), 1
        while depth and end < len(code):
            depth += (code[end] == "{") - (code[end] == "}")
            end += 1
        functions.append((match.start(), end, match[1]))
    declarations, stack = [], []
    for match in re.finditer(r"[{}]", code):
        if match[0] == "{":
            start = (
                max(
                    code.rfind(";", 0, match.start()),
                    code.rfind("}", 0, match.start()),
                    code.rfind("{", 0, match.start()),
                )
                + 1
            )
            prefix = code[start : match.start()]
            kind = (
                "definition_literal"
                if re.search(r"\b(enum|struct|union)\b", prefix)
                else "data_literal"
            )
            stack.append((match.start(), kind))
        elif stack:
            start, kind = stack.pop()
            declarations.append((start, match.end(), kind))
    return functions, declarations


def classify(code, offset, functions, declarations):
    """Return conservative review categories; none establishes a constant's meaning."""
    start = code.rfind("\n", 0, offset) + 1
    end = code.find("\n", offset)
    line = code[start : end if end >= 0 else len(code)]
    function = next((name for lo, hi, name in functions if lo <= offset < hi), None)
    if re.search(
        r"\b(sizeof|offsetof)\b|\w*(?:SizeCheck|OffsetCheck|OFFSET_CHECK|MEMBER_CHECK)\b",
        line,
    ):
        category = "verification_literal"
    elif re.match(r"\s*#\s*define\s+", line):
        category = "definition_literal"
    elif function is None and (
        scopes := [(hi - lo, kind) for lo, hi, kind in declarations if lo < offset < hi]
    ):
        category = min(scopes)[1]
    elif line[: offset - start].rfind("[") > line[: offset - start].rfind("]"):
        category = "array_bound_candidate"
    elif re.search(r"\bcase\b", line):
        category = "selector_candidate"
    elif re.search(r"(?<!&)&(?!&)|(?<!\|)\|(?!\|)|<<|>>|\^|~", line):
        category = "mask_candidate"
    else:
        category = "numeric_candidate"
    return category, function


def inventory(report, root):
    """Deduplicate owned diagnostics and reject stale source snapshots."""
    cache = {}
    for name, expected in report["source_hashes"].items():
        path = (root / name).resolve()
        if not path.is_relative_to(root) or not name.startswith(("src/", "include/")):
            raise ValueError(f"non-owned source path: {name}")
        blob = path.read_bytes()
        if hashlib.sha256(blob).hexdigest() != expected:
            raise ValueError(f"stale Clang report: {name}; regenerate the audit")
        text = blob.decode()
        code = code_only(text)
        cache[name] = blob, text, code, regions(code)
    unique, complete = {}, set()
    for unit in report["units"]:
        for finding in unit["findings"]:
            if finding["check"] != "readability-magic-numbers":
                continue
            key = finding["path"], finding["offset"], finding["message"]
            unique[key] = finding
            if unit["status"] == "complete":
                complete.add(key)
    entries = []
    for key, finding in sorted(unique.items()):
        name, byte_offset, message = key
        if name not in cache:
            raise ValueError(f"finding outside hashed ownership: {name}")
        blob, text, code, scopes = cache[name]
        if not 0 <= byte_offset < len(blob):
            raise ValueError(f"finding outside source: {name}:{byte_offset}")
        offset = len(blob[:byte_offset].decode())
        literal = NUMBER.match(code, offset)
        category, function = classify(code, offset, *scopes)
        if not literal:
            if not re.match(r"[A-Za-z_]\w*", code[offset:]):
                raise ValueError(
                    f"finding does not point at code: {name}:{byte_offset}"
                )
            category = "macro_expansion_candidate"
        start = text.rfind("\n", 0, offset) + 1
        end = text.find("\n", offset)
        entries.append(
            dict(
                path=name,
                offset=byte_offset,
                line=text.count("\n", 0, offset) + 1,
                literal=literal[0] if literal else None,
                subsystem=subsystem(name),
                function=function,
                category=category,
                context=text[start : end if end >= 0 else len(text)].strip(),
                analysis="complete" if key in complete else "partial-ast",
                next_evidence=POLICIES[category],
            )
        )
    groups = defaultdict(list)
    for entry in entries:
        # The numeric spelling is grouped only within its function/domain and category.
        groups[
            (
                entry["subsystem"],
                entry["path"],
                entry["function"],
                entry["category"],
                entry["literal"],
            )
        ].append(entry)
    return dict(
        schema_version=1,
        scope="Clang findings, not all numeric tokens or a semantic proof; lexical categories are review hints.",
        coverage=report["coverage"],
        occurrences=len(entries),
        affected_files=len({e["path"] for e in entries}),
        counts=dict(sorted(Counter(e["category"] for e in entries).items())),
        review_groups=len(groups),
        entries=entries,
    )


def render(result):
    lines = [
        f"Magic-number findings: {result['occurrences']} across {result['affected_files']} files; {result['review_groups']} scoped groups.",
        f"Clang coverage: {result['coverage']['complete']}/{result['coverage']['total']} units.",
        result["scope"],
    ]
    lines.extend(
        f"  {category}: {count} — {POLICIES[category]}"
        for category, count in result["counts"].items()
    )
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--report", type=Path, required=True, help="read-only Clang report.json"
    )
    parser.add_argument("--output", type=Path, default=ROOT / "build/magic-numbers")
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / "build/magic-numbers"):
        parser.error("output must be under build/magic-numbers/")
    try:
        result = inventory(json.loads(args.report.read_text()), ROOT)
        output.mkdir(parents=True, exist_ok=True)
        (output / "inventory.json").write_text(json.dumps(result, indent=2) + "\n")
        (output / "inventory.txt").write_text(render(result))
    except (OSError, ValueError, KeyError) as error:
        print(f"magic-numbers: {error}", file=sys.stderr)
        return 2
    print(render(result), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
