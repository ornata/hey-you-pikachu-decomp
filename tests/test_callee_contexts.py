"""All-program contexts preserve per-call identities and reject malformed evidence."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder.callee_contexts import literal_native_argument_contexts
import test_item_response_dispatch as fixtures
from test_item_response_dispatch import operand


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


if __name__=='__main__':
    unittest.main()
