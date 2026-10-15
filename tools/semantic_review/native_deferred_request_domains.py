"""Authored immediate requests retain operands for a later action handler."""

import struct


def experiments():
    """Check selector/initialization writes without executing deferred behavior."""
    from .cases import RAM, word
    from .native_helper_frame_domains import experiments as helper_experiments

    base = next(case for case in helper_experiments() if case['id'] == 'native.helper-frame.held-live')
    selectors = [
        ('actorScriptStartActionAndWait', 2),
        ('actorScriptStartLoopExitAndWait', 3),
        ('actorScriptStartLoopExitAndWaitAlternate', 3),
        ('actorScriptStartInterruptibleAction', 5),
        ('actorScriptStartHeldItemThrow', 6),
        ('actorScriptStartItemAcquisition', 7),
        ('actorScriptStartEatingHeldItem', 8),
        ('actorScriptStartCostedAction', 9),
        ('actorScriptStartControlSchemeChange', 10),
        ('actorScriptStartAreaTransition', 11),
        ('actorScriptStartTextWindow', 13),
        ('actorScriptStartKindTransformation', 14),
    ]
    cases = []
    for symbol, selector in selectors:
        for previous_initializing in (0, 7):
            patches = dict(base['patches'])
            vm = bytearray(patches[RAM + 0x2000])
            vm[0x18:0x1C] = word(6)
            vm[0x2C0:0x2C8] = word(99) + word(previous_initializing)
            patches[RAM + 0x2000] = bytes(vm)
            values = patches[RAM + 0x3000]
            patches[RAM + 0x3000] = values[:15] + b''.join(b'\0' + word(n) for n in (8, 14, 9)) + values[30:]
            observations = dict(patches)
            vm[0x2C0:0x2C8] = word(selector) + word(1)
            observations[RAM + 0x2000] = bytes(vm)
            cases.append(dict(id='native.deferred-request.' + symbol + '.' + str(previous_initializing),
                              symbol=symbol, args=[RAM], patches=list(patches.items()),
                              observations=list(observations.items()), expected_return=2,
                              limitations=['Direct native request only: pending values and protected frame data remain unchanged; only action selector and initialization flag change. Deferred handler operands/results, scheduler, later VM effects and retail reachability are not exercised. Pending values are opaque to this phase.']))
    for phase in ('request', 'poll-stopped', 'poll-running'):
        patches = dict(base['patches'])
        state = bytearray(patches[RAM + 0x1000])
        state[0x168:0x16C] = word(RAM + 0x7000 if phase == 'poll-running' else 0)
        patches[RAM + 0x1000] = bytes(state)
        vm = bytearray(patches[RAM + 0x2000])
        for offset, number in ((0, 0 if phase == 'request' else 2), (4, RAM + 0x8000),
                               (0x14, 0), (0x2AC, RAM + 0x9000), (0x2C0, 1), (0x2C4, 1)):
            vm[offset:offset + 4] = word(number)
        vm[0xC:0x14] = struct.pack('>2f', 1, 2)
        patches[RAM + 0x2000] = bytes(vm)
        patches[RAM + 0x8000] = b'\x1B\x02' + word(0) + b'\xFF'
        patches[RAM + 0x9000] = word(0x8011BEDC)
        patches[0x800AE4E4] = struct.pack('>f', 0.25)
        observations = dict(patches)
        vm[0xC:0x14] = struct.pack('>2f', 1.25, 2.25)
        vm[:4] = word(1 if phase == 'poll-stopped' else 2)
        if phase == 'request':
            vm[0x14:0x18] = word(6)
            vm[0x2C0:0x2C8] = word(2) + word(1)
        else:
            vm[0x2C4:0x2C8] = word(0)
        observations[RAM + 0x2000] = bytes(vm)
        cases.append(dict(id='native.action-update.' + phase, symbol='actorUpdateScriptVm', args=[RAM],
                          patches=list(patches.items()), observations=list(observations.items()), expected_return=None,
                          limitations=['Separately authored scheduler-entry states check request versus polling branches. Polling does not resume bytecode or push a result, and clears initialization after dispatch. These are not a recorded multi-frame retail trajectory or coverage of other deferred handlers.']))
    return cases
