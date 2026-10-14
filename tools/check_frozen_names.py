#!/usr/bin/env python3
"""Report functions whose frozen name (docs/FROZEN_NAMES.json) was renamed or removed.

Frozen names are SDK API and library internals, plus a few that the recompiled port
binds by name. Everything under frozen_paths (the libultra directories) is always an
error. Only individual frozen_functions may be "warn": game-side names the port binds
are listed with what to fix in the port and do not fail the check unless --strict.
Names are compared against a base git ref (default HEAD), so the check needs no
snapshot file and no build:

  src/<dir>      every named function defined in the directory's C sources or its
                 GLOBAL_ASM blocks at BASE must still be defined somewhere in src/
  asm/<dir>      every symbol_addrs.txt entry at BASE whose address falls inside a
                 pikachu.yaml hasm segment of that directory must keep its name
  frozen_functions
                 each listed name must still be defined (src/ or symbol_addrs.txt); an
                 anonymous func_XXXXXXXX must not have been given another name

Anonymous func_XXXXXXXX names are free to rename unless listed individually.

Run:  python3 tools/check_frozen_names.py [--base REF] [--strict]
"""
import argparse
import json
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from progress import strip_comments  # noqa: E402

DEF = re.compile(r"^[A-Za-z_][\w \t\*]*\b(\w+)\s*\([^;{}]*\)\s*\{", re.M)
KRDEF = re.compile(r"^[A-Za-z_][\w \t\*]*\b(\w+)\s*\([\w \t,]*\)\s*(?:[\w \t\*\[\],]+;\s*)+\{", re.M)
PRAGMA = re.compile(r'#\s*pragma\s+GLOBAL_ASM\s*\(\s*"([^"]+)"\s*\)')
LABEL = re.compile(r"^glabel (\w+)", re.M)
SYMBOL = re.compile(r"^\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", re.M)
ANON = re.compile(r"func_[0-9A-Fa-f]{8}(?:_\w+)?")
KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof"}


class WorkTree:
    def files(self, directory, suffix):
        return sorted(str(p.relative_to(ROOT)) for p in (ROOT / directory).rglob("*" + suffix))

    def read(self, path):
        p = ROOT / path
        return p.read_text(errors="ignore") if p.is_file() else None


class GitTree:
    def __init__(self, ref):
        self.ref = ref

    def _git(self, *args):
        return subprocess.run(["git", "-C", str(ROOT), *args], capture_output=True, text=True)

    def files(self, directory, suffix):
        out = self._git("ls-tree", "-r", "--name-only", self.ref, "--", directory)
        return sorted(f for f in out.stdout.split("\n") if f.endswith(suffix))

    def read(self, path):
        out = self._git("show", f"{self.ref}:{path}")
        return out.stdout if out.returncode == 0 else None


def defined_names(text, read):
    """Function names a C source defines, including GLOBAL_ASM function labels."""
    text = strip_comments(text)
    names = (set(DEF.findall(text)) | set(KRDEF.findall(text))) - KEYWORDS
    for relative in PRAGMA.findall(text):
        body = read(relative)
        if body:
            names |= set(LABEL.findall(body))
    return names


def source_names(tree, directory):
    names = set()
    for path in tree.files(directory, ".c"):
        text = tree.read(path)
        if text:
            names |= defined_names(text, tree.read)
    return names


def symbols(text):
    return {name: int(addr, 16) for name, addr in SYMBOL.findall(text or "")}


def hasm_ranges(yaml_text, directory, kinds=("hasm",)):
    """[start, end) vram ranges of hasm segments named <directory>/... in pikachu.yaml."""
    import yaml

    ranges = []
    for segment in yaml.safe_load(yaml_text)["segments"]:
        subs = segment.get("subsegments") if isinstance(segment, dict) else None
        if not subs or "vram" not in segment:
            continue
        delta = segment["vram"] - segment["start"]
        rows = [s for s in subs if isinstance(s, list) and isinstance(s[0], int)]
        for i, row in enumerate(rows):
            if len(row) > 2 and row[1] in kinds and str(row[2]).startswith(directory + "/"):
                end = rows[i + 1][0] if i + 1 < len(rows) else row[0] + 4
                ranges.append((row[0] + delta, end + delta))
    return ranges


