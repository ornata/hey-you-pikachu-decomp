#!/usr/bin/env python3
"""Slice the raw (not-yet-decompiled) segments out of the base ROM.

The ROM is one flat address space; the linker script reassembles it
from three kinds of objects:

   ROM offset   content                  built from
   ----------   ----------------------   -------------------------
   0x000000     header + boot (IPL3)     build/asm/header.s, build/assets/boot.bin
   0x001000     game code (~300 KB)      build/asm/*.s + src/*.c   <- the decomp
   0x04B790     assets (~15.7 MB)        build/assets/4B790.bin
   0x1000000    end of 16 MB ROM

The two .bin slices are data we have not split further yet (the boot
slice is Nintendo's IPL3 bootcode; the big slice holds graphics,
audio, and the voice-recognition data). They are carried as opaque
bytes so the final image can still match bit-for-bit.
"""
import pathlib
import sys

SLICES = {
    "boot.bin": (0x40, 0x1000),
    "4B790.bin": (0x4B790, 0x1000000),
}


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit(f"usage: {sys.argv[0]} <baserom.z64> <assets dir>")
    rom = open(sys.argv[1], "rb").read()
    out = pathlib.Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    for name, (start, end) in SLICES.items():
        (out / name).write_bytes(rom[start:end])
        print(f"ok: {name} [{start:#x}:{end:#x}] ({end - start} bytes)")


if __name__ == "__main__":
    main()
