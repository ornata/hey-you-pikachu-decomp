"""Authored policy enums and records guard source-name binding boundaries."""

import hashlib
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from tools.script_decoder.responses import (
    cost_policy_catalog, cost_policy_names, cost_policy_records, link_costed_request,
)


NAMES = [
    'ACTOR_COSTED_ACTION_POLICY_THUNDER',
    'ACTOR_COSTED_ACTION_POLICY_THUNDERBOLT',
    'ACTOR_COSTED_ACTION_POLICY_THUNDERSHOCK',
    'ACTOR_COSTED_ACTION_POLICY_THUNDER_SKIP_RESOURCE_CHECK',
    'ACTOR_COSTED_ACTION_POLICY_THUNDERBOLT_SKIP_RESOURCE_CHECK',
]


def declaration(entries=None):
    entries = entries or [f'{name} = {index}' for index, name in enumerate(NAMES)]
    return 'typedef enum ActorCostedActionPolicy {\n' + ',\n'.join(entries) + ',\n} ActorCostedActionPolicy;'


class PolicyNameTests(unittest.TestCase):
    def test_scoped_enum_ignores_comments_strings_and_other_enums(self):
        source = ('/* enum ActorCostedActionPolicy { bogus = 99 }; */\n'
                  'const char *text = "enum ActorCostedActionPolicy { bogus = 98 };";\n'
                  'enum Other { ACTOR_COSTED_ACTION_POLICY_DECOY = 200 };\n' +
                  declaration([f'{name} /* role */ = 0x{index:X}' for index, name in enumerate(NAMES)]))
        self.assertEqual(cost_policy_names(source), dict(enumerate(NAMES)))

    def test_malformed_or_changed_taxonomy_is_rejected(self):
        entries = [f'{name} = {index}' for index, name in enumerate(NAMES)]
        sources = [
            '', declaration() + declaration(), declaration(entries[:-1]),
            declaration(entries + ['EXTRA = 5']),
            declaration(entries[:-1] + [f'{NAMES[4]} = 0']),
            declaration(entries[:-1] + [f'{NAMES[0]} = 4']),
            declaration(entries[:-1] + [f'{NAMES[4]} = -1']),
            declaration(entries[:-1] + [f'{NAMES[4]} = 5']),
            declaration(entries[:-1] + [f'{NAMES[4]} = 2 + 2']),
            declaration(entries[:-1] + [NAMES[4]]),
            declaration([f'{NAMES[1]} = 0', f'{NAMES[0]} = 1'] + entries[2:]),
            declaration().replace('}', ''),
        ]
        for source in sources:
            with self.subTest(source=source), self.assertRaises(ValueError):
                cost_policy_names(source)

    def test_catalog_authenticates_before_binding_names_and_keeps_numeric_ids(self):
        raw = b''.join(struct.pack('>iifi', 300 + i, 400 + i, 2.5, i % 2) for i in range(5))

        class Image:
            def __init__(self):
                self.allowed = {'.boot'}

            def read_address(self, address, size):
                self.request = address, size
                return raw, {'section': '.ovl'}

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'include').mkdir()
            header = root / 'include/actor_script.h'
            header.write_text(declaration())
            image = Image()
            with patch('tools.script_decoder.responses.ROOT', root), patch(
                    'tools.script_decoder.responses.COST_POLICY_SHA256', hashlib.sha256(raw).hexdigest()):
                result = cost_policy_catalog(image)
                self.assertEqual(image.allowed, {'.boot'})
                self.assertEqual(image.request, (0x8012A660, 80))
                for index, record in enumerate(result['records']):
                    self.assertEqual(record['policy_name'], NAMES[index])
                    self.assertEqual(record['cost_selector'], index)
                    self.assertEqual(record['program_id'], 300 + index)
                    self.assertEqual(record['insufficient_program_id'], 400 + index)
                binding = result['name_source']
                self.assertEqual(binding['file'], 'include/actor_script.h')
                self.assertEqual(binding['enum'], 'ActorCostedActionPolicy')
                header.write_text('/* unrelated edit */\n' + declaration())
                self.assertEqual(cost_policy_catalog(image)['name_source']['sha256'], binding['sha256'])
                header.write_text(declaration().replace(NAMES[0], 'UNREVIEWED_NAME'))
                with self.assertRaisesRegex(ValueError, 'reviewed selectors'):
                    cost_policy_catalog(image)
            # Bad ROM data is rejected before even opening the source header.
            header.unlink()
            with patch('tools.script_decoder.responses.ROOT', root), self.assertRaisesRegex(
                    ValueError, 'table authentication'):
                cost_policy_catalog(image)

    def test_pure_records_and_outside_selector_are_not_named(self):
        raw = b''.join(struct.pack('>iifi', 300 + i, 400 + i, 1.0, 1) for i in range(5))
        records = cost_policy_records(raw)
        self.assertFalse(any('policy_name' in record for record in records))
        result = link_costed_request({'arguments': {'cost_selector': {'kind': 'integer', 'value': 5}}}, records)
        self.assertEqual(result['status'], 'outside_reviewed_selector_view')
        self.assertNotIn('policy', result)


if __name__ == '__main__':
    unittest.main()
