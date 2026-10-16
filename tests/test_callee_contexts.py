"""All-program contexts preserve per-call identities and reject malformed evidence."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder.callee_contexts import literal_native_argument_contexts
from tests import test_item_response_dispatch as fixtures
from tests.test_item_response_dispatch import operand


class CalleeContextTests(unittest.TestCase):
    def fixture(self):
        rows, _ = fixtures.LiteralNativeContextTests().fixture()
        snapshot = (dict(active={'prefixGetter':dict(arguments=0),
                                'actorScriptStartTimedAction':dict(arguments=2)}),
                    dict(stale_vm_functions=[], preserved_native_functions=['prefixGetter'],
                         native_review_sha256={'prefixGetter':'pin'}, vm_source_pins={'vm':'pin'}))
        return rows, snapshot

    def test_context_retains_operand_tags_header_and_alias_scope(self):
        rows,snapshot=self.fixture()
        result=literal_native_argument_contexts(rows,snapshot=snapshot)
        self.assertEqual(len(result),1)
        context=result[0]
        self.assertEqual(context['call_operands'],[operand(100,2)])
        self.assertEqual(context['callee_header_operands'],rows[5]['operands'])
        self.assertEqual(context['caller_producer_window'][2]['operands'],[operand(.5,1)])
        self.assertEqual(context['caller_actual_boxing'][1]['raw_literal_operand']['tag'],1)
        self.assertTrue(any('aliases' in clause for clause in context['preconditions']))
        self.assertTrue(any('reused incoming argument cells' in clause for clause in context['preconditions']))
        self.assertTrue(any('canonical authenticated decoding' in clause for clause in context['preconditions']))

    def test_duplicate_offsets_bad_call_edges_and_operand_arities_reject(self):
        for mutation in ('duplicate','target','continuation','callarity','headerarity','loadarity'):
            rows,snapshot=self.fixture()
            if mutation=='duplicate':rows.append(copy.deepcopy(rows[1]))
            if mutation=='target':rows[3]['successors'][1]['offset']=101
            if mutation=='continuation':rows[3]['successors'][0]['offset']=5
            if mutation=='callarity':rows[3]['operands'].append(operand(0))
            if mutation=='headerarity':rows[5]['operands'].append(operand(0))
            if mutation=='loadarity':rows[8]['operands'].append(operand(0))
            with self.subTest(mutation=mutation):
                self.assertEqual(literal_native_argument_contexts(rows,snapshot=snapshot),[])

    def test_native_folded_actual_is_not_a_literal_context(self):
        rows,snapshot=self.fixture()
        from tools.script_decoder import responses
        original=responses.recover_arguments
        def recover(*args,**kwargs):
            values=original(*args,**kwargs)
            if args[1]==3:
                values[0]['expression']=dict(function='actorScriptPassThroughInteger',operand=dict(kind='integer',value=87))
            return values
        with patch.object(responses,'recover_arguments',side_effect=recover):
            self.assertEqual(literal_native_argument_contexts(rows,snapshot=snapshot),[])

    def test_stale_full_vm_binding_rejects_context(self):
        rows,snapshot=self.fixture()
        snapshot[1]['context_vm_contract']=dict(current=False,stale=['script_vm.h'])
        self.assertEqual(literal_native_argument_contexts(rows,snapshot=snapshot),[])

    def test_shared_snapshot_stale_vm_and_missing_consumer_fail_closed(self):
        for stale in (True,False):
            rows,snapshot=self.fixture()
            if stale:snapshot[1]['stale_vm_functions']=['vm']
            else:snapshot[0]['active'].pop('actorScriptStartTimedAction')
            self.assertEqual(literal_native_argument_contexts(rows,snapshot=snapshot),[])


    def test_forwarded_same_frame_is_a_caller_time_capture_not_callee_variable(self):
        from tools.script_decoder.callee_contexts import forwarded_native_argument_contexts
        rows,snapshot=self.fixture()
        rows[1]['name']='SCRIPT_OP_LOAD_VARIABLE';rows[1]['operands']=[operand(1),operand(7)]
        context=forwarded_native_argument_contexts(rows,configuration=0x80123456,snapshot=snapshot)[0]
        capture=context['contextual_arguments']['script_id']
        self.assertEqual(capture['kind'],'caller_box_capture')
        self.assertEqual(capture['source_reference'],dict(kind='variable',frame=1,slot=7))
        self.assertEqual(capture['caller_origin']['load_instruction_offset'],1)
        self.assertEqual(capture['caller_origin']['call_offset'],3)
        self.assertEqual(capture['caller_origin']['configuration'],0x80123456)
        self.assertIsNone(capture['output_tag'])
        self.assertEqual(context['raw_callee_arguments']['script_id'],dict(kind='variable',frame=1,slot=0))
        self.assertEqual(context['caller_actual_boxing'][0]['raw_load_operands'],rows[1]['operands'])
        self.assertEqual(literal_native_argument_contexts(rows,snapshot=snapshot),[])
        self.assertIn('producer operand scratch',context['preconditions'][-2])

    def test_forwarded_keeps_negated_literal_companion_boxing(self):
        from tools.script_decoder.callee_contexts import forwarded_native_argument_contexts
        rows,snapshot=self.fixture()
        rows[1]['name']='SCRIPT_OP_LOAD_VARIABLE';rows[1]['operands']=[operand(-1),operand(7)]
        rows[3]['offset']=4;rows[3]['successors'][0]['offset']=5
        rows[4]['offset']=5
        rows.insert(3,fixtures.row(3,'NEGATE',successors=[('fallthrough',4)]))
        context=forwarded_native_argument_contexts(rows,configuration=1,snapshot=snapshot)[0]
        self.assertEqual(context['contextual_arguments']['duration_seconds']['value'],-.5)
        self.assertEqual(context['caller_actual_boxing'][1]['output_tag'],1)
        self.assertEqual([r['instruction_offset']for r in context['caller_actual_boxing'][1]['producer_instructions']],[2,3])
        self.assertEqual(context['caller_actuals'][0]['caller_origin']['call_offset'],4)

    def test_forwarded_requires_load_and_valid_configuration_and_source_coordinate(self):
        from tools.script_decoder.callee_contexts import forwarded_native_argument_contexts
        rows,snapshot=self.fixture()
        self.assertEqual(forwarded_native_argument_contexts(rows,configuration=1,snapshot=snapshot),[])
        for bad in (None,True,-1,0x100000000):
            with self.assertRaises(ValueError):forwarded_native_argument_contexts(rows,configuration=bad,snapshot=snapshot)
        rows[1]['name']='SCRIPT_OP_LOAD_VARIABLE'
        for frame,slot in ((-2,0),(32,0),(1,-1)):
            rows[1]['operands']=[operand(frame),operand(slot)]
            self.assertEqual(forwarded_native_argument_contexts(rows,configuration=1,snapshot=snapshot),[])

    def test_forwarded_rejects_native_call_return_array_and_symbolic_arithmetic(self):
        from tools.script_decoder.callee_contexts import forwarded_native_argument_contexts
        rows,snapshot=self.fixture()
        for name in ('CALL_NATIVE','CALL','LOAD_ELEMENT','ADD','NEGATE'):
            bad=copy.deepcopy(rows);bad[1]['name']='SCRIPT_OP_'+name
            bad[1]['operands']=[operand(1),operand(7)]
            self.assertEqual(forwarded_native_argument_contexts(bad,configuration=1,snapshot=snapshot),[])

    def test_forwarded_reanalysis_revokes_stale_capture_but_leaves_literal_field(self):
        from tools.script_decoder.responses import annotate_forwarded_native_contexts
        rows,snapshot=self.fixture();rows[1]['name']='SCRIPT_OP_LOAD_VARIABLE';rows[1]['operands']=[operand(0),operand(7)]
        graph=dict(script_calls=[dict(instruction_offset=3,target_offset=100,conditional_native_argument_context={'retained':True})])
        annotate_forwarded_native_contexts(rows,[graph],configuration=1,validated_snapshot=snapshot)
        self.assertIn('conditional_forwarded_native_argument_context',graph['script_calls'][0])
        snapshot[1]['context_vm_contract']=dict(current=False)
        annotate_forwarded_native_contexts(rows,[graph],configuration=1,validated_snapshot=snapshot)
        self.assertNotIn('conditional_forwarded_native_argument_context',graph['script_calls'][0])
        self.assertEqual(graph['script_calls'][0]['conditional_native_argument_context'],{'retained':True})


if __name__=='__main__':
    unittest.main()
