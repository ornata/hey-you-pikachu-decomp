"""Authored short normalization fixtures; no original RPS script payload."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder import rps_rounds as roles
from tools.script_decoder.context import opcode_names


class RpsRoundTests(unittest.TestCase):
    def setUp(self):
        self.bindings = copy.deepcopy(roles.NATIVE_BINDINGS)
        active, deferred = {}, {}
        for name, binding in self.bindings.items():
            active[name] = dict(arguments=0, fixture=name)
            binding['review_sha256'] = roles._digest(active[name])
            if 'deferred_review_sha256' in binding:
                deferred[name] = dict(arguments=1, fixture='deferred')
                binding['deferred_review_sha256'] = roles._digest(deferred[name])
        self.snapshot = ({'active':active, 'active_deferred':deferred}, {'context_vm_contract':{'current':True}})
        binding_patch = patch.object(roles, 'NATIVE_BINDINGS', self.bindings)
        binding_patch.start()
        self.addCleanup(binding_patch.stop)
        snapshot_patch = patch.object(roles, 'companion_snapshots', return_value=dict(
            status='conditional_snapshots', source_bindings={'status':'current'},
            snapshots=[dict(entry_offset=1030, canonical_sha256='authored', root_role_mapping={'30':1,'31':2})]))
        self.snapshot_mock = snapshot_patch.start()
        self.addCleanup(snapshot_patch.stop)

    def fixture(self):
        opcodes = {v:k for k,v in opcode_names('vm').items()}
        specs = [('ENTER_FRAME',[0,13,1]),('LOAD_VARIABLE',[0,30]),('DISCARD',[]),
                 ('LOAD_VARIABLE',[0,31]),('DISCARD',[]),('PUSH_LITERAL',[6]),('CALL_NATIVE',[6])]
        specs += [('CALL',[1000+i*6]) for i in range(6)]
        specs += [('PUSH_LITERAL',[0]),('RETURN',[])]
        rows, offset = [], 100
        for name,values in specs:
            tag = 2 if name in ('CALL','CALL_NATIVE') else 0
            operands = [dict(tag=tag,kind='code_reference' if tag==2 else 'integer',value=v) for v in values]
            size=1+5*len(operands)
            row=dict(offset=offset,opcode=opcodes['SCRIPT_OP_'+name],name='SCRIPT_OP_'+name,size=size,operands=operands)
            row['successors']=roles._edges(row,values)
            if name=='CALL_NATIVE':
                row['native']=dict(functions=['actorScriptRandomBelow'],address=self.bindings['actorScriptRandomBelow']['address'],inline_operands=[])
            rows.append(row)
            offset+=size
        for i in range(6):
            rows.append(dict(offset=1000+i*6,opcode=30,name='SCRIPT_OP_RETURN',size=1,operands=[],successors=[]))
        return rows

    def run_rows(self, rows):
        return roles.rps_rounds(rows,snapshot=self.snapshot)

    def test_complete_hash_and_snapshot_mapping_required(self):
        rows=self.fixture()
        graph=roles._normalize(rows,100,self.bindings)
        with patch.object(roles,'ROUND_SHA256',graph['sha256']):
            report=self.run_rows(rows)
            self.assertEqual(len(report['rounds']),1)
            self.assertFalse(report['rounds'][0]['completion_proven'])
            self.assertEqual(report['rounds'][0]['message_snapshot_binding']['category_variable'],[0,30])
            self.snapshot_mock.return_value['snapshots'][0]['root_role_mapping']={'30':2,'31':1}
            self.assertFalse(self.run_rows(rows)['rounds'])
        self.assertFalse(self.run_rows(rows)['rounds'])

    def test_bijective_roots_preserve_and_alias_collision_changes_hash(self):
        rows=self.fixture();baseline=roles._normalize(rows,100,self.bindings)['sha256']
        for row in rows:
            if row['name']=='SCRIPT_OP_LOAD_VARIABLE': row['operands'][1]['value']+=20
        self.assertEqual(roles._normalize(rows,100,self.bindings)['sha256'],baseline)
        rows[3]['operands'][1]['value']=rows[1]['operands'][1]['value']
        self.assertNotEqual(roles._normalize(rows,100,self.bindings)['sha256'],baseline)

    def test_opaque_call_identity_preserves_bijection_not_merges(self):
        rows=self.fixture();baseline=roles._normalize(rows,100,self.bindings)['sha256']
        calls=[r for r in rows if r['name']=='SCRIPT_OP_CALL']
        calls[1]['operands'][0]['value']=calls[0]['operands'][0]['value']
        calls[1]['successors'][1]['offset']=calls[0]['operands'][0]['value']
        self.assertNotEqual(roles._normalize(rows,100,self.bindings)['sha256'],baseline)

    def test_opaque_external_call_cannot_reenter_round(self):
        rows=self.fixture()
        call=next(r for r in rows if r['name']=='SCRIPT_OP_CALL')
        call['operands'][0]['value']=100
        call['successors'][1]['offset']=100
        with self.assertRaises(ValueError):roles._normalize(rows,100,self.bindings)

    def test_stale_native_dependency_deferred_vm_and_voice_header(self):
        for key in ('VM_UNIT_SHA256','VOICE_HEADER_SHA256'):
            with patch.object(roles,key,'stale'):
                self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')
        active=self.snapshot[0]['active']['actorScriptRandomBelow']
        active['dependency']='stale'
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')
        active.pop('dependency')
        deferred=self.snapshot[0]['active_deferred']['actorScriptStartActionAndWait']
        deferred['dependency']='stale'
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')
        deferred.pop('dependency')
        self.snapshot[1]['context_vm_contract']['current']=1
        self.assertEqual(self.run_rows(self.fixture())['status'],'unresolved')

    def test_opcode_arity_tag_native_and_competing_entries(self):
        for mutate in (
            lambda r:r[0].update(size=1),
            lambda r:r[1]['operands'][0].update(tag=True),
            lambda r:r[1].update(opcode=99),
            lambda r:r[1]['successors'][0].update(offset=9999),
        ):
            rows=self.fixture();mutate(rows)
            self.assertEqual(self.run_rows(rows)['status'],'unresolved')
        rows=self.fixture();rows[6]['native']['address']+=4
        with self.assertRaises(ValueError):roles._normalize(rows,100,self.bindings)
        rows=self.fixture()
        rows.insert(0,dict(offset=1,opcode=28,name='SCRIPT_OP_JUMP',size=6,
                           operands=[dict(tag=2,kind='code_reference',value=rows[1]['offset'])],
                           successors=[dict(kind='branch',offset=rows[1]['offset'])]))
        with self.assertRaises(ValueError):roles._normalize(rows,100,self.bindings)

    def test_missing_required_pin_and_frame_shape(self):
        rows=self.fixture()
        rows[0]['operands'][1]['value']=12
        with self.assertRaises(ValueError):roles._normalize(rows,100,self.bindings)
        rows=self.fixture()
        self.bindings.pop('actorScriptRandomBelow')
        self.assertEqual(self.run_rows(rows)['status'],'unresolved')

    def test_reachable_instruction_budget(self):
        rows=self.fixture();entry=rows[0];rows=[entry]
        offset=entry['offset']+entry['size']
        for _ in range(513):
            rows.append(dict(offset=offset,opcode=24,name='SCRIPT_OP_PUSH_LITERAL',size=6,
                             operands=[dict(tag=0,kind='integer',value=0)],successors=[dict(kind='fallthrough',offset=offset+6)]))
            offset+=6
        rows.append(dict(offset=offset,opcode=30,name='SCRIPT_OP_RETURN',size=1,operands=[],successors=[]))
        with self.assertRaises(ValueError):roles._normalize(rows,100,self.bindings)


if __name__=='__main__':unittest.main()
