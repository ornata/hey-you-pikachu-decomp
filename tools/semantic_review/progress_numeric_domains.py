"""Predeclared saved-field encoding boundaries and byte write footprints."""

def experiments():
    """Use retail field types with synthetic, aligned storage locations."""
    from .cases import word

    cases = []
    # The authenticated retail catalog at8009A754 and limits at8009A6F4:
    # (encoding, first field, bit width, minimum, maximum, signed load).
    # Encoding1 (signed halfword) has no field among the retained244 entries.
    formats = [
        (0, 1, 32, -2147483648, 2147483647, True),
        (2, 6, 16, 0, 65535, False),
        (3, 0, 8, -128, 127, True),
        (4, 2, 8, 0, 255, False),
        (5, 10, 4, 0, 15, False),
        (6, 12, 2, 0, 3, False),
        (7, 7, 1, 0, 1, False),
    ]
    for encoding, field, width, minimum, maximum, signed in formats:
        shifts = [0, 8 - width] if width < 8 else [0]
        for shift in shifts:
            layout = word(8) + word(shift)
            layout_address = 0x800F1D30 + field * 8
            for value in sorted(set([minimum, maximum, 0])):
                record = bytearray(b"\xa5" * 116)
                if width >= 8:
                    record[8 : 8 + width // 8] = value.to_bytes(
                        width // 8, "big", signed=signed
                    )
                else:
                    mask = maximum << shift
                    record[8] = (record[8] & ~mask) | (value << shift)
                patches = [
                    (layout_address, layout),
                    (0x800F24D0, bytes(record)),
                ]
                cases.append(
                    dict(
                        id=f"numeric.progress.read.{encoding}.{shift}.{value}",
                        symbol="progressReadFieldValue",
                        args=[field],
                        patches=patches,
                        observations=patches,
                        expected_return=value,
                        scope="retail encoding with synthetic aligned field layout",
                        limitations=[
                            "Does not validate the full retail layout allocator or EEPROM serialization."
                        ],
                    )
                )
            values = [minimum, maximum, 0]
            if minimum > -2147483648:
                values.append(minimum - 1)
            if maximum < 2147483647:
                values.append(maximum + 1)
            for value in sorted(set(values)):
                stored = min(maximum, max(minimum, value))
                record = b"\xa5" * 116
                expected = bytearray(record)
                if width >= 8:
                    expected[8 : 8 + width // 8] = stored.to_bytes(
                        width // 8, "big", signed=signed
                    )
                else:
                    mask = maximum << shift
                    expected[8] = (expected[8] & ~mask) | (stored << shift)
                cases.append(
                    dict(
                        id=f"numeric.progress.store.{encoding}.{shift}.{value}",
                        symbol="progressStoreFieldValue",
                        args=[field, value],
                        patches=[
                            (layout_address, layout),
                            (0x800F24D0, record),
                        ],
                        observations=[
                            (layout_address, layout),
                            (0x800F24D0, bytes(expected)),
                        ],
                        expected_return=stored,
                        scope="retail encoding with synthetic aligned field layout",
                        limitations=[
                            "Checks the complete116-byte record footprint; physical persistence is outside this case."
                        ],
                    )
                )
    return cases
