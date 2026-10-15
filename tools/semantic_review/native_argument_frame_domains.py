"""Authored handle queries distinguish object domains above protected VM values."""

import struct


def experiments():
    """Exercise one-argument replacement, lookup exclusions and numeric conversion."""
    from .cases import RAM, word
    from .native_helper_frame_domains import experiments as helper_experiments

    base = next(case for case in helper_experiments() if case['id'] == 'native.helper-frame.held-live')
    cases = []
    # Expected results are (item catalog kind, actor configuration kind, object tag).
    variants = [
        ('item-world', 134, 0, 1, 7, False, (5, 0, 134)),
        ('item-toolbox', 134, 4, 1, 7, False, (0, 0, 0)),
        ('actor', 37, 0, 1, 7, False, (0, 37, 37)),
        ('unknown-130', 130, 0, 1, 7, False, (0, 0, 130)),
        ('null', 134, 0, 1, 0, False, (0, 0, 0)),
        ('missing', 134, 0, 1, 11, False, (0, 0, 0)),
        ('destroy-pending', 134, 0, 0x10, 7, False, (0, 0, 0)),
        ('float-handle', 134, 0, 1, 7, True, (5, 0, 134)),
    ]
    for name, kind, ownership, flags, handle, floating, results in variants:
        for index, symbol in enumerate(('actorScriptGetItemKind', 'actorScriptGetActorKind', 'actorScriptGetObjectType')):
            patches = dict(base['patches'])
            target = bytearray(patches[RAM + 0x5000])
            target[0x34:0x38] = kind.to_bytes(2, 'big') + flags.to_bytes(2, 'big')
            patches[RAM + 0x5000] = bytes(target)
            item = bytearray(patches[RAM + 0x6000])
            item[6] = ownership
            patches[RAM + 0x6000] = bytes(item)
            vm = bytearray(patches[RAM + 0x2000])
            vm[0x18:0x1C] = word(4)
            patches[RAM + 0x2000] = bytes(vm)
            values = patches[RAM + 0x3000]
            argument = b'\x01' + struct.pack('>f', handle + 0.75) if floating else b'\0' + word(handle)
            patches[RAM + 0x3000] = values[:15] + argument + values[20:]
            observations = dict(patches)
            observations[RAM + 0x3000] = values[:15] + b'\0' + word(results[index]) + values[20:]
            cases.append(dict(id='native.argument-frame.' + name + '.' + str(index), symbol=symbol,
                              args=[RAM], patches=list(patches.items()), observations=list(observations.items()),
                              expected_return=0,
                              limitations=['Authored initialized handle pool and separate borrowed records; the operand lies above three protected values. Checks immediate replacement and read-only lookup footprint, not arbitrary tags, invalid conversions, scheduler, object lifetime or retail reachability. Tag 130 has no inferred private-state meaning.']))
    return cases
