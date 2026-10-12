#!/usr/bin/env python3
"""Tag an IDO -mips3 -32 object as o32 so GNU ld links it with -mips2 code.

IDO's -32 MIPS III objects carry no ABI tag, and GNU ld then refuses to mix
them with 32-bit objects. Only the ELF header e_flags word changes; no section
or instruction byte is touched.
"""
import struct
import sys
from pathlib import Path

EF_MIPS_ABI_O32 = 0x1000
EF_MIPS_32BITMODE = 0x100
EF_MIPS_ARCH = 0xF0000000
EF_MIPS_ARCH_3 = 0x20000000


def set_o32_abi(path):
    data = bytearray(path.read_bytes())
    if data[:6] != b"\x7fELF\x01\x02" or struct.unpack_from(">H", data, 18)[0] != 8:
        raise ValueError(f"{path}: expected a big-endian ELF32 MIPS object")
    flags = struct.unpack_from(">I", data, 36)[0]
    if flags & EF_MIPS_ARCH != EF_MIPS_ARCH_3:
        raise ValueError(f"{path}: expected a MIPS III object")
    struct.pack_into(">I", data, 36, flags | EF_MIPS_ABI_O32 | EF_MIPS_32BITMODE)
    path.write_bytes(data)


if __name__ == "__main__":
    for name in sys.argv[1:]:
        set_o32_abi(Path(name))
