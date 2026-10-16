"""Authored feedback/count-question fixtures keep message categories separate."""

import copy
import hashlib
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.script_decoder import vocabulary
from tests.vocabulary_fixture import authored_vocabulary


def digest(names):
    return hashlib.sha256(json.dumps(names, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


class FeedbackQuestionVocabularyTests(unittest.TestCase):
    names = {
        4: {0: 'VOICE_INSULT_AUTHORED_A'},
        5: {0: 'VOICE_PRAISE_AUTHORED_A', 1: 'VOICE_PRAISE_AUTHORED_B'},
        11: {0: 'VOICE_COUNT_QUESTION_AUTHORED_A'},
    }

    def bindings(self):
        bindings = copy.deepcopy(vocabulary.FEEDBACK_QUESTION_BINDINGS)
        for category, binding in bindings.items():
            binding['mapping_digest'] = digest(self.names[category])
        return bindings

    def header(self):
        lines = []
        for category, binding in self.bindings().items():
            lines.append(f"enum {{ {binding['category_constant']} = {category} }};")
            members = ', '.join(f'{name}={value}' for value, name in self.names[category].items())
            lines.append(f"typedef enum {binding['enum']} {{ {members} }} {binding['enum']};")
        return '\n'.join(lines)

    def parse(self, text, bindings=None):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'include').mkdir()
            (root / 'include/voice_feedback_questions.h').write_text(text)
            with patch.object(vocabulary, 'ROOT', root), patch.object(vocabulary, 'FEEDBACK_QUESTION_BINDINGS', self.bindings() if bindings is None else bindings):
                return vocabulary.feedback_question_names()

    def test_strict_each_category_enum_and_digest(self):
        text = self.header()
        self.assertEqual(self.parse(text), self.names)
        self.assertEqual(self.parse(text + '\n// VOICE_PRAISE_FAKE=99'), self.names)
        for bad in (text.replace('VOICE_INSULT_MESSAGE_TYPE = 4', 'VOICE_INSULT_MESSAGE_TYPE = 5'),
                    text.replace('VOICE_PRAISE_MESSAGE_TYPE = 5', 'VOICE_PRAISE_MESSAGE_TYPE = 5+0'),
                    text.replace('VOICE_PRAISE_AUTHORED_B=1', 'VOICE_PRAISE_AUTHORED_B=0'),
                    text.replace('VOICE_PRAISE_AUTHORED_B=1', 'VOICE_PRAISE_AUTHORED_A=1'),
                    text.replace('VOICE_PRAISE_AUTHORED_B=1', 'VOICE_PRAISE_AUTHORED_B=1+0'),
                    text.replace('VOICE_COUNT_QUESTION_AUTHORED_A=0', 'VOICE_COUNT_QUESTION_CHANGED_A=0'),
                    text + '\nenum { VOICE_INSULT_MESSAGE_TYPE = 4 };'):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                self.parse(bad)

    def test_registry_missing_extra_and_stale_fail_closed(self):
        for keys in (set(), {4, 5}, {4, 5, 9, 11}):
            bindings = {key: self.bindings().get(key, {}) for key in keys}
            with self.subTest(keys=keys), self.assertRaises(ValueError):
                self.parse(self.header(), bindings)
        bindings = self.bindings()
        bindings[5]['mapping_digest'] = 'stale'
        with self.assertRaises(ValueError):
            self.parse(self.header(), bindings)

    def catalog(self, *, missing_category=None, later_only=False, count=43, enabled=True):
        records = ((4, 0), (5, 0), (5, 1), (11, 0), (9, 0), (9, 1), (9, 2), (4, 999), (5, 999), (11, 999), (3, 0), (4, 1))
        dictionaries = []
        for identifier in range(count):
            words = [i for i in range(4) if records[i][0] != missing_category]
            if later_only and identifier < 43:
                words = [0, 1, 2]
            dictionaries.append(words or [4])
        image, bank = authored_vocabulary(records, dictionaries)
        with patch.object(vocabulary, 'VocabularyBank', bank), patch.object(vocabulary, 'feedback_question_names', return_value=self.names), patch.object(vocabulary, 'dictionary_names', return_value={}):
            return vocabulary.vocabulary_catalog(image, dictionary_count=count, validate_phrases=False,
                validate_general_commands=False, validate_map_destinations=False, validate_item_noun_groups=False,
                validate_directives=False, validate_feedback_questions=enabled)

    def test_categories_raw_nine_and_original_indexes_preserved(self):
        report = self.catalog()
        self.assertEqual([word['word_id'] for word in report['words']], list(range(12)))
        for index, category, value in ((0, 4, 0), (1, 5, 0), (2, 5, 1), (3, 11, 0)):
            self.assertEqual(report['words'][index]['recovered_feedback_question_name'], self.names[category][value])
            self.assertIn(dict(dictionary_id=42, entry_index=index), report['words'][index]['dictionary_memberships'])
        self.assertTrue(all('recovered_feedback_question_name' not in word for word in report['words'][4:]))
        self.assertEqual([word['command_value'] for word in report['words'][4:7]], [0, 1, 2])

    def test_membership_required_per_category_inside43(self):
        for category in (4, 5, 11):
            with self.subTest(category=category), self.assertRaises(ValueError):
                self.catalog(missing_category=category)
        with self.assertRaises(ValueError):
            self.catalog(later_only=True, count=44)
        self.assertEqual(len(self.catalog(count=44)['dictionaries']), 44)

    def test_independent_optout_and_prior_domains(self):
        self.assertTrue(all('recovered_feedback_question_name' not in word for word in self.catalog(enabled=False)['words']))
        self.assertEqual(len(vocabulary.general_command_names()), 23)
        self.assertEqual(len(vocabulary.directive_names()), 39)
        self.assertEqual(len(vocabulary.map_destination_names()), 11)
        self.assertEqual(len(vocabulary.item_noun_group_names()), 11)
        self.assertEqual(len(vocabulary.phrase_names()), 10)


if __name__ == '__main__':
    unittest.main()
