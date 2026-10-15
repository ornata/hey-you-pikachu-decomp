"""Predeclared holder-transfer and actor-flag observations from original instructions."""

import struct


def experiments():
    """Distinguish holder rules, forced release, unrelated flags and 16-bit writes."""
    from .cases import RAM, word

    cases = []

    def add(name, symbol, args, patches, observations, result=None, globals=()):
        cases.append(
            dict(
                id=name,
                symbol=symbol,
                args=args,
                patches=[*patches, *globals],
                observations=observations,
                expected_return=result,
                global_patches=[],
            )
        )

    # Original801144EC: bit30 bypasses specific owner restrictions, not all pickup rules.
    # Rows are owner0..5; columns are no permission/permission, each actor/player queries.
    allowed = [
        ((1, 1), (1, 1)),
        ((0, 1), (1, 1)),
        ((1, 0), (1, 1)),
        ((1, 0), (1, 1)),
        ((0, 0), (0, 0)),
        ((0, 0), (0, 0)),
    ]
    for owner, row in enumerate(allowed):
        for enabled, responses in enumerate(row):
            for player, expected in enumerate(responses):
                obj = bytearray(b"\xa5" * 0x60)
                obj[0x54:0x58] = word(RAM + 0x100)
                state = bytearray(b"\xa5" * 0x6C)
                state[:4] = word(0x40000000 if enabled else 0)
                state[6] = owner
                patches = [(RAM, bytes(obj)), (RAM + 0x100, bytes(state))]
                add(
                    f"numeric.item.transfer.{owner}.{enabled}.{player}",
                    "itemCanBePickedUp",
                    [RAM, player],
                    patches,
                    patches,
                    expected,
                )

    # Unrelated pickup masks continue to reject even with the holder permission set.
    for owner, flags, player in [
        (0, 0x40000080, 0),
        (1, 0x40000080, 0),
        (2, 0x44000040, 1),
    ]:
        obj = bytearray(b"\xa5" * 0x60)
        obj[0x54:0x58] = word(RAM + 0x100)
        state = bytearray(b"\xa5" * 0x6C)
        state[:4] = word(flags)
        state[6] = owner
        patches = [(RAM, bytes(obj)), (RAM + 0x100, bytes(state))]
        add(
            f"numeric.item.blocker.{owner}.{flags:x}.{player}",
            "itemCanBePickedUp",
            [RAM, player],
            patches,
            patches,
            0,
        )

    # Original80114044: a player-held kind0 item has no release callback in the retail catalog.
    # Permission permits an unforced release; forcing bypasses it. Successful release clears bit30.
    for enabled, force, expected in [(0, 0, 0), (1, 0, 1), (0, 1, 1), (1, 1, 1)]:
        obj = bytearray(b"\xa5" * 0x60)
        obj[0x36:0x38] = b"\0\0"
        obj[0x54:0x58] = word(RAM + 0x100)
        state = bytearray(b"\xa5" * 0x6C)
        state[:4] = word(0x20 | (0x40000000 if enabled else 0))
        state[4:8] = b"\0\0\x01\x03"
        state[8:12] = word(0)
        state[0x2C:0x30] = struct.pack(">f", 1.0)
        wanted = bytearray(state)
        if expected:
            wanted[:4] = word(0x20)
            wanted[6:8] = b"\0\0"
            wanted[0x2C:0x30] = word(0)
        patches = [(RAM, bytes(obj)), (RAM + 0x100, bytes(state))]
        observations = [
            (RAM, bytes(obj)),
            (RAM + 0x100, bytes(wanted)),
            (0x800EF724, word(0 if expected else 0x1234)),
        ]
        add(
            f"numeric.item.release.{enabled}.{force}",
            "itemTryRelease",
            [RAM, force],
            patches,
            observations,
            expected,
            [(0x800EF724, word(0x1234))],
        )

    # Ghidra80017190/800171A8: lhu/sh at actor state+1e8 preserve all other bytes.
    for mask in [1, 2, 4, 8, 0x10, 0x20, 0x40, 0x7F]:
        for operation, initial, expected in [
            ("Set", 0xA580, 0xA580 | mask),
            ("Clear", 0xA5FF, 0xA5FF & ~mask),
        ]:
            obj = bytearray(b"\xa5" * 0x60)
            obj[0x54:0x58] = word(RAM + 0x100)
            state = bytearray(b"\xa5" * 0x240)
            state[0x1E8:0x1EA] = struct.pack(">H", initial)
            wanted = bytearray(state)
            wanted[0x1E8:0x1EA] = struct.pack(">H", expected)
            add(
                f"numeric.actor.flags.{operation}.{mask:x}",
                f"actor{operation}BehaviorInhibitFlags",
                [RAM, mask],
                [(RAM, bytes(obj)), (RAM + 0x100, bytes(state))],
                [(RAM, bytes(obj)), (RAM + 0x100, bytes(wanted))],
            )
    return cases
