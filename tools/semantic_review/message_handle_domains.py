"""Predeclared native stack contracts for script-message handle producers."""


def experiments():
    """Observe one integer append with existing stack values and guarded VM/actor state."""
    from .cases import RAM, word

    cases = []
    for name, symbol, handle in (
        ('self', 'actorScriptPushSelfHandle', 0x1357),
        ('pikachu', 'actorScriptPushPikachuHandle', 0x2468),
        ('held', 'actorScriptPushHeldItemHandle', 0x3579),
        ('held-empty', 'actorScriptPushHeldItemHandle', 0),
        ('target-empty', 'actorScriptPushTargetHandle', 0),
    ):
        state_address, vm_address, values_address = RAM + 0x1000, RAM + 0x2000, RAM + 0x3000
        actor = bytearray(b'\xa5' * 0x60)
        actor[0x38:0x3C] = word(0x1357)
        actor[0x54:0x58] = word(state_address)
        state = bytearray(b'\xa5' * 0x408)
        state[0x190:0x194] = word(handle)
        state[0x198:0x19C] = word(0)
        state[0x1B8:0x1BC] = word(vm_address)
        vm = bytearray(b'\xa5' * 0x2C8)
        vm[0x18:0x24] = word(2) + word(8) + word(values_address)
        wanted_vm = bytearray(vm)
        wanted_vm[0x18:0x1C] = word(3)
        values = b'\x00' + word(0x11223344) + b'\x02' + word(0x55667788) + b'\xa5' * 30
        wanted_values = values[:10] + b'\x00' + word(handle) + values[15:]
        patches = [(RAM, bytes(actor)), (state_address, bytes(state)),
                   (vm_address, bytes(vm)), (values_address, values)]
        observations = [(RAM, bytes(actor)), (state_address, bytes(state)),
                        (vm_address, bytes(wanted_vm)), (values_address, wanted_values)]
        if name == 'pikachu':
            other = bytearray(b'\xa5' * 0x60)
            other[0x38:0x3C] = word(handle)
            patches.extend([(RAM + 0x100, bytes(other)), (0x800760A8, word(RAM + 0x100))])
            observations.extend([(RAM + 0x100, bytes(other)), (0x800760A8, word(RAM + 0x100))])
        cases.append(dict(id='message.handle.' + name, symbol=symbol,
                          args=[RAM], patches=patches, observations=observations,
                          expected_return=0,
                          limitations=['Synthetic handle storage; target case covers only a zero target. No liveness or gameplay reachability claim.']))
    return cases
