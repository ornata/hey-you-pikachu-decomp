"""Synthetic selector/header and dictionary fixtures contain no game word lists."""

import hashlib
import json
import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.script_decoder import vocabulary


def digest(names):
    return hashlib.sha256(json.dumps(names, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


class GeneralCommandTests(unittest.TestCase):
    names = {3: 'VOICE_GENERAL_COMMAND_AUTHORED_A', 7: 'VOICE_GENERAL_COMMAND_AUTHORED_B'}

    def header(self, body=None, category='2'):
        body = body or 'VOICE_GENERAL_COMMAND_AUTHORED_A = 3, VOICE_GENERAL_COMMAND_AUTHORED_B = 7'
        return f'enum {{ VOICE_GENERAL_COMMAND_MESSAGE_TYPE = {category} }};\ntypedef enum VoiceGeneralCommandValue {{ {body} }} VoiceGeneralCommandValue;\n'

    def parse(self, text):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'include').mkdir()
            (root / 'include/voice_phrases.h').write_text(text)
            with patch.object(vocabulary, 'ROOT', root), patch.object(vocabulary, 'GENERAL_COMMAND_MAPPING_SHA256', digest(self.names)):
                return vocabulary.general_command_names()

    def test_strict_mapping_and_comment_exclusion(self):
        self.assertEqual(self.parse(self.header() + '// VOICE_GENERAL_COMMAND_FAKE = 999\n'), self.names)
        self.assertEqual(self.parse(self.header().replace('= 3,', '= 0x3,').replace('= 7 ', '= 0x7 ')), self.names)
        for text in (self.header(category='3'), self.header(category='2+1'), self.header() + self.header(),
                     self.header('VOICE_GENERAL_COMMAND_AUTHORED_A = 3, VOICE_GENERAL_COMMAND_AUTHORED_B = 3'),
                     self.header('VOICE_GENERAL_COMMAND_AUTHORED_A = 3, VOICE_GENERAL_COMMAND_AUTHORED_A = 7'),
                     self.header('VOICE_GENERAL_COMMAND_AUTHORED_A = 3, VOICE_GENERAL_COMMAND_AUTHORED_B = 7 + 0'),
                     self.header('VOICE_GENERAL_COMMAND_AUTHORED_A = 3, VOICE_GENERAL_COMMAND_AUTHORED_B'),
                     self.header('VOICE_GENERAL_COMMAND_AUTHORED_A = 23, VOICE_GENERAL_COMMAND_AUTHORED_B = 7'),
                     self.header().replace('AUTHORED_B', 'CHANGED_B')):
            with self.subTest(text=text), self.assertRaises(ValueError):
                self.parse(text)

    def fixture(self, *, command_type=2, missing=False, outside_prefix=False, count=43, duplicate_named_value=False):
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
                    data = b''.join(struct.pack('>IIii', 500+i, 600+i, typ, value) for i, (typ, value) in enumerate(((2, 3), (command_type, 7), (2, 7 if duplicate_named_value else 0), (2, 4))))
                elif address == 164 and size == count * 8:
                    data = b''.join(struct.pack('>iI', 2, 10000+i*8) for i in range(count))
                elif 10000 <= address < 10000 + count*8:
                    identifier = (address-10000)//8
                    second = (2 if duplicate_named_value else 1) if not missing and (not outside_prefix or identifier >= 43) else 2
                    data = struct.pack('>ii', 0, second)
                else:
                    data = b'\0' * size
                return data, dict(address=address)
        return Image(), Bank

    def catalog(self, **options):
        image, bank = self.fixture(**options)
        with patch.object(vocabulary, 'VocabularyBank', bank), patch.object(vocabulary, 'general_command_names', return_value=self.names), patch.object(vocabulary, 'dictionary_names', return_value={}):
            return vocabulary.vocabulary_catalog(image, dictionary_count=options.get('count', 43), validate_phrases=False, validate_map_destinations=False)

    def test_category_scoping_and_original_indices_preserved(self):
        result = self.catalog()
        self.assertEqual([word['word_id'] for word in result['words']], [0, 1, 2, 3])
        self.assertEqual(result['words'][0]['recovered_general_command_name'], self.names[3])
        self.assertEqual(result['words'][1]['recovered_general_command_name'], self.names[7])
        self.assertNotIn('recovered_general_command_name', result['words'][2])
        self.assertNotIn('recovered_general_command_name', result['words'][3])
        self.assertEqual(result['words'][1]['dictionary_memberships'][0], dict(dictionary_id=0, entry_index=1))
        other_category = self.catalog(command_type=10, duplicate_named_value=True)
        self.assertNotIn('recovered_general_command_name', other_category['words'][1])
        self.assertEqual(other_category['words'][2]['recovered_general_command_name'], self.names[7])
        with self.assertRaises(ValueError):
            self.catalog(command_type=10)

    def test_membership_required_within_reviewed43_not_later_extension(self):
        with self.assertRaises(ValueError):
            self.catalog(missing=True)
        with self.assertRaises(ValueError):
            self.catalog(outside_prefix=True, count=44)
        self.assertEqual(len(self.catalog(count=44)['dictionaries']), 44)

    def test_independent_opt_out_does_not_load_general_enum(self):
        image, bank = self.fixture()
        with patch.object(vocabulary, 'VocabularyBank', bank), patch.object(vocabulary, 'general_command_names', side_effect=AssertionError('disabled')), patch.object(vocabulary, 'dictionary_names', return_value={}):
            result = vocabulary.vocabulary_catalog(image, validate_phrases=False, validate_general_commands=False, validate_map_destinations=False)
            self.assertTrue(all('recovered_general_command_name' not in word for word in result['words']))


if __name__ == '__main__':
    unittest.main()
