#!/usr/bin/env python3
"""Compare matching-build ELF objects, ignoring debug data and symbol spelling.

Allocated bytes and layout, defined symbol attributes, and relocation identities
must match. Undefined symbols are resolved through each build's pikachu.elf.
Relocation addends remain strict: a record-plus-offset rewrite may require review
and a new baseline even when the final ROM matches. This is not a ROM gate.
"""

import argparse
from pathlib import Path
import struct
import sys


class Elf:
    """Read the ELF32 big-endian MIPS format produced by this build."""

    def __init__(self, path):
        self.blob = path.read_bytes()
        self.header = struct.unpack_from('>16sHHIIIIIHHHHHH', self.blob)
        h = self.header
        if h[0][:7] != b'\x7fELF\x01\x02\x01' or h[2] != 8 or h[11] != 40:
            raise ValueError(f'{path}: expected ELF32 big-endian MIPS')
        self.sections = [struct.unpack_from('>10I', self.blob, h[6] + i * h[11]) for i in range(h[12])]
        strings = self.data(h[13])
        self.names = [self.string(strings, section[0]) for section in self.sections]

    @staticmethod
    def string(blob, offset):
        return blob[offset:blob.index(b'\0', offset)].decode()

    def data(self, index):
        section = self.sections[index]
        offset, size = section[4:6]
        if offset + size > len(self.blob):
            raise ValueError('truncated ELF section')
        return self.blob[offset:offset + size]

    def symbols(self, index):
        section = self.sections[index]
        if section[9] != 16:
            raise ValueError('unsupported symbol entry size')
        strings = self.data(section[6])
        for name, value, size, info, other, shndx in struct.iter_unpack('>IIIBBH', self.data(index)):
            yield self.string(strings, name), value, size, info, other, shndx


def global_addresses(path):
    elf = Elf(path)
    if elf.header[1] != 2:
        raise ValueError(f'{path}: expected linked executable')
    result = {}
    for index, section in enumerate(elf.sections):
        if section[1] == 2:
            for name, value, _, info, _, shndx in elf.symbols(index):
                if shndx and info >> 4 in (1, 2):
                    if name in result and result[name] != value:
                        raise ValueError(f'ambiguous linked symbol: {name}')
                    result[name] = value
    return result


def normalized(path, addresses):
    elf = Elf(path)
    h, sections, names = elf.header, elf.sections, elf.names
    if h[1] != 1:
        raise ValueError(f'{path}: expected relocatable object')
    allocated = {}
    for index, section in enumerate(sections):
        if section[2] & 2:
            name = names[index]
            if name in allocated:
                raise ValueError(f'duplicate allocated section: {name}')
            allocated[name] = (section[1], section[2], section[3], section[5], section[8], section[9],
                               None if section[1] == 8 else elf.data(index))
    symbols, defined = {}, []
    for index, section in enumerate(sections):
        if section[1] != 2:
            continue
        identities = []
        for name, value, size, info, other, shndx in elf.symbols(index):
            if shndx == 0:
                identity = ('extern', addresses.get(name, name))
            elif shndx < len(sections):
                identity = ('section', names[shndx], value)
            else:
                identity = ('special', shndx, value)
            identities.append(identity)
            if shndx and (shndx >= len(sections) or sections[shndx][2] & 2):
                defined.append((identity, size, info, other))
        symbols[index] = identities
    relocations = {}
    for index, section in enumerate(sections):
        if section[1] not in (4, 9) or not sections[section[7]][2] & 2:
            continue
        if section[1] == 4 or section[9] != 8:
            raise ValueError('unsupported relocation format')
        entries = [(offset, info & 255, symbols[section[6]][info >> 8])
                   for offset, info in struct.iter_unpack('>II', elf.data(index))]
        relocations.setdefault(names[section[7]], []).extend(entries)
    return {'ELF attributes': (h[1], h[2], h[3], h[7]), 'allocated sections': allocated,
            'defined symbols': sorted(defined, key=repr), 'relocations': relocations}


def compare(baseline, current):
    if baseline.resolve() == current.resolve():
        raise ValueError('baseline and current must be different build directories')
    old_files = {p.relative_to(baseline) for p in baseline.rglob('*.o')}
    new_files = {p.relative_to(current) for p in current.rglob('*.o')}
    if not old_files or not new_files:
        raise ValueError('both build directories must contain objects')
    differences = []
    for path in sorted(old_files - new_files):
        differences.append(f'{path}: missing object')
    for path in sorted(new_files - old_files):
        differences.append(f'{path}: extra object')
    old_addresses = global_addresses(baseline / 'pikachu.elf')
    new_addresses = global_addresses(current / 'pikachu.elf')
    for path in sorted(old_files & new_files):
        old = normalized(baseline / path, old_addresses)
        new = normalized(current / path, new_addresses)
        differences.extend(f'{path}: {category} differ' for category in old if old[category] != new[category])
    return len(old_files), differences


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path, nargs='?', default=Path('expected/build'))
    parser.add_argument('current', type=Path, nargs='?', default=Path('build'))
    args = parser.parse_args()
    try:
        count, differences = compare(args.baseline, args.current)
    except (OSError, ValueError, IndexError, KeyError, struct.error) as error:
        print(f'object comparison failed: {error}', file=sys.stderr)
        return 1
    if differences:
        print('\n'.join(differences), file=sys.stderr)
        return 1
    print(f'OK: {count} objects match (allocated bytes/layout, defined symbols, relocations)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
