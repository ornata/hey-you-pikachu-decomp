"""Small authored graph fixtures; no retail bytecode is embedded."""
import copy
import unittest
import tempfile
from pathlib import Path
from contextlib import ExitStack
from tools.script_decoder.native_frames import fingerprint
from tools.script_decoder.companion_snapshots import _digest
from unittest.mock import patch
from tools.script_decoder import tracked_creature_predicates as tracked
from tools.script_decoder.companion_snapshots import _edges
from tools.script_decoder.context import opcode_names


class TrackedCreaturePredicateTests(unittest.TestCase):
    def rows(self, root=12):
        names = opcode_names('vm')
        specs = [('ENTER_FRAME', [0, 0, 1]), ('LOAD_VARIABLE', [0, root]),
                 ('CALL', [100]), ('RETURN', [])]
        helper = [('ENTER_FRAME', [1, 1, 1]), ('LOAD_VARIABLE', [0, root]),
                  ('LOAD_VARIABLE', [0, root + 1]), ('ADD', []), ('RETURN', [])]
        rows = []
        for start, spec in [(0, specs), (100, helper)]:
            offset = start
            for name, values in spec:
                tag = 2 if name == 'CALL' else 0
                row = dict(offset=offset, name='SCRIPT_OP_' + name,
                    opcode=next(k for k,v in names.items() if v == 'SCRIPT_OP_' + name),
                    operands=[dict(tag=tag, kind='code_reference' if tag == 2 else 'integer', value=v) for v in values],
                    size=1+5*len(values))
                row['successors'] = _edges(row, values)
                rows.append(row);offset += row['size']
        return rows

    def test_complete_call_graph_and_shared_root_bijection(self):
        original = tracked._normalize_graph(self.rows(), 0, {})
        renamed = tracked._normalize_graph(self.rows(30), 0, {})
        self.assertEqual(original['sha256'], renamed['sha256'])
        self.assertEqual(len(original['nodes']), 2)
        changed = self.rows();changed[-3]['operands'][1]['value'] = 12
        self.assertNotEqual(tracked._normalize_graph(changed,0,{})['sha256'], original['sha256'])

    def test_missing_callee_and_wrong_cfg_rejected(self):
        rows=self.rows()
        with self.assertRaises(ValueError): tracked._normalize_graph(rows[:4],0,{})
        rows[2]['successors']=[]
        with self.assertRaises(ValueError): tracked._normalize_graph(rows,0,{})

    def test_competing_helper_interior_entry_rejected(self):
        rows=self.rows();rows[2]['operands'][0]['value']=rows[5]['offset']
        rows[2]['successors']=_edges(rows[2],[rows[5]['offset']])
        with self.assertRaises(ValueError): tracked._normalize_graph(rows,0,{})

    def test_callee_nonroot_and_operand_tags_preserved(self):
        rows=self.rows();base=tracked._normalize_graph(rows,0,{})['sha256']
        rows[5]['operands'][0]['value']=1
        self.assertNotEqual(tracked._normalize_graph(rows,0,{})['sha256'],base)
        rows=self.rows();rows[2]['operands'][0]['tag']=0
        with self.assertRaises(ValueError): tracked._normalize_graph(rows,0,{})

    def test_fresh_source_dependencies_and_incomplete_binding_fail_closed(self):
        with tempfile.TemporaryDirectory() as directory, ExitStack() as stack:
            root = Path(directory)
            native = 'int authoredNative(void *a) { return 0; }'
            dep = 'int dependency(void) { return 1; }'
            vm = 'int authoredVm(void) { return 0; }'
            for path, text in [('native.c', native), ('dep.c', dep), ('src/ovl/script_vm.c', vm)]:
                target=root/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(text)
            review=dict(arguments=1,dependencies=[dict(file='dep.c',kind='function',function='dependency',sha256=fingerprint(dep))])
            binding=dict(address=1,file='native.c',sha256=fingerprint(native),arguments=1,review_sha256=_digest(review))
            stack.enter_context(patch.object(tracked,'NATIVE_BINDINGS',{'authoredNative':binding}))
            stack.enter_context(patch.object(tracked,'REQUIRED_NATIVE_NAMES',frozenset({'authoredNative'})))
            stack.enter_context(patch.object(tracked,'VM_UNIT_SHA256',fingerprint(vm)))
            snapshot=({'active':{'authoredNative':review}},{'context_vm_contract':{'current':True}})
            self.assertEqual(tracked._source_bindings(root,snapshot)['status'],'current')
            (root/'dep.c').write_text(dep.replace('return 1','return 2'))
            self.assertEqual(tracked._source_bindings(root,snapshot)['status'],'unresolved')
            with patch.object(tracked,'NATIVE_BINDINGS',{}):
                self.assertEqual(tracked._source_bindings(root,snapshot)['status'],'unresolved')

    def test_invocation_local_index_and_ordering_fail_closed(self):
        rows = self.rows()
        prepared = tracked._graph_index(rows)
        self.assertEqual(tracked._normalize_graph(rows, 0, {}),
                         tracked._normalize_graph(rows, 0, {}, _prepared=prepared))
        with patch.object(tracked, '_source_bindings', return_value=dict(status='current')):
            with patch.object(tracked, '_graph_index', wraps=tracked._graph_index) as index:
                result = tracked.tracked_creature_predicates(rows)
                self.assertEqual(index.call_count, 1)
                self.assertEqual(result['predicates'], [])
            self.assertEqual(tracked.tracked_creature_predicates(list(reversed(rows)))['status'], 'unresolved')
            self.assertEqual(tracked.tracked_creature_predicates(rows + [rows[-1]])['status'], 'unresolved')

    def test_production_gate_rejects_authored_graph(self):
        with patch.object(tracked,'_source_bindings',return_value=dict(status='current')):
            self.assertEqual(tracked.tracked_creature_predicates(self.rows())['predicates'],[])
        with patch.object(tracked,'_source_bindings',return_value=dict(status='unresolved')):
            self.assertEqual(tracked.tracked_creature_predicates(self.rows())['status'],'unresolved')


if __name__ == '__main__':
    unittest.main()
