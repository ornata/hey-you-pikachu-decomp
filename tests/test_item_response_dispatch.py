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




class LocalSelectorFlowTests(unittest.TestCase):
    def graph(self):
        # Both local definitions join, then an explicit !=5 edge filters the sentinel.
        return [frame(10, 11),
            row(11, 'JUMP_IF_ZERO', [operand(15, 2)], [('fallthrough', 12), ('branch', 15)]),
            row(12, 'PUSH_LITERAL', [operand(0)], [('fallthrough', 13)]),
            row(13, 'STORE_VARIABLE', [operand(1), operand(0)], [('fallthrough', 14)]),
            row(14, 'JUMP', [operand(18, 2)], [('branch', 18)]),
            row(15, 'PUSH_LITERAL', [operand(5)], [('fallthrough', 16)]),
            row(16, 'STORE_VARIABLE', [operand(1), operand(0)], [('fallthrough', 17)]),
            row(17, 'JUMP', [operand(18, 2)], [('branch', 18)]),
            row(18, 'LOAD_VARIABLE', [operand(1), operand(0)], [('fallthrough', 19)]),
            row(19, 'PUSH_LITERAL', [operand(5)], [('fallthrough', 20)]),
            row(20, 'NOT_EQUAL', successors=[('fallthrough', 21)]),
            row(21, 'JUMP_IF_ZERO', [operand(30, 2)], [('fallthrough', 22), ('branch', 30)]),
            row(22, 'LOAD_VARIABLE', [operand(1), operand(0)], [('fallthrough', 23)]),
            row(23, 'PUSH_LITERAL', [operand(1)], [('fallthrough', 24)]),
            row(24, 'CALL_NATIVE', successors=[('resume_or_fallthrough', 25)], native=dict(functions=['actorScriptStartCostedAction'])),
            row(25, 'FINISH'), row(30, 'FINISH')]

    def analyze(self, instructions, **kwargs):
        from tools.script_decoder.local_flow import local_integer_domain
        # One valid nonargument caller local.
        instructions[0]['operands'][1] = operand(1)
        return local_integer_domain(instructions, 24, 1, 0, **kwargs)

    def test_reaching_stores_and_same_value_guard(self):
        result = self.analyze(self.graph())
        self.assertEqual(result['status'], 'bounded_local_integer_domain')
        self.assertEqual(result['values'], [0])
        self.assertEqual([r['value'] for r in result['definitions']], [0, 5])
        self.assertEqual(result['guards'][0]['excluded_value'], 5)
        self.assertTrue(result['conditional_on_frame_lifetimes'])

    def test_guard_bypass_does_not_filter(self):
        instructions = self.graph()
        instructions[7]['successors'] = [dict(kind='branch', offset=22)]
        self.assertEqual(self.analyze(instructions)['values'], [0, 5])

    def test_noninteger_and_competing_predicate_inputs_do_not_filter(self):
        instructions = self.graph()
        instructions[9]['operands'] = [dict(tag=1, kind='float', value=5.2)]
        self.assertEqual(self.analyze(instructions)['values'], [0, 5])
        instructions = self.graph()
        instructions[7]['successors'] = [dict(kind='branch', offset=20)]
        self.assertEqual(self.analyze(instructions)['values'], [0, 5])

    def test_uninitialized_and_unknown_definitions_fail(self):
        instructions = self.graph()
        instructions[1]['successors'][1]['offset'] = 18
        self.assertEqual(self.analyze(instructions)['status'], 'unresolved_local_integer_domain')
        instructions = self.graph()
        instructions[5]['name'] = 'SCRIPT_OP_LOAD_VARIABLE'
        instructions[5]['operands'] = [operand(1), operand(0)]
        self.assertEqual(self.analyze(instructions)['status'], 'unresolved_local_integer_domain')

    def test_unreviewed_and_missing_native_binding_clobber(self):
        for binding in ({}, dict(native=dict(functions=['unknownCommand']))):
            instructions = self.graph()
            instructions[12] = row(22, 'CALL_NATIVE', successors=[('resume_or_fallthrough', 23)], **binding)
            result = self.analyze(instructions)
            self.assertEqual(result['status'], 'unresolved_local_integer_domain')
            self.assertIn('native', result['diagnostics'][0])

    def test_indirect_unknown_opcode_and_out_of_frame_writes_fail(self):
        for name, operands in [('STORE_ELEMENT', [operand(1), operand(0)]),
                               ('STORE_VARIABLE', [operand(1), operand(100000)]),
                               ('STORE_VARIABLE', [operand(0), operand(100000)]),
                               ('UNREVIEWED', [])]:
            instructions = self.graph()
            instructions[12] = row(22, name, operands, [('fallthrough', 23)])
            self.assertEqual(self.analyze(instructions)['status'], 'unresolved_local_integer_domain')

    def test_loop_budget_and_competing_frame_entries_fail(self):
        self.assertEqual(self.analyze(self.graph(), limit=1)['status'], 'unresolved_local_integer_domain')
        instructions = self.graph()
        instructions.append(frame(40, 22))
        self.assertEqual(self.analyze(instructions)['status'], 'unresolved_local_integer_domain')
        instructions = self.graph()
        instructions[12]['successors'] = [dict(kind='fallthrough', offset=22)]
        self.assertEqual(self.analyze(instructions, limit=1)['status'], 'unresolved_local_integer_domain')

    def test_script_call_preservation_checks_nested_and_borrowed_extents(self):
        from tools.script_decoder.local_flow import local_integer_domain
        for coordinate, expected in (([1, 0], 'bounded_local_integer_domain'),
                                     ([1, 100000], 'unresolved_local_integer_domain'),
                                     ([0, 258], 'bounded_local_integer_domain'),
                                     ([0, 100000], 'unresolved_local_integer_domain')):
            instructions = self.graph()
            instructions[0]['operands'][1] = operand(1)
            instructions[12] = row(22, 'CALL', [operand(50, 2)], [('return_continuation', 23), ('call', 50)])
            instructions.extend([frame(50, 51), row(51, 'PUSH_LITERAL', [operand(9)], [('fallthrough', 52)]),
                row(52, 'STORE_VARIABLE', [operand(v) for v in coordinate], [('fallthrough', 53)]),
                row(53, 'PUSH_LITERAL', [operand(0)], [('fallthrough', 54)]), row(54, 'RETURN')])
            instructions[-5]['operands'][1] = operand(1)
            root = frame(0, 1)
            root['operands'][1:] = [operand(261), operand(0)]
            instructions = [root, row(1, 'FINISH')] + instructions
            result = local_integer_domain(instructions, 24, 1, 0)
            self.assertEqual(result['status'], expected, (coordinate, result))
            if coordinate == [0, 258]:
                self.assertEqual(result['root_frame_extent'], 261)
                self.assertEqual(result['borrowed_frame_writes'], [dict(frame=0, slot=258)])
                self.assertTrue(result['conditional_on_frame_lifetimes'])

    def test_recursive_script_preservation_fails_closed(self):
        instructions = self.graph()
        instructions[12] = row(22, 'CALL', [operand(50, 2)], [('return_continuation', 23), ('call', 50)])
        instructions.extend([frame(50, 51),
            row(51, 'CALL', [operand(50, 2)], [('return_continuation', 52), ('call', 50)]),
            row(52, 'PUSH_LITERAL', [operand(0)], [('fallthrough', 53)]), row(53, 'RETURN')])
        result = self.analyze(instructions)
        self.assertEqual(result['status'], 'unresolved_local_integer_domain')
        self.assertIn('recursive', result['diagnostics'][0])

    def test_changed_native_review_cannot_enable_preservation(self):
        import tempfile
        from pathlib import Path
        from tools.script_decoder.local_flow import local_preservation_contracts
        native = dict(function='safe', effect='append_scalar')
        digest = hashlib.sha256(__import__('json').dumps(native, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'src/ovl').mkdir(parents=True)
            (root / 'src/ovl/script_vm.c').write_text('/* synthetic source */')
            with patch('tools.script_decoder.local_flow.VM_PRESERVATION_PINS', {}), patch(
                    'tools.script_decoder.local_flow.NATIVE_PRESERVATION_PINS', {'safe': digest}):
                reviews = dict(active={'safe': native}, active_deferred={})
                self.assertEqual(local_preservation_contracts(reviews, root)['preserved_native_functions'], ['safe'])
                reviews['active']['safe'] = dict(native, effect='unreviewed_write')
                self.assertEqual(local_preservation_contracts(reviews, root)['preserved_native_functions'], [])


    def test_messenger_preservation_requires_present_unchanged_expanded_review(self):
        import tempfile
        import json
        from pathlib import Path
        from tools.script_decoder.local_flow import local_preservation_contracts
        name = 'actorScriptSetMessengerContactMode'
        native = dict(function=name, effect='replace_argument_with_scalar', arguments=1,
                      stack_delta=0, result_tag='integer', scheduling='continue',
                      dependencies=[dict(kind='function', function='syntheticSetter', sha256='frozen')])
        digest = hashlib.sha256(json.dumps(native, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'src/ovl').mkdir(parents=True)
            (root / 'src/ovl/script_vm.c').write_text('/* synthetic source */')
            with patch('tools.script_decoder.local_flow.VM_PRESERVATION_PINS', {}), patch(
                    'tools.script_decoder.local_flow.NATIVE_PRESERVATION_PINS', {name: digest}):
                absent = local_preservation_contracts(dict(active={}, active_deferred={}), root)
                self.assertEqual(absent['preserved_native_functions'], [])
                self.assertEqual(absent['stale_native_reviews'], [name])
                current = local_preservation_contracts(dict(active={name:native}, active_deferred={}), root)
                self.assertEqual(current['preserved_native_functions'], [name])
                changed = dict(native, dependencies=[dict(kind='function', function='syntheticSetter', sha256='changed')])
                stale = local_preservation_contracts(dict(active={name:changed}, active_deferred={}), root)
                self.assertEqual(stale['preserved_native_functions'], [])
                self.assertEqual(stale['stale_native_reviews'], [name])


class PairedSelectorFlowTests(unittest.TestCase):
    def fixture(self, extras=()):
        root = frame(0, 1)
        root['operands'][1:] = [operand(10), operand(0)]
        producer = [frame(100, 101),
            row(101, 'JUMP_IF_ZERO', [operand(110, 2)], [('fallthrough', 102), ('branch', 110)]),
            row(102, 'PUSH_LITERAL', [operand(6)], [('fallthrough', 103)]),
            row(103, 'STORE_VARIABLE', [operand(0), operand(3)], [('fallthrough', 104)]),
            row(104, 'PUSH_LITERAL', [operand(1)], [('fallthrough', 105)]), row(105, 'RETURN'),
            row(110, 'PUSH_LITERAL', [operand(1)], [('fallthrough', 111)]),
            row(111, 'STORE_VARIABLE', [operand(0), operand(3)], [('fallthrough', 112)]),
            row(112, 'PUSH_LITERAL', [operand(99)], [('fallthrough', 113)]), row(113, 'RETURN')]
        specs = [('CALL', [operand(100, 2)]), ('STORE_VARIABLE', [operand(1), operand(0)]),
                 *extras, ('LOAD_VARIABLE', [operand(1), operand(0)]),
                 ('LOAD_VARIABLE', [operand(0), operand(3)]), ('CALL', [operand(300, 2)]),
                 ('DISCARD', []), ('PUSH_LITERAL', [operand(0)]), ('RETURN', [])]
        caller = [frame(200, 201)]
        caller[0]['operands'][1] = operand(1)
        for off, (name, operands) in enumerate(specs, 201):
            successors = [] if name == 'RETURN' else [('fallthrough', off+1)]
            if name == 'CALL':
                successors = [('return_continuation', off+1), ('call', operands[0]['value'])]
            caller.append(row(off, name, operands, successors))
        consumer = [frame(300, 301, 2),
            row(301, 'LOAD_VARIABLE', [operand(1), operand(1)], [('fallthrough', 302)]),
            row(302, 'PUSH_LITERAL', [operand(6)], [('fallthrough', 303)]),
            row(303, 'EQUAL', successors=[('fallthrough', 304)]),
            row(304, 'JUMP_IF_ZERO', [operand(310, 2)], [('fallthrough', 305), ('branch', 310)]),
            row(305, 'LOAD_VARIABLE', [operand(1), operand(0)], [('fallthrough', 306)]),
            row(306, 'PUSH_LITERAL', [operand(1)], [('fallthrough', 307)]),
            row(307, 'CALL_NATIVE', successors=[('resume_or_fallthrough', 308)], native=dict(functions=['actorScriptStartCostedAction'])),
            row(308, 'DISCARD', successors=[('fallthrough', 309)]),
            row(309, 'JUMP', [operand(310, 2)], [('branch', 310)]),
            row(310, 'PUSH_LITERAL', [operand(0)], [('fallthrough', 311)]), row(311, 'RETURN')]
        request = dict(instruction_offset=307, arguments=dict(cost_selector=dict(kind='variable', frame=1, slot=0),
                       repetitions=dict(kind='integer', value=1)), cost_policy_link=dict(status='unresolved_selector'))
        return [root, row(1, 'FINISH'), *producer, *caller, *consumer], request

    def analyze(self, instructions, request):
        from tools.script_decoder.paired_flow import annotate_costed_caller_relations
        policies = cost_policy_records(b''.join(struct.pack('>iifi', 300+i, 400+i, 1.0, 1) for i in range(5)))
        annotate_costed_caller_relations(instructions, [request], policies)
        return request.get('conditional_caller_policy_relations', [])

    def test_mode_return_pair_excludes_timed_ids_without_resolving_shared_formal(self):
        instructions, request = self.fixture()
        relations = self.analyze(instructions, request)
        self.assertEqual(relations[0]['status'], 'conditional_paired_caller_relation')
        self.assertEqual(relations[0]['conditional_cost_selectors'], [1])
        pairs = relations[0]['producer_relations'][0]['mode_return_pairs']
        self.assertIn(dict(mode=1, return_value=99, return_offset=113), pairs)
        self.assertEqual(request['cost_policy_link']['status'], 'unresolved_selector')
        self.assertFalse(request['universal_policy_domain_established'])
        self.assertEqual(request['consumer_mode_guard']['required_mode'], 6)

    def test_missing_mode_definition_and_ambiguous_dispatch_fail(self):
        instructions, request = self.fixture()
        next(r for r in instructions if r['offset']==111)['name'] = 'SCRIPT_OP_DISCARD'
        self.assertEqual(self.analyze(instructions, request)[0]['status'], 'unresolved_caller_relation')
        instructions, request = self.fixture()
        next(r for r in instructions if r['offset']==302)['operands'] = [operand(5)]
        self.assertEqual(self.analyze(instructions, request), [])
        self.assertIn('mode6 guard', request['caller_relation_diagnostics'][0])

    def test_unknown_consumer_or_producer_native_clobber_fails(self):
        for entry, injected, following in ((300, 320, 301), (100, 120, 101)):
            instructions, request = self.fixture()
            next(r for r in instructions if r['offset']==entry)['successors'] = [dict(kind='fallthrough', offset=injected)]
            instructions.append(row(injected, 'CALL_NATIVE', successors=[('resume_or_fallthrough', following)]))
            relations = self.analyze(instructions, request)
            if entry == 300:
                self.assertEqual(relations, [])
                self.assertIn('frozen preservation', request['caller_relation_diagnostics'][0])
            else:
                self.assertEqual(relations[0]['status'], 'unresolved_caller_relation')
                self.assertIn('frozen preservation', relations[0]['reason'])

    def test_consumer_formal_and_indirect_writes_are_rejected_even_after_guard(self):
        for name, operands in [('STORE_VARIABLE', [operand(1), operand(0)]),
                               ('STORE_VARIABLE', [operand(1), operand(1)]),
                               ('STORE_ELEMENT', [operand(1), operand(0)]),
                               ('UNMODELED_EFFECT', [])]:
            instructions, request = self.fixture()
            row_to_change = next(r for r in instructions if r['offset']==305)
            row_to_change['name'] = 'SCRIPT_OP_' + name
            row_to_change['operands'] = operands
            self.assertEqual(self.analyze(instructions, request), [])
            self.assertIn('caller_relation_diagnostics', request)

    def test_intervening_mode_or_result_writer_fails(self):
        for coords in ([0,3], [1,0]):
            instructions, request = self.fixture([('PUSH_LITERAL', [operand(2)]),
                                                  ('STORE_VARIABLE', [operand(v) for v in coords])])
            self.assertEqual(self.analyze(instructions, request)[0]['status'], 'unresolved_caller_relation')

    def test_script_clobber_and_other_caller_prevent_universal_claim(self):
        instructions, request = self.fixture([('CALL', [operand(450, 2)])])
        bad = frame(450, 451)
        bad['operands'][1:] = [operand(1), operand(2)]
        instructions.extend([bad, row(451, 'PUSH_LITERAL', [operand(2)], [('fallthrough', 452)]),
            row(452, 'STORE_VARIABLE', [operand(1), operand(0)], [('fallthrough', 453)]),
            row(453, 'PUSH_LITERAL', [operand(0)], [('fallthrough', 454)]), row(454, 'RETURN')])
        self.assertEqual(self.analyze(instructions, request)[0]['status'], 'unresolved_caller_relation')
        instructions, request = self.fixture()
        other = frame(400, 401)
        other['operands'][1] = operand(1)
        instructions.extend([other, row(401, 'LOAD_VARIABLE', [operand(1), operand(0)], [('fallthrough', 402)]),
            row(402, 'LOAD_VARIABLE', [operand(0), operand(3)], [('fallthrough', 403)]),
            row(403, 'CALL', [operand(300, 2)], [('return_continuation', 404), ('call', 300)]),
            row(404, 'PUSH_LITERAL', [operand(0)], [('fallthrough', 405)]), row(405, 'RETURN')])
        relations = self.analyze(instructions, request)
        self.assertEqual([r['status'] for r in relations], ['conditional_paired_caller_relation', 'unresolved_caller_relation'])
        self.assertFalse(request['universal_policy_domain_established'])


class FormalForwardingTests(unittest.TestCase):
    def fixture(self):
        instructions, request = PairedSelectorFlowTests.fixture(self)
        instructions = [r for r in instructions if not 204 <= r['offset'] <= 208]
        next(r for r in instructions if r['offset']==203)['successors'] = [dict(kind='fallthrough',offset=204)]
        wrapper = frame(400,401,1)
        wrapper['operands'][1] = operand(2)
        instructions.extend([
            row(204,'CALL',[operand(400,2)],[('return_continuation',205),('call',400)]),
            row(205,'DISCARD',successors=[('fallthrough',206)]),
            row(206,'PUSH_LITERAL',[operand(0)],[('fallthrough',207)]),row(207,'RETURN'),wrapper,
            row(401,'LOAD_VARIABLE',[operand(1),operand(0)],[('fallthrough',402)]),
            row(402,'LOAD_VARIABLE',[operand(0),operand(3)],[('fallthrough',403)]),
            row(403,'CALL',[operand(300,2)],[('return_continuation',404),('call',300)]),
            # An unreviewed post-inner-call writer must not contaminate the prefix proof.
            row(404,'STORE_ELEMENT',[operand(1),operand(0)],[('fallthrough',405)]),
            row(405,'PUSH_LITERAL',[operand(0)],[('fallthrough',406)]),row(406,'RETURN')])
        return sorted(instructions,key=lambda r:r['offset']),request

    def analyze(self,instructions,request,preserved=()):
        from tools.script_decoder.paired_flow import annotate_costed_caller_relations
        policies=cost_policy_records(b''.join(struct.pack('>iifi',300+i,400+i,1.0,1)for i in range(5)))
        annotate_costed_caller_relations(instructions,[request],policies,preserved_natives=preserved)
        return request['conditional_caller_policy_relations'][0]

    def test_specific_actual_relation_survives_formal_rebinding(self):
        instructions,request=self.fixture()
        result=self.analyze(instructions,request)
        self.assertEqual(result['status'],'conditional_formal_forwarding_relation')
        forwarding=result['formal_forwarding']
        self.assertTrue(forwarding['prefix_preservation_proven'])
        self.assertNotIn(404,forwarding['prefix_instruction_offsets'])
        upstream=forwarding['upstream_callers'][0]
        self.assertEqual(upstream['caller_offset'],204)
        self.assertEqual(upstream['conditional_cost_selectors'],[1])
        self.assertIn(dict(mode=1,return_value=99,return_offset=113),upstream['producer_relations'][0]['mode_return_pairs'])
        self.assertFalse(forwarding['universal_formal_domain_established'])
        self.assertEqual(request['cost_policy_link']['status'],'unresolved_selector')

    def test_unknown_prefix_effect_retains_tree_without_activating_it(self):
        instructions,request=self.fixture()
        next(r for r in instructions if r['offset']==400)['successors']=[dict(kind='fallthrough',offset=410)]
        instructions.append(row(410,'CALL_NATIVE',successors=[('resume_or_fallthrough',401)],native=dict(functions=['unsafeCommand'])))
        result=self.analyze(instructions,request)
        self.assertEqual(result['status'],'unresolved_caller_relation')
        self.assertFalse(result['formal_forwarding']['prefix_preservation_proven'])
        self.assertEqual(result['formal_forwarding']['upstream_callers'][0]['status'],'conditional_paired_caller_relation')

    def test_recursive_forwarding_and_ambiguous_actual_fail_closed(self):
        instructions,request=self.fixture()
        # Recursive incoming call creates a cycle with no separate producer store.
        instructions=[r for r in instructions if not 200<=r['offset']<=207]
        next(r for r in instructions if r['offset']==400)['successors']=[dict(kind='fallthrough',offset=410)]
        instructions.extend([row(410,'LOAD_VARIABLE',[operand(1),operand(0)],[('fallthrough',411)]),
            row(411,'CALL',[operand(400,2)],[('return_continuation',412),('call',400)]),
            row(412,'DISCARD',successors=[('fallthrough',401)])])
        result=self.analyze(sorted(instructions,key=lambda r:r['offset']),request)
        self.assertEqual(result['status'],'unresolved_caller_relation')
        self.assertIn('recursive',str(result))
        instructions,request=self.fixture()
        instructions.append(row(499,'JUMP',[operand(204,2)],[('branch',204)]))
        result=self.analyze(instructions,request)
        self.assertEqual(result['status'],'unresolved_caller_relation')
        self.assertIn('control-flow',str(result))

    def test_redefinitions_and_invalid_frame_bounds_do_not_forward(self):
        instructions,request=self.fixture()
        next(r for r in instructions if r['offset']==400)['successors']=[dict(kind='fallthrough',offset=410)]
        instructions.extend([row(410,'PUSH_LITERAL',[operand(2)],[('fallthrough',411)]),
            row(411,'STORE_VARIABLE',[operand(1),operand(0)],[('fallthrough',401)])])
        result=self.analyze(sorted(instructions,key=lambda r:r['offset']),request)
        self.assertEqual(result['status'],'unresolved_caller_relation')
        self.assertNotIn('formal_forwarding',result)
        instructions,request=self.fixture()
        next(r for r in instructions if r['offset']==401)['operands'][1]=operand(2)
        self.assertEqual(self.analyze(instructions,request)['status'],'unresolved_caller_relation')
        instructions,request=self.fixture()
        next(r for r in instructions if r['offset']==400)['operands'][1]=operand(0)
        self.assertEqual(self.analyze(instructions,request)['status'],'unresolved_caller_relation')

    def guarded_fixture(self):
        instructions,request=self.fixture()
        instructions=[r for r in instructions if not 401<=r['offset']<=406]
        next(r for r in instructions if r['offset']==400)['successors']=[dict(kind='fallthrough',offset=410)]
        instructions.extend([
            row(410,'LOAD_VARIABLE',[operand(0),operand(3)],[('fallthrough',411)]),
            row(411,'PUSH_LITERAL',[operand(6)],[('fallthrough',412)]),
            row(412,'EQUAL',successors=[('fallthrough',413)]),
            row(413,'JUMP_IF_ZERO',[operand(420,2)],[('fallthrough',414),('branch',420)]),
            row(414,'LOAD_VARIABLE',[operand(1),operand(0)],[('fallthrough',415)]),
            row(415,'LOAD_VARIABLE',[operand(0),operand(3)],[('fallthrough',416)]),
            row(416,'CALL',[operand(300,2)],[('return_continuation',417),('call',300)]),
            row(417,'DISCARD',successors=[('fallthrough',418)]),
            row(418,'PUSH_LITERAL',[operand(0)],[('fallthrough',419)]),row(419,'RETURN'),
            row(420,'PUSH_LITERAL',[operand(0)],[('fallthrough',421)]),row(421,'RETURN')])
        return sorted(instructions,key=lambda r:r['offset']),request

    def test_dominating_mode_guard_is_recorded_but_bypass_is_not(self):
        instructions,request=self.guarded_fixture()
        result=self.analyze(instructions,request)
        guard=result['formal_forwarding']['mode_guards'][0]
        self.assertEqual(guard['branch_offset'],413)
        self.assertEqual(guard['literal_value'],6)
        self.assertTrue(guard['required_comparison_truth'])
        instructions,request=self.guarded_fixture()
        next(r for r in instructions if r['offset']==400)['successors']=[dict(kind='fallthrough',offset=401)]
        instructions.extend([row(401,'PUSH_LITERAL',[operand(1)],[('fallthrough',402)]),
            row(402,'JUMP_IF_ZERO',[operand(414,2)],[('fallthrough',410),('branch',414)])])
        result=self.analyze(sorted(instructions,key=lambda r:r['offset']),request)
        self.assertEqual(result['formal_forwarding']['mode_guards'],[])

    def test_mixed_entry_formal_and_producer_store_paths_are_ambiguous(self):
        instructions,request=self.guarded_fixture()
        instructions=[r for r in instructions if not 410<=r['offset']<=413]
        next(r for r in instructions if r['offset']==400)['successors']=[dict(kind='fallthrough',offset=409)]
        instructions.extend([
            row(409,'PUSH_LITERAL',[operand(1)],[('fallthrough',410)]),
            row(410,'JUMP_IF_ZERO',[operand(414,2)],[('fallthrough',411),('branch',414)]),
            row(411,'CALL',[operand(100,2)],[('return_continuation',412),('call',100)]),
            row(412,'STORE_VARIABLE',[operand(1),operand(0)],[('fallthrough',413)]),
            row(413,'JUMP',[operand(414,2)],[('branch',414)])])
        result=self.analyze(sorted(instructions,key=lambda r:r['offset']),request)
        self.assertEqual(result['status'],'unresolved_caller_relation')
        self.assertIn('mixes entry arguments',result['reason'])
        self.assertNotIn('formal_forwarding',result)

    def test_unsupported_literal_actual_is_retained_without_masking(self):
        instructions,request=self.fixture()
        node=next(r for r in instructions if r['offset']==203)
        node['name']='SCRIPT_OP_PUSH_LITERAL'
        node['operands']=[operand(65538)]
        result=self.analyze(instructions,request)
        upstream=result['formal_forwarding']['upstream_callers'][0]
        self.assertEqual(upstream['status'],'unresolved_caller_relation')
        self.assertEqual(upstream['arguments'][0]['value'],65538)
        self.assertNotIn('conditional_cost_selectors',upstream)


class NonCostCallerGuardTests(FormalForwardingTests):
    def test_suffix_exclusion_is_independent_of_unreviewed_producer_prefix(self):
        instructions,request=self.guarded_fixture()
        next(r for r in instructions if r['offset']==411)['operands']=[operand(1)]
        next(r for r in instructions if r['offset']==201)['native']=dict(functions=['unsafeProducer'])
        next(r for r in instructions if r['offset']==201)['name']='SCRIPT_OP_CALL_NATIVE'
        result=self.analyze(instructions,request)
        self.assertEqual(result['status'],'unresolved_caller_relation')
        exclusion=result['conditional_non_cost_caller']
        self.assertEqual(exclusion['excluded_dispatch_mode'],6)
        self.assertEqual(exclusion['guard']['literal_value'],1)
        self.assertNotIn(201,exclusion['suffix_instruction_offsets'])
        self.assertEqual(request['cost_policy_link']['status'],'unresolved_selector')

    def test_not_equal_six_and_malformed_literals(self):
        for value in (6,None,True,2147483648):
            instructions,request=self.guarded_fixture()
            next(r for r in instructions if r['offset']==411)['operands']=[operand(value)]
            next(r for r in instructions if r['offset']==412)['name']='SCRIPT_OP_NOT_EQUAL'
            result=self.analyze(instructions,request)
            self.assertEqual('conditional_non_cost_caller' in result,type(value)is int and value==6)

    def test_suffix_mode_write_and_unknown_effect_reject_exclusion(self):
        for name,operands in [('STORE_VARIABLE',[operand(0),operand(3)]),('STORE_ELEMENT',[operand(1),operand(0)]),('MISSING_OPCODE',[])]:
            instructions,request=self.guarded_fixture()
            next(r for r in instructions if r['offset']==411)['operands']=[operand(1)]
            next(r for r in instructions if r['offset']==414)['successors']=[dict(kind='fallthrough',offset=430)]
            instructions.append(row(430,name,operands,[('fallthrough',415)]))
            result=self.analyze(sorted(instructions,key=lambda r:r['offset']),request)
            self.assertNotIn('conditional_non_cost_caller',result)

    def test_exclusion_guard_bypass_rejects(self):
        instructions,request=self.guarded_fixture()
        next(r for r in instructions if r['offset']==411)['operands']=[operand(1)]
        next(r for r in instructions if r['offset']==400)['successors']=[dict(kind='fallthrough',offset=401)]
        instructions.extend([row(401,'PUSH_LITERAL',[operand(1)],[('fallthrough',402)]),row(402,'JUMP_IF_ZERO',[operand(414,2)],[('fallthrough',410),('branch',414)])])
        self.assertNotIn('conditional_non_cost_caller',self.analyze(sorted(instructions,key=lambda r:r['offset']),request))

    def test_adjacent_return_argument_preserves_pairs_without_fake_store(self):
        instructions,request=self.fixture()
        instructions=[r for r in instructions if r['offset'] not in (202,203)]
        next(r for r in instructions if r['offset']==201)['successors']=[dict(kind='return_continuation',offset=204),dict(kind='call',offset=100)]
        next(r for r in instructions if r['offset']==201)['size']=3
        result=self.analyze(instructions,request)
        upstream=result['formal_forwarding']['upstream_callers'][0]
        self.assertEqual(upstream['conditional_cost_selectors'],[1])
        self.assertEqual(upstream['reaching_producer_stores'],[])
        self.assertEqual(upstream['actual_producer_call']['instruction_offset'],201)
        next(r for r in instructions if r['offset']==201)['size']=1
        result=self.analyze(instructions,request)
        self.assertEqual(result['formal_forwarding']['upstream_callers'][0]['status'],'unresolved_caller_relation')


class VmComparisonContractTests(unittest.TestCase):
    def test_changed_helper_macro_and_duplicate_definition_fail_closed(self):
        import tempfile
        from pathlib import Path
        from tools.script_decoder.context import ROOT
        from tools.script_decoder.local_flow import vm_comparison_contract
        source=(ROOT/'src/ovl/script_vm.c').read_text()
        self.assertTrue(vm_comparison_contract()['current'])
        for changed in (source.replace('fabsf(fr - fl)', 'fabsf(fr + fl)',1),
                        source.replace('return integerValue;', 'return integerValue + 1;',1),
                        source.replace('SCRIPT_VALUE_RELATIVE_EQUALITY_TOLERANCE 0.001F','SCRIPT_VALUE_RELATIVE_EQUALITY_TOLERANCE 0.002F'),
                        source+'\n#define SCRIPT_VALUE_ABSOLUTE_EQUALITY_TOLERANCE 0.001F\n'):
            with tempfile.TemporaryDirectory() as directory:
                root=Path(directory);(root/'src/ovl').mkdir(parents=True)
                (root/'src/ovl/script_vm.c').write_text(changed)
                (root/'include').mkdir()
                (root/'include/script_vm.h').write_text((ROOT/'include/script_vm.h').read_text())
                self.assertFalse(vm_comparison_contract(root)['current'])

    def test_changed_packed_value_layout_fails_closed(self):
        import tempfile
        from pathlib import Path
        from tools.script_decoder.context import ROOT
        from tools.script_decoder.local_flow import vm_comparison_contract
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'src/ovl').mkdir(parents=True);(root/'include').mkdir()
            (root/'src/ovl/script_vm.c').write_text((ROOT/'src/ovl/script_vm.c').read_text())
            header=(ROOT/'include/script_vm.h').read_text()
            (root/'include/script_vm.h').write_text(header.replace('SCRIPT_VALUE_FLOAT = 1','SCRIPT_VALUE_FLOAT = 9'))
            result=vm_comparison_contract(root)
            self.assertFalse(result['current'])
            self.assertIn('include/script_vm.h',result['stale'])

    def test_stale_comparison_suppresses_only_independent_exclusion(self):
        from unittest.mock import patch
        helper=NonCostCallerGuardTests()
        instructions,request=helper.guarded_fixture()
        next(r for r in instructions if r['offset']==411)['operands']=[operand(1)]
        with patch('tools.script_decoder.paired_flow.vm_comparison_contract',return_value=dict(current=False,stale=['macro'])):
            result=helper.analyze(instructions,request)
        self.assertEqual(result['status'],'conditional_formal_forwarding_relation')
        self.assertNotIn('conditional_non_cost_caller',result)
        self.assertIn('stale VM comparison',str(result['non_cost_guard_diagnostics']))

    def test_finite_float_tolerance_near_six_is_not_exact_equality(self):
        import struct
        f32=lambda x:struct.unpack('>f',struct.pack('>f',x))[0]
        equal=lambda left,right:abs(f32(right-left))<=f32(f32(abs(left)*f32(.001))+f32(.001))
        self.assertTrue(equal(f32(6.006),6))
        self.assertFalse(equal(f32(6.008),6))
        for value in (f32(5.994),f32(6.0),f32(6.006)):
            self.assertTrue(equal(value,6))
            self.assertFalse(equal(value,1))


if __name__ == '__main__':
    unittest.main()
