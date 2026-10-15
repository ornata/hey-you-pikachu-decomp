"""Authored nested frames test caller-local protection in original VM execution."""


def experiments():
    """Contrast frame rebinding with an explicit cross-frame write to the caller."""
    from .cases import RAM, word

    value = lambda number: b'\0' + word(number)
    push = lambda number: bytes([24]) + value(number)
    frame = lambda number: bytes([32]) + value(0) + value(1) + value(number)
    store = bytes([10]) + value(1) + value(0)
    load = bytes([25]) + value(1) + value(0)
    prefix = frame(1) + bytes([27]) + value(0) + store
    tail = bytes([31]) + load + bytes([30])
    child_offset = 7 + len(prefix) + 6 + len(tail)
    continuation = 7 + len(prefix) + 6
    outer = prefix + bytes([26]) + value(child_offset) + tail
    cases = []
    for child_frame, result in ((1, 7), (2, 999)):
        child = frame(child_frame) + push(999) + store + push(0) + bytes([30])
        program = bytes([26]) + value(7) + b'\xff' + outer + child
        actor = bytearray(b'\xa5' * 0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(b'\xa5' * 0x408)
        state[0x1B8:0x1BC] = word(RAM + 0x2000)
        vm = bytearray(b'\xa5' * 0x2C8)
        for offset, number in [(4, RAM + 0x3000), (0x14, 0), (0x18, 0), (0x1C, 24),
                               (0x20, RAM + 0x4000), (0x24, 0), (0x128, 0),
                               (0x22C, 0), (0x230, 0), (0x234, 0), (0x2AC, RAM + 0x5000)]:
            vm[offset:offset + 4] = word(number)
        wanted_vm = bytearray(vm)
        for offset, number in [(0x14, 7), (0x18, 1), (0x28, 0), (0x2C, 3),
                               (0x12C, 6), (0x130, continuation)]:
            wanted_vm[offset:offset + 4] = word(number)
        values = b'\xa5' * 120
        wanted_values = b''.join(value(number) for number in (result, 0, 1, result, 0, child_frame, 0)) + values[35:]
        globals_before = word(11) + word(12) + word(7)
        patches = [(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)), (RAM + 0x2000, bytes(vm)),
                   (RAM + 0x3000, program), (RAM + 0x4000, values),
                   (RAM + 0x5000, word(0x8011D610)), (0x800AD43C, globals_before)]
        cases.append(dict(id=f'dictionary.frame-shadow.{child_frame}', symbol='actorRunScriptVm', args=[RAM],
                          patches=patches, observations=[(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)),
                                                       (RAM + 0x2000, bytes(wanted_vm)), (RAM + 0x4000, wanted_values),
                                                       patches[3], *patches[5:]], expected_return=5,
                          limitations=['Authored nested program and requested-ID native table; invalid opcode bounds execution. Checks frame rebinding/restoration and direct caller write, not retail reachability or arbitrary native alias effects.']))
    return cases
