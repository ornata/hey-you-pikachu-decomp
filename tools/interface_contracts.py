#!/usr/bin/env python3
"""Audit game C interface declarations without preprocessing SDK headers.

This lexical audit checks written declarations, direct-call arity, and shared
header coverage. It is not a C type checker or proof of ownership contracts.
Target compilation and strict object/ROM verification remain authoritative.
"""

import argparse
from collections import defaultdict
import json
from pathlib import Path
import re

from source_quality import LEXICAL

ROOT = Path(__file__).resolve().parents[1]
HEAD = re.compile(r'^(?P<returns>(?:(?:extern|static|const|unsigned|signed)\s+)*'
                  r'\w+[ \t*]+)(?P<name>\w+)\(', re.M)
WORDS = set('void char short int long float double signed unsigned const volatile '
            's8 u8 s16 u16 s32 u32 s64 u64 f32 f64'.split())


def mask_source(source):
    """Mask comments and literal contents while retaining literal arguments."""
    def replace(match):
        blank = re.sub(r'[^\n]', ' ', match[0])
        return blank if match[0].startswith(('/',)) else '0' + blank[1:]
    return LEXICAL.sub(replace, source)


def closing_parenthesis(code, start):
    depth = 1
    for index in range(start + 1, len(code)):
        depth += (code[index] == '(') - (code[index] == ')')
        if not depth:
            return index
    raise ValueError('Unclosed parameter or argument list')


def split_parameters(text):
    if not text.strip() or text.strip() == 'void':
        return []
    result, start, depth = [], 0, 0
    for index, char in enumerate(text):
        depth += (char in '([') - (char in ')]')
        if char == ',' and not depth:
            result.append(text[start:index].strip())
            start = index + 1
    result.append(text[start:].strip())
    return result


def type_spelling(parameter):
    """Normalize names, array parameter adjustment, and established float aliases."""
    text = parameter.strip()
    text = re.sub(r'\(\*\s*\w+\s*\)', '(*)', text)
    array = re.match(r'^(.*?)\b\w+\s*\[[^]]*\](.*)$', text)
    if array:
        base, tail = array.groups()
        text = base + ('(*)' + tail if tail.strip() else '*')
    elif '(' not in text:
        name = re.search(r'\b(\w+)\s*$', text)
        if (name and name[1] not in WORDS
                and len(re.findall(r'\b\w+\b', text)) > 1
                and not re.match(r'^struct\s+\w+$', text)):
            text = text[:name.start()]
    text = re.sub(r'\b(?:extern|static)\s+', '', text)
    text = re.sub(r'\bf32\b', 'float', text)
    text = re.sub(r'\bf64\b', 'double', text)
    return re.sub(r'\s+', '', text)


def declarations(source, file):
    code = mask_source(source)
    result = []
    for match in HEAD.finditer(code):
        if match['returns'].strip() in {'return', 'typedef'}:
            continue
        end = closing_parenthesis(code, match.end() - 1)
        tail = code[end + 1:]
        terminal = re.match(r'\s*([;{])', tail)
        knr = None
        if not terminal:
            knr = re.match(r'\s*((?:[\w \t*]+;\s*)+)\{', tail)
            if not knr:
                continue
        params = split_parameters(code[match.end():end])
        definition = not terminal or terminal[1] == '{'
        if knr:
            named = {}
            for declaration in knr[1].split(';'):
                name = re.search(r'\b(\w+)\s*$', declaration)
                if name:
                    named[name[1]] = declaration.strip()
            params = [named.get(param, param) for param in params]
        body_span = None
        if definition:
            opening = end + 1 + (knr.end() if knr else terminal.end()) - 1
            depth = 1
            for brace in re.finditer(r'[{}]', code[opening + 1:]):
                depth += 1 if brace[0] == '{' else -1
                if not depth:
                    body_span = [opening, opening + 2 + brace.start()]
                    break
        result.append({
            'file': file, 'line': source.count('\n', 0, match.start()) + 1,
            'name': match['name'], 'returns': type_spelling(match['returns']),
            'parameters': [type_spelling(param) for param in params],
            'spelling': ' '.join(source[match.start():end + 1].split()),
            'definition': definition, 'knr': bool(knr),
            'unspecified': not code[match.end():end].strip(),
            'span': [match.start(), end + 1],
            'body_span': body_span,
        })
    return result


