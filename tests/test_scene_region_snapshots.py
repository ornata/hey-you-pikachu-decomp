"""Authored small fixtures test scoped recognition and fail-closed source pins."""

import copy
import tempfile
import unittest
from contextlib import ExitStack
from pathlib import Path
from unittest.mock import patch

from tools.script_decoder import scene_region_snapshots as regions
from tools.script_decoder.companion_snapshots import _edges
from tools.script_decoder.context import opcode_names
from tools.script_decoder.native_frames import fingerprint


class SceneRegionSnapshotTests(unittest.TestCase):
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
            'src/highlight.c': 'void sceneRegionUpdateHighlight(void *object) { }',
            'src/constructor.c': 'void sceneRegionCreate(void *object) { }',
            'include/object_template.h': 'typedef struct ObjectTemplate { int kind; void *child; } ObjectTemplate;',
            'src/dependency.c': 'int authoredDependency(void) { return 1; }',
            'src/ovl/script_vm.c': 'int authoredVm(void) { return 0; }',
            'include/scene_types.h': 'typedef struct SceneRegionState { float width; float depth; '
                'unsigned char highlighted; unsigned char padding[3]; float *points; } SceneRegionState;',
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
            'SUPPORT_BINDING': dict(file='src/highlight.c', function='sceneRegionUpdateHighlight',
                                   sha256=fingerprint(sources['src/highlight.c'])),
            'CONSTRUCTOR_BINDING': dict(file='src/constructor.c', function='sceneRegionCreate', sha256=fingerprint(sources['src/constructor.c'])),
            'TEMPLATE_LAYOUT_SHA256': regions._layout_fingerprint(sources['include/object_template.h'], 'ObjectTemplate'),
            'LAYOUT_SHA256': regions._layout_fingerprint(sources['include/scene_types.h']),
            'VM_UNIT_SHA256': fingerprint(sources['src/ovl/script_vm.c']),
        }.items():
            stack.enter_context(patch.object(regions, name, value))
        snapshot = ({'active': {'authoredNative': review}}, {'context_vm_contract': {'current': True}})
        return root, snapshot

    def canonical_fixture(self, stack, rows):
        root, snapshot = self.source_fixture(stack)
        normalized = regions._normalize_prefix(rows, rows, native_bindings=regions.NATIVE_BINDINGS)
        for name, value in {'PREFIX_INSTRUCTIONS': len(rows),
                            'PREFIX_BYTES': rows[-1]['offset'] + rows[-1]['size'] - rows[0]['offset'],
                            'PREFIX_SHA256': normalized['sha256']}.items():
            stack.enter_context(patch.object(regions, name, value))
        return root, snapshot

    def test_authored_fixture_cannot_match_production_canonical_gate(self):
        with ExitStack() as stack:
            root, snapshot = self.source_fixture(stack)
            result = regions.scene_region_snapshots(self.rows(), snapshot=snapshot, root=root)
            self.assertEqual(result['status'], 'conditional_scene_region_snapshots')
            self.assertEqual(result['snapshots'], [])

    def test_scoped_output_and_bijective_root_roles(self):
        with ExitStack() as stack:
            rows = self.rows()
            root, snapshot = self.canonical_fixture(stack, rows)
            result = regions.scene_region_snapshots(rows, snapshot=snapshot, root=root)
            self.assertEqual(len(result['snapshots']), 1)
            record = result['snapshots'][0]
            self.assertEqual(record['frame_contract'], dict(arguments=0, total_local_cells=0, frame_index=1))
            self.assertEqual(record['selected_handle_variable'], [0, 11])
            self.assertEqual(record['status_word_variable'], [0, 22])
            self.assertEqual(record['conditional_addition']['role'], 'selected_scene_region_highlight')
            self.assertNotIn('flags', result)
            self.assertIn('not bitwise OR', result['scope'])
            renamed = self.rows(71, 92, start=300)
            self.assertEqual(len(regions.scene_region_snapshots(renamed, snapshot=snapshot, root=root)['snapshots']), 1)
            aliased = self.rows(71, 71)
            self.assertEqual(regions.scene_region_snapshots(aliased, snapshot=snapshot, root=root)['snapshots'], [])

    def test_frame_boundary_and_native_address_changes_fail_closed(self):
        with ExitStack() as stack:
            rows = self.rows()
            root, snapshot = self.canonical_fixture(stack, rows)
            self.assertEqual(regions.scene_region_snapshots(rows[:-1], snapshot=snapshot, root=root)['snapshots'], [])
            changed = copy.deepcopy(rows)
            changed[0]['operands'][1]['value'] = 1
            self.assertEqual(regions.scene_region_snapshots(changed, snapshot=snapshot, root=root)['snapshots'], [])
            changed = copy.deepcopy(rows)
            changed[2]['native']['address'] += 4
            self.assertEqual(regions.scene_region_snapshots(changed, snapshot=snapshot, root=root)['snapshots'], [])
            changed = copy.deepcopy(rows)
            changed[2]['native']['inline_operands'] = [dict(value=0)]
            self.assertEqual(regions.scene_region_snapshots(changed, snapshot=snapshot, root=root)['snapshots'], [])

    def test_external_interior_entry_and_operand_target_rejected(self):
        with ExitStack() as stack:
            rows = self.rows()
            root, snapshot = self.canonical_fixture(stack, rows)
            external = dict(offset=0, opcode=26, name='SCRIPT_OP_CALL', size=6,
                            operands=[dict(tag=2, kind='code_reference', value=rows[4]['offset'])])
            external['successors'] = _edges(external, [rows[4]['offset']])
            self.assertEqual(regions.scene_region_snapshots([external] + rows, snapshot=snapshot, root=root)['snapshots'], [])
            changed = copy.deepcopy(rows)
            changed[7]['operands'][0]['value'] = rows[8]['offset'] + 1
            changed[7]['successors'] = _edges(changed[7], [rows[8]['offset'] + 1])
            self.assertEqual(regions.scene_region_snapshots(changed, snapshot=snapshot, root=root)['snapshots'], [])

    def test_fresh_body_dependency_and_vm_checks_with_cached_snapshot(self):
        for file in ['src/native.c', 'src/highlight.c', 'src/constructor.c', 'src/dependency.c', 'src/ovl/script_vm.c']:
            with self.subTest(file=file), ExitStack() as stack:
                root, snapshot = self.canonical_fixture(stack, self.rows())
                self.assertEqual(regions._source_bindings(root, snapshot)['status'], 'current')
                path = root / file
                path.write_text(path.read_text().replace('return 0;', 'return 2;').replace('return 1;', 'return 2;')
                                .replace('{ }', '{ int changed = 1; }'))
                self.assertEqual(regions._source_bindings(root, snapshot)['status'], 'unresolved')
                self.assertEqual(regions.scene_region_snapshots(self.rows(), snapshot=snapshot, root=root)['snapshots'], [])

    def test_layout_code_changes_stale_but_comments_do_not(self):
        with ExitStack() as stack:
            root, snapshot = self.canonical_fixture(stack, self.rows())
            path = root / 'include/scene_types.h'
            original = path.read_text()
            path.write_text('/// Authored documentation.\n' + original.replace('float width;', 'float width; /* extent */'))
            self.assertEqual(regions._source_bindings(root, snapshot)['status'], 'current')
            path.write_text(original.replace('unsigned char highlighted;', 'unsigned short highlighted;'))
            self.assertEqual(regions._source_bindings(root, snapshot)['status'], 'unresolved')

    def test_object_template_layout_staleness(self):
        with ExitStack() as stack:
            root, snapshot = self.canonical_fixture(stack, self.rows())
            path = root / 'include/object_template.h'
            original = path.read_text()
            path.write_text('/* documentation */' + original)
            self.assertEqual(regions._source_bindings(root, snapshot)['status'], 'current')
            path.write_text(original.replace('int kind;', 'short kind;'))
            self.assertEqual(regions._source_bindings(root, snapshot)['status'], 'unresolved')

    def test_stale_review_and_vm_snapshot_are_unresolved(self):
        with ExitStack() as stack:
            root, snapshot = self.canonical_fixture(stack, self.rows())
            changed = copy.deepcopy(snapshot)
            changed[0]['active']['authoredNative']['arguments'] = 2
            self.assertEqual(regions._source_bindings(root, changed)['status'], 'unresolved')
            changed = copy.deepcopy(snapshot)
            changed[1]['context_vm_contract']['current'] = False
            self.assertEqual(regions._source_bindings(root, changed)['status'], 'unresolved')


if __name__ == '__main__':
    unittest.main()
