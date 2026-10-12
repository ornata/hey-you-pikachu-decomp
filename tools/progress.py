#!/usr/bin/env python3
"""Report decompilation progress.

Counting model (post re-split): every not-yet-matched function has a
per-function asm file under asm/nonmatchings/ (splat omits the file
once the function lives in C), so:

    unmatched = nonmatchings .s files
    matched   = function definitions in src/**/*.c
    excluded  = standalone assembly units (including locally generated SDK units)

Bytes trend to zero: 4 bytes per instruction-comment line in the
remaining nonmatchings.

Run:  uv run python tools/progress.py [--json]
"""
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
INSN = re.compile(r"^\s*/\* [0-9A-Fa-f]+ [0-9A-Fa-f]{8} [0-9A-Fa-f]{8} \*/", re.M)
# a C function definition: type on its own line(s) ending `name(args) {`
CDEF = re.compile(r"^[A-Za-z_][\w \t\*]*\b\w+\s*\([^;{}]*\)\s*\{", re.M)
# a K&R definition: `name(a, b) type a; type b; {` — decls between ) and {
KRDEF = re.compile(
    r"^[A-Za-z_][\w \t\*]*\b\w+\s*\([\w \t,]*\)\s*(?:[\w \t\*\[\],]+;\s*)+\{", re.M)
GLABEL = re.compile(r"^glabel ", re.M)


def strip_comments(text):
    """Drop comments so commented-out code is not counted, keeping string literals intact."""
    tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*', re.S)
    return tokens.sub(lambda match: "" if match.group().startswith(("/*", "//")) else match.group(), text)


def main() -> None:
    # unmatched = pragmas still referenced from C (stale .s files linger
    # on disk until the next re-split, so files alone over-count)
    import re as _re
    PRAG = _re.compile(r'#pragma GLOBAL_ASM\("(asm/nonmatchings/[^"]+)"\)')
    referenced = set()
    for c in ROOT.glob("src/**/*.c"):
        referenced.update(PRAG.findall(strip_comments(c.read_text())))
    unmatched = 0
    unmatched_bytes = 0
    for rel in referenced:
        p = ROOT / rel
        if p.exists():
            text = p.read_text(errors="ignore")
            if "glabel " not in text:
                continue  # a rodata carrier pragma, not a function
            unmatched += 1
            unmatched_bytes += 4 * len(INSN.findall(text))
        else:
            unmatched += 1

    matched = 0
    for c in ROOT.glob("src/**/*.c"):
        text = strip_comments(c.read_text())
        matched += len(CDEF.findall(text)) + len(KRDEF.findall(text))

    excluded = 0
    for s in ROOT.glob("asm/**/*.s"):
        if "nonmatchings" in s.parts or "data" in s.parts:
            continue
        excluded += len(GLABEL.findall(s.read_text(errors="ignore")))

    total = matched + unmatched
    if "--json" in sys.argv:
        print(json.dumps({
            "funcs_matched": matched, "funcs_total": total,
            "asm_bytes_remaining": unmatched_bytes,
            "funcs_excluded_hasm": excluded,
        }))
        return
    pct = 100.0 * matched / total if total else 0.0
    print(f"funcs: {matched}/{total} ({pct:.1f}%)   "
          f"asm remaining: {unmatched_bytes} bytes   "
          f"standalone asm: {excluded} funcs")


if __name__ == "__main__":
    main()
