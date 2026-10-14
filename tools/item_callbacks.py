#!/usr/bin/env python3
"""Validate retained item-kind and behavior-mode callback targets."""
import argparse
import json
from pathlib import Path
import struct

from compare_objects import Elf
from interface_contracts import report as interface_report

ROOT = Path(__file__).resolve().parents[1]
TABLES = {
    'gItemKinds': (166, 0x74, {
        0x44: ('hold', 'void', ['u8*']),
        0x48: ('release', 'void', ['u8*', 's32']),
        0x4C: ('launch', 'void', ['u8*']),
        0x50: ('collide', 's32', ['u8*', 'u16'])}),
    'gItemModeUpdateHandlers': (7, 4, {0: ('update', 'void', ['u8*'])})}


def audit_table(data, count, stride, slots, names, definitions, require_all=False):
    """Check an exact table extent and every populated callback slot."""
    if count <= 0 or stride <= 0 or len(data) != count * stride:
        raise ValueError('item table extent does not match the audited records')
    if any(offset < 0 or offset % 4 or offset + 4 > stride for offset in slots):
        raise ValueError('callback slot is outside its record or unaligned')
    entries, errors = [], []
    for index in range(count):
        for offset, (slot, returns, parameters) in slots.items():
            address = struct.unpack_from('>I', data, index * stride + offset)[0]
            if not address:
                if require_all:
                    errors.append(f'entry {index} {slot}: required callback is null')
                continue
            entry = {'index': index, 'slot': slot, 'address': f'0x{address:08X}'}
            candidates = [name for name in names.get(address, []) if name in definitions]
            if len(candidates) != 1:
                errors.append(f'entry {index} {slot}: expected one written C definition')
            else:
                name = candidates[0]
                definition = definitions[name]
                entry.update(function=name, **{key: definition[key] for key in
                                               ('file', 'line', 'returns', 'parameters')})
                if definition['returns'] != returns or definition['parameters'] != parameters:
                    errors.append(f'entry {index} {slot} ({name}): incompatible callback signature')
            entries.append(entry)
    return {'entries': entries, 'errors': errors}


def report(root, path):
    """Read the explicitly audited table prefixes, independent of symbol extents."""
    elf = Elf(path)
    symbols = list(elf.symbols(elf.names.index('.symtab')))
    names = {}
    for symbol in symbols:
        if symbol[3] & 15 == 2:
            names.setdefault(symbol[1], []).append(symbol[0])
    definitions = {item['name']: item for item in interface_report(root)['declarations']
                   if item['definition']}
    entries, errors = [], []
    for table, (count, stride, slots) in TABLES.items():
        symbol = next(item for item in symbols if item[0] == table)
        if symbol[5] >= len(elf.sections):
            raise ValueError(f'{table}: table has no linked section')
        offset = symbol[1] - elf.sections[symbol[5]][3]
        data = elf.data(symbol[5])
        if offset < 0 or offset + count * stride > len(data):
            raise ValueError(f'{table}: table exceeds linked section')
        result = audit_table(data[offset:offset + count * stride], count, stride, slots, names, definitions,
                             require_all=table == 'gItemModeUpdateHandlers')
        entries.extend(dict(table=table, **item) for item in result['entries'])
        errors.extend(f'{table}: {error}' for error in result['errors'])
    return {'entries': entries, 'errors': errors,
            'scope': 'All 166 retained item kinds and seven behavior modes; excludes alternate tables, resource validity and callback side effects.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, default=ROOT / 'build/pikachu.elf')
    parser.add_argument('--json', action='store_true')
    args = parser.parse_args()
    result = report(ROOT, args.elf)
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        for error in result['errors']:
            print('ERROR: ' + error)
        if not result['errors']:
            print(f"OK: {len(result['entries'])} item callback slots agree across 166 kinds and seven modes")
    return bool(result['errors'])


if __name__ == '__main__':
    raise SystemExit(main())
