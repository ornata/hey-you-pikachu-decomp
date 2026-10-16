"""Authored policy requests preserve full-word and domain distinctions."""

from pathlib import Path
import tempfile
import unittest

from tools.script_decoder.contacts import messenger_contact_policy_catalog, messenger_contact_requests


def value(number, tag=0):
    return dict(tag=tag, value=number, kind='float' if tag == 1 else 'integer')


def instruction(offset, name, operands=(), successors=(), **extra):
    return dict(offset=offset, size=1, name='SCRIPT_OP_'+name, operands=list(operands),
                successors=[dict(kind=kind, offset=target) for kind, target in successors], **extra)


class MessengerContactTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root/'include').mkdir()
        self.header = self.root/'include/messenger_contact.h'
        self.enum = '''typedef enum MessengerContactPolicy {
            MESSENGER_CONTACT_DIRECT = 0, MESSENGER_CONTACT_REBOUND = 1,
            MESSENGER_CONTACT_ACTION_ORBIT = 2, MESSENGER_CONTACT_PREFER_ORBIT = 3
        } MessengerContactPolicy;'''
        self.header.write_text(self.enum)
        self.catalog = messenger_contact_policy_catalog(self.root)

    def request(self, number, tag=0):
        return messenger_contact_requests([
            instruction(0, 'PUSH_LITERAL', [value(number, tag)], [('fallthrough', 1)]),
            instruction(1, 'CALL_NATIVE', native=dict(functions=['actorScriptSetMessengerContactMode']))], self.catalog)[0]

    def test_full_signed_word_keeps_high_bits_and_arbitrary_values(self):
        for number in (-2147483648, -65535, -1, 4, 65537, 2147483647):
            result = self.request(number)
            self.assertEqual(result['constant_policy'], number)
            self.assertEqual(result['policy_status'], 'unnamed_signed_word')
            self.assertNotIn('policy_name', result)
        for number in range(4):
            self.assertEqual(self.request(number)['policy_status'], 'known_policy')
        self.assertEqual(self.request(1)['policy_name'], 'MESSENGER_CONTACT_REBOUND')
        self.assertIn('recipient', self.request(1)['argument_domain'])

    def test_float_truncation_and_conversion_boundaries(self):
        self.assertEqual(self.request(1.9, 1)['constant_policy'], 1)
        self.assertEqual(self.request(-2.9, 1)['constant_policy'], -2)
        self.assertEqual(self.request(-2147483648.0, 1)['constant_policy'], -2147483648)
        for number in (2147483648.0, -2147483904.0, float('nan'), float('inf')):
            result = self.request(number, 1)
            self.assertEqual(result['policy_status'], 'unsupported_integer_conversion')
            self.assertNotIn('constant_policy', result)

    def test_dynamic_missing_and_ambiguous_binding_are_not_named(self):
        dynamic = [instruction(0, 'LOAD_VARIABLE', [value(1), value(7)], [('fallthrough', 1)]),
                   instruction(1, 'CALL_NATIVE', native=dict(functions=['actorScriptSetMessengerContactMode']))]
        result = messenger_contact_requests(dynamic, self.catalog)[0]
        self.assertEqual(result['policy_status'], 'dynamic_policy')
        self.assertEqual(result['policy_expression'], dict(kind='variable', frame=1, slot=7))
        missing = messenger_contact_requests(dynamic[1:], self.catalog)[0]
        self.assertEqual(missing['status'], 'unresolved_argument')
        dynamic[1]['native']['functions'].append('unrelatedFlightCommand')
        self.assertIn('not unique', messenger_contact_requests(dynamic, self.catalog)[0]['reason'])
        # A competing predecessor prevents attributing another path's literal.
        dynamic[1]['native']['functions'] = ['actorScriptSetMessengerContactMode']
        dynamic.append(instruction(3, 'JUMP', [value(1, 2)], [('branch', 1)]))
        self.assertEqual(messenger_contact_requests(dynamic, self.catalog)[0]['status'], 'unresolved_argument')

    def test_scoped_enum_parser_rejects_duplicate_and_implicit_members(self):
        self.header.write_text('enum FlightState { FLIGHT_REBOUND=88 };\n'+self.enum)
        self.assertEqual(len(messenger_contact_policy_catalog(self.root)['known_policies']), 4)
        for text in (self.enum + '\n' + self.enum, self.enum.replace('REBOUND = 1', 'REBOUND = 0'),
                     self.enum.replace('PREFER_ORBIT = 3', 'REBOUND = 3'),
                     self.enum.replace('REBOUND = 1', 'REBOUND'),
                     self.enum.replace('PREFER_ORBIT = 3', 'PREFER_ORBIT = 4'),
                     self.enum.replace('REBOUND = 1', 'REBOUND = (1 << 0)'),
                     'enum Unrelated { MESSENGER_CONTACT_DIRECT=0 };'):
            self.header.write_text(text)
            with self.assertRaises(ValueError):
                messenger_contact_policy_catalog(self.root)


if __name__ == '__main__':
    unittest.main()
