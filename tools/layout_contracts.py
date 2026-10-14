#!/usr/bin/env python3
"""Inventory opaque game record members without inferring their meaning."""
import argparse
import json
from pathlib import Path
import re

from source_quality import code_only

ROOT = Path(__file__).resolve().parents[1]
RECORD = re.compile(r'\b(struct|union)\s*(\w+)?\s*\{')
MEMBER = re.compile(r'\b(?:[A-Za-z_]\w*\s+)+(?:\*\s*)*'
                    r'(unknown[0-9A-Fa-f_]+)\s*(?:\[[^\]\n]+\])*\s*;')


def inspect_records(source, filename):
    """Locate opaque scalar/pointer/array declarations inside named or anonymous records."""
    code = code_only(source)
    ranges = []
    for match in RECORD.finditer(code):
        start = match.end() - 1
        depth = 1
        end = start + 1
        while end < len(code) and depth:
            depth += (code[end] == '{') - (code[end] == '}')
            end += 1
        if depth:
            raise ValueError(f'{filename}: unclosed record at line {code.count(chr(10), 0, start) + 1}')
        tail = re.match(r'\s*(\w+)\s*(?:;|=)', code[end:])
        name = match[2] or (tail[1] if tail else f'anonymous@{code.count(chr(10), 0, start) + 1}')
        ranges.append((start, end, name))
    findings = []
    for match in MEMBER.finditer(code):
        owners = [item for item in ranges if item[0] < match.start() < item[1]]
        if not owners:
            continue
        owners.sort(key=lambda item: item[0])
        findings.append({'file': filename, 'record': '.'.join(item[2] for item in owners),
                         'member': match[1], 'line': code.count('\n', 0, match.start()) + 1,
                         'declaration': ' '.join(match[0].split())})
    return findings


def inventory(root):
    """Exclude locally supplied SDK files and inventory game header/C records."""
    manifest = json.loads((root / 'docs/LOCAL_SDK_FILES.json').read_text())
    excluded = {entry['path'] for entry in manifest['files']}
    findings = []
    for directory, extension in [('include', '*.h'), ('src', '*.c')]:
        for path in sorted((root / directory).rglob(extension)):
            relative = path.relative_to(root).as_posix()
            if relative not in excluded:
                findings.extend(inspect_records(path.read_text(), relative))
    return {'scope': 'Opaque scalar/pointer/array members named unknown plus an offset suffix in game struct/union definitions. '
                     'Comments, literals, local variables and local SDK files are excluded. Other opaque naming, function-pointer declarations, '
                     'raw offsets and ownership/callback evidence need separate audit; this is not completion proof.',
            'members': len(findings), 'files': len({item['file'] for item in findings}), 'findings': findings}


def finding_key(finding):
    """Identify a declaration independently of source-line movement."""
    return tuple(finding[key] for key in ('file', 'record', 'member', 'declaration'))


def review_findings(findings, reviews):
    """Reject new, changed, duplicate, stale or evidence-free review entries."""
    errors = []
    actual = {finding_key(item): item for item in findings}
    accepted = {}
    for item in reviews:
        key = finding_key(item)
        if key in accepted:
            errors.append(f'duplicate review: {key}')
        accepted[key] = item
        for field in ('known_facts', 'reason', 'next_evidence', 'matching_constraint', 'evidence'):
            if not isinstance(item.get(field), str) or not item[field].strip():
                errors.append(f'missing {field}: {key}')
    for key in actual.keys() - accepted.keys():
        errors.append(f'unreviewed declaration: {key}')
    for key in accepted.keys() - actual.keys():
        errors.append(f'stale review: {key}')
    return sorted(errors)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--json', action='store_true')
    parser.add_argument('--check', action='store_true', help='Require a current, evidenced review for every finding')
    args = parser.parse_args()
    result = inventory(ROOT)
    if args.check:
        ledger = json.loads((ROOT / 'docs/OPAQUE_LAYOUTS.json').read_text())
        errors = review_findings(result['findings'], ledger['reviews'])
        for error in errors:
            print('ERROR: ' + error)
        if errors:
            raise SystemExit(1)
        print(f"OK: {result['members']} opaque declarations have current reviews")
    elif args.json:
        print(json.dumps(result, indent=2))
    else:
        print(f"Opaque record members: {result['members']} across {result['files']} files")
        for finding in result['findings']:
            print(f"{finding['file']}:{finding['line']}: {finding['record']}.{finding['member']}")


if __name__ == '__main__':
    main()
