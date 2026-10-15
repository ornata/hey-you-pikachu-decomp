"""Independent numeric expectations for timed actor-cue selection and control."""

import struct


def experiments():
    """Exercise original cue records without importing the recovered C selectors."""
    from .cases import RAM, word

    cases = []
    actor, state, cursor, clock, program = (
        RAM, RAM + 0x100, RAM + 0x300, RAM + 0x304, RAM + 0x400
    )

    def scalar(value):
        return struct.pack(">f", value)

    def command(code, time=0, payload=b""):
        return word(code) + scalar(time) + payload.ljust(12, b"\0")

    def actor_state(kind=0, duration=0, remaining=0):
        obj = bytearray(b"\xa5" * 0x60)
        obj[0x54:0x58] = word(state)
        data = bytearray(b"\xa5" * 0x180)
        data[0x134:0x13C] = scalar(duration) + scalar(remaining)
        data[0x13E:0x140] = struct.pack(">h", kind)
        return [(actor, bytes(obj)), (state, bytes(data))]

    # Original80024F9C: signed action-kind tag,18 fallback/direct,49 terminator.
    listing = command(1) + command(44) + command(18) + command(48) + command(49)
    for kind, expected in [(1, program + 20), (0, program + 60),
                           (17, program + 60), (-1, program + 60), (18, program)]:
        for unused in [0, 1]:
            patches = actor_state(kind) + [(program, listing)]
            cases.append(dict(
                id=f"numeric.cue.select.{kind}.{unused}",
                symbol="actorSelectActionScript", args=[actor, program, unused],
                patches=patches, observations=patches, expected_return=expected,
            ))
    for kind in [1, 18]:
        patches = actor_state(kind)
        cases.append(dict(
            id=f"numeric.cue.select.null.{kind}", symbol="actorSelectActionScript",
            args=[actor, 0, 0], patches=patches, observations=patches, expected_return=0,
        ))
    patches = actor_state(1) + [(program, command(49))]
    cases.append(dict(
        id="numeric.cue.select.end", symbol="actorSelectActionScript",
        args=[actor, program, 0], patches=patches, observations=patches, expected_return=0,
    ))

    def update(name, commands, expected_cursor, expected_clock,
               duration=0, remaining=0, initial_cursor=None, extra=(), wanted_extra=()):
        base = actor_state(duration=duration, remaining=remaining)
        if commands:
            base += [(program, commands)]
        base += [(0x800AE4E4, scalar(0.25))]
        pointer = program if initial_cursor is None else initial_cursor
        cases.append(dict(
            id="numeric.cue.update." + name, symbol="actorUpdateActionScript",
            args=[actor, cursor, clock],
            patches=base + [(cursor, word(pointer) + scalar(1))] + list(extra),
            observations=base + [(cursor, word(expected_cursor) + scalar(expected_clock))]
                         + list(wanted_extra),
            expected_return=None,
        ))

    update("null", b"", 0, 1.25, initial_cursor=0)
    update("future", command(48, 2), program, 1.25)
    update("due-boundary", command(48, 1.25), 0, 1.25)
    for code in [48, 49]:
        update(f"stop.{code}", command(code), 0, 1.25)
    for tag in [-1, 0, 18]:
        update(f"skip-tag.{tag}", command(tag) + command(48, 2), program + 20, 1.25)
    for reset in [0, 1, -1]:
        records = command(46, payload=word(program + 40) + scalar(reset))
        records += command(48) + command(48, 3)
        update(f"jump.{reset}", records, program + 40, 0 if reset else 1.25)
    # Original80024DD0 compares float motion-duration minus remaining against
    # strict +/-0.001F limits. Negative synthetic timers do not assert reachability.
    for duration, accepted in [(0, True), (0.0005, True), (-0.0005, True),
                               (0.001, False), (-0.001, False), (0.01, False)]:
        update(f"sync.{duration}", command(47) + command(48, 2),
               program + 20 if accepted else program, 0 if accepted else 1.25,
               duration=duration)
    for amount, expected in [(0.75, 2), (-0.5, 0.75)]:
        update(f"advance.{amount}", command(44, payload=word(0) + scalar(amount)) + command(48, 4),
               program + 20, expected)
    # Original8003AB68 RNG shift/XOR with first two words zero yields old word2.
    # The final store rounds the double clock sum back to single precision.
    for sample, expected in [(0, 1.25), (0x80000000, 2.25), (0xFFFFFFFF, 3.25)]:
        before = word(0) + word(0) + word(sample) + word(0)
        after = word(0) + word(sample) + word(0) + word(sample)
        update(f"random-clock.{sample:x}",
               command(45, payload=word(1) + scalar(2)) + command(48, 4),
               program + 20, expected,
               extra=[(0x800F2970, before)], wanted_extra=[(0x800F2970, after)])
    for count in [0, -1]:
        random_state = b"\xa5" * 16
        update(f"random-clock.inactive.{count}",
               command(45, payload=word(count) + scalar(2)) + command(48, 4),
               program + 20, 1.25,
               extra=[(0x800F2970, random_state)],
               wanted_extra=[(0x800F2970, random_state)])
    return cases
