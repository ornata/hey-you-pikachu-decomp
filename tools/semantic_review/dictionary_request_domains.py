"""Predeclared requested/uploading/uploaded vocabulary distinctions."""


def experiments():
    """Observe dictionary dispatch acknowledgement and the native requested-ID query."""
    from .cases import RAM, word

    cases = []
    for identifier in (0, 31, -1):
        globals_before = word(11) + word(12) + word(7)
        request = b'\xa5' * 4 + word(2) + word(1) + word(identifier) + b'\xa5' * 4
        wanted = request[:8] + word(0) + request[12:]
        cases.append(dict(id=f'dictionary.dispatch.{identifier}', symbol='micSessionDispatchRequest', args=[RAM],
                          patches=[(RAM, request), (0x800AD43C, globals_before)],
                          observations=[(RAM, wanted), (0x800AD43C, word(11) + word(12) + word(identifier))],
                          expected_return=None,
                          limitations=['Authored request; acknowledgement updates requested state only. No queue submission, dictionary validity, device upload or hardware proof.']))
        actor = bytearray(b'\xa5' * 0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(b'\xa5' * 0x408)
        state[0x1B8:0x1BC] = word(RAM + 0x2000)
        vm = bytearray(b'\xa5' * 0x2C8)
        vm[0x18:0x24] = word(2) + word(8) + word(RAM + 0x3000)
        wanted_vm = bytearray(vm)
        wanted_vm[0x18:0x1C] = word(3)
        values = b'\0' + word(0x1234) + b'\2' + word(0x5678) + b'\xa5' * 30
        globals_after = word(11) + word(12) + word(identifier)
        patches = [(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)), (RAM + 0x2000, bytes(vm)),
                   (RAM + 0x3000, values), (0x800AD43C, globals_after)]
        cases.append(dict(id=f'dictionary.query.{identifier}', symbol='actorScriptQueryRequestedMicDictionary', args=[RAM],
                          patches=patches, observations=[(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)),
                                                       (RAM + 0x2000, bytes(wanted_vm)),
                                                       (RAM + 0x3000, values[:10] + b'\0' + word(identifier) + values[15:]),
                                                       (0x800AD43C, globals_after)], expected_return=0,
                          limitations=['Authored distinct requested/uploading/uploaded IDs; checks query stack effect and continue result, not upload or dictionary validity.']))
    return cases
