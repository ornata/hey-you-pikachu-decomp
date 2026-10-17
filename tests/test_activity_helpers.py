"""Short authored graphs exercise helper bindings without original script bytes."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder import activity_helpers as roles
from tools.script_decoder.context import opcode_names
from tools.script_decoder.companion_snapshots import _edges


class ActivityHelperTests(unittest.TestCase):
    def setUp(self):
        self.bindings=copy.deepcopy(roles.NATIVE_BINDINGS)
        active,deferred={},{}
        for name,binding in self.bindings.items():
            active[name]={'fixture':name};binding['review_sha256']=roles._digest(active[name])
            if 'deferred_review_sha256' in binding:
                deferred[name]={'fixture':name};binding['deferred_review_sha256']=roles._digest(deferred[name])
        self.snapshot=({'active':active,'active_deferred':deferred},{'context_vm_contract':{'current':True}})
        self.caller={'entry':100,'workflow_variable':[0,34],
            'cleanup_binding':{'entry':9000,'workflow_variable':[0,34],'normalized_sha256':'cleanup'},
            'message_snapshot_binding':{'entry':8000,'canonical_sha256':'snapshot'},
            'external_cuts':[{'identity':k,'target_offset':v} for k,v in ((0,1000),(1,8000),(6,2000),(7,3000),(8,9000))]}
        self.cleanup={'entry':9000,'workflow_variable':[0,34],'normalized_sha256':'cleanup',
            'functions':[{'entry':9000},{'entry':2000},{'entry':2100}]}
        self.producer={'entry_offset':8000,'canonical_sha256':'snapshot',
            'root_role_mapping':{27:12,28:14,30:6,29:13,39:22}}
        self.mocks={}
        for name,value in (
            ('NATIVE_BINDINGS',self.bindings),
            ('activity_response_roles',{'status':'conditional_responses','source_bindings':{},'responses':[self.caller]}),
            ('workflow_cleanup_roles',{'status':'conditional_saved_workflows','source_bindings':{},'cleanups':[self.cleanup]}),
            ('companion_snapshots',{'status':'conditional_snapshots','source_bindings':{},'snapshots':[self.producer]})):
            p=patch.object(roles,name,value) if name=='NATIVE_BINDINGS' else patch.object(roles,name,return_value=value)
            self.mocks[name]=p.start();self.addCleanup(p.stop)
        self.opcodes={v:k for k,v in opcode_names('vm').items()}

    def rows(self,start,specs):
        rows=[];offset=start
        for name,values in specs:
            operands=[{'tag':2 if name in ('CALL','CALL_NATIVE','JUMP','JUMP_IF_ZERO') and i==0 else 0,
                       'kind':'code_reference' if name in ('CALL','CALL_NATIVE','JUMP','JUMP_IF_ZERO') and i==0 else 'integer','value':v} for i,v in enumerate(values)]
            row={'offset':offset,'opcode':self.opcodes['SCRIPT_OP_'+name],'name':'SCRIPT_OP_'+name,
                 'size':1+5*len(values),'operands':operands}
            row['successors']=_edges(row,values)
            if name=='CALL_NATIVE':
                n='actorScriptResetTimer';row['native']={'functions':[n],'address':self.bindings[n]['address'],'inline_operands':[]}
            rows.append(row);offset+=row['size']
        return rows

    def fixture(self):
        init=[('ENTER_FRAME',[0,0,1]),('CALL_NATIVE',[1]),('DISCARD',[])]
        for slot,value in ((33,1),(27,0),(28,0),(30,0),(29,0),(39,100)):
            init += [('PUSH_LITERAL',[value]),('STORE_VARIABLE',[0,slot])]
        init += [('PUSH_LITERAL',[0]),('RETURN',[])]
        rows=self.rows(1000,init)
        rows+=self.rows(2000,[('ENTER_FRAME',[0,0,1]),('CALL',[2100]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(2100,[('ENTER_FRAME',[0,0,1]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(3000,[('ENTER_FRAME',[0,0,1]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(9000,[('RETURN',[])])
        return sorted(rows,key=lambda r:r['offset'])

    def pin(self,rows):
        return patch.object(roles,'HELPER_HASHES',{k:(roles._graph(rows,[e],self.bindings)['sha256'],) for k,e in ((0,1000),(6,2000),(7,3000))})
    def run_rows(self,rows):return roles.activity_helper_roles(rows,snapshot=self.snapshot)

    def test_complete_helpers_and_conditionally_named_writes(self):
        rows=self.fixture()
        with self.pin(rows):
            out=self.run_rows(rows);self.assertEqual(out['status'],'conditional_helpers');self.assertEqual(len(out['helpers']),3)
            init=out['helpers'][0];self.assertFalse(init['unresolved_flag']['purpose_recovered'])
            self.assertEqual([x['snapshot_root_role'] for x in init['root_writes']],[None,12,14,6,13,22])
            self.assertIn('not distance traveled',init['distance_scope']);self.assertFalse(init['workflow_clear_proven'])
            self.assertEqual(out['helpers'][2]['action_program'],18)

    def test_actual_caller_cleanup_pair_not_other_roots(self):
        rows=self.fixture()
        with self.pin(rows):
            for key,value in (('workflow_variable',[0,35]),('entry',9100),('normalized_sha256','different')):
                old=self.cleanup[key];self.cleanup[key]=value
                self.assertEqual(self.run_rows(rows)['helpers'],[]);self.cleanup[key]=old
            self.caller['cleanup_binding']['workflow_variable']=[0,35]
            self.assertEqual(self.run_rows(rows)['helpers'],[])

    def test_actual_snapshot_entry_hash_and_five_role_aliases(self):
        rows=self.fixture()
        with self.pin(rows):
            for key,value in (('entry_offset',8100),('canonical_sha256','other')):
                old=self.producer[key];self.producer[key]=value
                self.assertEqual(self.run_rows(rows)['helpers'],[]);self.producer[key]=old
            self.producer['root_role_mapping'][27]=13
            self.assertEqual([x['cut_identity'] for x in self.run_rows(rows)['helpers']],[6,7])

    def test_shifted_root_bijection_allowed_only_with_actual_producer(self):
        rows=self.fixture();shifted=copy.deepcopy(rows)
        for row in shifted:
            if row['name']=='SCRIPT_OP_STORE_VARIABLE':row['operands'][1]['value']+=10
        with self.pin(rows):
            self.assertEqual([x['cut_identity'] for x in self.run_rows(shifted)['helpers']],[6,7])
            self.producer['root_role_mapping']={slot+10:r for slot,r in self.producer['root_role_mapping'].items()}
            self.assertEqual(len(self.run_rows(shifted)['helpers']),3)

    def test_cut_six_must_be_node_and_descendant_closure_of_same_cleanup(self):
        rows=self.fixture()
        with self.pin(rows):
            self.cleanup['functions']=[{'entry':9000},{'entry':2000}]
            self.assertEqual([x['cut_identity'] for x in self.run_rows(rows)['helpers']],[0,7])
            self.cleanup['functions']=[{'entry':9000},{'entry':2100}]
            self.assertEqual(self.run_rows(rows)['helpers'],[])

    def test_stale_native_deferred_vm_and_required_pin(self):
        rows=self.fixture()
        with self.pin(rows):
            for group,name in (('active','actorScriptResetTimer'),('active_deferred','actorScriptStartLoopExitAndWait')):
                old=self.snapshot[0][group][name];self.snapshot[0][group][name]={'stale':True}
                self.assertEqual(self.run_rows(rows)['status'],'unresolved');self.snapshot[0][group][name]=old
            self.snapshot[1]['context_vm_contract']['current']=1
            self.assertEqual(self.run_rows(rows)['status'],'unresolved')
            self.snapshot[1]['context_vm_contract']['current']=True
            with patch.object(roles,'VM_UNIT_SHA256','wrong'):self.assertEqual(self.run_rows(rows)['status'],'unresolved')
            self.bindings.pop('actorScriptResetTimer');self.assertEqual(self.run_rows(rows)['status'],'unresolved')

    def test_changed_literal_spacing_native_and_complete_callee(self):
        rows=self.fixture()
        with self.pin(rows):
            for mutation in ('literal','spacing','native','callee'):
                bad=copy.deepcopy(rows)
                if mutation=='literal':next(r for r in bad if r['name']=='SCRIPT_OP_PUSH_LITERAL')['operands'][0]['value']=9
                elif mutation=='spacing':next(r for r in bad if r['name']=='SCRIPT_OP_STORE_VARIABLE')['operands'][1]['value']+=1
                elif mutation=='native':next(r for r in bad if r['name']=='SCRIPT_OP_CALL_NATIVE')['native']['address']+=4
                else:bad=[r for r in bad if not 2100<=r['offset']<2200]
                self.assertLess(len(self.run_rows(bad)['helpers']),3)

    def test_malformed_opcode_tags_cfg_competing_entry_and_recursion(self):
        rows=self.fixture()
        with self.pin(rows):
            for key,value in (('opcode',999),('successors',[])):
                bad=copy.deepcopy(rows);bad[0][key]=value
                self.assertEqual(self.run_rows(bad)['status'],'unresolved')
            bad=copy.deepcopy(rows);bad[0]['operands'][0]['tag']=99
            self.assertEqual(self.run_rows(bad)['status'],'unresolved')
            bad=copy.deepcopy(rows)+self.rows(9500,[('JUMP',[rows[1]['offset']])])
            bad.sort(key=lambda r:r['offset'])
            self.assertNotIn(0,[x['cut_identity'] for x in self.run_rows(bad)['helpers']])
            bad=copy.deepcopy(rows)
            r=next(r for r in bad if r['name']=='SCRIPT_OP_CALL');r['operands'][0]['value']=2000;r['successors']=_edges(r,[2000])
            self.assertNotIn(6,[x['cut_identity'] for x in self.run_rows(bad)['helpers']])

    def test_function_budget_and_ambiguous_native_selector(self):
        oversized=self.rows(1000,[('ENTER_FRAME',[0,0,1])]+[('DISCARD',[])]*255+[('PUSH_LITERAL',[0]),('RETURN',[])])
        with self.assertRaises(ValueError):roles._graph(oversized,[1000],self.bindings)
        rows=self.rows(1000,[('ENTER_FRAME',[0,0,1]),('CALL_NATIVE',[1]),('DISCARD',[]),
            ('CALL_NATIVE',[1]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        calls=[r for r in rows if r['name']=='SCRIPT_OP_CALL_NATIVE']
        name='actorScriptPushOriginDistance'
        calls[1]['native']={'functions':[name],'address':self.bindings[name]['address'],'inline_operands':[]}
        with self.assertRaises(ValueError):roles._graph(rows,[1000],self.bindings)


if __name__=='__main__':unittest.main()
