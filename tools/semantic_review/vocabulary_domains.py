"""Authored dictionaries isolate original word-ID and command-metadata lookup."""


def experiments():
    """Distinguish local entry IDs, global word IDs, categories and command values."""
    from .cases import RAM, word

    cases = []
    for field, symbol, results in (('value', 'voiceDictionaryGetCommandValue', (23, 99)),
                                    ('type', 'voiceDictionaryGetCommandType', (2, 6))):
        for name, index, selected, result in (('first', 0, 0, results[0]),
                                               ('second', 1, 0, results[1]),
                                               ('none', 0, -1, -1),
                                               ('out-of-range', 2, 0, -1)):
            record_a = b'\xa5' * 8 + word(2) + word(23)
            record_b = b'\xa5' * 8 + word(6) + word(99)
            patches = [(0x8009D534, word(selected)), (0x800F8AD0, word(2)),
                       (0x80106D2C, word(2) + word(RAM)),
                       (RAM, word(17) + word(3) + b'\xa5' * 24),
                       (0x8010452C + 17 * 16, record_a),
                       (0x8010452C + 3 * 16, record_b)]
            cases.append(dict(id='vocabulary.' + field + '.' + name, symbol=symbol,
                              args=[index], patches=patches, observations=patches,
                              expected_return=result,
                              limitations=['Authored two-entry dictionary and word metadata; names/device text are unused guards. No retail vocabulary contents, recognition success, hardware or gameplay reachability proof.']))
    return cases
