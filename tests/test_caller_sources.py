"""Authored source-cell paths distinguish provenance from later substitution."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder import caller_sources as sources
from tools.script_decoder.callee_contexts import forwarded_native_argument_contexts


def op(value):return dict(tag=0,kind='integer',value=value)
def row(offset,name,operands=(),successors=None,**extra):
    if successors is None:successors=[dict(kind='fallthrough',offset=offset+1)]
    return dict(offset=offset,size=1,name='SCRIPT_OP_'+name,operands=list(operands),successors=successors,**extra)


class CallerSourceTests(unittest.TestCase):
    def fixture(self,native=None,arity=0):
        rows=[row(0,'ENTER_FRAME',[op(0),op(10),op(1)])]
        for i in range(arity):rows.append(row(len(rows),'PUSH_LITERAL',[op(i+1)]))
        producer=len(rows)
        if native:rows.append(row(producer,'CALL_NATIVE',successors=[dict(kind='resume_or_fallthrough',offset=producer+1)],native=dict(functions=[native])))
        else:rows.append(row(producer,'PUSH_LITERAL',[op(1)]))
        store=len(rows);rows.append(row(store,'STORE_VARIABLE',[op(1),op(7)]))
        load=len(rows);rows.append(row(load,'LOAD_VARIABLE',[op(1),op(7)]))
        call=len(rows);rows.append(row(call,'CALL',[dict(tag=2,value=100)],successors=[dict(kind='call',offset=100),dict(kind='return_continuation',offset=call+1)]))
        rows.append(row(len(rows),'RETURN',successors=[]))
        rows.extend([row(100,'ENTER_FRAME',[op(1),op(1),op(1)]),row(101,'LOAD_VARIABLE',[op(1),op(0)]),row(102,'CALL_NATIVE',successors=[dict(kind='resume_or_fallthrough',offset=103)],native=dict(functions=['consumer'])),row(103,'RETURN',successors=[])])
        reviews=dict(active={'consumer':dict(arguments=1)})
        if native:reviews['active'][native]=dict(arguments=arity,result_tag='integer')
        snapshot=(reviews,dict(stale_vm_functions=[],preserved_native_functions=[],native_review_sha256={},vm_source_pins={},context_vm_contract=dict(current=True,vm_unit_sha256='vm')))
        contexts=forwarded_native_argument_contexts(rows,configuration=1,snapshot=snapshot)
        self.assertEqual(len(contexts),1)
        return rows,contexts,snapshot,producer,store,load,call

    def test_literal_provenance_preserves_capture_and_all_existing_fields(self):
        rows,contexts,snapshot,producer,store,load,call=self.fixture();original=copy.deepcopy(contexts)
        sources.annotate_caller_source_provenance(rows,contexts,snapshot=snapshot)
        proof=contexts[0].pop('caller_source_provenance')[0]
        self.assertEqual(contexts,original)
        self.assertEqual(proof['producer_result']['raw_operand'],op(1))
        self.assertEqual(proof['store_to_load_instruction_offsets'],[store,load])
        self.assertEqual(proof['caller_origin']['load_instruction_offset'],load)
        self.assertIn('scratch',proof['preconditions'][2])

    def test_exact_reviewed_native_role_and_arity_without_outcome_claim(self):
        for native,arity in [('actorScriptFindObjectInFacingCone',4),('actorScriptCreateItem',6),('actorScriptRandomBelow',1),('actorScriptPassThroughInteger',1)]:
            rows,contexts,snapshot,*_=self.fixture(native,arity)
            binding=dict(arguments=arity,review_sha256=sources._digest(snapshot[0]['active'][native]),result_role='opaque reviewed result')
            with patch.dict(sources.PRODUCER_BINDINGS,{native:binding},clear=True):
                sources.annotate_caller_source_provenance(rows,contexts,snapshot=snapshot)
                result=contexts[0]['caller_source_provenance'][0]['producer_result']
                self.assertEqual(result['output_tag'],0)
                self.assertEqual([a['value']for a in result['arguments']],list(range(1,arity+1)))
                self.assertNotIn('value',result)
                snapshot[0]['active'][native]['changed']=True
                sources.annotate_caller_source_provenance(rows,contexts,snapshot=snapshot)
                self.assertNotIn('caller_source_provenance',contexts[0])

    def test_revokes_on_stale_vm_missing_review_duplicate_and_changed_load(self):
        for change in ('vm','consumer','duplicate','load','edge','origin'):
            rows,contexts,snapshot,producer,store,load,call=self.fixture()
            sources.annotate_caller_source_provenance(rows,contexts,snapshot=snapshot)
            if change=='vm':snapshot[1]['context_vm_contract']['current']=False
            if change=='consumer':snapshot[0]['active'].clear()
            if change=='duplicate':rows.append(copy.deepcopy(rows[1]))
            if change=='load':rows[load]['operands'][1]=op(8)
            if change=='edge':rows[store]['successors']=[]
            if change=='origin':contexts[0]['caller_actuals'][0]['caller_origin']['call_offset']=999
            sources.annotate_caller_source_provenance(rows,contexts,snapshot=snapshot)
            self.assertNotIn('caller_source_provenance',contexts[0],change)

    def test_immediate_script_producer_requires_fresh_full_helper_shape(self):
        from tests import test_path_item_helpers as fixture_helpers
        from tools.script_decoder import path_item_helpers
        rows,contexts,snapshot,producer,store,load,call=self.fixture(arity=1)
        helper_rows,helper_snapshot,bindings=fixture_helpers.PathItemHelperTests().fixture()
        for row in helper_rows:
            row['offset']+=1000
            for edge in row['successors']:edge['offset']+=1000
        target=helper_rows[0]['offset']
        rows[producer].update(name='SCRIPT_OP_CALL',operands=[dict(tag=2,value=target)],
            successors=[dict(kind='call',offset=target),dict(kind='return_continuation',offset=store)])
        rows+=helper_rows;snapshot[0]['active'].update(helper_snapshot[0]['active'])
        contexts=forwarded_native_argument_contexts(rows,configuration=1,snapshot=snapshot)
        with patch.dict(path_item_helpers.NATIVE_BINDINGS,bindings,clear=True):
            sources.annotate_caller_source_provenance(rows,contexts,snapshot=snapshot)
            result=contexts[0]['caller_source_provenance'][0]['producer_result']
            self.assertEqual(result['kind'],'reviewed_script_helper_boxed_result')
            self.assertEqual(result['target_offset'],target)
            self.assertEqual(result['arguments'][0]['value'],1)
            self.assertEqual(result['input_expression_epoch']['instruction_offset'],producer)
            correct_edges=copy.deepcopy(rows[producer]['successors'])
            rows[producer]['successors']=[dict(kind='fallthrough',offset=store)]
            sources.annotate_caller_source_provenance(rows,contexts,snapshot=snapshot)
            self.assertNotIn('caller_source_provenance',contexts[0])
            rows[producer]['successors']=correct_edges
            sources.annotate_caller_source_provenance(rows,contexts,snapshot=snapshot)
            self.assertIn('caller_source_provenance',contexts[0])
            helper_rows[1]['operands'][0]['value']=1
            sources.annotate_caller_source_provenance(rows,contexts,snapshot=snapshot)
            self.assertNotIn('caller_source_provenance',contexts[0])

    def test_effects_competing_paths_other_stores_and_unreviewed_producers_reject(self):
        for name in ('CALL_NATIVE','CALL','ENTER_FRAME','STORE_ELEMENT','JUMP_IF_ZERO','NEGATE','LOAD_ELEMENT'):
            rows,contexts,snapshot,producer,*_=self.fixture();rows[producer]['name']='SCRIPT_OP_'+name
            sources.annotate_caller_source_provenance(rows,contexts,snapshot=snapshot)
            self.assertNotIn('caller_source_provenance',contexts[0],name)
        rows,contexts,snapshot,producer,store,load,*_=self.fixture()
        rows.append(row(200,'JUMP',[op(load)],successors=[dict(kind='branch',offset=load)]))
        sources.annotate_caller_source_provenance(rows,contexts,snapshot=snapshot)
        self.assertNotIn('caller_source_provenance',contexts[0])
        with self.assertRaises(ValueError):sources.annotate_caller_source_provenance(rows,contexts,snapshot=snapshot,limit=True)


if __name__=='__main__':unittest.main()
