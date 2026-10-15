"""Numeric controller identifiers and empty-voice channel write contracts."""

import struct


def experiments():
    """Check controller storage without claiming audible or voice-lifecycle effects."""
    from .cases import RAM, STACK, word

    cases = []
    # Original8000692C and channel stride0x1C: bank/pan/expression are14-bit
    # coarse/fine values. RPN0 alone accepts data entry as bend sensitivity.
    for controller, value, high, low, writes in [
        (0, 127, 0, 0, [(0x10, 2, 0x3F80)]),
        (32, 5, 0, 0, [(0x10, 2, 0x1205)]),
        (1, 73, 0, 0, [(0x17, 1, 73), (0x18, 1, 73)]),
        (5, 33, 0, 0, [(0x16, 1, 33)]),
        (6, 12, 0, 0, [(6, 2, 0x600)]),
        (38, 34, 0, 0, [(6, 2, 0x1222)]),
        (6, 12, 1, 0, []), (38, 34, 0, 1, []),
        (7, 90, 0, 0, [(0xB, 1, 90)]),
        (10, 100, 0, 0, [(8, 2, 0x3200)]),
        (42, 14, 0, 0, [(8, 2, 0x120E)]),
        (11, 99, 0, 0, [(0xE, 2, 0x3180)]),
        (64, 0, 0, 0, [(0xD, 1, 0)]),
        (64, 63, 0, 0, [(0xD, 1, 63)]),
        (64, 64, 0, 0, [(0xD, 1, 64)]),
        (65, 1, 0, 0, [(0x15, 1, 1)]),
        (84, 77, 0, 0, [(0x14, 1, 77)]),
        (91, 127, 0, 0, [(0xC, 1, 127)]),
        (100, 9, 0, 0, [(0x13, 1, 9)]),
        (101, 10, 0, 0, [(0x12, 1, 10)]),
        (120, 0, 0, 0, []), (123, 0, 0, 0, []), (99, 127, 0, 0, []),
    ]:
        sequence = bytearray(b"\xa5" * 0x200)
        sequence[0x10:0x14] = word(0)
        channel = 0x14 + 3 * 0x1C
        for offset in [6, 8, 0xE, 0x10]:
            sequence[channel + offset : channel + offset + 2] = struct.pack(">H", 0x1234)
        sequence[channel + 0x12 : channel + 0x14] = bytes([high, low])
        sequence[channel + 0xD] = 64
        expected = bytearray(sequence)
        for offset, width, stored in writes:
            expected[channel + offset : channel + offset + width] = stored.to_bytes(width, "big")
        driver = (RAM + 0x400, b"\0" * 0x28)
        cases.append(dict(
            id=f"numeric.midi.controller.{controller}.{value}.{high}.{low}",
            symbol="sequenceEngineControlChange", args=[driver[0], RAM, 3, controller],
            patches=[(RAM, bytes(sequence)), driver, (STACK + 0x10, word(value))],
            observations=[(RAM, bytes(expected)), driver, (STACK + 0x10, word(value))], expected_return=None,
            scope="controller write footprint with an empty voice list",
            limitations=[
                "Does not exercise active voices, hardware synthesis, audible output or controller standards conformance.",
                "Portamento value1 is retained unchanged; identifier names do not add switch-value normalization."
            ],
        ))
    return cases
