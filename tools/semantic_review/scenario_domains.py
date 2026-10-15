"""Predeclared chapter-driven scenario returns and presentation output contracts."""


def experiments():
    """Contrast supported chapters with fallback output preservation in original selectors."""
    from .cases import RAM, word

    cases = []
    for area, symbol, selections in (
        ('forest-edge', 'presentationSelectViridianEdgeScenario',
         ((1, 0, 0xF4, 0x15), (7, 1, 0xF4, 0x15), (0, 0, None, None))),
        ('credits', 'presentationSelectCreditsScenario',
         ((0, 1, None, 0), (9, 0, 0x32, 0x15), (1, 0, None, None))),
    ):
        for chapter, scenario, action, audio in selections:
            outputs = word(0x11223344) + word(0x55667788) + b'\xa5' * 16
            wanted = word(0x11223344 if action is None else action) + word(0x55667788 if audio is None else audio) + outputs[8:]
            record = bytearray(b'\xa5' * 116)
            record[8] = chapter
            # Retail field zero is a signed-byte chapter. The location is
            # deliberately synthetic; this case does not validate save packing.
            patches = [(RAM, outputs), (0x800F1D30, word(8) + word(0)),
                       (0x800F24D0, bytes(record))]
            cases.append(dict(id=f'scenario.{area}.chapter-{chapter}', symbol=symbol,
                              args=[RAM, RAM + 4], patches=patches,
                              observations=[(RAM, wanted), *patches[1:]], expected_return=scenario,
                              limitations=['Synthetic chapter storage; validates selector return/output contracts, not scene reachability, save layout or rendered card/actor content.']))
    return cases
