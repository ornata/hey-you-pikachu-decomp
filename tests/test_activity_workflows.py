"""Authored activity selectors and short complete graph fixtures; no retail payload."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder import activity_workflows as roles
from tools.script_decoder.context import opcode_names


class ActivityWorkflowTests(unittest.TestCase):
    def setUp(self):
        self.bindings=copy.deepcopy(roles.NATIVE_BINDINGS)
        active,deferred={},{}
        for name,b in self.bindings.items():
            active[name]=dict(arguments=0,fixture=name)
            b['review_sha256']=roles._digest(active[name])
            if 'deferred_review_sha256' in b:
                deferred[name]=dict(arguments=1,fixture=name)
                b['deferred_review_sha256']=roles._digest(deferred[name])
        self.snapshot=({'active':active,'active_deferred':deferred},{'context_vm_contract':{'current':True}})
        pin=patch.object(roles,'NATIVE_BINDINGS',self.bindings);pin.start();self.addCleanup(pin.stop)
        rps=patch.object(roles,'rps_rounds',return_value=dict(status='conditional_rounds',source_bindings={'fixture':True},
                  rounds=[dict(entry=2100,message_snapshot_binding={'entry_offset':1800},sampled_message_fields={'type':[0,11]})]))
        self.rps_mock=rps.start();self.addCleanup(rps.stop)
        snapshot=patch.object(roles,'companion_snapshots',return_value=dict(status='conditional_snapshots',snapshots=[dict(entry_offset=1800,status_variable=[0,10])]))
        self.snapshot_mock=snapshot.start();self.addCleanup(snapshot.stop)
        self.opcodes={v:k for k,v in opcode_names('vm').items()}

    def rows(self,start,specs):
        result=[];offset=start
        for name,values in specs:
            tag=2 if name in ('CALL','CALL_NATIVE','JUMP','JUMP_IF_ZERO') else 0
            row=dict(offset=offset,opcode=self.opcodes['SCRIPT_OP_'+name],name='SCRIPT_OP_'+name,size=1+5*len(values),
                     operands=[dict(tag=tag,kind='code_reference' if tag==2 else 'integer',value=v) for v in values])
            row['successors']=roles._edges(row,values)
            if name=='CALL_NATIVE':
                row['native']=dict(functions=['actorScriptRandomBelow'],address=self.bindings['actorScriptRandomBelow']['address'],inline_operands=[])
            result.append(row);offset+=row['size']
        return result

    def fixture(self):
        rows=self.rows(100,[('PUSH_LITERAL',[116]),('PUSH_LITERAL',[124]),('PUSH_LITERAL',[116]),('SUBTRACT',[]),
                            ('CALL_NATIVE',[6]),('ADD',[]),('STORE_VARIABLE',[0,34])])
        start=137;exit=433
        for selector in range(116,124):
            target=1000 if selector==122 else 2100 if selector==123 else 1500+(selector-116)*6
            rows+=self.rows(start,[('LOAD_VARIABLE',[0,34]),('PUSH_LITERAL',[selector]),('EQUAL',[]),
                ('JUMP_IF_ZERO',[start+37]),('CALL',[target]),('DISCARD',[]),('JUMP',[exit])]);start+=37
        rows+=self.rows(exit,[('RETURN',[])])
        rows+=self.rows(1000,[('ENTER_FRAME',[0,0,1]),*[( 'CALL',[t]) for t in (1600,1610,1800,1620,2000,2030)],
                             ('LOAD_VARIABLE',[0,11]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(2000,[('ENTER_FRAME',[0,0,1]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(2030,[('ENTER_FRAME',[0,0,1]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(1620,[('ENTER_FRAME',[1,1,1]),('LOAD_VARIABLE',[0,10]),('LOAD_VARIABLE',[1,0]),('BITWISE_AND',[]),('RETURN',[])])
        for target in (1500,1506,1512,1518,1524,1530,1600,1610,1800,2100):rows+=self.rows(target,[('RETURN',[])])
        return sorted(rows,key=lambda r:r['offset'])

    def run_rows(self,rows):return roles.activity_workflow_roles(rows,snapshot=self.snapshot)

    def test_producer_all_selectors_and_exact_rps_episode_links(self):
        rows=self.fixture();g=roles._episode(rows,1000,self.bindings)
        with patch.object(roles,'EPISODE_SHA256',g['sha256']):
            v=self.run_rows(rows)
            self.assertEqual(v['status'],'conditional_workflows')
            self.assertEqual(len(v['producers']),1);self.assertEqual(len(v['dispatches']),8);self.assertEqual(len(v['episodes']),1)
            self.assertEqual(v['workflows'][0]['selectors'],list(range(116,124)))
            self.assertEqual([d['selector'] for d in v['dispatches'] if d['role']],[122,123])
            self.assertFalse(v['producers'][0]['reaching_initialization_proven'])
            self.assertFalse(v['episodes'][0]['completion_proven'])

    def test_same_root_and_actual_rps_target_required_not_local123(self):
        rows=self.fixture();rows[6]['operands'][1]['value']=35
        self.assertFalse(self.run_rows(rows)['workflows'])
        rows=self.fixture()
        for r in rows:
            if r['name']=='SCRIPT_OP_LOAD_VARIABLE' and r['operands'][1]['value']==34:r['operands'][0]['value']=1
        self.assertFalse(self.run_rows(rows)['workflows'])
        rows=self.fixture();self.rps_mock.return_value['rounds'][0]['entry']=2110
        self.assertFalse(self.run_rows(rows)['workflows'])

    def test_stale_native_deferred_vm_action_header_and_required_set(self):
        for key in ('VM_UNIT_SHA256','ACTION_HEADER_SHA256'):
            with patch.object(roles,key,'stale'):self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')
        self.snapshot[1]['context_vm_contract']['current']=1
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')
        self.snapshot[1]['context_vm_contract']['current']=True
        a=self.snapshot[0]['active']['actorScriptRandomBelow'];a['dependency']='stale'
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved');a.pop('dependency')
        a=self.snapshot[0]['active_deferred']['actorScriptStartActionAndWait'];a['dependency']='stale'
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved');a.pop('dependency')
        self.bindings.pop('actorScriptChooseEscapeFromPlayer')
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')

    def test_opcode_cfg_arity_and_competing_producer_entry(self):
        for mutate in (lambda r:r[0].update(opcode=99),lambda r:r[0].update(size=1),
                       lambda r:r[0]['operands'][0].update(tag=True),lambda r:r[0]['successors'][0].update(offset=9999)):
            rows=self.fixture();mutate(rows);self.assertEqual(self.run_rows(rows)['status'],'unresolved')
        rows=self.fixture();rows.insert(0,self.rows(1,[('JUMP',[106])])[0])
        self.assertFalse(self.run_rows(rows)['workflows'])

    def test_episode_internal_closure_and_snapshot_root_coupling(self):
        rows=self.fixture();g=roles._episode(rows,1000,self.bindings)
        with patch.object(roles,'EPISODE_SHA256',g['sha256']):
            self.rps_mock.return_value['rounds'][0]['sampled_message_fields']['type']=[0,12]
            self.assertFalse(self.run_rows(rows)['episodes'])
        rows=self.fixture();rows=[r for r in rows if r['offset']!=2030]
        with self.assertRaises(ValueError):roles._episode(rows,1000,self.bindings)

    def test_bijective_root_and_external_call_aliases(self):
        rows=self.fixture();g=roles._episode(rows,1000,self.bindings)
        changed=copy.deepcopy(rows)
        for r in changed:
            if r['name']=='SCRIPT_OP_LOAD_VARIABLE' and r['operands'][1]['value']==11:r['operands'][1]['value']=90
        self.assertEqual(roles._episode(changed,1000,self.bindings)['sha256'],g['sha256'])
        call=next(r for r in changed if r['name']=='SCRIPT_OP_CALL' and r['operands'][0]['value']==1610)
        call['operands'][0]['value']=1600;call['successors'][1]['offset']=1600
        with self.assertRaises(ValueError):roles._episode(changed,1000,self.bindings)

    def test_dispatch_false_edge_cannot_also_enter_call(self):
        rows=self.fixture();jump=next(r for r in rows if r['name']=='SCRIPT_OP_JUMP_IF_ZERO')
        jump['operands'][0]['value']=jump['offset']+jump['size'];jump['successors'][1]['offset']=jump['operands'][0]['value']
        result=self.run_rows(rows)
        self.assertNotIn(116,[d['selector'] for d in result['dispatches']])

    def test_complete_status_mask_getter_and_snapshot_root_match(self):
        rows=self.fixture()
        binding=roles._mask_helper(rows,1620,[0,10])
        self.assertEqual(binding['bytes'],40)
        with self.assertRaises(ValueError):roles._mask_helper(rows,1620,[0,20])
        g=roles._episode(rows,1000,self.bindings)
        with patch.object(roles,'EPISODE_SHA256',g['sha256']):
            self.snapshot_mock.return_value['snapshots'][0]['status_variable']=[0,20]
            self.assertFalse(self.run_rows(rows)['episodes'])
        rows=self.fixture();next(r for r in rows if r['offset']==1620)['operands'][0]['value']=0
        with self.assertRaises(ValueError):roles._mask_helper(rows,1620,[0,10])

    def test_random_producer_rebinding_or_alias_refuses_workflow(self):
        for change in (lambda n:n.update(address=0),lambda n:n.update(functions=['actorScriptRandomBelow','alias'])):
            rows=self.fixture();change(rows[4]['native'])
            self.assertFalse(self.run_rows(rows)['workflows'])


if __name__=='__main__':unittest.main()
