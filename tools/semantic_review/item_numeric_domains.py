"""Predeclared catalog identities and the nominal live-item availability threshold."""


def experiments():
    """Check original name pointers/text and counts around the nominal limit."""
    from .cases import word

    cases = []
    # Original catalog at80125A40 has a 0x74-byte stride and two name pointers.
    # These short labels are semantic expectations, not executable ROM bytes.
    names = [
        (0x51, 0, 0x8012EA34, b"megaphone\0"),
        (0x51, 1, 0x8012EA40, b"megaphone\0"),
        (0x8A, 0, 0x8012FD24, b"box\0"),
        (0x8A, 1, 0x8012FD28, b"cardboard box\0"),
    ]
    for kind, variant, pointer, text in names:
        cases.append(
            dict(
                id=f"numeric.item.name.{kind:x}.{variant}",
                symbol="itemGetDisplayName",
                args=[kind, variant],
                patches=[],
                observations=[(pointer, text)],
                expected_return=pointer,
            )
        )
    for kind, pointer, text in [
        (0x51, 0x8012EA34, b"megaphone\0"),
        (0x8A, 0x8012FD24, b"box\0"),
    ]:
        cases.append(
            dict(
                id=f"numeric.item.name.{kind:x}.invalid",
                symbol="itemGetDisplayName",
                args=[kind, 2],
                patches=[],
                observations=[(pointer, text)],
                expected_return=0,
            )
        )
    # Original80114FC0 subtracts the unchanged counter at80134040 from60.
    # Count61 is a synthetic out-of-nominal-limit case, not a claim of gameplay reachability.
    for count, expected in [(0, 60), (59, 1), (60, 0), (61, -1)]:
        patches = [(0x80134040, word(count))]
        cases.append(
            dict(
                id=f"numeric.item.capacity.{count}",
                symbol="itemGetAvailableSlots",
                args=[],
                patches=patches,
                observations=patches,
                expected_return=expected,
            )
        )
    return cases
