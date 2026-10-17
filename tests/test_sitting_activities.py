"""Authored small sitting graphs test full closure and shared-caller association."""
import copy
import unittest
from unittest.mock import patch

from tools.script_decoder import sitting_activities as roles
from tools.script_decoder.context import opcode_names


class SittingActivityTests(unittest.TestCase):
    def setUp(self):
        self.bindings=copy.deepcopy(roles.NATIVE_BINDINGS);active,deferred={},{}
        for name,binding in self.bindings.items():
            active[name]={'fixture':name};binding['review_sha256']=roles._digest(active[name])
            if 'deferred_review_sha256' in binding:
                deferred[name]={'fixture':name};binding['deferred_review_sha256']=roles._digest(deferred[name])
        self.snapshot=({'active':active,'active_deferred':deferred},{'context_vm_contract':{'current':True}})
        self.dispatch={'selector':120,'target_offset':1000,'variable':[0,34],'call_offset':9000}
        self.cleanup={'entry':4500,'workflow_variable':[0,34],'normalized_sha256':'cleanup'}
        self.producer={'entry_offset':4100,'canonical_sha256':'snapshot','status_variable':[0,10],'root_role_mapping':{11:1}}
        self.init={'entry':4000,'cut_identity':0,'caller_entry':6000,'workflow_variable':[0,34],
            'cleanup_binding':copy.deepcopy(self.cleanup),'snapshot_binding':{'entry':4100,'canonical_sha256':'snapshot'}}
        self.policies=[dict(entry=e,cut_identity=k,caller_entry=6000,workflow_variable=[0,34],
            cleanup_binding=copy.deepcopy(self.cleanup),normalized_sha256='policy'+str(k),
            functions=[{'entry':e},{'entry':4600,'span_bytes':50,'reachable_bytes':50,'instruction_count':10}]) for k,e in ((3,4300),(4,4400),(5,4700))]
        self.mocks={}
        values={'activity_workflow_roles':dict(status='conditional_workflows',dispatches=[self.dispatch]),
            'companion_snapshots':dict(status='conditional_snapshots',snapshots=[self.producer]),
            'activity_helper_roles':dict(status='conditional_helpers',helpers=[self.init]),
            'activity_policy_roles':dict(status='conditional_policies',policies=self.policies),
            'workflow_cleanup_roles':dict(status='conditional_saved_workflows',cleanups=[self.cleanup])}
        p=patch.object(roles,'NATIVE_BINDINGS',self.bindings);p.start();self.addCleanup(p.stop)
        for name,value in values.items():
            value['source_bindings']={};p=patch.object(roles,name,return_value=value);self.mocks[name]=p.start();self.addCleanup(p.stop)
        self.opcodes={v:k for k,v in opcode_names('vm').items()}

    def rows(self,start,specs):
        rows=[];offset=start
        for name,values in specs:
            operands=[dict(tag=2 if name in ('CALL','CALL_NATIVE','JUMP','JUMP_IF_ZERO') and i==0 else 0,
                kind='code_reference' if name in ('CALL','CALL_NATIVE','JUMP','JUMP_IF_ZERO') and i==0 else 'integer',value=v) for i,v in enumerate(values)]
            r=dict(offset=offset,opcode=self.opcodes['SCRIPT_OP_'+name],name='SCRIPT_OP_'+name,size=1+5*len(values),operands=operands)
            r['successors']=roles._edges(r,values)
            if name=='CALL_NATIVE':
                n='actorScriptYield';r['native']=dict(functions=[n],address=self.bindings[n]['address'],inline_operands=[])
            rows.append(r);offset+=r['size']
        return rows

    def fixture(self):
        specs=[('ENTER_FRAME',[0,3,1]),('LOAD_VARIABLE',[0,11]),('DISCARD',[]),('CALL_NATIVE',[1]),('DISCARD',[])]
        for target in (4000,4100,4200,4300,3000,4400,2000):specs += [('CALL',[target]),('DISCARD',[])]
        rows=self.rows(1000,specs+[('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(2000,[('ENTER_FRAME',[0,0,1]),('CALL',[4500]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(3000,[('ENTER_FRAME',[0,0,1]),('CALL',[4600]),('DISCARD',[]),('CALL',[4500]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(4200,[('ENTER_FRAME',[1,1,1]),('LOAD_VARIABLE',[0,10]),('LOAD_VARIABLE',[1,0]),('BITWISE_AND',[]),('RETURN',[])])
        for target in (4000,4100,4300,4400,4500,4600):rows+=self.rows(target,[('ENTER_FRAME',[0,0,1]),('PUSH_LITERAL',[0]),('RETURN',[])])
        return sorted(rows,key=lambda r:r['offset'])
    def pin(self,rows):return patch.object(roles,'FAMILY_SHA256',roles._family(rows,1000,self.bindings)['sha256'])
    def run_rows(self,rows):return roles.sitting_activity_roles(rows,snapshot=self.snapshot)

    def test_complete_three_functions_and_request_scope(self):
        rows=self.fixture()
        with self.pin(rows):
            result=self.run_rows(rows);self.assertEqual(result['status'],'conditional_sitting_activities');self.assertEqual(len(result['activities']),1)
            activity=result['activities'][0];self.assertEqual(len(activity['functions']),3)
            self.assertEqual(activity['loop_default_action_mode'],1);self.assertEqual(activity['completion_request']['action_program'],277)
            self.assertFalse(activity['completion_proven']);self.assertEqual(activity['sampled_message_type'],[0,11])
            self.assertIn('native return yields the VM',activity['initialization']['sequence'])
            self.assertIn('yields and execution resumes',activity['repeated_request']['counter_effect'])

    def test_dispatch_namespace_target_and_workflow_pairing(self):
        rows=self.fixture()
        with self.pin(rows):
            for key,value in (('selector',119),('target_offset',2000),('variable',[0,35])):
                old=self.dispatch[key];self.dispatch[key]=value;self.assertEqual(self.run_rows(rows)['activities'],[]);self.dispatch[key]=old

    def test_snapshot_category_mask_and_actual_initializer_pairing(self):
        rows=self.fixture()
        with self.pin(rows):
            for key,value in (('entry_offset',4900),('root_role_mapping',{12:1}),('status_variable',[0,12])):
                old=self.producer[key];self.producer[key]=value;self.assertEqual(self.run_rows(rows)['activities'],[]);self.producer[key]=old
            for key,value in (('entry',4900),('canonical_sha256','bad')):
                old=self.init['snapshot_binding'][key];self.init['snapshot_binding'][key]=value;self.assertEqual(self.run_rows(rows)['activities'],[]);self.init['snapshot_binding'][key]=old

    def test_policies_exact_targets_caller_cleanup_and_tracking_node(self):
        rows=self.fixture()
        with self.pin(rows):
            for key,value in (('entry',4900),('caller_entry',6100),('workflow_variable',[0,35])):
                old=self.policies[0][key];self.policies[0][key]=value;self.assertEqual(self.run_rows(rows)['activities'],[]);self.policies[0][key]=old
            self.policies[1]['cleanup_binding']['entry']=4800;self.assertEqual(self.run_rows(rows)['activities'],[])
            self.policies[1]['cleanup_binding']['entry']=4500
            for key,value in (('entry',4700),('span_bytes',103),('instruction_count',23)):
                node=self.policies[2]['functions'][1];old=node[key];node[key]=value
                self.assertEqual(self.run_rows(rows)['activities'],[]);node[key]=old

    def test_no_union_or_ambiguous_initializer_cleanup_acceptance(self):
        rows=self.fixture()
        with self.pin(rows):
            other=copy.deepcopy(self.init);other['workflow_variable']=[0,35]
            self.mocks['activity_helper_roles'].return_value['helpers']=[other]
            self.assertEqual(self.run_rows(rows)['activities'],[])
            self.mocks['activity_helper_roles'].return_value['helpers']=[self.init,self.init]
            self.assertEqual(self.run_rows(rows)['activities'],[])

    def test_stale_vm_header_native_deferred_required_set(self):
        rows=self.fixture()
        with self.pin(rows):
            for field in ('VM_UNIT_SHA256','ACTION_HEADER_SHA256'):
                with patch.object(roles,field,'stale'):self.assertEqual(self.run_rows(rows)['status'],'unresolved')
            for kind,name in (('active','actorScriptSetTargetPosition'),('active_deferred','actorScriptStartActionAndWait')):
                old=self.snapshot[0][kind][name];self.snapshot[0][kind][name]={'bad':True}
                self.assertEqual(self.run_rows(rows)['status'],'unresolved');self.snapshot[0][kind][name]=old
            self.snapshot[1]['context_vm_contract']['current']=1;self.assertEqual(self.run_rows(rows)['status'],'unresolved')
            self.snapshot[1]['context_vm_contract']['current']=True
            self.bindings.pop('actorScriptYield');self.assertEqual(self.run_rows(rows)['status'],'unresolved')

    def test_altered_aliases_interior_external_missing_body_and_literal(self):
        rows=self.fixture()
        with self.pin(rows):
            for mode in ('alias','interior','body','literal'):
                bad=copy.deepcopy(rows)
                if mode=='body':bad=[r for r in bad if not 3000<=r['offset']<3100]
                elif mode=='literal':next(r for r in bad if r['name']=='SCRIPT_OP_PUSH_LITERAL')['operands'][0]['value']=9
                else:
                    r=next(r for r in bad if r['name']=='SCRIPT_OP_CALL');r['operands'][0]['value']=4100 if mode=='alias' else bad[1]['offset'];r['successors']=roles._edges(r,[r['operands'][0]['value']])
                self.assertEqual(self.run_rows(bad)['activities'],[])

    def test_malformed_opcode_tags_native_cfg_and_competing_entry(self):
        rows=self.fixture()
        with self.pin(rows):
            for key,value in (('opcode',999),('successors',[])):
                bad=copy.deepcopy(rows);bad[0][key]=value;self.assertEqual(self.run_rows(bad)['status'],'unresolved')
            bad=copy.deepcopy(rows);bad[0]['operands'][0]['tag']=99;self.assertEqual(self.run_rows(bad)['status'],'unresolved')
            bad=copy.deepcopy(rows);next(r for r in bad if r['name']=='SCRIPT_OP_CALL_NATIVE')['native']['address']+=4
            self.assertEqual(self.run_rows(bad)['activities'],[])
            bad=copy.deepcopy(rows)+self.rows(9500,[('JUMP',[rows[1]['offset']])]);bad.sort(key=lambda r:r['offset'])
            self.assertEqual(self.run_rows(bad)['activities'],[])


if __name__=='__main__':unittest.main()
