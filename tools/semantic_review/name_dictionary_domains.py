"""Authored bedroom group bytes isolate the original native's indexing and tag."""


def experiments():
    """Check endpoint indices and unsigned group conversion with full state guards."""
    from .cases import RAM, word

    cases = []
    for name, index, group in (('first', 0, 2), ('last', 152, 4), ('unsigned', 10, 255)):
        actor = bytearray(b'\xa5' * 0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(b'\xa5' * 0x408)
        state[0x1B8:0x1BC] = word(RAM + 0x2000)
        vm = bytearray(b'\xa5' * 0x2C8)
        vm[0x18:0x24] = word(2) + word(8) + word(RAM + 0x3000)
        values = b'\2' + word(0x12345678) + b'\0' + word(index) + b'\xa5' * 30
        wanted = values[:5] + b'\0' + word(group) + values[10:]
        groups = bytearray(b'\xa5' * 153)
        groups[index] = group
        patches = [(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)), (RAM + 0x2000, bytes(vm)),
                   (RAM + 0x3000, values), (0x8015516C, bytes(groups))]
        cases.append(dict(id='name.dictionary-group.' + name, symbol='actorScriptGetPokemonNameDictionaryGroup',
                          args=[RAM], patches=patches,
                          observations=[(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)),
                                        (RAM + 0x2000, bytes(vm)), (RAM + 0x3000, wanted), (0x8015516C, bytes(groups))],
                          expected_return=0,
                          limitations=['Authored group table; checks valid-index byte access, unsigned conversion and replacement stack result. Value255 is not a retained retail group. No name-activity reachability or vocabulary upload proof.']))
    return cases
