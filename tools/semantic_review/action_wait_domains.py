"""Predeclared distinctions between sequencer stop and motion playback state."""


def experiments():
    """Contrast the command cursor with the opposite motion-activity state."""
    from .cases import RAM, word

    cases = []
    for name, command, playback, result in (
        ('stopped-with-timed-motion', 0, 2, 1),
        ('running-with-disabled-motion', RAM + 0x4000, 0, 2),
    ):
        actor = bytearray(b'\xa5' * 0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(b'\xa5' * 0x408)
        state[0x116:0x118] = playback.to_bytes(2, 'big')
        state[0x168:0x16C] = word(command)
        state[0x1B8:0x1BC] = word(RAM + 0x2000)
        vm = bytearray(b'\xa5' * 0x2C8)
        vm[0x18:0x24] = word(2) + word(8) + word(RAM + 0x3000)
        wanted_vm = bytearray(vm)
        values = b'\x00' + word(0x11223344) + b'\x02' + word(0x55667788) + b'\xa5' * 30
        wanted_values = values
        if result == 1:
            wanted_vm[0x18:0x1C] = word(3)
            wanted_values = values[:10] + b'\0' * 5 + values[15:]
        cases.append(dict(id='action.wait.' + name, symbol='actorScriptWaitForSequencerStop',
                          args=[RAM], patches=[(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)),
                                               (RAM + 0x2000, bytes(vm)), (RAM + 0x3000, values)],
                          observations=[(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)),
                                        (RAM + 0x2000, bytes(wanted_vm)), (RAM + 0x3000, wanted_values)],
                          expected_return=result,
                          limitations=['Synthetic command cursor and playback mode; checks wait predicate and stack footprint, not motion advancement or rendered behavior.']))
    return cases
