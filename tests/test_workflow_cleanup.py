"""Authored small closure fixtures; original saved-workflow bytes are not embedded."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder import workflow_cleanup as roles
from tools.script_decoder.context import opcode_names


class WorkflowCleanupTests(unittest.TestCase):
    def setUp(self):
        self.bindings=copy.deepcopy(roles.NATIVE_BINDINGS);active,deferred={},{}
        for n,b in self.bindings.items():
            active[n]=dict(arguments=0,fixture=n);b['review_sha256']=roles._digest(active[n])
            if 'deferred_review_sha256' in b:
                deferred[n]=dict(arguments=1,fixture=n);b['deferred_review_sha256']=roles._digest(deferred[n])
        self.snapshot=({'active':active,'active_deferred':deferred},{'context_vm_contract':{'current':True}})
        p=patch.object(roles,'NATIVE_BINDINGS',self.bindings);p.start();self.addCleanup(p.stop)
        p=patch.object(roles,'activity_workflow_roles',return_value=dict(status='conditional_workflows',source_bindings={'authored':True},
            workflows=[dict(variable=[0,34])],dispatches=[dict(role='source_bound_player_relative_episode',target_offset=2000,variable=[0,34])],episodes=[dict(entry=2000,external_calls=[dict(target_offset=3000,opaque_identity=5)])]))
        self.activity=p.start();self.addCleanup(p.stop)
        self.opcodes={v:k for k,v in opcode_names('vm').items()}

    def rows(self,start,specs):
        out=[];offset=start
        for name,values in specs:
            tag=2 if name in ('CALL','CALL_NATIVE','JUMP','JUMP_IF_ZERO') else 0
            row=dict(offset=offset,opcode=self.opcodes['SCRIPT_OP_'+name],name='SCRIPT_OP_'+name,size=1+5*len(values),
                     operands=[dict(tag=tag,kind='code_reference' if tag==2 else 'integer',value=v) for v in values])
            row['successors']=roles._edges(row,values)
            if name=='CALL_NATIVE':row['native']=dict(functions=['actorScriptPushTrackingMode'],address=self.bindings['actorScriptPushTrackingMode']['address'],inline_operands=[])
            out.append(row);offset+=row['size']
        return out

    def fixture(self,bedroom=False):
        roots=[95,45,35,50,36,55,37,60,70,65,75,80,85,90,34,38]
        reads=[]
        for r in roots:reads += [('LOAD_VARIABLE',[0,r]),('DISCARD',[])]
        rows=self.rows(4000,[('ENTER_FRAME',[0,3,1]),*reads,('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(5000,[('ENTER_FRAME',[0,4,1]),*reads,('CALL_NATIVE',[93]),('DISCARD',[]),('CALL',[5500]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(5500,[('ENTER_FRAME',[0,2,1]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(3000,[('ENTER_FRAME',[0,0,1]),('PUSH_LITERAL',[0]),('STORE_VARIABLE',[0,40]),('CALL',[3100]),('DISCARD',[]),('CALL',[4000]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(3100,[('ENTER_FRAME',[0,0,1]),('CALL',[3200]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(3200,[('ENTER_FRAME',[0,0,1]),('CALL',[3300]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
        rows+=self.rows(3300,[('ENTER_FRAME',[0,0,1]),*([('CALL',[3400]),('DISCARD',[])] if bedroom else []),('PUSH_LITERAL',[0]),('RETURN',[])])
        if bedroom:
            rows+=self.rows(3400,[('ENTER_FRAME',[0,0,1]),('CALL',[3500]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
            rows+=self.rows(3500,[('ENTER_FRAME',[0,0,1]),('CALL',[3600]),('DISCARD',[]),('PUSH_LITERAL',[0]),('RETURN',[])])
            rows+=self.rows(3600,[('ENTER_FRAME',[0,0,1]),('PUSH_LITERAL',[0]),('RETURN',[])])
        return sorted(rows,key=lambda r:r['offset'])

    def run_rows(self,rows):return roles.workflow_cleanup_roles(rows,snapshot=self.snapshot)

    def patches(self,rows,bedroom=False):
        values={'RESTORE_SHA256':roles._graph(rows,[4000],self.bindings)['sha256'],
                'STORE_SHA256':roles._graph(rows,[5000,4000],self.bindings)['sha256'],
                'BEDROOM_CLEANUP_SHA256' if bedroom else 'CLEANUP_SHA256':roles._graph(rows,[3000],self.bindings)['sha256']}
        return patch.multiple(roles,**values)

    def test_five_and_eight_function_closures_are_complete_and_distinct(self):
        for bedroom in (False,True):
            rows=self.fixture(bedroom)
            with self.patches(rows,bedroom):
                v=self.run_rows(rows)
                self.assertEqual([len(v[k]) for k in ('stores','restores','cleanups')],[1,1,1])
                self.assertEqual(len(v['cleanups'][0]['functions']),8 if bedroom else 5)
                self.assertFalse(v['cleanups'][0]['zero_workflow_postcondition_proven'])
                self.assertFalse(v['restores'][0]['search_result_used_for_tracking'])
                self.assertEqual(v['stores'][0]['required_integer_index_range'],[-1,4])
                self.assertEqual(v['restores'][0]['required_integer_index_range'],[0,4])

    def test_coordinate_shift_preserves_spacing_but_column_overlap_does_not(self):
        rows=self.fixture();g=roles._graph(rows,[5000,4000],self.bindings)
        for r in rows:
            if r['name'] in ('SCRIPT_OP_LOAD_VARIABLE','SCRIPT_OP_STORE_VARIABLE'):r['operands'][1]['value']+=3
        self.assertEqual(roles._graph(rows,[5000,4000],self.bindings)['sha256'],g['sha256'])
        for r in rows:
            if r['name']=='SCRIPT_OP_LOAD_VARIABLE' and r['operands'][1]['value']==53:r['operands'][1]['value']=49
        self.assertNotEqual(roles._graph(rows,[5000,4000],self.bindings)['sha256'],g['sha256'])

    def test_shared_index_alias_and_save_restore_correspondence(self):
        rows=self.fixture();g=roles._graph(rows,[5000,4000],self.bindings)
        for r in rows:
            if r['offset']>=5000 and r['name']=='SCRIPT_OP_LOAD_VARIABLE' and r['operands'][1]['value']==95:r['operands'][1]['value']=94
        self.assertNotEqual(roles._graph(rows,[5000,4000],self.bindings)['sha256'],g['sha256'])

    def test_actual_cleanup_target_and_workflow_coordinate_coupling(self):
        rows=self.fixture()
        with self.patches(rows):
            self.activity.return_value['episodes'][0]['external_calls'][0]['target_offset']=3100
            self.assertFalse(self.run_rows(rows)['cleanups'])
            self.activity.return_value['episodes'][0]['external_calls'][0]['target_offset']=3000
            self.activity.return_value['workflows'][0]['variable']=[0,99]
            self.assertFalse(self.run_rows(rows)['restores'])

    def test_cleanup_root_cannot_borrow_an_unrelated_episode_root(self):
        rows=self.fixture()
        with self.patches(rows):
            self.activity.return_value['workflows'].append(dict(variable=[0,99]))
            self.activity.return_value['dispatches'][0]['variable']=[0,99]
            self.activity.return_value['dispatches'].append(dict(role='source_bound_player_relative_episode',target_offset=2010,variable=[0,34]))
            self.activity.return_value['episodes'].append(dict(entry=2010,external_calls=[dict(target_offset=3100,opaque_identity=5)]))
            self.assertFalse(self.run_rows(rows)['cleanups'])

    def test_shared_cleanup_target_with_conflicting_episode_roots_is_ambiguous(self):
        rows=self.fixture()
        with self.patches(rows):
            self.activity.return_value['workflows'].append(dict(variable=[0,99]))
            self.activity.return_value['dispatches'].append(dict(role='source_bound_player_relative_episode',target_offset=2010,variable=[0,99]))
            self.activity.return_value['episodes'].append(dict(entry=2010,external_calls=[dict(target_offset=3000,opaque_identity=5)]))
            self.assertFalse(self.run_rows(rows)['cleanups'])

    def test_stale_context_native_dependency_deferred_and_source(self):
        for constant in ('VM_UNIT_SHA256',):
            with patch.object(roles,constant,'stale'):self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')
        self.snapshot[1]['context_vm_contract']['current']=1
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')
        self.snapshot[1]['context_vm_contract']['current']=True
        active=self.snapshot[0]['active']['actorScriptSetTrackingMode'];active['dependency']='stale'
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved');active.pop('dependency')
        deferred=self.snapshot[0]['active_deferred']['actorScriptStartLoopExitAndWait'];deferred['dependency']='stale'
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')

    def test_cfg_arity_opcode_native_and_missing_functions(self):
        for mutate in (lambda r:r[0].update(opcode=99),lambda r:r[0].update(size=1),
                       lambda r:r[0]['operands'][0].update(tag=True),lambda r:r[0]['successors'][0].update(offset=9999)):
            rows=self.fixture();mutate(rows);self.assertEqual(self.run_rows(rows)['status'],'unresolved')
        rows=self.fixture();native=next(r for r in rows if r['name']=='SCRIPT_OP_CALL_NATIVE');native['native']['address']+=4
        with self.assertRaises(ValueError):roles._graph(rows,[5000,4000],self.bindings)
        rows=self.fixture();rows=[r for r in rows if r['offset']!=3300]
        with self.assertRaises(ValueError):roles._graph(rows,[3000],self.bindings)

    def test_recursive_call_and_competing_interior_entry_rejected(self):
        rows=self.fixture();call=next(r for r in rows if r['name']=='SCRIPT_OP_CALL' and r['operands'][0]['value']==3300)
        call['operands'][0]['value']=3100;call['successors'][1]['offset']=3100
        with self.assertRaises(ValueError):roles._graph(rows,[3000],self.bindings)
        rows=self.fixture();rows.insert(0,self.rows(1,[('JUMP',[3016])])[0])
        with self.assertRaises(ValueError):roles._graph(rows,[3000],self.bindings)

    def test_required_pins_and_instruction_budget(self):
        rows=self.fixture();bindings=dict(self.bindings);bindings.pop('actorScriptPushTrackingMode')
        with patch.object(roles,'NATIVE_BINDINGS',bindings):self.assertEqual(self.run_rows(rows)['status'],'unresolved')
        specs=[('ENTER_FRAME',[0,0,1])]+[('PUSH_LITERAL',[0])]*257+[('RETURN',[])]
        with self.assertRaises(ValueError):roles._graph(self.rows(10000,specs),[10000],self.bindings)


if __name__=='__main__':unittest.main()
