"""Predeclared actor/prop query boundaries through a synthetic live handle registry."""


def experiments():
    """Distinguish actor props, both Pikachu kinds, the search alias and another object domain."""
    from .cases import RAM, word

    cases = []
    for kind, actor_kind, category in ((16, 16, 4), (20, 20, 4), (36, 36, 4),
                                      (37, 37, 4), (38, 0, 0), (130, 0, 5)):
        obj = bytearray(b'\xa5' * 0x60)
        obj[0x34:0x38] = kind.to_bytes(2, 'big') + b'\0\0'
        obj[0x38:0x3C] = word(7)
        entry = word(7) + word(RAM) + b'\0' * 12
        pool = word(RAM + 0x100) + b'\0' * 16 + word(1) + word(8) + word(1)
        patches = [(RAM, bytes(obj)), (RAM + 0x100, entry), (0x800AE388, pool)]
        for symbol, args, result in (('actorGetKindByHandle', [7], actor_kind),
                                      ('objectGetScriptCategory', [0, 7], category)):
            cases.append(dict(id=f'actor.kind.{symbol}.{kind}', symbol=symbol,
                              args=args, patches=patches, observations=patches, expected_return=result,
                              limitations=['Synthetic one-entry live registry; validates query domains and read-only footprints, not rendering, actor creation or pool mutation.']))
    return cases
