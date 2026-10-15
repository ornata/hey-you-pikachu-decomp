"""Predeclared actor-mailbox impact payloads, independent of recovered enum names."""

import struct


def experiments():
    """Check item handles, speed magnitude and pending-message rejection without collisions."""
    from .cases import RAM, word

    actor, item, actor_state, item_state = RAM, RAM + 0x100, RAM + 0x1000, RAM + 0x2000
    cases = []
    for name, pending, velocity, speed in (
        ('stationary', 0, (0, 0, 0), 0),
        ('upward', 0, (3, -4, 0), 5),
        ('downward', 0, (-3, 4, 0), 5),
        ('pending', 1, (3, -4, 0), 5),
    ):
        obj = bytearray(b'\xa5' * 0x60)
        obj[0x54:0x58] = word(actor_state)
        held = bytearray(b'\xa5' * 0x60)
        held[0x38:0x3C] = word(0x2468)
        held[0x54:0x58] = word(item_state)
        state = bytearray(b'\xa5' * 0x408)
        state[0x218:0x21C] = word(pending)
        wanted = bytearray(state)
        if not pending:
            wanted[0x218:0x234] = word(1) + b''.join(word(v) for v in (14, 0x2468, 0, speed, 0, 0x2468))
        motion = bytearray(b'\xa5' * 0x6C)
        x, downward, z = velocity
        motion[0x10:0x1C] = struct.pack('>3f', downward, x, z)
        patches = [(actor, bytes(obj)), (item, bytes(held)),
                   (actor_state, bytes(state)), (item_state, bytes(motion))]
        cases.append(dict(
            id='message.kind.impact.' + name, symbol='actorPostItemImpactMessage',
            args=[actor, item], patches=patches,
            observations=[(actor, bytes(obj)), (item, bytes(held)),
                          (actor_state, bytes(wanted)), (item_state, bytes(motion))],
            expected_return=0 if pending else 1,
            limitations=['Synthetic velocity and mailbox state; no physical collision, item lifetime or gameplay reachability claim.'],
        ))
    return cases
