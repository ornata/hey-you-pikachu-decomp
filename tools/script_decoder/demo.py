"""Newly authored examples; these contain no extracted game script content."""

import struct


def value(number, tag=0):
    return bytes([tag]) + struct.pack('>i', number)


def sample(kind):
    """Return an invented, bounded program demonstrating each functional format."""
    if kind == 'vm':
        # if42<100, invoke native slot1; either path finishes at offset25.
        return bytes([24]) + value(42) + bytes([24]) + value(100) + bytes([14, 29]) + value(25) + bytes([27]) + value(1) + bytes([35])
    if kind == 'cue':
        return (struct.pack('>ififI', 18, 0, 0, 0, 0)
                + struct.pack('>ififI', 25, 0.5, 5, 1.25, 0)
                + struct.pack('>ifIII', 48, 1, 0, 0, 0)
                + struct.pack('>ifIII', 49, 0, 0, 0, 0))
    return struct.pack('>ifii', 0, 0, 123, 18) + struct.pack('>ifii', 4, 2.5, 0, 0)