def audit_sources(sources):
    records, by_name, findings = [], defaultdict(list), []
    for file, source in sorted(sources.items()):
        for record in declarations(source, file):
            records.append(record)
            by_name[record['name']].append(record)

    def finding(kind, record, shape):
        item = {'kind': kind, 'file': record['file'],
                'line': record['line'], 'name': record['name'], 'shape': shape}
        if 'caller' in record:
            item['caller'] = record['caller']
        findings.append(item)

    defined_names = {name for name, group in by_name.items() if any(r['definition'] for r in group)}
    reference_files = defaultdict(set)
    for file, source in sources.items():
        if file.startswith('src/'):
            for token in re.finditer(r'\b\w+\b', mask_source(source)):
                if token[0] in defined_names:
                    reference_files[token[0]].add(file)
    canonical = {}
    for name, group in by_name.items():
        definitions = [record for record in group if record['definition']]
        if definitions:
            canonical[name] = definitions[0]
        for record in group:
            if record['unspecified']:
                finding('unspecified-parameters', record, record['spelling'])
            if record['knr']:
                finding('knr-definition', record, record['spelling'])
            if definitions and not record['unspecified']:
                expected = (definitions[0]['returns'], definitions[0]['parameters'])
                actual = (record['returns'], record['parameters'])
                if actual != expected:
                    finding('signature-mismatch', record,
                            {'expected': expected, 'actual': actual})
        if (definitions and len(reference_files[name]) > 1
                and not any(r['file'].startswith('include/') for r in group)):
            finding('missing-shared-header', definitions[0], name)

    for file, source in sorted(sources.items()):
        code = mask_source(source)
        spans = {tuple(record['span']) for record in records if record['file'] == file}
        bodies = [(record['body_span'], record['name']) for record in records
                  if record['file'] == file and record['body_span']]
        for match in re.finditer(r'\b(\w+)\s*\(', code):
            name = match[1]
            if name not in canonical or any(start <= match.start() < end for start, end in spans):
                continue
            end = closing_parenthesis(code, match.end() - 1)
            count = len(split_parameters(code[match.end():end]))
            params = canonical[name]['parameters']
            if count != len(params) and '...' not in params:
                caller = next((name for span, name in bodies if span[0] < match.start() < span[1]), None)
                finding('call-arity', {'file': file, 'line': source.count('\n', 0, match.start()) + 1,
                                      'name': name, 'caller': caller},
                        {'expected': len(params), 'actual': count})
        for match in re.finditer(r'\(\*\s*(\w+)(?:\[[^]]*\])?\s*\)\s*\(\s*\)', code):
            finding('unspecified-callback', {'file': file, 'line': source.count('\n', 0, match.start()) + 1,
                                            'name': match[1]}, match[0])
    return {'declarations': records, 'findings': findings,
            'totals': {'declarations': len(records), 'definitions': len(canonical),
                       'findings': len(findings)}}


def finding_key(finding):
    return json.dumps([finding[key] for key in ('kind', 'file', 'name', 'shape')]
                      + [finding.get('caller')], sort_keys=True)


def report(root):
    paths = list((root / 'src').rglob('*.c'))
    # Top-level game headers only; generated SDK and toolchain trees are excluded.
    paths += list((root / 'include').glob('*.h'))
    manifest = root / 'docs/LOCAL_SDK_FILES.json'
    sdk = {item['path'] for item in json.loads(manifest.read_text())['files']} if manifest.exists() else set()
    paths = [path for path in paths if path.relative_to(root).as_posix() not in sdk]
    return audit_sources({path.relative_to(root).as_posix(): path.read_text() for path in paths})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--json', action='store_true')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if not (args.root / 'src').is_dir() or not (args.root / 'include').is_dir():
        parser.error('--root must contain src/ and include/')
    result = report(args.root)
    ledger = args.root / 'docs/INTERFACE_CONTRACTS.json'
    reviewed = json.loads(ledger.read_text())['exceptions'] if ledger.exists() else []
    accepted = {finding_key(item): item for item in reviewed}
    found = {finding_key(item) for item in result['findings']}
    unreviewed = [item for item in result['findings'] if finding_key(item) not in accepted]
    stale = [item for key, item in accepted.items() if key not in found]
    incomplete = [item for item in reviewed if not item.get('reason') or not item.get('next_evidence')]
    result['unreviewed'] = unreviewed
    result['stale_exceptions'] = stale
    result['incomplete_exceptions'] = incomplete
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(f"Interface audit: {result['totals']['definitions']} defined functions, "
              f"{result['totals']['declarations']} declarations; "
              f"{len(found)} findings, {len(unreviewed)} unreviewed, {len(stale)} stale exceptions")
        for item in unreviewed:
            print(f"{item['file']}:{item['line']}: {item['kind']}: {item['name']} {item['shape']}")
        for item in stale:
            print(f"{item['file']}: stale exception: {item['name']}")
        for item in incomplete:
            print(f"{item['file']}: missing review reason or next evidence: {item['name']}")
    if args.check and (unreviewed or stale or incomplete):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
