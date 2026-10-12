#!/usr/bin/env python3
"""Normalize an N64 ROM dump to big-endian .z64 and verify it.

 .------------------------------------------------------------.
 | N64 dumps come in three byte orders, named by their first  |
 | four bytes (the PI BSD DOM1 config in the ROM header):     |
 |                                                            |
 |   .z64  80 37 12 40   big-endian     (native)              |
 |   .v64  37 80 40 12   16-bit swapped (Doctor V64 dumper)   |
 |   .n64  40 12 37 80   32-bit swapped (little-endian words) |
 '------------------------------------------------------------'

Every tool in this repo consumes big-endian only, so we normalize
once, up front, and verify the SHA-1 so a bad dump fails loudly
here instead of as a mystery diff at the end of the build.
"""
import array
import hashlib
import sys

GOOD_SHA1 = "da1a1e47a86720f9d54fb2d2d247480041bda824"  # Hey You, Pikachu! (USA), big-endian


def to_big_endian(data: bytes) -> bytes:
    magic = data[:4]
    if magic == b"\x80\x37\x12\x40":  # already .z64
        return data
    if magic == b"\x37\x80\x40\x12":  # .v64: swap each u16
        a = array.array("H")
        a.frombytes(data)
        a.byteswap()
        return a.tobytes()
    if magic == b"\x40\x12\x37\x80":  # .n64: swap each u32
        a = array.array("I")
        a.frombytes(data)
        a.byteswap()
        return a.tobytes()
    sys.exit(f"error: unrecognized ROM magic {magic.hex()}; not an N64 dump?")


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit(f"usage: {sys.argv[0]} <input rom> <output .z64>")
    data = to_big_endian(open(sys.argv[1], "rb").read())
    digest = hashlib.sha1(data).hexdigest()
    if digest != GOOD_SHA1:
        sys.exit(
            f"error: SHA-1 mismatch after normalization\n"
            f"  got:      {digest}\n"
            f"  expected: {GOOD_SHA1}\n"
            f"This is not a good dump of Hey You, Pikachu! (USA)."
        )
    open(sys.argv[2], "wb").write(data)
    print(f"ok: wrote {sys.argv[2]} ({len(data)} bytes, sha1 verified)")


if __name__ == "__main__":
    main()
