#!/usr/bin/env python3
"""Check the retained actor native-command table against written C signatures."""
import argparse
import json
from pathlib import Path
import struct

from compare_objects import Elf
from interface_contracts import report as interface_report

ROOT = Path(__file__).resolve().parents[1]
TABLE = 'D_8012B400'


def audit_entries(values, names, definitions):
    """Resolve non-null entries to C definitions and require the actor-only ABI."""
    entries = []
    errors = []
    for selector, address in enumerate(values):
        entry = {'selector': selector, 'address': f'0x{address:08X}'}
        if address == 0:
            entry['function'] = None
            if selector != 0:
                errors.append(f'selector {selector}: unexpected null callback')
        else:
            candidates = [name for name in names.get(address, []) if name in definitions]
            if len(candidates) != 1:
                errors.append(f'selector {selector}: expected one written C definition at {address:#x}')
            else:
                name = candidates[0]
                definition = definitions[name]
                entry.update({key: definition[key] for key in ('file', 'line', 'returns', 'parameters')})
                entry['function'] = name
                if definition['returns'] != 's32' or definition['parameters'] != ['u8*']:
                    errors.append(f'selector {selector} ({name}): incompatible native-command signature')
        entries.append(entry)
    if not values or values[0] != 0:
        errors.append('selector zero must be the reserved null entry')
    return {'entries': entries, 'errors': errors}


def report(root, path):
    """Read the linked table extent and compare every target with game C definitions."""
    elf = Elf(path)
    symbols = list(elf.symbols(elf.names.index('.symtab')))
    table = next(symbol for symbol in symbols if symbol[0] == TABLE)
    address, size, section = table[1], table[2], table[5]
    if not size or size % 4 or section >= len(elf.sections):
        raise ValueError('native-command table has no bounded word-array extent')
    offset = address - elf.sections[section][3]
    data = elf.data(section)
    if offset < 0 or offset + size > len(data):
        raise ValueError('native-command table exceeds its linked section')
    values = struct.unpack(f'>{size // 4}I', data[offset:offset + size])
    names = {}
    for symbol in symbols:
        if symbol[3] & 15 == 2:
            names.setdefault(symbol[1], []).append(symbol[0])
    definitions = {item['name']: item for item in interface_report(root)['declarations'] if item['definition']}
    result = audit_entries(values, names, definitions)
    result.update(table=TABLE, address=f'0x{address:08X}', end=f'0x{address + size:08X}',
                  scope='Retained actor native table and written C signatures only; not arbitrary caller-supplied VM tables or stack effects.')
    return result


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
            print(f"OK: {len(result['entries']) - 1} native handlers agree with the actor-only callback contract")
    return bool(result['errors'])


if __name__ == '__main__':
    raise SystemExit(main())
