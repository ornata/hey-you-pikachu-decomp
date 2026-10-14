#!/usr/bin/env python3
"""Check lifecycle, drawing and response callback types for the known retail object-kind range."""
import argparse
import json
from pathlib import Path
import struct

from compare_objects import Elf
from interface_contracts import report as interface_report

ROOT = Path(__file__).resolve().parents[1]
CONTRACTS = {0: ('init', 'void', ['u8*']),
             1: ('destroy', 'void', ['u8*']),
             2: ('update', 'void', ['u8*']),
             3: ('draw', 'Gfx*', ['u8*']),
             4: ('drawChild', 'void', ['u8*']),
             5: ('response', 'void', ['u8*', 'ObjectResponse*'])}


def audit_records(records, names, definitions):
    """Check every non-null hook against its written C callback contract."""
    findings = []
    errors = []
    for kind, record in enumerate(records):
        if len(record) != 6:
            raise ValueError('object callback record must contain six words')
        for index, (slot, returns, parameters) in CONTRACTS.items():
            address = record[index]
            if not address:
                continue
            entry = {'kind': kind, 'slot': slot, 'address': f'0x{address:08X}'}
            candidates = [name for name in names.get(address, []) if name in definitions]
            if len(candidates) != 1:
                errors.append(f'kind {kind:#x} {slot}: expected one written C definition')
            else:
                name = candidates[0]
                definition = definitions[name]
                entry.update(function=name, **{key: definition[key] for key in ('file', 'line', 'returns', 'parameters')})
                if definition['returns'] != returns or definition['parameters'] != parameters:
                    errors.append(f'kind {kind:#x} {slot} ({name}): incompatible callback signature')
            findings.append(entry)
    return {'entries': findings, 'errors': errors}


def report(root, path, kind_count):
    """Read the explicit audited prefix of the retained six-word kind table."""
    if kind_count <= 0:
        raise ValueError('kind count must be positive')
    elf = Elf(path)
    symbols = list(elf.symbols(elf.names.index('.symtab')))
    table = next(symbol for symbol in symbols if symbol[0] == 'gObjectKindCallbacks')
    section = table[5]
    if section >= len(elf.sections):
        raise ValueError('object kind table has no allocated section')
    offset = table[1] - elf.sections[section][3]
    data = elf.data(section)
    if offset < 0 or offset + kind_count * 24 > len(data):
        raise ValueError('audited object kind range exceeds its linked section')
    records = list(struct.iter_unpack('>6I', data[offset:offset + kind_count * 24]))
    names = {}
    for symbol in symbols:
        if symbol[3] & 15 == 2:
            names.setdefault(symbol[1], []).append(symbol[0])
    definitions = {item['name']: item for item in interface_report(root)['declarations'] if item['definition']}
    result = audit_records(records, names, definitions)
    result.update(kind_count=kind_count,
                  scope='Lifecycle, drawing, child-drawing and response slots in the explicitly selected retained kind range; excludes alternate tables and resource validity.')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, default=ROOT / 'build/pikachu.elf')
    parser.add_argument('--kind-count', type=lambda value: int(value, 0), default=0xB4)
    parser.add_argument('--json', action='store_true')
    args = parser.parse_args()
    result = report(ROOT, args.elf, args.kind_count)
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        for error in result['errors']:
            print('ERROR: ' + error)
        if not result['errors']:
            print(f"OK: {len(result['entries'])} object callback slots agree across {result['kind_count']} audited kinds")
    return bool(result['errors'])


if __name__ == '__main__':
    raise SystemExit(main())
