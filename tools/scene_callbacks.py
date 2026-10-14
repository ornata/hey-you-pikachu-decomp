#!/usr/bin/env python3
"""Validate the retained runtime program and per-overlay scene entry callbacks."""
import argparse
import json
from pathlib import Path
import re
import struct

from compare_objects import Elf
from interface_contracts import mask_source, report as interface_report
from item_callbacks import audit_table

ROOT = Path(__file__).resolve().parents[1]
RUNTIME = ('gRuntimeProgramUpdateEntries', 'gRuntimeProgramDrawEntries',
           'gRuntimeProgramInitializeEntries')


def configuration_locations(source):
    """Read the explicit 23-entry scene catalog, rejecting unfamiliar expressions."""
    code = mask_source(source)
    table = re.search(r'\bgAreaConfigurations\[23\]\s*=\s*\{([^}]+)\}', code)
    if not table:
        raise ValueError('expected the retained 23-entry scene configuration catalog')
    entries = [entry.strip() for entry in table[1].split(',') if entry.strip()]
    locations = []
    for entry in entries:
        match = re.fullmatch(r'\(SceneConfigurationPrefix\s*\*\)\s*\((scene_\w+_DATA_START)\s*\+\s*(0x[\dA-Fa-f]+)\)', entry)
        if not match:
            raise ValueError('unrecognized scene configuration expression')
        locations.append((match[1], int(match[2], 16)))
    if len(locations) != 23 or len({base for base, _ in locations}) != 23:
        raise ValueError('expected 23 distinct scene overlay configurations')
    return locations


def section_bytes(elf, symbol, offset, length):
    """Read a bounded view in the symbol's own overlay section."""
    if symbol[5] >= len(elf.sections):
        raise ValueError('callback resource has no linked section')
    start = symbol[1] - elf.sections[symbol[5]][3] + offset
    data = elf.data(symbol[5])
    if start < 0 or start + length > len(data):
        raise ValueError('callback resource exceeds its linked section')
    return data[start:start + length]


def report(root, path):
    """Resolve scene targets within each overlay, never by virtual address alone."""
    elf = Elf(path)
    symbols = list(elf.symbols(elf.names.index('.symtab')))
    by_name = {symbol[0]: symbol for symbol in symbols}
    names_by_section = {}
    for symbol in symbols:
        if symbol[3] & 15 == 2:
            names_by_section.setdefault(symbol[5], {}).setdefault(symbol[1], []).append(symbol[0])
    definitions = {item['name']: item for item in interface_report(root)['declarations'] if item['definition']}
    entries, errors = [], []

    def check(label, data, stride, slots, section, required=False):
        result = audit_table(data, 1, stride, slots, names_by_section.get(section, {}), definitions, required)
        entries.extend(dict(resource=label, **item) for item in result['entries'])
        errors.extend(f'{label}: {error}' for error in result['errors'])

    # Only program zero exists in the retained table. Symbol extents are interior views.
    for name in RUNTIME:
        symbol = by_name[name]
        check(name, section_bytes(elf, symbol, 0, 4), 4,
              {0: ('callback', 'void', [])}, symbol[5])
    locations = configuration_locations((root / 'src/ovl/area_terrain_maze.c').read_text())
    catalog = section_bytes(elf, by_name['gAreaConfigurations'], 0, 23 * 4)
    for index, (base, offset) in enumerate(locations):
        symbol = by_name[base]
        address = struct.unpack_from('>I', catalog, index * 4)[0]
        if address != symbol[1] + offset:
            raise ValueError(f'{base}: linked catalog disagrees with source overlay selection')
        data = section_bytes(elf, symbol, offset, 0x7C)
        # The declaration inventory canonicalizes f32 to float.
        check(base, data, 0x7C, {0x3C: ('sampleCell', 's32', ['float'] * 3)}, symbol[5], True)
        check(base, data, 0x7C, {0x40: ('initializeScene', 'void', [])}, symbol[5])
    return {'entries': entries, 'errors': errors,
            'scope': 'Retained runtime program zero and sampler/initializer slots of all 23 scene configurations. Excludes cell lifecycle/event tables, resource validity and alternate programs.'}


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
            print(f"OK: {len(result['entries'])} runtime/scene entry callbacks agree across program zero and 23 overlays")
    return bool(result['errors'])


if __name__ == '__main__':
    raise SystemExit(main())
