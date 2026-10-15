"""Authored coordinate, packed-counter and creation-argument VM observations."""

import struct


def experiments():
    """Guard borrowed storage and saved values across numeric getter branches."""
    from .cases import RAM, word
    from .native_helper_frame_domains import experiments as helper_experiments

    base = next(case for case in helper_experiments() if case['id'] == 'native.helper-frame.held-live')
    cases = []

    def add(name, symbol, argument, result, *, floating=False, code_reference=False, result_float=False, changes=None):
        patches = dict(base['patches'])
        if changes:
            patches.update(changes)
        vm = bytearray(patches[RAM + 0x2000])
        vm[0x18:0x1C] = word(4)
        patches[RAM + 0x2000] = bytes(vm)
        values = patches[RAM + 0x3000]
        operand = b'\x01' + struct.pack('>f', argument) if floating else bytes([2 if code_reference else 0]) + word(argument)
        patches[RAM + 0x3000] = values[:15] + operand + values[20:]
        observations = dict(patches)
        output = b'\x01' + struct.pack('>f', result) if result_float else b'\0' + word(result)
        observations[RAM + 0x3000] = values[:15] + output + values[20:]
        cases.append(dict(id='native.numeric-getter.' + name, symbol=symbol, args=[RAM],
                          patches=list(patches.items()), observations=list(observations.items()), expected_return=0,
                          limitations=['Authored initialized actor/VM and separate borrowed records; one valid numeric operand lies above three protected values. Checks immediate result and memory footprint, not invalid conversions, negative axes, scheduler, later lifetime or retail reachability.']))

    actor = bytearray(dict(base['patches'])[RAM])
    actor[0xC:0x18] = struct.pack('>3f', -90, 45, 180)
    actor[0x44:0x50] = struct.pack('>3f', 1.25, 2.5, 3.75)
    state = bytearray(dict(base['patches'])[RAM + 0x1000])
    state[0x1EC:0x1F8] = struct.pack('>3f', 9.25, 8.5, 7.75)
    coordinates = {RAM: bytes(actor), RAM + 0x1000: bytes(state),
                   0x800EF6A0: struct.pack('>3f', -12.5, 34.25, 56.75)}
    for domain, symbol, numbers, bounded in (
            ('world', 'actorScriptGetWorldCoordinate', (1.25, 2.5, 3.75), True),
            ('target', 'actorScriptGetTargetCoordinate', (9.25, 8.5, 7.75), True),
            ('rotation', 'actorScriptGetRotationCoordinate', (-90, 45, 180), False),
            ('player', 'actorScriptGetPlayerCoordinate', (-12.5, 34.25, 56.75), False)):
        for axis in range(3):
            add(domain + '.axis-' + str(axis), symbol, axis, numbers[axis], result_float=True, changes=coordinates)
        add(domain + '.fractional-axis', symbol, 1.75, numbers[1], floating=True, result_float=True, changes=coordinates)
        add(domain + '.code-reference-axis', symbol, 2, numbers[2], code_reference=True, result_float=True, changes=coordinates)
        if bounded:
            for axis in (3, 0x7FFFFFFF):
                add(domain + '.out-of-range-' + str(axis), symbol, axis, -65536, result_float=True, changes=coordinates)

    counters = bytearray(b'\xA5' * 83)
    counters[0], counters[82] = 0xD2, 0x7E
    for index, expected in ((0, 2), (1, 13), (164, 14), (165, 7)):
        add('profile.index-' + str(index), 'actorScriptGetProfileCounter', index, expected,
            changes={0x800F0168: bytes(counters)})
    add('profile.fractional-index', 'actorScriptGetProfileCounter', 165.75, 7, floating=True,
        changes={0x800F0168: bytes(counters)})
    add('profile.code-reference-index', 'actorScriptGetProfileCounter', 1, 13, code_reference=True,
        changes={0x800F0168: bytes(counters)})

    target = bytearray(dict(base['patches'])[RAM + 0x5000])
    target[0x34:0x36] = (37).to_bytes(2, 'big')
    target_state = bytearray(b'\xA5' * 0x408)
    target_state[0x238:0x23C] = word(0x87654321)
    for name, handle, flags, expected, floating in (
            ('live', 7, 1, 0x87654321, False), ('null', 0, 1, -1, False),
            ('missing', 11, 1, -1, False), ('destroy-pending', 7, 0x10, -1, False),
            ('fractional-handle', 7.75, 1, 0x87654321, True)):
        target[0x36:0x38] = flags.to_bytes(2, 'big')
        add('creation.' + name, 'actorScriptGetCreationScriptArgument', handle, expected, floating=floating,
            changes={RAM + 0x5000: bytes(target), RAM + 0x6000: bytes(target_state)})
    return cases
