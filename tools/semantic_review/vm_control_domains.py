"""Predeclared bytecode cursor, variable, frame and native scheduling observations.

Instruction bytes and native entry addresses are retail encoding facts. Expected
states are declared explicitly rather than read from recovered C enum definitions.
"""


def experiments():
    """Exercise each recovered control-flow family with guarded packed values."""
    from .cases import RAM, word

    def value(number, tag=0):
        return bytes([tag]) + word(number)

    cases = []

    def add(
        name,
        program,
        initial_values,
        final_values,
        cursor,
        depth,
        *,
        configure=None,
        expected=5,
    ):
        actor = bytearray(b"\xa5" * 0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(b"\xa5" * 0x240)
        state[0x1B8:0x1BC] = word(RAM + 0x2000)
        state[0x21C:0x220] = word(0x1234)
        vm = bytearray(b"\xa5" * 0x2C8)
        for offset, number in [
            (4, RAM + 0x3000),
            (0x14, 0),
            (0x18, len(initial_values)),
            (0x1C, 16),
            (0x20, RAM + 0x4000),
            (0x24, 0),
            (0x128, 0),
            (0x22C, 0),
            (0x2AC, RAM + 0x5000),
        ]:
            vm[offset : offset + 4] = word(number)
        wanted_vm = bytearray(vm)
        wanted_vm[0x14:0x18] = word(cursor)
        wanted_vm[0x18:0x1C] = word(depth)
        if configure:
            configure(vm, wanted_vm)
        values = bytearray(b"\xa5" * 96)
        for i, number in enumerate(initial_values):
            values[i * 5 : i * 5 + 5] = value(number)
        wanted_values = bytearray(values)
        for i, number in final_values.items():
            wanted_values[i * 5 : i * 5 + 5] = value(number)
        patches = [
            (RAM, bytes(actor)),
            (RAM + 0x1000, bytes(state)),
            (RAM + 0x2000, bytes(vm)),
            (RAM + 0x3000, program),
            (RAM + 0x4000, bytes(values)),
        ]
        # Original native entries: message type/continue, yield, start action/wait.
        native = {
            "native-continue": 0x8011C8B0,
            "native-yield": 0x8011BEB0,
            "native-wait": 0x8011BEDC,
            "code-reference.native-continue": 0x8011C8B0,
            "code-reference.native-yield": 0x8011BEB0,
            "code-reference.native-wait": 0x8011BEDC,
        }
        if name in native:
            patches.append((RAM + 0x5000, word(native[name])))
        cases.append(
            dict(
                id="numeric.vm.control." + name,
                symbol="actorRunScriptVm",
                args=[RAM],
                patches=patches,
                observations=[
                    (RAM, bytes(actor)),
                    (RAM + 0x1000, bytes(state)),
                    (RAM + 0x2000, bytes(wanted_vm)),
                    (RAM + 0x4000, bytes(wanted_values)),
                ],
                expected_return=expected,
            )
        )

    # Original80116284 consumes each byte0..2 without a stack operation.
    # A following literal distinguishes a one-byte no-op from an operand prefix.
    add("ignored-byte.baseline", b"\xff", [11, 22], {}, 1, 2)
    for code in (0, 1, 2):
        add(f"ignored-byte.{code}", bytes([code, 255]), [11, 22], {}, 2, 2)
        add(f"ignored-byte.before-literal.{code}", bytes([code, 24]) + value(123) + b"\xff",
            [11, 22], {2: 123}, 8, 3)
    add("ignored-byte.mixed", bytes([0, 1, 2, 24]) + value(123) + b"\xff",
        [11, 22], {2: 123}, 10, 3)

    add("jump", bytes([28]) + value(7) + b"\xff\xff", [], {}, 8, 0)
    for condition, cursor in [(0, 8), (1, 7), (-1, 7)]:
        add(
            f"jump-if-zero-{condition}",
            bytes([29]) + value(7) + b"\xff\xff",
            [condition],
            {},
            cursor,
            0,
        )
    reference = value(0) + value(1)
    add("load-variable", bytes([25]) + reference + b"\xff", [11, 22], {2: 22}, 12, 3)
    add(
        "store-variable",
        bytes([10]) + reference + b"\xff",
        [11, 22, 33],
        {1: 33},
        12,
        2,
    )
    reference = value(0) + value(0)
    add(
        "load-element",
        bytes([33]) + reference + b"\xff",
        [11, 22, 33, 1],
        {3: 22},
        12,
        4,
    )
    add(
        "store-element",
        bytes([34]) + reference + b"\xff",
        [11, 22, 33, 1, 44],
        {1: 44},
        12,
        3,
    )

    def frame(initial, final):
        initial[0x22C:0x230] = word(0x1234)
        final[0x22C:0x230] = word(0x1234)
        final[0x28:0x2C] = word(0)
        final[0x12C:0x130] = word(6)

    program = (
        bytes([26])
        + value(7)
        + b"\xff"
        + bytes([32])
        + value(0)
        + value(2)
        + value(0)
        + bytes([24])
        + value(42)
        + bytes([30])
    )
    add(
        "call-frame-return",
        program,
        [],
        {0: 42, 1: 0, 2: 0x1234, 3: 0, 4: 42},
        7,
        1,
        configure=frame,
    )
    program = bytes([27]) + value(0) + b"\xff"
    add("native-continue", program, [], {0: 0x1234}, 7, 1)
    add("native-yield", program, [], {0: 0}, 6, 1, expected=1)

    def wait(initial, final):
        final[0x2C0:0x2C8] = word(2) + word(1)

    add("native-wait", program, [], {}, 6, 0, configure=wait, expected=2)
    # Retail control operands use tag2; expectations remain independent numbers.
    add("code-reference.jump", bytes([28]) + value(7, 2) + b"\xff\xff", [], {}, 8, 0)
    for condition, cursor in ((0, 8), (1, 7)):
        add(f"code-reference.conditional.{condition}", bytes([29]) + value(7, 2) + b"\xff\xff",
            [condition], {}, cursor, 0)
    program = bytes([27]) + value(0, 2) + b"\xff"
    add("code-reference.native-continue", program, [], {0: 0x1234}, 7, 1)
    add("code-reference.native-yield", program, [], {0: 0}, 6, 1, expected=1)
    add("code-reference.native-wait", program, [], {}, 6, 0, configure=wait, expected=2)
    return cases
