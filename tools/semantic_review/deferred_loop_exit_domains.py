"""Authored selector-three dispatch checks loop requests and completion values."""

import struct


def experiments():
    """Guard saved VM values through running/stopped deferred loop-exit branches."""
    from .cases import RAM, word
    from .native_helper_frame_domains import experiments as helper_experiments

    base = next(case for case in helper_experiments() if case['id'] == 'native.helper-frame.held-live')
    cases = []
    for stopped in (False, True):
        for initializing in (0, 1):
            for update in (False, True):
                patches = dict(base['patches'])
                state = bytearray(patches[RAM + 0x1000])
                state[0x116:0x118] = (2 if stopped else 0).to_bytes(2, 'big')
                state[0x11E:0x120] = b'\0\0'
                state[0x168:0x16C] = word(0 if stopped else RAM + 0x7000)
                patches[RAM + 0x1000] = bytes(state)
                vm = bytearray(patches[RAM + 0x2000])
                vm[:4] = word(2)
                vm[0xC:0x14] = struct.pack('>2f', 1, 2)
                vm[0x14:0x18] = word(123)
                vm[0x2C0:0x2C8] = word(3) + word(initializing)
                patches[RAM + 0x2000] = bytes(vm)
                patches[0x800AE4E4] = struct.pack('>f', 0.25)
                observations = dict(patches)
                state[0x11E:0x120] = b'\0\x01'
                observations[RAM + 0x1000] = bytes(state)
                if stopped:
                    vm[0x18:0x1C] = word(4)
                    values = patches[RAM + 0x3000]
                    observations[RAM + 0x3000] = values[:15] + b'\0' * 5 + values[20:]
                if update:
                    vm[:4] = word(1 if stopped else 2)
                    vm[0xC:0x14] = struct.pack('>2f', 1.25, 2.25)
                    vm[0x2C4:0x2C8] = word(0)
                observations[RAM + 0x2000] = bytes(vm)
                cases.append(dict(id=f'deferred.loop-exit.{int(stopped)}.{initializing}.{int(update)}',
                                  symbol='actorUpdateScriptVm' if update else 'actorScriptDispatchActionHandler',
                                  args=[RAM], patches=list(patches.items()), observations=list(observations.items()),
                                  expected_return=None if update else (1 if stopped else 2),
                                  limitations=['Authored selector-three entry states with protected locals/frame records below the active depth and free append capacity. Checks loop-exit halfword, command-cursor predicate, conditional zero append, scheduler clear and timer changes; no motion advancement, guaranteed termination or retail trajectory proof.']))
    return cases
