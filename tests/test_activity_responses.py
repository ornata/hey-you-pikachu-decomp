"""Authored short local/cut graphs; no original activity script data."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder import activity_responses as roles
from tools.script_decoder.context import opcode_names


class ActivityResponseTests(unittest.TestCase):
    def setUp(self):
        self.bindings=copy.deepcopy(roles.NATIVE_BINDINGS);active,deferred={},{}
        for n,b in self.bindings.items():
            active[n]=dict(arguments=0,fixture=n);b['review_sha256']=roles._digest(active[n])
            if 'deferred_review_sha256' in b:
                deferred[n]=dict(arguments=1,fixture=n);b['deferred_review_sha256']=roles._digest(deferred[n])
        self.snapshot=({'active':active,'active_deferred':deferred},{'context_vm_contract':{'current':True}})
        p=patch.object(roles,'NATIVE_BINDINGS',self.bindings);p.start();self.addCleanup(p.stop)
        p=patch.object(roles,'activity_workflow_roles',return_value=dict(status='conditional_workflows',source_bindings={},
            dispatches=[dict(selector=118,target_offset=100,call_offset=7,variable=[0,34])]))
        self.activity=p.start();self.addCleanup(p.stop)
        p=patch.object(roles,'workflow_cleanup_roles',return_value=dict(status='conditional_saved_workflows',source_bindings={},
            cleanups=[dict(entry=1900,normalized_sha256='authored',workflow_variable=[0,34])]))
        self.saved=p.start();self.addCleanup(p.stop)
        p=patch.object(roles,'companion_snapshots',return_value=dict(status='conditional_snapshots',source_bindings={},
            snapshots=[dict(entry_offset=1200,canonical_sha256='authored',root_role_mapping={'10':0,'11':1,'12':2},status_variable=[0,10])]))
        self.snapshots=p.start();self.addCleanup(p.stop)
        self.opcodes={v:k for k,v in opcode_names('vm').items()}

    def rows(self,start,specs):
        out=[];offset=start
        for name,values in specs:
            tag=2 if name in ('CALL','CALL_NATIVE','JUMP','JUMP_IF_ZERO') else 0
            row=dict(offset=offset,opcode=self.opcodes['SCRIPT_OP_'+name],name='SCRIPT_OP_'+name,size=1+5*len(values),
                     operands=[dict(tag=tag,kind='code_reference' if tag==2 else 'integer',value=v) for v in values])
            row['successors']=roles._edges(row,values)
            if name=='CALL_NATIVE':row['native']=dict(functions=['actorScriptPassThroughInteger'],address=self.bindings['actorScriptPassThroughInteger']['address'],inline_operands=[])
            out.append(row);offset+=row['size']
        return out

    def fixture(self):
        specs=[('ENTER_FRAME',[0,3,1]),*[( 'CALL',[t]) for t in (1100,1200,1300,1400,2000,1500,1600,1700)],
               ('LOAD_VARIABLE',[0,11]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])]
        rows=self.rows(100,specs)
        rows+=self.rows(2000,[('ENTER_FRAME',[0,0,1]),('CALL',[1800]),('DISCARD',[]),('LOAD_VARIABLE',[0,12]),('CALL_NATIVE',[7]),('DISCARD',[]),('CALL',[1900]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(1300,[('ENTER_FRAME',[1,1,1]),('LOAD_VARIABLE',[0,10]),('LOAD_VARIABLE',[1,0]),('BITWISE_AND',[]),('RETURN',[])])
        for t in (1100,1200,1400,1500,1600,1700,1800,1900):rows+=self.rows(t,[('RETURN',[])])
        return sorted(rows,key=lambda r:r['offset'])

    def run_rows(self,rows):return roles.activity_response_roles(rows,snapshot=self.snapshot)
    def pin(self,rows):return patch.object(roles,'FAMILY_SHA256',roles._family(rows,100,self.bindings)['sha256'])

    def test_complete_local_family_and_nine_explicit_cuts(self):
        rows=self.fixture()
        with self.pin(rows):
            v=self.run_rows(rows);self.assertEqual(v['status'],'conditional_responses');self.assertEqual(len(v['responses']),1)
            result=v['responses'][0]
            self.assertEqual(len(result['functions']),2);self.assertEqual(len(result['external_cuts']),9)
            self.assertEqual(result['parity_response']['even']['effect_definition'],0)
            self.assertEqual(result['parity_response']['even']['animation_track'],168)
            self.assertFalse(result['completion_proven']);self.assertFalse(result['external_cuts_preserve_context_proven'])

    def test_actual_dispatch_selector_and_target_are_required(self):
        rows=self.fixture()
        with self.pin(rows):
            self.activity.return_value['dispatches'][0]['selector']=237
            self.assertFalse(self.run_rows(rows)['responses'])
            self.activity.return_value['dispatches'][0]['selector']=118
            self.activity.return_value['dispatches'][0]['target_offset']=1100
            self.assertFalse(self.run_rows(rows)['responses'])

    def test_cleanup_target_and_same_dispatch_root_not_union_membership(self):
        rows=self.fixture()
        with self.pin(rows):
            self.saved.return_value['cleanups'][0]['workflow_variable']=[0,99]
            self.saved.return_value['cleanups'].append(dict(entry=1910,normalized_sha256='other',workflow_variable=[0,34]))
            self.assertFalse(self.run_rows(rows)['responses'])
            self.saved.return_value['cleanups'][0]['workflow_variable']=[0,34]
            self.activity.return_value['dispatches'][0]['variable']=[0,99]
            self.assertFalse(self.run_rows(rows)['responses'])

    def test_snapshot_fields_and_complete_mask_getter_are_coupled(self):
        rows=self.fixture()
        with self.pin(rows):
            for mapping in ({'10':0,'11':2,'12':1},{'10':0,'21':1,'12':2},{'10':0,'11':1,'22':2}):
                self.snapshots.return_value['snapshots'][0]['root_role_mapping']=mapping
                self.assertFalse(self.run_rows(rows)['responses'])
            self.snapshots.return_value['snapshots'][0]['root_role_mapping']={'10':0,'11':1,'12':2}
            next(r for r in rows if r['offset']==1316)['operands'][1]['value']=20
            self.assertFalse(self.run_rows(rows)['responses'])

    def test_bijective_roots_preserve_shape_but_alias_merge_does_not(self):
        rows=self.fixture();g=roles._family(rows,100,self.bindings)
        for r in rows:
            if r['name']=='SCRIPT_OP_LOAD_VARIABLE' and r['operands'][1]['value'] in (11,12):r['operands'][1]['value']+=30
        self.assertEqual(roles._family(rows,100,self.bindings)['sha256'],g['sha256'])
        for r in rows:
            if r['name']=='SCRIPT_OP_LOAD_VARIABLE' and r['operands'][1]['value']==42:r['operands'][1]['value']=41
        self.assertNotEqual(roles._family(rows,100,self.bindings)['sha256'],g['sha256'])

    def test_multiple_dispatch_roots_cannot_share_cleanup_proof(self):
        rows=self.fixture()
        with self.pin(rows):
            self.activity.return_value['dispatches'].append(dict(selector=118,target_offset=100,call_offset=8,variable=[0,99]))
            self.saved.return_value['cleanups'].append(dict(entry=1910,normalized_sha256='other',workflow_variable=[0,99]))
            report=self.run_rows(rows)
            self.assertEqual(len(report['responses']),1)
            self.assertEqual(report['responses'][0]['workflow_variable'],[0,34])

    def test_stale_native_dependency_deferred_vm_header_and_required_set(self):
        for key in ('VM_UNIT_SHA256','ACTION_HEADER_SHA256'):
            with patch.object(roles,key,'stale'):self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')
        self.snapshot[1]['context_vm_contract']['current']=1
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved');self.snapshot[1]['context_vm_contract']['current']=True
        a=self.snapshot[0]['active']['actorScriptPassThroughInteger'];a['dependency']='stale'
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved');a.pop('dependency')
        a=self.snapshot[0]['active_deferred']['actorScriptStartActionAndWait'];a['dependency']='stale'
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved');a.pop('dependency')
        b=dict(self.bindings);b.pop('actorScriptRandomBelow')
        with patch.object(roles,'NATIVE_BINDINGS',b):self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')

    def test_cfg_tags_opcode_arity_native_and_missing_local_closure(self):
        for mutate in (lambda r:r[0].update(opcode=99),lambda r:r[0].update(size=1),
                       lambda r:r[0]['operands'][0].update(tag=True),lambda r:r[0]['successors'][0].update(offset=9999)):
            rows=self.fixture();mutate(rows);self.assertEqual(self.run_rows(rows)['status'],'unresolved')
        rows=self.fixture();next(r for r in rows if r['name']=='SCRIPT_OP_CALL_NATIVE')['native']['address']+=4
        with self.assertRaises(ValueError):roles._family(rows,100,self.bindings)
        rows=self.fixture();rows=[r for r in rows if r['offset']!=2000]
        with self.assertRaises(ValueError):roles._family(rows,100,self.bindings)

    def test_external_alias_or_interior_rebinding_and_competing_entry(self):
        rows=self.fixture();call=next(r for r in rows if r['name']=='SCRIPT_OP_CALL' and r['operands'][0]['value']==1100)
        call['operands'][0]['value']=1200;call['successors'][1]['offset']=1200
        with self.assertRaises(ValueError):roles._family(rows,100,self.bindings)
        rows=self.fixture();call=next(r for r in rows if r['name']=='SCRIPT_OP_CALL')
        call['operands'][0]['value']=116;call['successors'][1]['offset']=116
        with self.assertRaises(ValueError):roles._family(rows,100,self.bindings)
        rows=self.fixture();rows.insert(0,self.rows(1,[('JUMP',[116])])[0])
        with self.assertRaises(ValueError):roles._family(rows,100,self.bindings)


if __name__=='__main__':unittest.main()
