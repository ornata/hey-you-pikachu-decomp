"""Authored small fixtures test scoped recognition and fail-closed source pins."""

import copy
import tempfile
import unittest
from contextlib import ExitStack
from pathlib import Path
from unittest.mock import patch

from tools.script_decoder import private_status_snapshots as regions
from tools.script_decoder.companion_snapshots import _edges
from tools.script_decoder.context import opcode_names
from tools.script_decoder.native_frames import fingerprint


class PrivateStatusSnapshotTests(unittest.TestCase):
    def rows(self, selected=11, status=22, start=100):
        specs = [('ENTER_FRAME', [0, 0, 1]), ('PUSH_LITERAL', [0]), ('CALL_NATIVE', [7]),
                 ('STORE_VARIABLE', [0, selected]), ('LOAD_VARIABLE', [0, selected]),
                 ('PUSH_LITERAL', [0]), ('NOT_EQUAL', []), ('JUMP_IF_ZERO', [0]),
                 ('LOAD_VARIABLE', [0, status]), ('PUSH_LITERAL', [16]), ('ADD', []),
                 ('STORE_VARIABLE', [0, status])]
        names = opcode_names('vm')
        rows, offset = [], start
        for name, values in specs:
            tag = 2 if name in ('CALL_NATIVE', 'JUMP_IF_ZERO') else 0
            row = dict(offset=offset, opcode=next(k for k, v in names.items() if v == 'SCRIPT_OP_' + name),
                       name='SCRIPT_OP_' + name, operands=[dict(tag=tag,
                           kind='code_reference' if tag == 2 else 'integer', value=v) for v in values],
                       size=1 + 5 * len(values))
            if name == 'CALL_NATIVE':
                row.update(native_selector=7, native=dict(functions=['authoredNative'],
                           address=0x80100000, inline_operands=[]))
            row['successors'] = _edges(row, values)
            rows.append(row)
            offset += row['size']
        rows[7]['operands'][0]['value'] = offset
        rows[7]['successors'] = _edges(rows[7], [offset])
        return rows

    def source_fixture(self, stack):
        directory = stack.enter_context(tempfile.TemporaryDirectory())
        root = Path(directory)
        sources = {
            'src/native.c': 'int authoredNative(void *actor) { return 0; }',
            'src/dependency.c': 'int authoredDependency(void) { return 1; }',
            'src/ovl/script_vm.c': 'int authoredVm(void) { return 0; }',

        }
        for file, source in sources.items():
            path = root / file
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(source)
        review = dict(arguments=1, dependencies=[dict(kind='function', file='src/dependency.c',
                      function='authoredDependency', sha256=fingerprint(sources['src/dependency.c']))])
        binding = dict(address=0x80100000, file='src/native.c', sha256=fingerprint(sources['src/native.c']),
                       review_sha256=regions._digest(review), arguments=1)
        for name, value in {
            'NATIVE_BINDINGS': {'authoredNative': binding},
            'REQUIRED_NATIVE_NAMES': frozenset({'authoredNative'}),
            'VM_UNIT_SHA256': fingerprint(sources['src/ovl/script_vm.c']),
        }.items():
            stack.enter_context(patch.object(regions, name, value))
        snapshot = ({'active': {'authoredNative': review}}, {'context_vm_contract': {'current': True}})
        return root, snapshot

    def canonical_fixture(self, stack, rows):
        root, snapshot = self.source_fixture(stack)
        # Tiny authored function tests the generic gate without shipping retail bytes.
        ret = dict(offset=rows[-1]['offset'] + rows[-1]['size'], opcode=27,
                   name='SCRIPT_OP_RETURN', operands=[], size=1, successors=[])
        from tools.script_decoder.context import opcode_names
        ret['opcode'] = next(k for k,v in opcode_names('vm').items() if v == ret['name'])
        rows.append(ret)
        rows[7]['operands'][0]['value'] = ret['offset']
        rows[7]['successors'] = _edges(rows[7], [ret['offset']])
        normalized = regions._normalize_prefix(rows, rows, native_bindings=regions.NATIVE_BINDINGS)
        pattern = dict(instructions=len(rows), bytes=ret['offset'] + 1 - rows[0]['offset'],
                       frame=[0,0,1], sha256=normalized['sha256'], root_roles=2)
        stack.enter_context(patch.object(regions, 'PATTERNS', {'fishing_timer':pattern}))
        return root, snapshot

    def test_short_authored_fixture_does_not_match_production(self):
        with ExitStack() as stack:
            root, snapshot = self.source_fixture(stack)
            self.assertEqual(regions.private_status_snapshots(self.rows(), root=root, snapshot=snapshot)['snapshots'], [])

    def test_complete_function_root_roles_and_return(self):
        with ExitStack() as stack:
            rows = self.rows()
            root,snapshot = self.canonical_fixture(stack, rows)
            result = regions.private_status_snapshots(rows, root=root, snapshot=snapshot)
            self.assertEqual(len(result['snapshots']),1)
            record=result['snapshots'][0]
            self.assertEqual(record['return_value'],dict(tag='integer',value=0))
            self.assertEqual(record['predicates']['deadline_variable'],[0,11])
            self.assertEqual(record['predicates']['status_word_variable'],[0,22])
            self.assertEqual([x['value'] for x in record['predicates']['additions']], [1<<24,1<<25,1<<26])
            self.assertIn('not OR',result['scope'])
            self.assertNotIn('flags',result)
            renamed=copy.deepcopy(rows)
            for row in renamed:
                if row['name'] in ('SCRIPT_OP_LOAD_VARIABLE','SCRIPT_OP_STORE_VARIABLE') and row['operands'][0]['value']==0:
                    row['operands'][1]['value']={11:71,22:92}[row['operands'][1]['value']]
            self.assertEqual(len(regions.private_status_snapshots(renamed,root=root,snapshot=snapshot)['snapshots']),1)
            changed=copy.deepcopy(rows)
            changed[-1]['name']='SCRIPT_OP_FINISH'
            self.assertEqual(regions.private_status_snapshots(changed,root=root,snapshot=snapshot)['snapshots'],[])

    def test_boundary_frame_and_native_staleness(self):
        with ExitStack() as stack:
            rows=self.rows(); root,snapshot=self.canonical_fixture(stack,rows)
            for mutate in [lambda x:x[0]['operands'][1].update(value=1),
                           lambda x:x[2]['native'].update(address=0x80100004),
                           lambda x:x[4]['operands'][1].update(value=22)]:
                changed=copy.deepcopy(rows);mutate(changed)
                self.assertEqual(regions.private_status_snapshots(changed,root=root,snapshot=snapshot)['snapshots'],[])
            self.assertEqual(regions.private_status_snapshots(rows[:-1],root=root,snapshot=snapshot)['snapshots'],[])

    def test_external_entry_and_operand_branch_rejected(self):
        with ExitStack() as stack:
            rows=self.rows();root,snapshot=self.canonical_fixture(stack,rows)
            external=dict(offset=0,opcode=26,name='SCRIPT_OP_CALL',size=6,
                          operands=[dict(tag=2,kind='code_reference',value=rows[4]['offset'])])
            external['successors']=_edges(external,[rows[4]['offset']])
            self.assertEqual(regions.private_status_snapshots([external]+rows,root=root,snapshot=snapshot)['snapshots'],[])
            changed=copy.deepcopy(rows);changed[7]['operands'][0]['value']=rows[8]['offset']+1
            changed[7]['successors']=_edges(changed[7],[rows[8]['offset']+1])
            self.assertEqual(regions.private_status_snapshots(changed,root=root,snapshot=snapshot)['snapshots'],[])

    def test_cached_snapshot_rechecks_body_dependency_vm(self):
        for file in ['src/native.c','src/dependency.c','src/ovl/script_vm.c']:
            with self.subTest(file=file),ExitStack() as stack:
                rows=self.rows();root,snapshot=self.canonical_fixture(stack,rows)
                self.assertEqual(regions._source_bindings(root,snapshot)['status'],'current')
                path=root/file;path.write_text(path.read_text().replace('return 0;','return 9;').replace('return 1;','return 9;'))
                self.assertEqual(regions.private_status_snapshots(rows,root=root,snapshot=snapshot)['status'],'unresolved')

    def test_review_stale_and_coast_limits_explicit(self):
        with ExitStack() as stack:
            root,snapshot=self.source_fixture(stack)
            changed=copy.deepcopy(snapshot);changed[0]['active']['authoredNative']['arguments']=2
            self.assertEqual(regions._source_bindings(root,changed)['status'],'unresolved')
        coast=regions._predicates('coast_selected_items',{0:3,1:110})
        self.assertIn('-1',coast['unavailable_distance'])
        self.assertIn('preserves',coast['list_scan']['effects'])
        self.assertEqual(coast['first_search']['state_filter'],6)
        self.assertEqual(coast['second_search']['kind_bounds'],[77,77])

if __name__ == '__main__':
    unittest.main()
