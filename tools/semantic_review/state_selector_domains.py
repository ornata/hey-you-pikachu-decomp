"""Bounded observations for unresolved VM value tags, without guessing their origin."""

import struct


def experiments():
    """Contrast integer-tag aliases, decimal parsing and fractional float conversion."""
    from .cases import RAM, word

    cases = []
    for tag in (0, 2):
        for number in (0, -1, -2147483648, 2147483647):
            storage = b"\xa5" + bytes([tag]) + word(number) + b"\xa5" * 10
            for symbol, expected in (("scriptValueGetInteger", number),
                                     ("scriptValueParseDecimal", 0)):
                # Decimal parsing rejects these numeric tags before consulting vm.
                args = [RAM + 1] if symbol == "scriptValueGetInteger" else [0, RAM + 1]
                cases.append(dict(
                    id=f"selector.value.{symbol}.{tag}.{number}", symbol=symbol,
                    args=args, patches=[(RAM, storage)], observations=[(RAM, storage)],
                    expected_return=expected,
                    limitations=["Synthetic tag2 inputs establish consumer behavior, not its source-language meaning or retail use."],
                ))
    for number, expected in ((7.75, 7), (-7.75, -7)):
        storage = b"\xa5\x01" + struct.pack(">f", number) + b"\xa5" * 10
        cases.append(dict(
            id=f"selector.value.float-to-integer.{number}", symbol="scriptValueGetInteger",
            args=[RAM + 1], patches=[(RAM, storage)], observations=[(RAM, storage)],
            expected_return=expected,
        ))
    return cases
