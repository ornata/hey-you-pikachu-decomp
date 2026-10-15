"""Numeric runtime requests retain raw modes and account time before dispatch."""


def experiments():
    """Check request writes independently of graphics, menu and EEPROM behavior."""
    from .cases import word

    cases = []
    for mode in [-1, *range(20)]:
        milliseconds = 0x100000005 if mode == 18 else 1234
        record = bytearray(b"\xa5" * 116)
        record[8:12] = word(200)
        expected = bytearray(record)
        expected[8:12] = word(204)
        before = word(99) + word(0x77777777) + word(0x55555555) + word(0xA5A5A5A5)
        after = word(99) + word(mode) + word(milliseconds) + word(0xA5A5A5A5)
        unchanged = [
            (0x80063830, milliseconds.to_bytes(8, "big")),
            (0x80063838, (104).to_bytes(8, "big")),
            (0x800F1D50, word(8) + word(0)),
        ]
        cases.append(dict(
            id=f"numeric.runtime.request.{mode}",
            symbol="runtimeRequestGameModeChange", args=[mode],
            patches=unchanged + [
                (0x800AE500, before), (0x800B6770, word(99)),
                (0x800B66F8, word(99)), (0x800B66E8, (100).to_bytes(8, "big")),
                (0x800F24D0, bytes(record)),
            ],
            observations=unchanged + [
                (0x800AE500, after), (0x800B6770, word(1)),
                (0x800B66F8, word(0)), (0x800B66E8, (104).to_bytes(8, "big")),
                (0x800F24D0, bytes(expected)),
            ],
            expected_return=None,
            scope="raw mode request and time accounting with synthetic saved-field layout",
            limitations=[
                "Does not establish dispatcher side effects, rendering, save persistence or reachability of raw modes.",
                "The extended millisecond value tests low-word timestamp storage, not a real long-running session."
            ],
        ))
    return cases