def linker_symbols(tree):
    """Read explicit symbols, including alias scripts before and after their move."""
    paths = ("symbol_addrs.txt", "rodata_aliases.ld", "linker/rodata_aliases.ld")
    return symbols("\n".join(tree.read(path) or "" for path in paths))


def check(manifest, base, now):
    """Return [(severity, message)] for every frozen name that changed."""
    problems = []
    functions = manifest.get("frozen_functions", {})
    now_src = source_names(now, "src")
    now_syms = linker_symbols(now)
    yaml_base = base.read("pikachu.yaml")
    yaml_now = now.read("pikachu.yaml")
    base_syms_all = linker_symbols(base)

    def report(entry, message):
        fix = entry.get("port_fix")
        problems.append((entry.get("severity", "error"), message + (f" [port: {fix}]" if fix else "")))

    for path, reason in manifest.get("frozen_paths", {}).items():
        entry = {"severity": "error", "reason": reason}  # libultra and friends: never downgraded
        kind, _, directory = path.partition("/")
        if kind == "src":
            generated = hasm_ranges(yaml_now, directory, ("asm", "hasm", "data", "rodata")) if yaml_now else []
            for name in sorted(source_names(base, path) - now_src):
                # Locally generated SDK assembly preserves the same symbol and address.
                addr = now_syms.get(name)
                if (addr is not None and (name not in base_syms_all or addr == base_syms_all[name])
                        and any(lo <= addr < hi for lo, hi in generated)):
                    continue
                if not ANON.fullmatch(name) or name in functions:
                    report(entry, f"{path}: {name} is no longer defined (renamed or removed)")
            if yaml_base:
                ranges = hasm_ranges(yaml_base, directory, ("asm", "hasm", "data", "rodata"))
                for name, addr in sorted(base_syms_all.items()):
                    if any(lo <= addr < hi for lo, hi in ranges) and now_syms.get(name) != addr:
                        report(entry, f"{path}: generated SDK symbol {name} @ {addr:#010x} changed")
        elif kind == "asm":
            ranges = hasm_ranges(yaml_base, directory) if yaml_base else []
            base_syms = linker_symbols(base)
            by_addr = {}
            for name, addr in now_syms.items():
                by_addr.setdefault(addr, set()).add(name)
            for name, addr in sorted(base_syms.items(), key=lambda kv: kv[1]):
                if any(lo <= addr < hi for lo, hi in ranges) and name not in by_addr.get(addr, set()):
                    if not ANON.fullmatch(name) or name in functions:
                        found = ", ".join(sorted(by_addr.get(addr, {"<nothing>"})))
                        report(entry, f"{path}: {name} @ {addr:#010x} is now {found}")

    base_src = source_names(base, "src")
    for name, entry in sorted(functions.items()):
        if name in now_src or name in now_syms:
            continue
        if name in base_src or not ANON.fullmatch(name):
            report(entry, f"frozen function {name} is not defined anywhere")
        else:
            # an auto-generated address name is intact until symbol_addrs.txt names that address
            addr = int(name[5:13], 16)
            renamed = sorted(n for n, a in now_syms.items() if a == addr)
            if renamed:
                report(entry, f"frozen function {name} @ {addr:#010x} is now {', '.join(renamed)}")
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--base", default="HEAD", help="git ref holding the names to preserve")
    parser.add_argument("--strict", action="store_true", help="treat warnings as failures")
    args = parser.parse_args()
    manifest = json.loads((ROOT / "docs/FROZEN_NAMES.json").read_text())
    problems = check(manifest, GitTree(args.base), WorkTree())
    errors = [m for severity, m in problems if severity == "error" or args.strict]
    warnings = [m for severity, m in problems if severity != "error" and not args.strict]
    for line in warnings:
        print(f"WARN: {line}")
    for line in errors:
        print(f"FAIL: {line}")
    if errors:
        print(f"{len(errors)} frozen name(s) changed; see docs/FROZEN_NAMES.json for why they are frozen.")
        return 1
    if warnings:
        print(f"{len(warnings)} port-bound name(s) changed: regenerate the port's symbols and update the "
              f"overrides listed above.")
    else:
        print(f"OK: frozen names unchanged since {args.base}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
