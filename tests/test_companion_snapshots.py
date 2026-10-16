"""Small authored rows test strict snapshot normalization; no retail prefix is stored."""

import copy
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.script_decoder import companion_snapshots as snapshots
from tools.script_decoder.native_frames import fingerprint


class CompanionSnapshotTests(unittest.TestCase):
    def rows(self, specs, start):
        names = snapshots.opcode_names('vm')
        rows = []
        offset = start
        for name, values in specs:
            tag = 2 if name in ('CALL', 'CALL_NATIVE', 'JUMP', 'JUMP_IF_ZERO') else None
            operands = [dict(tag=1 if type(v) is float else 0 if tag is None else tag,
                             kind='float' if type(v) is float else 'integer' if tag is None else 'code_reference', value=v) for v in values]
            row = dict(offset=offset, opcode=next(k for k,v in names.items() if v == 'SCRIPT_OP_' + name),
                       name='SCRIPT_OP_' + name, operands=operands, size=1+5*len(values))
            if name == 'CALL_NATIVE':
                row.update(native_selector=values[0], native=dict(functions=['authoredNative'], address=0x80100000, inline_operands=[]))
            row['successors'] = snapshots._edges(row, values)
            rows.append(row)
            offset += row['size']
        return rows

    def fixture(self, first=5, second=8):
        prefix = self.rows([('ENTER_FRAME',[0,4,1]), ('LOAD_VARIABLE',[0,first]), ('STORE_VARIABLE',[0,second]),
                            ('LOAD_VARIABLE',[0,first]), ('LOAD_VARIABLE',[1,2]), ('CALL_NATIVE',[7]),
                            ('CALL',[1000]), ('JUMP_IF_ZERO',[0]), ('PUSH_LITERAL',[360.0]),
                            ('ADD',[]), ('STORE_VARIABLE',[0,second])], 100)
        end = prefix[-1]['offset'] + prefix[-1]['size']
        prefix[7]['operands'][0]['value'] = end
        prefix[7]['successors'] = snapshots._edges(prefix[7], [end])
        helper = self.rows([('ENTER_FRAME',[1,1,1]), ('LOAD_VARIABLE',[1,0]), ('PUSH_LITERAL',[0]),
                            ('LESS_THAN',[]), ('JUMP_IF_ZERO',[0]), ('LOAD_VARIABLE',[1,0]),
                            ('NEGATE',[]), ('RETURN',[]), ('LOAD_VARIABLE',[1,0]), ('RETURN',[])], 1000)
        helper[4]['operands'][0]['value'] = helper[8]['offset']
        helper[4]['successors'] = snapshots._edges(helper[4], [helper[8]['offset']])
        return prefix, prefix + helper

    def normalize(self, prefix, all_rows):
        return snapshots._normalize_prefix(prefix, all_rows, native_bindings={'authoredNative':dict(address=0x80100000)})

    def test_bijective_root_renaming_only_and_full_helper_validation(self):
        prefix, rows = self.fixture()
        baseline = self.normalize(prefix, rows)
        renamed, renamed_rows = self.fixture(70,90)
        self.assertEqual(self.normalize(renamed, renamed_rows)['sha256'], baseline['sha256'])
        aliased, aliased_rows = self.fixture(70,70)
        self.assertNotEqual(self.normalize(aliased, aliased_rows)['sha256'], baseline['sha256'])
        self.assertEqual(baseline['root_role_mapping'], {5:0,8:1})
        self.assertEqual(baseline['absolute_helpers'][0]['bytes'], 65)
        changed, changed_rows = self.fixture()
        changed[4]['operands'][1]['value'] = 3
        self.assertNotEqual(self.normalize(changed, changed_rows)['sha256'], baseline['sha256'])

    def test_tags_widths_opcode_and_native_metadata_fail_closed(self):
        for change in ('tag','width','opcode','payload','selector','address','aliases','inline'):
            prefix, rows = self.fixture()
            if change == 'tag':
                prefix[1]['operands'][0]['tag'] = 1
            elif change == 'width':
                prefix[1]['size'] += 5
            elif change == 'opcode':
                prefix[1]['opcode'] = True
            elif change == 'payload':
                prefix[8]['operands'][0]['payload_hex'] = '00000000'
            elif change == 'selector':
                prefix[5]['native_selector'] = 8
            elif change == 'address':
                prefix[5]['native']['address'] += 4
            elif change == 'aliases':
                prefix[5]['native']['functions'].append('alias')
            else:
                prefix[5]['native']['inline_operands'].append('count')
            with self.subTest(change=change), self.assertRaises(ValueError):
                self.normalize(prefix, rows)

    def test_branch_edges_internal_entries_and_actual_abs_callee_required(self):
        prefix, rows = self.fixture()
        with self.assertRaises(ValueError):
            self.normalize(prefix, prefix)
        for change in ('target','edge','interior','abs_body','abs_branch','abs_entry'):
            prefix, rows = self.fixture()
            if change == 'target':
                prefix[7]['operands'][0]['value'] += 1
                prefix[7]['successors'] = snapshots._edges(prefix[7], [prefix[7]['operands'][0]['value']])
            elif change == 'edge':
                prefix[7]['successors'].reverse()
            elif change == 'interior':
                rows += self.rows([('JUMP',[prefix[3]['offset']])],2000)
            elif change == 'abs_body':
                rows[-4]['name'] = 'SCRIPT_OP_ADD'
            elif change == 'abs_branch':
                rows[-6]['operands'][0]['value'] = rows[-1]['offset']
                rows[-6]['successors'] = snapshots._edges(rows[-6], [rows[-1]['offset']])
            else:
                rows += self.rows([('JUMP',[rows[-5]['offset']])],2000)
            with self.subTest(change=change), self.assertRaises(ValueError):
                self.normalize(prefix, rows)

    def test_constants_and_native_selector_binding_consistency(self):
        prefix, rows = self.fixture()
        original = self.normalize(prefix, rows)['sha256']
        prefix[8]['operands'][0]['value'] = 359.0
        self.assertNotEqual(self.normalize(prefix, rows)['sha256'], original)
        prefix, rows = self.fixture()
        # Distinct selector numbers may denote the same reviewed function, but
        # one selector may never resolve to two different identities.
        duplicate = copy.deepcopy(prefix[5])
        duplicate.update(offset=prefix[5]['offset'] + 6)
        duplicate['native']['functions'] = ['otherAuthoredNative']
        duplicate['native']['address'] += 4
        prefix = self.rows([('CALL_NATIVE',[7]), ('CALL_NATIVE',[7])],100)
        prefix[1]['native'] = duplicate['native']
        with self.assertRaises(ValueError):
            snapshots._normalize_prefix(prefix,prefix,native_bindings={'authoredNative':dict(address=0x80100000), 'otherAuthoredNative':dict(address=0x80100004)})

    def test_short_recipe_is_never_a_production_snapshot(self):
        prefix, rows = self.fixture()
        with patch.object(snapshots,'_source_bindings',return_value=dict(status='current')), patch.object(snapshots,'_flag_names',return_value={}):
            report = snapshots.companion_snapshots(rows)
        self.assertFalse(report['snapshots'])
        self.assertEqual(report['status'],'conditional_snapshots')

    def test_stale_expanded_dependencies_vm_and_source_revoke_roles(self):
        body='s32 authoredNative(void) { return 0; }'
        review=dict(arguments=0,dependencies=dict(layout='original'))
        binding=dict(address=0x80100000,file='source.c',function='authoredNative',sha256=fingerprint(body),review_sha256=snapshots._digest(review),arguments=0)
        context=(dict(active={'authoredNative':review}),dict(context_vm_contract=dict(current=True)))
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'source.c').write_text(body)
            with patch.object(snapshots,'NATIVE_BINDINGS',{'authoredNative':binding}), patch.object(snapshots,'REQUIRED_NATIVE_NAMES',frozenset(('authoredNative',))), patch.object(snapshots,'SUPPORT_BINDINGS',{}), patch.object(snapshots,'REQUIRED_SUPPORT_NAMES',frozenset()):
                self.assertEqual(snapshots._source_bindings(root,context)['status'],'current')
                stale=copy.deepcopy(context);stale[0]['active']['authoredNative']['dependencies']['layout']='changed'
                self.assertEqual(snapshots._source_bindings(root,stale)['status'],'unresolved')
                stale=copy.deepcopy(context);stale[1]['context_vm_contract']['current']=False
                self.assertEqual(snapshots._source_bindings(root,stale)['status'],'unresolved')
                (root/'source.c').write_text(body.replace('return 0','return 1'))
                self.assertEqual(snapshots._source_bindings(root,context)['status'],'unresolved')
        with patch.object(snapshots,'NATIVE_BINDINGS',{}):
            self.assertEqual(snapshots._source_bindings(Path('.'),context)['status'],'unresolved')

    def test_flag_header_staleness_and_partial_domains(self):
        names=snapshots._flag_names(snapshots.ROOT)
        self.assertEqual(len(names),15)
        self.assertNotIn(2048,names);self.assertNotIn(4096,names);self.assertNotIn(0x1000000,names)
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'include').mkdir()
            code=(snapshots.ROOT/'include/companion_snapshot_flags.h').read_text()
            (root/'include/companion_snapshot_flags.h').write_text(code.replace('COMPANION_SNAPSHOT_MOVEMENT_COLLISION','COMPANION_SNAPSHOT_CHANGED_COLLISION'))
            with self.assertRaises(ValueError):
                snapshots._flag_names(root)
            with patch.object(snapshots,'_source_bindings',return_value=dict(status='current')):
                report=snapshots.companion_snapshots([],root=root)
            self.assertEqual(report['status'],'unresolved');self.assertFalse(report['flags']);self.assertFalse(report['snapshots'])


if __name__=='__main__':
    unittest.main()
