"""Authored active-frame states check reviewed scalar-append callback footprints."""


def experiments():
    """Guard saved locals and frame metadata below each callback's appended value."""
    from .cases import RAM, word

    cases = []
    entries = [
        ('mode', 'actorScriptPushDefaultActionMode', 0, -3, 0),
        ('total-time', 'actorScriptPushTotalElapsedTime', 1, 0x41140000, 0),
        ('friendship', 'actorScriptPushFriendshipLevel', 0, 123, 0),
        ('game-mode', 'actorScriptPushGameMode', 0, 14, 0),
        ('held-item', 'actorScriptPushHeldItemHandle', 0, 0x1234, 0),
        ('message-type', 'actorScriptPushMessageType', 0, 17, 0),
        ('argument0', 'actorScriptPushMessageArgument0', 0, -7, 0),
        ('argument2', 'actorScriptPushMessageArgument2', 0, 12345, 0),
        ('argument3', 'actorScriptPushMessageArgument3', 0, 0x112233, 0),
        ('argument4', 'actorScriptPushMessageArgument4', 0, 0x2478, 0),
        ('secondary-meter', 'actorScriptPushPikachuSecondaryMeter', 1, 0x422A0000, 0),
        ('distance-allowance', 'actorScriptPushTargetDistanceAllowance', 1, 0x40700000, 0),
        ('timer', 'actorScriptPushTimer', 1, 0x40200000, 0),
        ('yield', 'actorScriptYield', 0, 0, 1),
        ('blend-done', 'actorScriptIsPrimaryOverlayBlendComplete', 0, 1, 0),
        ('blend-pending', 'actorScriptIsPrimaryOverlayBlendComplete', 0, 0, 0),
    ]
    for name, symbol, tag, payload, result in entries:
        actor = bytearray(b'\xa5' * 0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(b'\xa5' * 0x408)
        state[0x13C:0x13E] = b'\xff\xfd'
        state[0x190:0x194] = word(0x1234)
        state[0x1A4:0x1A8] = word(0x40700000)
        state[0x1B8:0x1BC] = word(RAM + 0x2000)
        state[0x21C:0x234] = b''.join(word(number) for number in (17, -7, 9, 12345, 0x112233, 0x2478))
        state[0x3F8:0x3FC] = word(0x422A0000)
        vm = bytearray(b'\xa5' * 0x2C8)
        for offset, number in [(0xC, 0x40200000), (0x10, 0x41140000), (0x18, 3), (0x1C, 8),
                               (0x20, RAM + 0x3000), (0x24, 1), (0x28, 0),
                               (0x128, 1), (0x12C, 0x44), (0x230, 0)]:
            vm[offset:offset + 4] = word(number)
        wanted_vm = bytearray(vm)
        wanted_vm[0x18:0x1C] = word(4)
        values = b''.join(b'\0' + word(number) for number in (31, 0, 1)) + b'\xa5' * 25
        wanted_values = values[:15] + bytes([tag]) + word(payload) + values[20:]
        profile = bytearray(b'\xa5' * 16)
        profile[6] = 123
        globals_before = [(0x800760A8, word(RAM)), (0x800AE504, word(14)),
                          (0x800EF790, bytes(profile)),
                          (0x800ABDC0, word(0x3F800000 if name == 'blend-pending' else 0))]
        patches = [(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)), (RAM + 0x2000, bytes(vm)),
                   (RAM + 0x3000, values), *globals_before]
        cases.append(dict(id='native.scalar-frame.' + name, symbol=symbol, args=[RAM], patches=patches,
                          observations=[(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)),
                                        (RAM + 0x2000, bytes(wanted_vm)), (RAM + 0x3000, wanted_values), *globals_before],
                          expected_return=result,
                          limitations=['Authored active frame with saved ID and metadata below stack depth; checks one scalar append and handler return. Negative mode is a read-width probe, not a retained gameplay-mode claim. No scheduler, concurrent mutation or lifetime proof.']))
    return cases
