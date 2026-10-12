#!/usr/bin/env python3
"""Verify or import local SDK-related headers without downloading SDK material.

Use --from DIRECTORY to copy the manifest's include/... paths from a locally
supplied directory. With no arguments, or --check, only verify existing files.
Hashes identify build inputs; they do not establish permission to distribute them.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / 'docs/LOCAL_SDK_FILES.json'


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(entries, root):
    problems = []
    for entry in entries:
        path = root / entry['path']
        if not path.is_file():
            problems.append(f"{entry['path']}: missing")
        elif sha256(path) != entry['sha256']:
            problems.append(f"{entry['path']}: checksum differs")
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--check', action='store_true', help='verify only (the default)')
    mode.add_argument('--from', dest='source', type=Path, help='local directory containing include/...')
    args = parser.parse_args()
    entries = json.loads(MANIFEST.read_text())['files']
    source = args.source.resolve() if args.source else ROOT
    problems = verify(entries, source)
    if problems:
        print('\n'.join(problems), file=sys.stderr)
        print('See docs/LOCAL_SDK.md for required local inputs.', file=sys.stderr)
        return 1
    if args.source:
        for entry in entries:
            src, dest = source / entry['path'], ROOT / entry['path']
            if src.resolve() == dest.resolve():
                continue
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(src, dest)
    print(f'{len(entries)} local SDK-related headers verified')
    return 0


if __name__ == '__main__':
    sys.exit(main())
