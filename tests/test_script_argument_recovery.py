"""Exercise expression recovery with authored bytecode, without ROM assets."""

import struct
import unittest

from tools.script_decoder.formats import decode_vm
from tools.script_decoder.progress_fields import progress_field_requests
from tools.script_decoder.stack import recover_arguments

NAMES = {
    3: 'SCRIPT_OP_NEGATE', 5: 'SCRIPT_OP_ADD', 6: 'SCRIPT_OP_SUBTRACT',
    7: 'SCRIPT_OP_MULTIPLY', 8: 'SCRIPT_OP_DIVIDE', 9: 'SCRIPT_OP_REMAINDER',
    24: 'SCRIPT_OP_PUSH_LITERAL',
    25: 'SCRIPT_OP_LOAD_VARIABLE', 27: 'SCRIPT_OP_CALL_NATIVE', 35: 'SCRIPT_OP_FINISH',
}
NATIVES = {
    7: dict(address=0x8011C270, functions=['actorScriptPassThroughInteger'], inline_operands=[]),
    236: dict(address=0x801202F8, functions=['actorScriptSetProgressValue'], inline_operands=[]),
    237: dict(address=0x80120348, functions=['actorScriptGetProgressValue'], inline_operands=[]),
    238: dict(address=0x80120388, functions=['actorScriptGetProgressMaximum'], inline_operands=[]),
    239: dict(address=0x801203C8, functions=['actorScriptGetProgressMinimum'], inline_operands=[]),
    999: dict(address=0x80000000, functions=['unreviewedCommand'], inline_operands=[]),
}


def value(number, tag=0):
    payload = struct.pack('>f', number) if tag == 1 else struct.pack('>i', number)
    return bytes([tag]) + payload


def literal(number, tag=0):
    return bytes([24]) + value(number, tag)


def variable(frame, slot):
    return bytes([25]) + value(frame) + value(slot)


def native(selector):
    return bytes([27]) + value(selector, 2)


def listing(*parts):
    report = decode_vm(b''.join(parts) + bytes([35]), NAMES, natives=NATIVES)
    assert not report['diagnostics'], report['diagnostics']
    return report['instructions'][:-1]


def arguments(rows, **options):
    return recover_arguments(rows, len(rows) - 1, 2, **options)


