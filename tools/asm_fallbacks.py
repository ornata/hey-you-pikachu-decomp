#!/usr/bin/env python3
"""Report assembly fallbacks: GLOBAL_ASM function blocks and standalone asm units."""
import argparse
from pathlib import Path
import re

from progress import strip_comments

ROOT = Path(__file__).resolve().parent.parent
PRAGMA = re.compile(r'#\s*pragma\s+GLOBAL_ASM\s*\(\s*"([^"]+)"\s*\)')
LABEL = re.compile(r'^\s*(?:glabel\s+|nonmatching\s+)(\w+)', re.M)


def inventory(root=ROOT):
    records = []
    data = set()
    seen = set()
    for source in sorted((root / 'src').rglob('*.c')):
        for relative in PRAGMA.findall(strip_comments(source.read_text())):
            if relative in seen:
                continue
            seen.add(relative)
            assembly = root / relative
            if not assembly.exists():
                records.append((str(source.relative_to(root)), relative, '(assembly not generated)'))
                continue
            names = list(dict.fromkeys(LABEL.findall(assembly.read_text())))
            if names:
                records.extend((str(source.relative_to(root)), relative, name) for name in names)
            else:
                data.add(relative)

    for assembly in sorted((root / 'asm').rglob('*.s')):
        relative = str(assembly.relative_to(root))
        if relative in seen or 'nonmatchings' in assembly.parts or 'data' in assembly.parts:
            continue
        records.extend((relative, relative, name) for name in dict.fromkeys(LABEL.findall(assembly.read_text())))
    return records, data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verbose', action='store_true', help='list every fallback and its source')
    args = parser.parse_args()
    records, data = inventory()
    missing = sum(name == '(assembly not generated)' for _, _, name in records)
    print(f'ASM fallbacks: {len(records) - missing} function entries; '
          f'{len(data)} data-only GLOBAL_ASM blocks.')
    if missing or not (ROOT / 'asm').exists():
        print(f'ASM fallback inventory incomplete: generate asm/ with make split '
              f'({missing} unresolved GLOBAL_ASM paths).')
    if args.verbose:
        for source, assembly, name in records:
            print(f'  {source}: {name} [{assembly}]')
    else:
        print('Run make asm-fallbacks for the full list.')


if __name__ == '__main__':
    main()
