"""Authored selector-two startup, waiting and completion footprints."""

import struct


def experiments():
    """Guard protected values when a later handler consumes its program operand."""
    from .cases import RAM, word
    from .native_helper_frame_domains import experiments as helper_experiments

    base = next(case for case in helper_experiments() if case['id'] == 'native.helper-frame.held-live')
    variants = [('start-integer', 0, 0, True), ('start-float', 0.75, 1, True),
                ('start-code-reference', 0, 2, True), ('start-no-program', -1, 0, True),
                ('waiting', 0, 0, False), ('complete', 0, 0, False)]
    cases = []
    for name, argument, tag, startup in variants:
        for update in (False, True):
            patches = dict(base['patches'])
            state = bytearray(patches[RAM + 0x1000])
            state[0x116:0x118] = (2 if name == 'complete' or startup else 0).to_bytes(2, 'big')
            state[0x168:0x16C] = word(0 if name == 'complete' else RAM + 0x7000)
            patches[RAM + 0x1000] = bytes(state)
            vm = bytearray(patches[RAM + 0x2000])
            vm[:4] = word(2)
            vm[0xC:0x14] = struct.pack('>2f', 1, 2)
            vm[0x14:0x18] = word(123)
            vm[0x18:0x1C] = word(4 if startup else 3)
            vm[0x2C0:0x2C8] = word(2) + word(int(startup))
            patches[RAM + 0x2000] = bytes(vm)
            if startup:
                values = patches[RAM + 0x3000]
                payload = struct.pack('>f', argument) if tag == 1 else word(argument)
                patches[RAM + 0x3000] = values[:15] + bytes([tag]) + payload + values[20:]
            patches[0x800AE4E4] = struct.pack('>f', 0.25)
            observations = dict(patches)
            if startup:
                identifier = -1 if name == 'start-no-program' else 0
                # Retained program zero and the authenticated null sentinel;
                # these expectations are independent of recovered C enums.
                state[0x158:0x178] = b''.join(word(n) for n in
                    (identifier, 0, 0xBF800000, 0, 0 if identifier == -1 else 0x8006EE60, 0, 0, 0))
                observations[RAM + 0x1000] = bytes(state)
                vm[0x18:0x1C] = word(3)
            elif name == 'complete':
                vm[0x18:0x1C] = word(4)
                values = patches[RAM + 0x3000]
                observations[RAM + 0x3000] = values[:15] + b'\0' * 5 + values[20:]
            if update:
                vm[:4] = word(1 if name == 'complete' else 2)
                vm[0xC:0x14] = struct.pack('>2f', 1.25, 2.25)
                vm[0x2C4:0x2C8] = word(0)
            observations[RAM + 0x2000] = bytes(vm)
            cases.append(dict(id=f'deferred.play-action.{name}.{int(update)}',
                              symbol='actorUpdateScriptVm' if update else 'actorScriptDispatchActionHandler',
                              args=[RAM], patches=list(patches.items()), observations=list(observations.items()),
                              expected_return=None if update else (1 if name == 'complete' else 2),
                              limitations=['Authored selector-two phases with one startup operand above protected locals/frame records and free completion capacity. Checks program selection, one-time operand consumption under scheduler initialization clear, unchanged waiting values and zero completion append. No program advancement, termination or complete retail trajectory proof. Negative sentinel uses authenticated original-target backing, not portable C array indexing.']))
    return cases
