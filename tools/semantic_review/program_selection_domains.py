"""Predeclared timed-program selection, no-program sentinel and retained-ID contracts."""


def experiments():
    """Keep current motion guarded while selecting commands and observing natural completion."""
    from .cases import RAM, word

    cases = []

    def actor_state():
        actor = bytearray(b'\xa5' * 0x60)
        actor[0x54:0x58] = word(RAM + 0x1000)
        state = bytearray(b'\xa5' * 0x408)
        state[0x116:0x118] = b'\0\x02'  # Timed motion remains independently active.
        return actor, state

    for identifier, program in ((-1, 0), (0, 0x8006EE60)):
        actor, state = actor_state()
        wanted = bytearray(state)
        wanted[0x158:0x178] = b''.join(word(value) for value in (identifier, 0, 0xBF800000, 0, program, 0, 0, 0))
        cases.append(dict(id=f'program.select.{identifier}', symbol='actionScriptStart', args=[RAM, identifier],
                          patches=[(RAM, bytes(actor)), (RAM + 0x1000, bytes(state))],
                          observations=[(RAM, bytes(actor)), (RAM + 0x1000, bytes(wanted))], expected_return=None,
                          limitations=['Uses the retained ROM table, including the negative sentinel backing word; verifies selection/write footprint, not motion advancement.']))
    for identifier, command in ((168, 0), (-1, 0), (87, RAM + 0x4000)):
        actor, state = actor_state()
        state[0x158:0x15C] = word(identifier)
        state[0x168:0x16C] = word(command)
        patches = [(RAM, bytes(actor)), (RAM + 0x1000, bytes(state))]
        cases.append(dict(id=f'program.selected-id.{identifier}', symbol='actionScriptGetId', args=[RAM],
                          patches=patches, observations=patches, expected_return=identifier,
                          limitations=['Synthetic stored selection and command cursor; checks that a retained ID is independent of command activity.']))
    actor, state = actor_state()
    state[0x158:0x178] = b''.join(word(value) for value in (168, 0, 0xBF800000, 0, RAM + 0x4000, 0, 0, 0))
    wanted = bytearray(state)
    wanted[0x15C:0x160] = word(0x3E800000)
    wanted[0x164:0x168] = word(0x3E800000)
    wanted[0x168:0x16C] = word(0)
    command = word(4) + b'\0' * 12  # Authored END record, ready at time zero.
    patches = [(RAM, bytes(actor)), (RAM + 0x1000, bytes(state)),
               (RAM + 0x4000, command), (0x800AE4E4, word(0x3E800000))]
    cases.append(dict(id='program.natural-end-retains-id', symbol='actionScriptUpdate', args=[RAM],
                      patches=patches, observations=[(RAM, bytes(actor)), (RAM + 0x1000, bytes(wanted)), *patches[2:]],
                      expected_return=-1,
                      limitations=['Synthetic ready END record and quarter-second frame; checks command completion and retained selection, not rendering or complete action lifecycle.']))
    return cases
