#!/usr/bin/env python3
"""Inventory recovery placeholders in game C, excluding comments and literals.

Raw offset expressions are lexical review candidates, not diagnosed structure
accesses: byte streams and hardware addresses can legitimately use arithmetic.
"""

import argparse
from collections import Counter
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
LEXICAL = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*', re.S)
PATTERNS = {
    'field_accesses': re.compile(r'\bM2C_FIELD\s*\('),
    'unknown_types': re.compile(r'\bM2C_UNK(?:8|16|32|64)?\b'),
    'address_globals': re.compile(r'\bD_[0-9A-Fa-f]{8}\b'),
    'temporary_names': re.compile(r'\btemp_[A-Za-z0-9_]+\b'),
    'raw_offset_candidates': re.compile(r'\b[A-Za-z_]\w*\s*[+-]\s*(?:0[xX][0-9A-Fa-f]+|[0-9]+)[uUlL]*\b'),
}


def code_only(text):
    """Preserve positions and newlines while masking comments and literals."""
    return LEXICAL.sub(lambda match: re.sub(r'[^\n]', ' ', match[0]), text)


def inspect_source(text):
    code = code_only(text)
    spans = []
    definition = re.compile(r'^\w[^\n;{}]*\b(\w+)\([^;{}]*\)\s*\{', re.M)
    for match in definition.finditer(code):
        end, depth = match.end(), 1
        while depth and end < len(code):
            depth += (code[end] == '{') - (code[end] == '}')
            end += 1
        spans.append((match.start(), end, match[1]))
    result = {}
    for category, pattern in PATTERNS.items():
        items = []
        for match in pattern.finditer(code):
            start = text.rfind('\n', 0, match.start()) + 1
            end = text.find('\n', match.end())
            if end == -1:
                end = len(text)
            items.append({'line': text.count('\n', 0, match.start()) + 1,
                          'column': match.start() - start + 1,
                          'token': match[0].strip(),
                          'function': next((name for start, end, name in spans if start <= match.start() < end), None),
                          'context': text[start:end].strip()})
        result[category] = items
    return result


def inventory(root):
    files = {}
    for path in sorted((root / 'src').rglob('*.c')):
        items = inspect_source(path.read_text())
        files[path.relative_to(root).as_posix()] = items
    totals = {}
    for category in PATTERNS:
        tokens = Counter(item['token'] for items in files.values() for item in items[category])
        totals[category] = {'occurrences': sum(tokens.values()), 'unique_spellings': len(tokens),
                            'files': sum(bool(items[category]) for items in files.values())}
    return {'schema_version': 1, 'totals': totals, 'files': files}


LEDGER_POLICIES = {
    'field_accesses': {
        'reason': 'Container layout or safe matching representation remains unresolved beyond the recovered views.',
        'next_evidence': 'Trace the base to its allocation and initialization; reconcile every access width with callers and ROM assembly. Check the file matching exceptions before changing a known offset.'},
    'unknown_types': {
        'reason': 'A complete, matching declaration has not been established for this token.',
        'next_evidence': 'Find the definition and all callers or data references; determine signedness, pointer depth, return use and callback ABI. Local SDK declarations are evidence only and must remain excluded.'},
    'address_globals': {
        'reason': 'Address spelling is retained; a semantic name and all cross-unit/linker references have not been validated together.',
        'next_evidence': 'Inspect Ghidra data references, writers, readers, initialization and overlay ownership; retain separate interior symbols when relocation addends require them.'},
    'temporary_names': {
        'reason': 'The local role or multiple live ranges have not been established sufficiently for a semantic rename.',
        'next_evidence': 'Trace definitions and uses within the function; distinguish reused register values before assigning a role. Preserve expression order and statement boundaries.'},
    'raw_offset_candidates': {
        'reason': 'Review candidate only; the expression may be scalar arithmetic, a byte stream, a hardware address or an unresolved record access.',
        'next_evidence': 'Establish the base type and ownership before classifying; retain intentional byte arithmetic and hardware addressing.'},
}


def recovery_ledger(root, report):
    notes_path = root / 'docs/TYPE_RECOVERY_NOTES.json'
    notes = json.loads(notes_path.read_text()) if notes_path.exists() else {}
    files = {}
    for name, categories in report['files'].items():
        if not any(categories.values()):
            continue
        groups = {}
        for category, items in categories.items():
            tokens = {}
            for item in items:
                tokens.setdefault(item['token'], []).append(f"{item['line']}:{item['column']}")
            if tokens:
                groups[category] = tokens
        text = (root / name).read_text()
        files[name] = {
            'known_views': sorted(set(re.findall(r'^#include "([^"\n]+)"', text, re.M)) - {'common.h'}),
            'matching_exceptions': notes.get('matching_exceptions', {}).get(name, {}),
            'locations': groups,
        }
    return {'schema_version': 1, 'baseline_ref': notes.get('baseline_ref'),
            'baseline_totals': notes.get('baseline_totals'),
            'scope': notes.get('scope'), 'totals': report['totals'],
            'policies': LEDGER_POLICIES, 'files': files}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT, help='repository containing src/')
    parser.add_argument('--json', action='store_true', help='include every occurrence and its source location')
    parser.add_argument('--ledger', action='store_true', help='emit the annotated recovery ledger')
    parser.add_argument('--check-ledger', action='store_true', help='fail if docs/TYPE_RECOVERY.json is stale')
    args = parser.parse_args()
    if not (args.root / 'src').is_dir():
        parser.error('--root must contain the game src/ directory')
    report = inventory(args.root)
    if args.check_ledger:
        path = args.root / 'docs/TYPE_RECOVERY.json'
        if not path.exists() or json.loads(path.read_text()) != recovery_ledger(args.root, report):
            parser.exit(1, 'Recovery ledger is stale; regenerate with source_quality.py --ledger.\n')
        print('OK: recovery ledger matches current source locations and counts')
    elif args.ledger:
        print(json.dumps(recovery_ledger(args.root, report), indent=2))
    elif args.json:
        print(json.dumps(report, indent=2))
    else:
        for category, counts in report['totals'].items():
            print(f"{category}: {counts['occurrences']} occurrences, {counts['files']} files, "
                  f"{counts['unique_spellings']} unique spellings")
        print('Raw offsets are review candidates; scalar arithmetic is included.')


if __name__ == '__main__':
    main()
