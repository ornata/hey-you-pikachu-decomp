"""Invented vocabulary records exercise DMA bounds and metadata provenance."""

import struct

import pytest

from tools.script_decoder.vocabulary import VocabularyBank, vocabulary_catalog


class Image:
    def __init__(self):
        self.base = 0x80100680
        self.start = 0xA9800
        self.data = bytearray(0xA470)
        self.addresses = dict(assets_a_ROM_START=self.start, assets_a_ROM_END=0xB3C70,
                              gVoiceVocabularyData=self.base, gVoiceWordDefinitions=self.base + 0x100,
                              gVoiceDictionaries=self.base + 0x130)
        self.data[0x10:0x19] = b'Sample A\0'
        self.data[0x20:0x29] = b'Sample B\0'
        self.data[0x30:0x39] = b'Sample C\0'
        self.data[0x40:0x43] = b'\x81\x82\0'
        for index, (kind, value) in enumerate(((2, 23), (2, 24), (13, -1))):
            struct.pack_into('>IIii', self.data, 0x100 + index * 16,
                             self.base + 0x10 + index * 0x10, self.base + 0x40, kind, value)
        struct.pack_into('>iIiI', self.data, 0x130, 3, self.base + 0x160, 1, self.base + 0x16C)
        struct.pack_into('>4i', self.data, 0x160, 0, 1, 0, 2)

    def symbol(self, name):
        return name, self.addresses[name], 0

    def read_rom(self, offset, size):
        assert self.start <= offset < offset + size <= self.start + len(self.data)
        return bytes(self.data[offset - self.start:offset - self.start + size])


def test_vocabulary_links_metadata_and_ordered_memberships_without_merging_words():
    result = vocabulary_catalog(Image(), dictionary_count=2, validate_phrases=False)
    assert result['word_view']['count'] == 3
    assert result['dictionaries'][0]['word_ids'] == [0, 1, 0]
    word = result['words'][0]
    assert word['name']['ascii'] == 'Sample A'
    assert word['device_text']['ascii'] is None
    assert word['device_text']['bytes_hex'] == '8182'
    assert word['command_type'] == 2 and word['command_value'] == 23
    assert word['dictionary_memberships'] == [dict(dictionary_id=0, entry_index=0), dict(dictionary_id=0, entry_index=2)]
    assert result['words'][2]['command_value'] == -1
    assert result['dictionary_view']['following_bytes_hex'] == '00' * 8
    assert 'no recognition success' in result['scope']


def test_vocabulary_rejects_unreviewed_mapping_and_outside_or_unterminated_text():
    image = Image()
    bank = VocabularyBank(image)
    for address, size in ((image.base - 1, 1), (image.base, 0), (image.base + len(image.data) - 1, 2)):
        with pytest.raises(ValueError, match='DMA span'):
            bank.read(address, size)
    with pytest.raises(ValueError, match='null'):
        bank.text(0)
    image.data[0x200:0x300] = b'x' * 256
    with pytest.raises(ValueError, match='bounded read'):
        bank.text(image.base + 0x200)
    image.addresses['assets_a_ROM_END'] += 4
    with pytest.raises(ValueError, match='DMA mapping'):
        VocabularyBank(image)


def test_vocabulary_rejects_bad_record_extents_counts_references_and_word_ids():
    for bad_id in (-1, 3):
        image = Image()
        struct.pack_into('>i', image.data, 0x160, bad_id)
        with pytest.raises(ValueError, match='word ID'):
            vocabulary_catalog(image, 2, validate_phrases=False)
    for count in (0, -1, 257):
        image = Image()
        struct.pack_into('>i', image.data, 0x130, count)
        with pytest.raises(ValueError, match='word count'):
            vocabulary_catalog(image, 2, validate_phrases=False)
    image = Image()
    struct.pack_into('>I', image.data, 0x134, image.base + len(image.data) - 4)
    with pytest.raises(ValueError, match='DMA span'):
        vocabulary_catalog(image, 2, validate_phrases=False)
    image = Image()
    image.addresses['gVoiceDictionaries'] -= 1
    with pytest.raises(ValueError, match='aligned'):
        vocabulary_catalog(image, 2, validate_phrases=False)
    with pytest.raises(ValueError, match='prefix bound'):
        vocabulary_catalog(Image(), 0)


def test_named_phrase_requires_matching_label_and_dictionary_membership(monkeypatch):
    monkeypatch.setattr('tools.script_decoder.vocabulary.phrase_names', lambda: {23: 'VOICE_PHRASE_SAMPLE_A'})
    image = Image()
    image.data[0x10:0x19] = b'Sample-A\0'
    result = vocabulary_catalog(image, 2)
    assert result['words'][0]['recovered_phrase_name'] == 'VOICE_PHRASE_SAMPLE_A'
    image.data[0x10:0x19] = b'Changed!\0'
    with pytest.raises(ValueError, match='vocabulary label'):
        vocabulary_catalog(image, 2)
    image.data[0x10:0x19] = b'Sample-A\0'
    struct.pack_into('>3i', image.data, 0x160, 1, 1, 1)
    with pytest.raises(ValueError, match='membership'):
        vocabulary_catalog(image, 2)
