"""Authored instruction graphs exercise response-domain and closure boundaries."""

import unittest
import struct
import hashlib
from unittest.mock import patch

from tools.script_decoder.actions import CONTRACTS
from tools.script_decoder.responses import (bounded_graph, helper_closure,
                                             dispatch_branches, item_response_dispatch,
                                             cost_policy_records, link_costed_request, costed_action_requests)


def operand(value, tag=0):
    return dict(tag=tag, kind='integer' if tag == 0 else 'code_reference', value=value)


def row(offset, name, operands=(), successors=(), **extra):
    return dict(offset=offset, name='SCRIPT_OP_' + name, operands=list(operands),
                successors=[dict(kind=kind, offset=target) for kind, target in successors],
                size=1, **extra)


def frame(offset, following, arguments=0):
    return row(offset, 'ENTER_FRAME', [operand(arguments), operand(arguments), operand(1)],
               [('fallthrough', following)])


class ResponseDispatchTests(unittest.TestCase):
    def test_conditional_call_union_and_recursive_graph_is_bounded(self):
        instructions = [frame(0, 1), row(1, 'JUMP_IF_ZERO', [operand(5, 2)], [('fallthrough', 2), ('branch', 5)]),
            row(2, 'CALL', [operand(10, 2)], [('return_continuation', 3), ('call', 10)]),
            row(3, 'PUSH_LITERAL', [operand(0)], [('fallthrough', 4)]), row(4, 'RETURN'),
            row(5, 'CALL', [operand(20, 2)], [('return_continuation', 3), ('call', 20)]),
            frame(10, 11), row(11, 'CALL', [operand(0, 2)], [('return_continuation', 12), ('call', 0)]),
            row(12, 'PUSH_LITERAL', [operand(0)], [('fallthrough', 13)]), row(13, 'RETURN'),
            frame(20, 21), row(21, 'PUSH_LITERAL', [operand(0)], [('fallthrough', 22)]), row(22, 'RETURN')]
        result = helper_closure(instructions, [0])
        self.assertEqual([g['entry_offset'] for g in result['helpers']], [0, 10, 20])
        self.assertTrue(result['static_graph_complete'])
        self.assertIn('need not execute', result['scope'])
        self.assertEqual([e['target_offset'] for e in result['helpers'][0]['script_calls']], [20, 10])
        self.assertFalse(helper_closure(instructions, [0], limit=1)['static_graph_complete'])

    def test_bad_callee_and_missing_continuation_are_not_complete(self):
        instructions = [frame(0, 1), row(1, 'CALL', [dict(tag=4, value=9)], [])]
        result = helper_closure(instructions, [0])
        self.assertFalse(result['static_graph_complete'])
        self.assertIsNone(result['helpers'][0]['script_calls'][0]['target_offset'])
        self.assertIn('reason', result['helpers'][0]['script_calls'][0])

    def test_derived_branch_code_and_call_arguments(self):
        instructions = [frame(0, 1, 1),
            row(1, 'LOAD_VARIABLE', [operand(1), operand(0)], [('fallthrough', 2)]),
            row(2, 'PUSH_LITERAL', [operand(7)], [('fallthrough', 3)]),
            row(3, 'EQUAL', successors=[('fallthrough', 4)]),
            row(4, 'JUMP_IF_ZERO', [operand(8, 2)], [('fallthrough', 5), ('branch', 8)]),
            row(5, 'PUSH_LITERAL', [operand(42)], [('fallthrough', 6)]),
            row(6, 'CALL', [operand(20, 2)], [('return_continuation', 7), ('call', 20)]),
            row(7, 'JUMP', [operand(10, 2)], [('branch', 10)]),
            row(8, 'PUSH_LITERAL', [operand(0)], [('fallthrough', 9)]), row(9, 'RETURN'),
            row(10, 'PUSH_LITERAL', [operand(0)], [('fallthrough', 11)]), row(11, 'RETURN'),
            frame(20, 21, 1), row(21, 'PUSH_LITERAL', [operand(0)], [('fallthrough', 22)]), row(22, 'RETURN')]
        branches = dispatch_branches(instructions, 0)
        self.assertEqual(branches[0]['response_code'], 7)
        self.assertEqual(branches[0]['script_calls'][0]['arguments'][0]['value'], 42)
        instructions[7]['name'] = 'SCRIPT_OP_DISCARD'
        instructions[7]['successors'] = [dict(kind='fallthrough', offset=7)]
        with self.assertRaisesRegex(ValueError, 'cyclic'):
            dispatch_branches(instructions, 0)

    def test_cost_selector_is_not_a_timed_program_and_eating_is(self):
        instructions = [frame(0, 1), row(1, 'PUSH_LITERAL', [operand(1)], [('fallthrough', 2)]),
            row(2, 'PUSH_LITERAL', [operand(1)], [('fallthrough', 3)]),
            row(3, 'CALL_NATIVE', successors=[('resume_or_fallthrough', 4)], native=dict(functions=['actorScriptStartCostedAction'])),
            row(4, 'DISCARD', successors=[('fallthrough', 5)]),
            row(5, 'PUSH_LITERAL', [operand(107)], [('fallthrough', 6)]),
            row(6, 'CALL_NATIVE', successors=[('resume_or_fallthrough', 7)], native=dict(functions=['actorScriptStartEatingHeldItem'])),
            row(7, 'FINISH')]
        result = bounded_graph(instructions, 0)
        self.assertEqual([r['constant_script_id'] for r in result['timed_action_requests']], [107])
        self.assertEqual(result['native_calls'][0]['arguments']['cost_selector']['value'], 1)
        self.assertIn('not_timed', result['native_calls'][0]['argument_domain'])
        self.assertEqual(CONTRACTS['actorScriptStartEatingHeldItem']['consumption'], 'deferred')
        self.assertNotIn('actorScriptStartCostedAction', CONTRACTS)

    def test_cost_policy_conditional_programs_and_selector_bounds(self):
        raw = b''.join(struct.pack('>iifi', 300 + i, 400 + i, 2.5, i != 4) for i in range(5))
        policies = cost_policy_records(raw)
        request = dict(arguments=dict(cost_selector=dict(kind='integer', value=1),
                                      repetitions=dict(kind='variable', frame=1, slot=0)))
        linked = link_costed_request(request, policies)
        self.assertEqual(linked['policy']['possible_timed_program_ids'], [301, 401])
        self.assertEqual(linked['repetitions_expression']['kind'], 'variable')
        request['arguments']['cost_selector']['value'] = 4
        linked = link_costed_request(request, policies)
        self.assertEqual(linked['policy']['possible_timed_program_ids'], [304])
        self.assertEqual(linked['policy']['referenced_timed_program_ids'], [304, 404])
        for value in (-1, 5):
            request['arguments']['cost_selector']['value'] = value
            self.assertEqual(link_costed_request(request, policies)['status'], 'outside_reviewed_selector_view')
        request['arguments']['cost_selector'] = dict(kind='variable', frame=1, slot=0)
        self.assertEqual(link_costed_request(request, policies)['status'], 'unresolved_selector')
        request['arguments']['cost_selector'] = dict(kind='float', value=float('nan'))
        self.assertEqual(link_costed_request(request, policies)['status'], 'unresolved_selector')
        with self.assertRaises(ValueError):
            cost_policy_records(raw[:-1])
        with self.assertRaises(ValueError):
            cost_policy_records(struct.pack('>iifi', 1, 2, float('nan'), 1) + raw[16:])


    def test_global_costed_requests_keep_order_dynamic_and_unsupported_selectors(self):
        policies = cost_policy_records(b''.join(struct.pack('>iifi', 300+i, 400+i, 2.5, 1) for i in range(5)))
        instructions = [frame(0, 1),
            row(1, 'PUSH_LITERAL', [operand(3)], [('fallthrough', 2)]),
            row(2, 'PUSH_LITERAL', [operand(9)], [('fallthrough', 3)]),
            row(3, 'CALL_NATIVE', successors=[('resume_or_fallthrough', 4)], native=dict(functions=['actorScriptStartCostedAction'])),
            row(4, 'DISCARD', successors=[('fallthrough', 5)]),
            row(5, 'LOAD_VARIABLE', [operand(1), operand(0)], [('fallthrough', 6)]),
            row(6, 'PUSH_LITERAL', [operand(2)], [('fallthrough', 7)]),
            row(7, 'CALL_NATIVE', successors=[('resume_or_fallthrough', 8)], native=dict(functions=['actorScriptStartCostedAction'])),
            row(8, 'DISCARD', successors=[('fallthrough', 9)]),
            row(9, 'PUSH_LITERAL', [operand(5)], [('fallthrough', 10)]),
            row(10, 'PUSH_LITERAL', [operand(1)], [('fallthrough', 11)]),
            row(11, 'CALL_NATIVE', successors=[('resume_or_fallthrough', 12)], native=dict(functions=['actorScriptStartCostedAction'])),
            row(12, 'FINISH')]
        requests = costed_action_requests(instructions, policies)
        self.assertEqual(len(requests), 3)
        self.assertEqual(requests[0]['arguments']['cost_selector']['value'], 3)
        self.assertEqual(requests[0]['arguments']['repetitions']['value'], 9)
        self.assertEqual(requests[0]['cost_policy_link']['policy']['referenced_timed_program_ids'], [303, 403])
        self.assertEqual(requests[1]['status'], 'recovered_arguments')
        self.assertEqual(requests[1]['cost_policy_link']['status'], 'unresolved_selector')
        self.assertEqual(requests[2]['cost_policy_link']['status'], 'outside_reviewed_selector_view')
        self.assertFalse(any('constant_script_id' in r for r in requests))
        # A second entry into an operand prevents a guessed request.
        instructions.append(row(20, 'JUMP', [operand(2, 2)], [('branch', 2)]))
        self.assertEqual(costed_action_requests(instructions, policies)[0]['status'], 'unresolved_arguments')
        instructions[3]['native']['functions'].append('otherCommand')
        self.assertIn('not unique', costed_action_requests(instructions, policies)[0]['reason'])

    def test_cost_policy_authentication_rejects_changed_table(self):
        class Image:
            allowed = {'.ovl'}
            def read_address(self, address, size):
                if address == 0x8014FED4:
                    return (0x8013C8B8).to_bytes(4, 'big') + bytes(8), {}
                return bytes(size), {}
        cfg = dict(address=0x8014FED4, section='.scene_bedroom', program=0x8013C8B8, window_bytes=79388)
        with patch('tools.script_decoder.responses.PROGRAM_SHA256', hashlib.sha256(bytes(79388)).hexdigest()):
            with self.assertRaisesRegex(ValueError, 'cost policy table authentication'):
                item_response_dispatch(Image(), [cfg], {})

    def test_authentication_rejects_identity_and_changed_bytes(self):
        class Image:
            allowed = {'.ovl'}
            def read_address(self, address, size):
                return bytes(size), {}
        cfg = dict(address=0x8014FED4, section='.scene_bedroom', program=0, window_bytes=79388)
        with self.assertRaisesRegex(ValueError, 'identity'):
            item_response_dispatch(Image(), [cfg], {})
        cfg['program'] = 0x8013C8B8
        image = Image()
        with self.assertRaisesRegex(ValueError, 'authentication'):
            item_response_dispatch(image, [cfg], {})
        self.assertEqual(image.allowed, {'.ovl'})


if __name__ == '__main__':
    unittest.main()
