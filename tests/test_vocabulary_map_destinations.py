"""Synthetic destination selectors remain distinct from internal map indices."""

import hashlib
import json
import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.script_decoder import vocabulary


class MapDestinationTests(unittest.TestCase):
    names = {0: 'VOICE_MAP_DESTINATION_AUTHORED_A', 10: 'VOICE_MAP_DESTINATION_AUTHORED_B'}

    def header(self, body=None):
        body = body or 'VOICE_MAP_DESTINATION_AUTHORED_A = 0, VOICE_MAP_DESTINATION_AUTHORED_B = 10'
        return f'typedef enum VoiceMapDestinationValue {{ {body} }} VoiceMapDestinationValue;'

    def parse(self, header, category='18'):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'include').mkdir()
            (root / 'include/voice_map_destination.h').write_text(header)
            (root / 'include/script_message.h').write_text(f'enum {{ SCRIPT_MESSAGE_WORLD_MAP_DESTINATION = {category} }};')
            digest = hashlib.sha256(json.dumps(self.names, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
            with patch.object(vocabulary, 'ROOT', root), patch.object(vocabulary, 'MAP_DESTINATION_MAPPING_SHA256', digest):
                return vocabulary.map_destination_names()

    def test_strict_category_enum_and_review_digest(self):
        self.assertEqual(self.parse(self.header()), self.names)
        for header, category in ((self.header(), '2'), (self.header(), '18+0'),
                                 (self.header() + self.header(), '18'),
                                 (self.header('VOICE_MAP_DESTINATION_AUTHORED_A=0, VOICE_MAP_DESTINATION_AUTHORED_B=0'), '18'),
                                 (self.header('VOICE_MAP_DESTINATION_AUTHORED_A=0, VOICE_MAP_DESTINATION_AUTHORED_A=10'), '18'),
                                 (self.header('VOICE_MAP_DESTINATION_AUTHORED_A=0, VOICE_MAP_DESTINATION_AUTHORED_B=10+0'), '18'),
                                 (self.header().replace('AUTHORED_B', 'CHANGED_B'), '18')):
            with self.subTest(header=header, category=category), self.assertRaises(ValueError):
                self.parse(header, category)

    def catalog(self, *, include31=True, wrong_category=False, dictionary_count=43, enabled=True):
        class Image:
            def symbol(self, name):
                return (None, {'gVoiceWordDefinitions': 100, 'gVoiceDictionaries': 164}[name])
            def read_rom(self, start, size):
                return b'x' * size
        class Bank:
            start, end, base, size = 1, 2, 3, 1
            def __init__(self, image):
                pass
            def text(self, address):
                return dict(address=address, ascii='synthetic', bytes_hex='')
            def read(self, address, size):
                if address == 100:
                    data = b''.join(struct.pack('>IIii', 500+i, 600+i, typ, value) for i, (typ, value) in enumerate(((18, 0), (2 if wrong_category else 18, 10), (2, 10), (18, 999))))
                elif address == 164 and size == dictionary_count*8:
                    data = b''.join(struct.pack('>iI', 2, 10000+i*8) for i in range(dictionary_count))
                elif 10000 <= address < 10000+dictionary_count*8:
                    identifier = (address-10000)//8
                    data = struct.pack('>ii', 0, 1 if include31 or identifier != 31 else 3)
                else:
                    data = b'\0' * size
                return data, dict(address=address)
        with patch.object(vocabulary, 'VocabularyBank', Bank), patch.object(vocabulary, 'map_destination_names', return_value=self.names), patch.object(vocabulary, 'dictionary_names', return_value={}):
            return vocabulary.vocabulary_catalog(Image(), dictionary_count=dictionary_count, validate_phrases=False,
                                                  validate_general_commands=False, validate_map_destinations=enabled, validate_item_noun_groups=False)

    def test_category_only_annotation_preserves_records_and_memberships(self):
        report = self.catalog()
        self.assertEqual([word['word_id'] for word in report['words']], [0, 1, 2, 3])
        self.assertEqual(report['words'][0]['recovered_map_destination_name'], self.names[0])
        self.assertEqual(report['words'][1]['recovered_map_destination_name'], self.names[10])
        self.assertNotIn('recovered_map_destination_name', report['words'][2])
        self.assertNotIn('recovered_map_destination_name', report['words'][3])
        self.assertIn(dict(dictionary_id=31, entry_index=1), report['words'][1]['dictionary_memberships'])
        self.assertEqual(report['words'][1]['command_value'], 10)
        self.assertNotIn('map_index', report['words'][1])

    def test_required_dictionary31_not_other_membership(self):
        for options in (dict(include31=False), dict(wrong_category=True), dict(dictionary_count=31)):
            with self.subTest(options=options), self.assertRaises(ValueError):
                self.catalog(**options)

    def test_map_opt_out_and_existing_name_parsers(self):
        self.assertTrue(all('recovered_map_destination_name' not in row for row in self.catalog(enabled=False)['words']))
        self.assertEqual(len(vocabulary.general_command_names()), 23)
        self.assertEqual(len(vocabulary.phrase_names()), 10)


if __name__ == '__main__':
    unittest.main()
