"""Check callback-table audit boundaries independently of the retail build."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from native_commands import audit_entries


class NativeCommandTests(unittest.TestCase):
    def definition(self, **changes):
        return dict({'file': 'src/command.c', 'line': 1, 'returns': 's32', 'parameters': ['u8*']}, **changes)

    def test_reserved_slot_and_actor_signature(self):
        result = audit_entries([0, 0x1000], {0x1000: ['command']}, {'command': self.definition()})
        self.assertEqual(result['errors'], [])
        self.assertEqual(result['entries'][1]['function'], 'command')

    def test_extra_argument_integer_address_and_wrong_result_are_rejected(self):
        for changes in [{'parameters': ['u8*', 'ScriptVm*']}, {'parameters': ['s32']}, {'returns': 'void'}]:
            with self.subTest(changes=changes):
                result = audit_entries([0, 0x1000], {0x1000: ['command']}, {'command': self.definition(**changes)})
                self.assertTrue(any('incompatible' in error for error in result['errors']))

    def test_null_slots_and_unwritten_targets_are_rejected(self):
        result = audit_entries([0, 0, 0x1000], {0x1000: ['assemblyOnly']}, {})
        self.assertTrue(any('null callback' in error for error in result['errors']))
        self.assertTrue(any('written C definition' in error for error in result['errors']))

    def test_missing_reserved_slot_and_ambiguous_definitions_are_rejected(self):
        result = audit_entries([0x1000], {0x1000: ['first', 'second']},
                               {'first': self.definition(), 'second': self.definition()})
        self.assertTrue(any('selector zero' in error for error in result['errors']))
        self.assertTrue(any('one written C definition' in error for error in result['errors']))
