#!/usr/bin/env python3
"""Demote conflicting NON_MATCHING object aliases in a resident analysis ELF.

Use a separate output file for Ghidra; never use it as a build/ROM input.
The assembly processor's function-marker objects can prevent Ghidra from
decoding real functions at the same addresses. Preserve the marker names but
make them zero-size NOTYPE symbols. Loaded bytes must remain unchanged.
"""

import argparse
import json
from pathlib import Path
import re
import struct


def prepare(raw):
    if raw[:6] != b"\x7fELF\x01\x02" or struct.unpack_from(">H", raw, 18)[0] != 8:
        raise ValueError("expected a big-endian ELF32 MIPS analysis input")
    data = bytearray(raw)
    shoff = struct.unpack_from(">I", raw, 32)[0]
    shentsize, shnum, shstrndx = struct.unpack_from(">HHH", raw, 46)
    sections = [struct.unpack_from(">10I", raw, shoff + i * shentsize) for i in range(shnum)]

    def contents(section):
        return raw[section[4]:section[4] + section[5]]

    def cstr(blob, offset):
        return blob[offset:blob.index(b"\0", offset)].decode()

    names = contents(sections[shstrndx])
    section_names = [cstr(names, section[0]) for section in sections]
    changes = []
    for section in sections:
        if section[1] != 2:  # SHT_SYMTAB
            continue
        strings = contents(sections[section[6]])
        symbols = []
        for offset in range(section[4], section[4] + section[5], section[9]):
            name, value, size, info, other, index = struct.unpack_from(">IIIBBH", raw, offset)
            symbols.append((offset, cstr(strings, name), value, info, index))
        functions = {(name, value, index) for _, name, value, info, index in symbols if info & 15 == 2}
        for offset, name, value, info, index in symbols:
            match = re.fullmatch(r"(func_[0-9A-Fa-f]{8})\.NON_MATCHING", name)
            if not match or info & 15 != 1 or index >= shnum:
                continue
            if section_names[index] not in (".boot", ".ovl"):
                continue
            if (match[1], value, index) not in functions:
                continue
            data[offset + 12] = info & 0xF0  # Retain binding, set STT_NOTYPE.
            struct.pack_into(">I", data, offset + 8, 0)
            changes.append({"name": name, "address": hex(value), "section": section_names[index]})

    phoff = struct.unpack_from(">I", raw, 28)[0]
    phentsize, phnum = struct.unpack_from(">HH", raw, 42)
    for i in range(phnum):
        header = struct.unpack_from(">8I", raw, phoff + i * phentsize)
        if header[0] == 1 and raw[header[1]:header[1] + header[4]] != data[header[1]:header[1] + header[4]]:
            raise ValueError("symbol edits overlap a load segment")
    for section in sections:
        if section[2] & 2 and section[1] != 8:
            if contents(section) != data[section[4]:section[4] + section[5]]:
                raise ValueError("symbol edits overlap an allocated section")
    return data, changes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.source.resolve() == args.output.resolve():
        parser.error("source and analysis output must be different files")
    data, changes = prepare(args.source.read_bytes())
    args.output.write_bytes(data)
    print(json.dumps({"output": str(args.output), "symbols_changed": len(changes),
                      "load_bytes_unchanged": True, "changes": changes}, indent=2))


if __name__ == "__main__":
    main()
