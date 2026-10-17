"""Authored short closure fixtures; no original policy bytecode is embedded."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder import activity_policies as roles
from tools.script_decoder.context import opcode_names


class ActivityPolicyTests(unittest.TestCase):
    def setUp(self):
        self.bindings=copy.deepcopy(roles.NATIVE_BINDINGS);active,deferred={},{}
        for n,b in self.bindings.items():
            active[n]=dict(arguments=0,fixture=n);b['review_sha256']=roles._digest(active[n])
            if 'deferred_review_sha256' in b:
                deferred[n]=dict(arguments=1,fixture=n);b['deferred_review_sha256']=roles._digest(deferred[n])
        self.snapshot=({'active':active,'active_deferred':deferred},{'context_vm_contract':{'current':True}})
        p=patch.object(roles,'NATIVE_BINDINGS',self.bindings);p.start();self.addCleanup(p.stop)
        self.caller=dict(entry=100,workflow_variable=[0,34],cleanup_binding=dict(entry=9000,workflow_variable=[0,34]),
            external_cuts=[dict(identity=3,target_offset=2000),dict(identity=4,target_offset=3000),dict(identity=5,target_offset=1000),dict(identity=8,target_offset=9000)])
        p=patch.object(roles,'activity_response_roles',return_value=dict(status='conditional_responses',source_bindings={},responses=[self.caller]))
        self.callers=p.start();self.addCleanup(p.stop)
        self.opcodes={v:k for k,v in opcode_names('vm').items()}

    def rows(self,start,specs):
        out=[];offset=start
        for name,values in specs:
            operands=[]
            for i,v in enumerate(values):
                tag=2 if name in ('CALL','CALL_NATIVE','JUMP','JUMP_IF_ZERO') and i==0 else 0
                operands.append(dict(tag=tag,kind='code_reference' if tag==2 else 'integer',value=v))
            row=dict(offset=offset,opcode=self.opcodes['SCRIPT_OP_'+name],name='SCRIPT_OP_'+name,size=1+5*len(values),operands=operands)
            row['successors']=roles._edges(row,values)
            if name=='CALL_NATIVE':row['native']=dict(functions=['actorScriptEnqueueMessage'],address=self.bindings['actorScriptEnqueueMessage']['address'],inline_operands=['string_count'])
            out.append(row);offset+=row['size']
        return out

    def fixture(self,bedroom=False):
        rows=self.rows(1000,[('ENTER_FRAME',[0,0,1]),('CALL',[1100]),('DISCARD',[]),('CALL',[9000]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(1100,[('ENTER_FRAME',[0,0,1]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(2000,[('ENTER_FRAME',[0,0,1]),('CALL',[2100]),('DISCARD',[]),('CALL',[9000]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(2100,[('ENTER_FRAME',[0,0,1]),*([('CALL',[2200]),('DISCARD',[])] if bedroom else []),('PUSH_LITERAL',[0]),('RETURN',[])])
        if bedroom:
            rows+=self.rows(2200,[('ENTER_FRAME',[0,0,1]),('CALL',[2300]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
            rows+=self.rows(2300,[('ENTER_FRAME',[0,0,1]),('LOAD_VARIABLE',[-1,0]),('LOGICAL_NOT',[]),('JUMP_IF_ZERO',[2341]),('CALL',[2400]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
            rows+=self.rows(2400,[('ENTER_FRAME',[0,0,1]),('LOAD_VARIABLE',[0,258]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(3000,[('ENTER_FRAME',[0,0,1]),('LOAD_VARIABLE',[0,4]),('CALL_NATIVE',[11,1]),('DISCARD',[]),('CALL',[9000]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(9000,[('RETURN',[])])+self.rows(9100,[('RETURN',[])])
        return sorted(rows,key=lambda r:r['offset'])

    def pin(self,rows):
        hashes={r:(roles._closure(rows,e,9000,self.bindings)['sha256'],) for r,e in ((3,2000),(4,3000),(5,1000))}
        return patch.object(roles,'POLICY_HASHES',hashes)
    def run_rows(self,rows):return roles.activity_policy_roles(rows,snapshot=self.snapshot)

    def test_complete_three_policies_and_expanded_bedroom_closure(self):
        for bedroom in (False,True):
            rows=self.fixture(bedroom)
            with self.pin(rows):
                result=self.run_rows(rows);self.assertEqual(result['status'],'conditional_policies');self.assertEqual(len(result['policies']),3)
                category=next(p for p in result['policies'] if p['cut_identity']==3)
                self.assertEqual(len(category['functions']),5 if bedroom else 2)
                self.assertEqual(category['requests']['effect_definition'],0)
                self.assertEqual(category['requests']['animation_track'],167)
                self.assertFalse(category['completion_proven'])
                if bedroom:
                    self.assertEqual(category['shared_global_guard']['variable'],[-1,0])
                    self.assertIn('FLOAT-tagged',category['shared_global_guard']['condition'])
                    self.assertTrue(category['shared_global_guard']['meaning_unresolved'])

    def test_same_caller_workflow_and_cleanup_cut_not_unioned(self):
        rows=self.fixture()
        with self.pin(rows):
            other=copy.deepcopy(self.caller);other['entry']=200;other['workflow_variable']=[0,99]
            other['cleanup_binding']=dict(entry=9100,workflow_variable=[0,99]);other['external_cuts'][-1]['target_offset']=9100
            self.callers.return_value['responses'].append(other)
            result=self.run_rows(rows)
            self.assertEqual(len(result['policies']),3)
            self.assertTrue(all(p['workflow_variable']==[0,34] for p in result['policies']))
            self.caller['cleanup_binding']['workflow_variable']=[0,99]
            self.assertFalse(self.run_rows(rows)['policies'])

    def test_cleanup_target_must_match_actual_caller_cut(self):
        rows=self.fixture()
        with self.pin(rows):
            self.caller['external_cuts'][-1]['target_offset']=9100
            self.assertFalse(self.run_rows(rows)['policies'])

    def test_inline_string_count_is_retained_in_full_hash(self):
        rows=self.fixture()
        with self.pin(rows):
            native=next(r for r in rows if r['name']=='SCRIPT_OP_CALL_NATIVE');native['operands'][1]['value']=2
            self.assertEqual({p['cut_identity'] for p in self.run_rows(rows)['policies']},{3,5})

    def test_global_coordinate_is_not_root_or_caller_frame(self):
        rows=self.fixture(True);base=roles._closure(rows,2000,9000,self.bindings)['sha256']
        shared=next(r for r in rows if r['name']=='SCRIPT_OP_LOAD_VARIABLE' and r['operands'][0]['value']==-1)
        shared['operands'][0]['value']=1
        self.assertNotEqual(roles._closure(rows,2000,9000,self.bindings)['sha256'],base)

    def test_stale_source_vm_native_dependency_and_deferred(self):
        with patch.object(roles,'VM_UNIT_SHA256','stale'):self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')
        self.snapshot[1]['context_vm_contract']['current']=1
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved');self.snapshot[1]['context_vm_contract']['current']=True
        active=self.snapshot[0]['active']['actorScriptEnqueueMessage'];active['dependency']='stale'
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved');active.pop('dependency')
        deferred=self.snapshot[0]['active_deferred']['actorScriptStartActionAndWait'];deferred['dependency']='stale'
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')

    def test_missing_pins_opcode_tags_arity_native_address_and_local_body(self):
        bindings=dict(self.bindings);bindings.pop('actorScriptYield')
        with patch.object(roles,'NATIVE_BINDINGS',bindings):self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')
        for mutate in (lambda r:r[0].update(opcode=99),lambda r:r[0].update(size=1),lambda r:r[0]['operands'][0].update(tag=True),lambda r:r[0]['successors'][0].update(offset=9999)):
            rows=self.fixture();mutate(rows);self.assertEqual(self.run_rows(rows)['status'],'unresolved')
        rows=self.fixture();next(r for r in rows if r['name']=='SCRIPT_OP_CALL_NATIVE')['native']['address']+=4
        with self.assertRaises(ValueError):roles._closure(rows,3000,9000,self.bindings)
        rows=self.fixture();rows=[r for r in rows if r['offset']!=1100]
        with self.assertRaises(ValueError):roles._closure(rows,1000,9000,self.bindings)

    def test_root_bijection_preserved_and_cleanup_not_internal(self):
        rows=self.fixture();base=roles._closure(rows,3000,9000,self.bindings)['sha256']
        next(r for r in rows if r['name']=='SCRIPT_OP_LOAD_VARIABLE')['operands'][1]['value']=44
        self.assertEqual(roles._closure(rows,3000,9000,self.bindings)['sha256'],base)
        with self.assertRaises(ValueError):roles._closure(rows,3000,3016,self.bindings)

    def test_recursion_competing_entry_and_reachable_budget(self):
        rows=self.fixture();call=next(r for r in rows if r['name']=='SCRIPT_OP_CALL' and r['operands'][0]['value']==1100)
        call['operands'][0]['value']=1000;call['successors'][1]['offset']=1000
        with self.assertRaises(ValueError):roles._closure(rows,1000,9000,self.bindings)
        rows=self.fixture();rows.insert(0,self.rows(1,[('JUMP',[1016])])[0])
        with self.assertRaises(ValueError):roles._closure(rows,1000,9000,self.bindings)
        rows=self.rows(10000,[('ENTER_FRAME',[0,0,1])]+[('PUSH_LITERAL',[0])]*257+[('RETURN',[])])
        with self.assertRaises(ValueError):roles._closure(rows,10000,9000,self.bindings)


if __name__=='__main__':unittest.main()
