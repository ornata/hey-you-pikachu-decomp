"""Authored helper-backed getters preserve saved VM values under valid storage."""


def experiments():
    """Check lookup branches, unsigned collision flags and independent view flags."""
    from .cases import RAM, word

    entries = [
        ('chapter', 'actorScriptPushCurrentChapter', 7),
        ('held-live', 'actorScriptPushHeldItemKind', 5),
        ('held-destroy-pending', 'actorScriptPushHeldItemKind', 0),
        ('held-missing', 'actorScriptPushHeldItemKind', 0),
        ('collision', 'actorScriptPushMovementCollisionFlags', 0x8001),
        ('tracking', 'actorScriptPushTrackingMode', 2),
        ('player-held', 'actorScriptPushPlayerHeldItem', 0x2468),
        ('view-only', 'actorScriptPushPlayerViewControlSelected', 1),
        ('input-only', 'actorScriptPushPlayerViewControlSelected', 0),
        ('view-and-input', 'actorScriptPushPlayerViewControlSelected', 1),
        ('view-none', 'actorScriptPushPlayerViewControlSelected', 0),
        ('sequencer-stopped', 'actorScriptPushActionScriptStopped', 1),
        ('sequencer-running', 'actorScriptPushActionScriptStopped', 0),
        ('interaction', 'actorScriptPushInteractionCandidate', 0x1357),
    ]
    cases = []
    for name, symbol, result in entries:
        actor = bytearray(b'\xa5' * 0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(b'\xa5' * 0x408)
        state[0x158:0x15C] = word(168)
        state[0x168:0x16C] = word(RAM + 0x7000 if name == 'sequencer-running' else 0)
        state[0x190:0x194] = word(11 if name == 'held-missing' else 7)
        state[0x19C:0x1A0] = word(2)
        state[0x1B8:0x1BC] = word(RAM + 0x2000)
        state[0x1EA:0x1EC] = b'\x80\x01'
        vm = bytearray(b'\xa5' * 0x2C8)
        for offset, number in [(0x18, 3), (0x1C, 8), (0x20, RAM + 0x3000),
                               (0x24, 1), (0x28, 0), (0x128, 1), (0x12C, 0x44), (0x230, 0)]:
            vm[offset:offset + 4] = word(number)
        wanted_vm = bytearray(vm)
        wanted_vm[0x18:0x1C] = word(4)
        values = b''.join(b'\0' + word(number) for number in (31, 0, 1)) + b'\xa5' * 25
        wanted_values = values[:15] + b'\0' + word(result) + values[20:]
        held = bytearray(b'\xa5' * 0x60)
        held[0x34:0x3C] = (134).to_bytes(2, 'big') + (0x10 if name == 'held-destroy-pending' else 1).to_bytes(2, 'big') + word(7)
        held[0x54:0x58] = word(RAM + 0x6000)
        item = bytearray(b'\xa5' * 0xF0)
        item[:4] = word(0)
        item[4:6] = b'\0\x05'
        item[6:12] = b'\0' * 6
        pool = b''.join(word(number) for number in (RAM + 0x4000, 0, RAM + 0x4000, RAM + 0x4000, 0, 1, 8, 1))
        entry = word(7) + word(RAM + 0x5000) + b'\0' * 12
        record = bytearray(b'\xa5' * 116)
        record[8] = 7
        permissions = {'view-only': 0x8000, 'input-only': 0x200, 'view-and-input': 0x8200}.get(name, 0)
        extra = [(RAM + 0x4000, entry), (RAM + 0x5000, bytes(held)), (RAM + 0x6000, bytes(item)),
                 (RAM + 0x7000, word(4) + b'\0' * 12), (0x800AE388, pool),
                 (0x800EF6C4, word(permissions)), (0x800EF724, word(0x2468)),
                 (0x800B6C54, word(0x1357)), (0x8009A754, word(4)),
                 (0x800F1D30, word(8) + word(0)), (0x800F24D0, bytes(record))]
        patches = [(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)), (RAM + 0x2000, bytes(vm)),
                   (RAM + 0x3000, values), *extra]
        cases.append(dict(id='native.helper-frame.' + name, symbol=symbol, args=[RAM], patches=patches,
                          observations=[(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)),
                                        (RAM + 0x2000, bytes(wanted_vm)), (RAM + 0x3000, wanted_values), *extra],
                          expected_return=0,
                          limitations=['Authored pool, progress and player state with protected saved values/frame metadata; checks immediate integer append and read-only helper footprint. No liveness beyond the fixture, scheduler, concurrency or gameplay reachability proof.']))
    return cases
