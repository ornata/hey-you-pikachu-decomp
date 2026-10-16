"""Synthetic noun selectors do not globally rename direct item aliases."""

import hashlib
import json
import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.script_decoder import vocabulary


class ItemNounVocabularyTests(unittest.TestCase):
    names = {166: 'VOICE_ITEM_NOUN_GROUP_AUTHORED_A', 179: 'VOICE_ITEM_NOUN_GROUP_AUTHORED_B'}

    def parse(self, body=None, category='6'):
        body = body or 'VOICE_ITEM_NOUN_GROUP_AUTHORED_A=166, VOICE_ITEM_NOUN_GROUP_AUTHORED_B=179'
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'include').mkdir()
            (root / 'include/voice_item_selector.h').write_text(f'enum {{ VOICE_ITEM_NOUN_MESSAGE_TYPE={category} }}; typedef enum VoiceItemNounGroupValue {{ {body} }} VoiceItemNounGroupValue;')
            digest = hashlib.sha256(json.dumps(self.names, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
            with patch.object(vocabulary, 'ROOT', root), patch.object(vocabulary, 'ITEM_NOUN_GROUP_MAPPING_SHA256', digest):
                return vocabulary.item_noun_group_names()

    def test_strict_reviewed_enum_and_category(self):
        self.assertEqual(self.parse(), self.names)
        for body, category in ((None, '2'), (None, '6+0'),
                               ('VOICE_ITEM_NOUN_GROUP_AUTHORED_A=166, VOICE_ITEM_NOUN_GROUP_AUTHORED_B=166', '6'),
                               ('VOICE_ITEM_NOUN_GROUP_AUTHORED_A=166, VOICE_ITEM_NOUN_GROUP_AUTHORED_B=179+0', '6'),
                               ('VOICE_ITEM_NOUN_GROUP_AUTHORED_A=166, VOICE_ITEM_NOUN_GROUP_CHANGED_B=179', '6')):
            with self.subTest(body=body, category=category), self.assertRaises(ValueError):
                self.parse(body, category)

    def catalog(self, *, missing=False, count=43, later_only=False, enabled=True):
        records = ((6, 166), (6, 179), (2, 166), (6, 168), (6, 177), (6, 178), (6, 51), (6, 53))
        class Image:
            def symbol(self, name):
                return (None, {'gVoiceWordDefinitions': 100, 'gVoiceDictionaries': 228}[name])
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
                    data = b''.join(struct.pack('>IIii', 500+i, 600+i, typ, value) for i, (typ, value) in enumerate(records))
                elif address == 228 and size == count*8:
                    data = b''.join(struct.pack('>iI', 2, 10000+i*8) for i in range(count))
                elif 10000 <= address < 10000+count*8:
                    identifier = (address-10000)//8
                    second = 1 if not missing and (not later_only or identifier >= 43) else 3
                    data = struct.pack('>ii', 0, second)
                else:
                    data = b'\0' * size
                return data, dict(address=address)
        with patch.object(vocabulary, 'VocabularyBank', Bank), patch.object(vocabulary, 'item_noun_group_names', return_value=self.names), patch.object(vocabulary, 'dictionary_names', return_value={}):
            return vocabulary.vocabulary_catalog(Image(), dictionary_count=count, validate_phrases=False,
                validate_general_commands=False, validate_map_destinations=False, validate_item_noun_groups=enabled)

    def test_scoped_annotations_preserve_records_and_entry_indexes(self):
        report = self.catalog()
        self.assertEqual([w['word_id'] for w in report['words']], list(range(8)))
        self.assertEqual(report['words'][0]['recovered_item_noun_group_name'], self.names[166])
        self.assertEqual(report['words'][1]['recovered_item_noun_group_name'], self.names[179])
        self.assertTrue(all('recovered_item_noun_group_name' not in row for row in report['words'][2:]))
        self.assertIn(dict(dictionary_id=42, entry_index=1), report['words'][1]['dictionary_memberships'])
        self.assertEqual(report['words'][6]['command_value'], 51)

    def test_reviewed43_membership_and_independent_optout(self):
        with self.assertRaises(ValueError):
            self.catalog(missing=True)
        with self.assertRaises(ValueError):
            self.catalog(later_only=True, count=44)
        self.assertTrue(all('recovered_item_noun_group_name' not in row for row in self.catalog(enabled=False)['words']))


if __name__ == '__main__':
    unittest.main()
