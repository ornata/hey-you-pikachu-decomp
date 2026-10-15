"""Predeclared numeric-domain claims against original VM instructions.

Fixtures intentionally use numeric byte values, not recovered C enum definitions.
They test bounded operator behavior, not gameplay reachability or an R4300 proof.
"""

import struct


def experiments():
    """Exercise operator identity, operand order and selected valid boundaries."""
    from .cases import RAM, word

    cases = []

    def add(op, left, right, expected, *, tag=0, label="integer"):
        unary = right is None
        vm = bytearray(b"\xa5" * 0x2C8)
        vm[0x18:0x1C] = word(1 if unary else 2)
        vm[0x1C:0x20] = word(8)
        vm[0x20:0x24] = word(RAM + 0x1000)
        values = bytearray(b"\xa5" * 64)

        def packed(value):
            return bytes([tag]) + (
                struct.pack(">f", value) if tag == 1 else word(value)
            )

        values[:5] = packed(left)
        if not unary:
            values[5:10] = packed(right)
        wanted_vm = bytearray(vm)
        wanted_vm[0x18:0x1C] = word(1)
        wanted_values = bytearray(values)
        wanted_values[:5] = b"\0" + word(expected)
        symbol = (
            "scriptVmApplyUnaryOperator"
            if unary
            else (
                "scriptVmApplyArithmeticOperator"
                if 5 <= op <= 9
                else (
                    "scriptVmApplyBitwiseOperator"
                    if 20 <= op <= 23
                    else "scriptVmApplyComparisonOperator"
                )
            )
        )
        cases.append(
            dict(
                id=f"numeric.vm.{label}.{op}.{left}.{right}",
                symbol=symbol,
                args=[RAM, op],
                patches=[(RAM, bytes(vm)), (RAM + 0x1000, bytes(values))],
                observations=[
                    (RAM, bytes(wanted_vm)),
                    (RAM + 0x1000, bytes(wanted_values)),
                ],
                expected_return=None,
                global_patches=[],
            )
        )

    # Ghidra80115724: 3 negates, 4 complements and 17 normalizes logical NOT.
    for op, value, expected in [
        (3, 7, -7),
        (3, -7, 7),
        (4, 0, -1),
        (4, 0x13579BDF, 0xECA86420),
        (17, 0, 1),
        (17, 2, 0),
    ]:
        add(op, value, None, expected)
    # Ghidra801158D0: second-from-top is the left operand; division truncates to zero.
    for op, left, right, expected in [
        (5, 7, 3, 10),
        (6, 7, 3, 4),
        (7, 7, 3, 21),
        (8, 7, 3, 2),
        (9, 7, 3, 1),
        (8, -7, 3, -2),
        (9, -7, 3, -1),
    ]:
        add(op, left, right, expected)
    # Ghidra80115B38: arithmetic signed right shift; valid C shift-count boundaries.
    for op, left, right, expected in [
        (20, 0x55, 0x0F, 5),
        (21, 0x55, 0x0F, 0x5F),
        (22, 256, 4, 16),
        (22, -256, 4, -16),
        (23, 17, 3, 136),
        (22, 17, 0, 17),
        (22, 0, 31, 0),
        (23, 1, 30, 0x40000000),
        (23, 0, 31, 0),
    ]:
        add(op, left, right, expected)
    # Ghidra80115CBC: 13/15 are greater-than/greater-or-equal; 14/16 are less-than/less-or-equal.
    expectations = {
        (7, 3): [0, 1, 1, 0, 1, 0],
        (3, 7): [0, 1, 0, 1, 0, 1],
        (7, 7): [1, 0, 0, 0, 1, 1],
    }
    for (left, right), results in expectations.items():
        for op, expected in zip(range(11, 17), results):
            add(op, left, right, expected)
    for op, left, right, expected in [
        (18, 7, 3, 1),
        (18, 0, 3, 0),
        (19, 0, 0, 0),
        (19, 0, 3, 1),
    ]:
        add(op, left, right, expected)
    # Exact binary fractions on either side of the retained .001F absolute tolerance.
    # These are interpreter checks; integer ISA probes do not cross-validate FPU behavior.
    for op, right, expected in [
        (11, 1 / 2048, 1),
        (11, 1 / 512, 0),
        (12, 1 / 2048, 0),
        (12, 1 / 512, 1),
    ]:
        add(op, 0.0, right, expected, tag=1, label="float-equality")
    return cases
