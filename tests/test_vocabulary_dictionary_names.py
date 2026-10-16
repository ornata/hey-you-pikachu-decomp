"""Dictionary-ID parsing is scoped and preserves its signed no-selection sentinel."""

import hashlib
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.script_decoder import vocabulary
from tests.vocabulary_fixture import authored_vocabulary


class DictionaryNameTests(unittest.TestCase):
    names = {
        -1: 'VOICE_DICTIONARY_NONE',
        0: 'VOICE_DICTIONARY_DEFAULT',
        1: 'VOICE_DICTIONARY_ITEM_NOUNS',
        3: 'VOICE_DICTIONARY_HELD_ITEM_COMMANDS',
        4: 'VOICE_DICTIONARY_BEDROOM_HELD_ITEM_COMMANDS',
        10: 'VOICE_DICTIONARY_CREATURE_SEARCH_AND_COUNT_QUESTIONS',
        11: 'VOICE_DICTIONARY_DIRECTION_AND_STRIKE',
        13: 'VOICE_DICTIONARY_DRAWING_FEEDBACK',
        31: 'VOICE_DICTIONARY_WORLD_MAP',
        34: 'VOICE_DICTIONARY_FISHING_COMMANDS',
        42: 'VOICE_DICTIONARY_ROCK_PAPER_SCISSORS',
    }
    body = ', '.join(f'{name}={value}' for value, name in names.items())

    def header(self, body=None):
        return 'typedef enum VoiceDictionaryId { ' + (self.body if body is None else body) + ' } VoiceDictionaryId;'

    def parse(self, text):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'include').mkdir()
            (root / 'include/voice_recognition.h').write_text(text)
            with patch.object(vocabulary, 'ROOT', root):
                return vocabulary.dictionary_names()

    def test_signed_decimal_hex_and_outside_aliases_are_not_promoted(self):
        self.assertEqual(self.parse(self.header()), self.names)
        self.assertEqual(self.parse(self.header(self.body.replace('-1', '-0x1').replace('=13', '=0xD').replace('=31', '=0x1F') + ',')), self.names)
        unrelated = ('\nenum { VOICE_DICTIONARY_OUTSIDE_ALIAS=13 };\n'
                     '// VOICE_DICTIONARY_COMMENT_ALIAS=99\n'
                     'const char *text="typedef enum VoiceDictionaryId { VOICE_DICTIONARY_STRING_ALIAS=0 } VoiceDictionaryId;";')
        self.assertEqual(self.parse(self.header() + unrelated), self.names)

    def test_duplicate_values_names_alias_expressions_and_implicit_fail(self):
        cases = (self.body.replace('FEEDBACK=13', 'FEEDBACK=0'),
                 self.body.replace('VOICE_DICTIONARY_WORLD_MAP=31', 'VOICE_DICTIONARY_DEFAULT=31'),
                 self.body.replace('FEEDBACK=13', 'FEEDBACK=VOICE_DICTIONARY_DEFAULT'),
                 self.body.replace('FEEDBACK=13', 'FEEDBACK=13+0'),
                 self.body.replace('VOICE_DICTIONARY_DRAWING_FEEDBACK=13', 'VOICE_DICTIONARY_DRAWING_FEEDBACK'),
                 self.body + ',VOICE_DICTIONARY_SECOND_NAME=13',
                 self.body.replace('FEEDBACK=13', 'FEEDBACK=-13'),
                 self.body.replace('NONE=-1', 'NONE=-2'),
                 self.body.replace('NONE=-1', 'NONE=-2147483649'),
                 self.body.replace('WORLD_MAP=31', 'WORLD_MAP=2147483648'))
        for body in cases:
            with self.subTest(body=body), self.assertRaises(ValueError):
                self.parse(self.header(body))
        with self.assertRaises(ValueError):
            self.parse(self.header() + self.header())
        with self.assertRaises(ValueError):
            self.parse(self.header().replace('VoiceDictionaryId', 'OtherDictionaryId'))

    def test_negative_extension_is_explicit_and_does_not_weaken_other_enums(self):
        code = 'typedef enum AuthoredValue { AUTHORED_NEGATIVE=-1 } AuthoredValue;'
        digest = hashlib.sha256(json.dumps({-1: 'AUTHORED_NEGATIVE'}, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
        with self.assertRaises(ValueError):
            vocabulary._reviewed_enum(code, 'AuthoredValue', 'AUTHORED_', digest)
        self.assertEqual(vocabulary._reviewed_enum(code, 'AuthoredValue', 'AUTHORED_', digest, allow_negative=True), {-1: 'AUTHORED_NEGATIVE'})
        self.assertEqual(vocabulary.dictionary_names(), self.names)

    def test_catalog_descriptor13_named_without_category9_selector_aliases(self):
        image, bank = authored_vocabulary(((9, 0), (9, 1), (9, 2)), [[0, 1, 2] for _ in range(43)])
        with patch.object(vocabulary, 'VocabularyBank', bank):
            report = vocabulary.vocabulary_catalog(image, validate_phrases=False, validate_general_commands=False,
                validate_map_destinations=False, validate_item_noun_groups=False, validate_directives=False,
                validate_feedback_questions=False)
        self.assertEqual(report['dictionaries'][13]['recovered_name'], 'VOICE_DICTIONARY_DRAWING_FEEDBACK')
        self.assertEqual(report['dictionaries'][13]['word_ids'], [0, 1, 2])
        self.assertEqual(report['words'][1]['dictionary_memberships'][13], dict(dictionary_id=13, entry_index=1))
        self.assertTrue(all(not any(key.startswith('recovered_') for key in word) for word in report['words']))

    def test_activity_descriptor_names_preserve_scoped_records_and_duplicate_entries(self):
        records = ((6, 51), (3, 26), (2, 35), (11, 0), (9, 1))
        dictionaries = [[(identifier + 1) % 5, identifier % 5, identifier % 5] for identifier in range(43)]
        image, bank = authored_vocabulary(records, dictionaries)
        with patch.object(vocabulary, 'VocabularyBank', bank):
            report = vocabulary.vocabulary_catalog(image, validate_phrases=False, validate_general_commands=False,
                validate_map_destinations=False, validate_item_noun_groups=False, validate_directives=False,
                validate_feedback_questions=False)
        for identifier in (1, 3, 4, 10, 11, 34, 42):
            descriptor = report['dictionaries'][identifier]
            self.assertEqual(descriptor['recovered_name'], self.names[identifier])
            self.assertEqual(descriptor['word_ids'], dictionaries[identifier])
            for entry_index, word_id in enumerate(dictionaries[identifier]):
                self.assertIn(dict(dictionary_id=identifier, entry_index=entry_index), report['words'][word_id]['dictionary_memberships'])
        for identifier in (2, 6, 12):
            self.assertNotIn('recovered_name', report['dictionaries'][identifier])
        self.assertEqual([word['word_id'] for word in report['words']], list(range(5)))
        self.assertEqual([(word['command_type'], word['command_value']) for word in report['words']], list(records))
        self.assertTrue(all(not any(key.startswith('recovered_') for key in word) for word in report['words']))
        with self.assertRaises(ValueError):
            self.parse(self.header('VOICE_DICTIONARY_NONE=-1,VOICE_DICTIONARY_DEFAULT=0,VOICE_DICTIONARY_DRAWING_FEEDBACK=13,VOICE_DICTIONARY_WORLD_MAP=31'))


if __name__ == '__main__':
    unittest.main()
