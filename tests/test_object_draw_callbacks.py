"""Exercise retained object callback signature checks without requiring a ROM."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from object_draw_callbacks import audit_records


class ObjectDrawCallbackTests(unittest.TestCase):
    def definition(self, returns, parameters):
        return {'file': 'src/callback.c', 'line': 1, 'returns': returns, 'parameters': parameters}

    def test_all_three_supported_contracts(self):
        definitions = {'draw': self.definition('Gfx*', ['u8*']),
                       'child': self.definition('void', ['u8*']),
                       'response': self.definition('void', ['u8*', 'ObjectResponse*'])}
        result = audit_records([(0, 0, 0, 1, 2, 3)], {1: ['draw'], 2: ['child'], 3: ['response']}, definitions)
        self.assertEqual(result['errors'], [])
        self.assertEqual(len(result['entries']), 3)

    def test_legacy_result_extra_child_argument_and_raw_response_fail(self):
        for slot, definition in [(3, self.definition('s32', ['u8*'])),
                                 (4, self.definition('void', ['u8*', 'u8*'])),
                                 (5, self.definition('void', ['u8*', 's32*']))]:
            with self.subTest(slot=slot):
                record = [0] * 6
                record[slot] = 1
                result = audit_records([record], {1: ['callback']}, {'callback': definition})
                self.assertTrue(any('incompatible' in error for error in result['errors']))

    def test_optional_nulls(self):
        self.assertEqual(audit_records([(0, 0, 0, 0, 0, 0)], {}, {})['errors'], [])

    def test_lifecycle_slots_require_actor_pointer_and_void_return(self):
        for slot in range(3):
            for returns, parameters, valid in [('void', ['u8*'], True),
                                                ('s32', ['u8*'], False),
                                                ('void', ['RuntimeObjectPrefix*'], False),
                                                ('void', ['u8*', 'u8*'], False)]:
                with self.subTest(slot=slot, returns=returns, parameters=parameters):
                    record = [0] * 6
                    record[slot] = 1
                    result = audit_records([record], {1: ['hook']},
                                           {'hook': self.definition(returns, parameters)})
                    self.assertEqual(not result['errors'], valid)

    def test_unwritten_or_ambiguous_callback_targets_fail(self):
        for names, definitions in [({1: ['missing']}, {}),
                                   ({1: ['a', 'b']}, {'a': self.definition('Gfx*', ['u8*']),
                                                     'b': self.definition('Gfx*', ['u8*'])})]:
            self.assertTrue(audit_records([(0, 0, 0, 1, 0, 0)], names, definitions)['errors'])

    def test_wrong_record_stride_fails(self):
        with self.assertRaisesRegex(ValueError, 'six words'):
            audit_records([(0, 0, 0, 0)], {}, {})
