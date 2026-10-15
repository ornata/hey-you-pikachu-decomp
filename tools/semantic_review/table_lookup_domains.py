"""Authored guarded lookup programs checked against original interpreter code."""


def experiments():
    """Check guard endpoints, fallback and fractional indices with full stack guards."""
    from .cases import RAM, word

    value = lambda number, tag=0: bytes([tag]) + word(number)
    push = lambda number: bytes([24]) + value(number)
    load = bytes([25]) + value(0) + value(5)
    guard = load + push(7) + bytes([15]) + load + push(9) + bytes([16, 18])
    body = b''.join(push(i) + push(number) + bytes([34]) + value(1) + value(1)
                    for i, number in enumerate((101, 202, 303)))
    body += load + push(7) + bytes([6, 33]) + value(1) + value(1) + b'\xff'
    prefix = guard + bytes([29]) + value(len(guard) + 6 + len(body), 2)
    program = prefix + body + push(-1) + b'\xff'
    cases = []
    for name, bits, tag, chosen in [('below', 6, 0, None), ('first', 7, 0, 101),
                                    ('middle', 8, 0, 202), ('last', 9, 0, 303),
                                    ('above', 10, 0, None), ('fraction', 0x40F80000, 1, 101)]:
        actor = bytearray(b'\xa5' * 0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(b'\xa5' * 0x240)
        state[0x1B8:0x1BC] = word(RAM + 0x2000)
        vm = bytearray(b'\xa5' * 0x2C8)
        for offset, number in [(4, RAM + 0x3000), (0x14, 0), (0x18, 12), (0x1C, 24),
                               (0x20, RAM + 0x4000), (0x24, 0), (0x128, 0),
                               (0x22C, 0), (0x230, 8)]:
            vm[offset:offset + 4] = word(number)
        stack = bytearray(value(0) * 12 + b'\xa5' * 60)
        stack[25:30] = value(bits, tag)
        wanted_stack = bytearray(stack)
        wanted_stack[60:65] = value(-1 if chosen is None else chosen)
        wanted_stack[65:70] = value(7 if chosen is not None else int(bits <= 9))
        wanted_stack[70:75] = value(9)
        if chosen is not None:
            wanted_stack[45:60] = b''.join(value(number) for number in (101, 202, 303))
        wanted_vm = bytearray(vm)
        wanted_vm[0x14:0x18] = word(len(prefix) + len(body) if chosen is not None else len(program))
        wanted_vm[0x18:0x1C] = word(13)
        patches = [(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)), (RAM + 0x2000, bytes(vm)),
                   (RAM + 0x3000, program), (RAM + 0x4000, bytes(stack))]
        cases.append(dict(id='script.table-lookup.' + name, symbol='actorRunScriptVm', args=[RAM],
                          patches=patches, observations=[(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)),
                                                       (RAM + 0x2000, bytes(wanted_vm)),
                                                       (RAM + 0x3000, program), (RAM + 0x4000, bytes(wanted_stack))],
                          expected_return=5,
                          limitations=['Authored local table and pre-established frame bases; invalid opcode bounds execution. No retail path reachability, call-frame lifecycle or gameplay proof.']))
    return cases