class ScriptArgumentRecoveryTests(unittest.TestCase):
    def test_product_links_selector_without_clamping_the_requested_value(self):
        rows = listing(literal(134), literal(5), literal(60), bytes([7]), native(236))
        catalog = dict(fields=[dict(field_id=134, enum_name='BEST_SECONDS')], area_selectors=[])
        request = progress_field_requests(rows, catalog)[0]
        self.assertEqual(request['constant_selector'], 134)
        self.assertEqual(request['value_expression']['value'], 300)
        self.assertEqual(request['value_expression']['expression']['operator'], 'integer_multiply')
        self.assertEqual(request['field']['enum_name'], 'BEST_SECONDS')

    def test_unknown_value_conversion_does_not_hide_the_independent_selector(self):
        rows = listing(literal(117), variable(-1, 37), native(7), native(236))
        selector, converted = arguments(rows, allow_symbolic=True)
        self.assertEqual(selector['value'], 117)
        self.assertEqual(converted['kind'], 'integer_conversion')
        self.assertEqual(converted['operand'], dict(kind='variable', frame=-1, slot=37))
        self.assertNotIn('value', converted)
        with self.assertRaises(ValueError):
            arguments(rows)

    def test_integer_conversion_retains_signed_edges_and_truncates_fractions(self):
        for number, tag, expected in [(-2147483648, 0, -2147483648),
                                      (2147483647, 2, 2147483647),
                                      (-7.75, 1, -7), (7.75, 1, 7), (-0.0, 1, 0),
                                      (2147483520.0, 1, 2147483520)]:
            with self.subTest(number=number, tag=tag):
                rows = listing(literal(117), literal(number, tag), native(7), native(236))
                converted = arguments(rows, allow_float=True)[1]
                self.assertEqual((converted['kind'], converted['value']), ('integer', expected))
                self.assertEqual(converted['expression']['operator'], 'to_integer')

    def test_invalid_float_and_string_conversions_stop_recovery(self):
        for number, tag in [(2147483648.0, 1), (float('inf'), 1),
                            (float('nan'), 1), (3, 3), (3, 4)]:
            with self.subTest(number=number, tag=tag):
                rows = listing(literal(117), literal(number, tag), native(7), native(236))
                with self.assertRaises(ValueError):
                    arguments(rows, allow_float=True, allow_symbolic=True)

    def test_multiply_preserves_signedness_and_rejects_overflow(self):
        for left, right, expected in [(-5, 60, -300), (-5, -60, 300),
                                      (0, 2147483647, 0), (-2147483648, 1, -2147483648),
                                      (1073741823, 2, 2147483646)]:
            rows = listing(literal(134), literal(left), literal(right), bytes([7]), native(236))
            self.assertEqual(arguments(rows)[1]['value'], expected)
        for left, right in [(2147483647, 2), (-2147483648, -1)]:
            rows = listing(literal(134), literal(left), literal(right), bytes([7]), native(236))
            with self.assertRaises(ValueError):
                arguments(rows, allow_symbolic=True)

    def test_float_or_unknown_multiplication_stays_symbolic(self):
        for source in [literal(1.25, 1), variable(-1, 37)]:
            rows = listing(literal(134), source, literal(60), bytes([7]), native(236))
            product = arguments(rows, allow_float=True, allow_symbolic=True)[1]
            self.assertEqual((product['kind'], product['operator']), ('vm_operator', 'multiply'))
            self.assertNotIn('value', product)
            with self.assertRaises(ValueError):
                arguments(rows, allow_float=True)

    def test_division_and_remainder_use_truncation_toward_zero(self):
        for left, right, quotient, remainder in [(7, 3, 2, 1), (-7, 3, -2, -1),
                                                  (7, -3, -2, 1), (-7, -3, 2, -1),
                                                  (-2147483648, 1, -2147483648, 0)]:
            for opcode, expected in [(8, quotient), (9, remainder)]:
                rows = listing(literal(155), literal(left), literal(right), bytes([opcode]), native(236))
                self.assertEqual(arguments(rows)[1]['value'], expected)
        for left, right in [(7, 0), (-2147483648, -1)]:
            for opcode in (8, 9):
                rows = listing(literal(155), literal(left), literal(right), bytes([opcode]), native(236))
                with self.assertRaisesRegex(ValueError, 'outside the supported signed-32 domain'):
                    arguments(rows, allow_symbolic=True)

    def test_saved_value_stays_unknown_in_a_halving_request(self):
        rows = listing(literal(155), literal(155), native(237), literal(2), bytes([8]), native(236))
        request = progress_field_requests(rows, dict(fields=[dict(field_id=155)], area_selectors=[]))[-1]
        self.assertEqual(request['constant_selector'], 155)
        result = request['value_expression']
        self.assertEqual((result['kind'], result['operator']), ('vm_operator', 'divide'))
        self.assertEqual(result['operands'][0]['function'], 'actorScriptGetProgressValue')
        self.assertEqual(result['operands'][0]['arguments'][0]['value'], 155)
        self.assertNotIn('value', result['operands'][0])
        self.assertNotIn('value', result)

    def test_progress_getter_identity_and_area_selector_are_preserved(self):
        for selector, function in [(237, 'actorScriptGetProgressValue'),
                                   (238, 'actorScriptGetProgressMaximum'),
                                   (239, 'actorScriptGetProgressMinimum')]:
            rows = listing(literal(155), literal(0x40000001), native(selector), native(236))
            queried = arguments(rows, allow_symbolic=True)[1]
            self.assertEqual(queried['function'], function)
            self.assertEqual(queried['result_tag'], 'integer')
            self.assertEqual(queried['arguments'][0]['value'], 0x40000001)
            self.assertNotIn('value', queried)

    def test_unknown_native_or_competing_entry_stops_recovery(self):
        rows = listing(literal(117), literal(3), native(999), native(236))
        with self.assertRaises(ValueError):
            arguments(rows, allow_symbolic=True)
        rows = listing(literal(117), literal(3), native(7), native(236))
        incoming = {rows[i]['offset']: {rows[i - 1]['offset']} for i in range(1, len(rows))}
        incoming[rows[2]['offset']].add(1000)
        with self.assertRaisesRegex(ValueError, 'Another control-flow edge'):
            arguments(rows, incoming=incoming, allow_symbolic=True)

    def test_nested_conversions_keep_the_producer_budget(self):
        rows = listing(literal(117), literal(3), *[native(7) for _ in range(30)], native(236))
        self.assertEqual(arguments(rows)[1]['value'], 3)
        rows = listing(literal(117), literal(3), *[native(7) for _ in range(31)], native(236))
        with self.assertRaisesRegex(ValueError, 'bounded producer window'):
            arguments(rows)


if __name__ == '__main__':
    unittest.main()
