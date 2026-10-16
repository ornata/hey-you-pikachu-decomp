"""Authored directive fixtures preserve category scoping and unknown selectors."""

import hashlib
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.script_decoder import vocabulary
from tests.vocabulary_fixture import authored_vocabulary


class DirectiveVocabularyTests(unittest.TestCase):
    names = {1: 'VOICE_DIRECTIVE_AUTHORED_A', 2: 'VOICE_DIRECTIVE_AUTHORED_B'}

    def parse(self, body=None, category='3', extra=''):
        body = body or 'VOICE_DIRECTIVE_AUTHORED_A=1, VOICE_DIRECTIVE_AUTHORED_B=2'
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'include').mkdir()
            (root / 'include/voice_directives.h').write_text(f'enum {{ VOICE_DIRECTIVE_MESSAGE_TYPE={category} }}; typedef enum VoiceDirectiveValue {{ {body} }} VoiceDirectiveValue; {extra}')
            digest = hashlib.sha256(json.dumps(self.names, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
            with patch.object(vocabulary, 'ROOT', root), patch.object(vocabulary, 'DIRECTIVE_MAPPING_SHA256', digest):
                return vocabulary.directive_names()

    def test_strict_mapping_category_and_duplicates(self):
        self.assertEqual(self.parse(extra='// VOICE_DIRECTIVE_FAKE=999'), self.names)
        self.assertEqual(self.parse('VOICE_DIRECTIVE_AUTHORED_A=0x1, VOICE_DIRECTIVE_AUTHORED_B=0x2,'), self.names)
        cases = ((None, '2', ''), (None, '3+0', ''),
                 ('VOICE_DIRECTIVE_AUTHORED_A=1, VOICE_DIRECTIVE_AUTHORED_B=1', '3', ''),
                 ('VOICE_DIRECTIVE_AUTHORED_A=1, VOICE_DIRECTIVE_AUTHORED_A=2', '3', ''),
                 ('VOICE_DIRECTIVE_AUTHORED_A=1, VOICE_DIRECTIVE_AUTHORED_B=2+0', '3', ''),
                 ('VOICE_DIRECTIVE_AUTHORED_A=1, VOICE_DIRECTIVE_AUTHORED_B', '3', ''),
                 ('VOICE_DIRECTIVE_AUTHORED_A=1, VOICE_DIRECTIVE_CHANGED_B=2', '3', ''),
                 (None, '3', 'typedef enum VoiceDirectiveValue { VOICE_DIRECTIVE_OTHER=1 } VoiceDirectiveValue;'))
        for body, category, extra in cases:
            with self.subTest(body=body, category=category, extra=extra), self.assertRaises(ValueError):
                self.parse(body, category, extra)

    def catalog(self, *, missing=False, later_only=False, count=43, enabled=True):
        records = ((3, 1), (3, 2), (2, 1), (3, 0), (3, 16), (3, 17), (3, 999))
        dictionaries = [[0, 1 if not missing and (not later_only or index >= 43) else 3] for index in range(count)]
        image, bank = authored_vocabulary(records, dictionaries)
        with patch.object(vocabulary, 'VocabularyBank', bank), patch.object(vocabulary, 'directive_names', return_value=self.names), patch.object(vocabulary, 'dictionary_names', return_value={}):
            return vocabulary.vocabulary_catalog(image, dictionary_count=count, validate_phrases=False,
                validate_general_commands=False, validate_map_destinations=False, validate_item_noun_groups=False,
                validate_directives=enabled)

    def test_scoped_annotations_preserve_unknowns_ids_and_indexes(self):
        report = self.catalog()
        self.assertEqual([word['word_id'] for word in report['words']], list(range(7)))
        self.assertEqual(report['words'][0]['recovered_directive_name'], self.names[1])
        self.assertEqual(report['words'][1]['recovered_directive_name'], self.names[2])
        self.assertTrue(all('recovered_directive_name' not in row for row in report['words'][2:]))
        self.assertEqual(report['words'][1]['dictionary_memberships'][0], dict(dictionary_id=0, entry_index=1))
        self.assertIn(dict(dictionary_id=42, entry_index=1), report['words'][1]['dictionary_memberships'])
        self.assertEqual([word['command_value'] for word in report['words']], [1, 2, 1, 0, 16, 17, 999])

    def test_reviewed43_membership_required(self):
        for options in (dict(missing=True), dict(later_only=True, count=44)):
            with self.subTest(options=options), self.assertRaises(ValueError):
                self.catalog(**options)
        self.assertEqual(len(self.catalog(count=44)['dictionaries']), 44)

    def test_independent_optout_and_prior_domains_preserved(self):
        self.assertTrue(all('recovered_directive_name' not in row for row in self.catalog(enabled=False)['words']))
        self.assertEqual(len(vocabulary.general_command_names()), 23)
        self.assertEqual(len(vocabulary.map_destination_names()), 11)
        self.assertEqual(len(vocabulary.item_noun_group_names()), 11)
        self.assertEqual(len(vocabulary.phrase_names()), 10)


if __name__ == '__main__':
    unittest.main()
