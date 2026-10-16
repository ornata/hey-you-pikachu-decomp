"""Synthetic object-kind consumers retain namespace and adjacency boundaries."""

import copy
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.script_decoder import object_query_roles as roles


class ObjectQueryRoleTests(unittest.TestCase):
    def fixture(self, capture=False, literal=134, comparison='EQUAL'):
        specs = [('CALL_NATIVE', [103])]
        if capture:
            specs += [('STORE_VARIABLE', [1, 7]), ('LOAD_VARIABLE', [1, 7])]
        specs += [('PUSH_LITERAL', [literal]), (comparison, [])]
        rows = []
        offset = 100
        for name, values in specs:
            native = name == 'CALL_NATIVE'
            size = 1 + len(values) * 5
            opcode = next(code for code, spelling in roles.opcode_names('vm').items() if spelling == 'SCRIPT_OP_' + name)
            row = dict(offset=offset, opcode=opcode, name='SCRIPT_OP_' + name, size=size,
                       operands=[dict(tag=2 if native else 0, kind='code_reference' if native else 'integer', value=v) for v in values],
                       successors=[dict(kind='resume_or_fallthrough' if native else 'fallthrough', offset=offset + size)])
            if native:
                row['native'] = dict(functions=[roles.NATIVE_NAME])
            rows.append(row)
            offset += size
        return rows

    def test_direct_and_captured_comparisons(self):
        for capture in (False, True):
            for comparison in ('EQUAL', 'NOT_EQUAL'):
                report = roles.recognize_object_query_roles(self.fixture(capture, comparison=comparison), symbolic_names={134: ['existing_name']})
                self.assertEqual(len(report['captures']), int(capture))
                self.assertEqual(len(report['comparisons']), 1)
                observed = report['comparisons'][0]
                self.assertEqual(observed['via_adjacent_capture'], capture)
                self.assertEqual(observed['literal'], 134)
                self.assertEqual(observed['symbolic_names'], ['existing_name'])
                self.assertFalse(observed['result_proven'])
                if capture:
                    self.assertEqual(report['captures'][0]['role'], 'runtime_object_kind_u16_widened_integer')

    def test_raw_integer_width_not_u16_truncation(self):
        for value in (-1, 65536 + 134):
            report = roles.recognize_object_query_roles(self.fixture(literal=value), symbolic_names={134: ['item']})
            self.assertEqual(report['comparisons'][0]['literal'], value)
            self.assertFalse(report['comparisons'][0]['symbolic_names'])
        self.assertFalse(roles.recognize_object_query_roles(self.fixture(literal=0x100000086))['comparisons'])

    def test_malformed_tags_sizes_arities_and_native_aliases_fail_closed(self):
        for index, field, value in ((0, 'size', 11), (1, 'size', 5), (2, 'size', 6)):
            rows = self.fixture()
            rows[index][field] = value
            self.assertFalse(roles.recognize_object_query_roles(rows)['comparisons'])
        for tag in (1, 2, True):
            rows = self.fixture()
            rows[1]['operands'][0]['tag'] = tag
            self.assertFalse(roles.recognize_object_query_roles(rows)['comparisons'])
        rows = self.fixture()
        rows[0]['native']['functions'].append('alias')
        self.assertFalse(roles.recognize_object_query_roles(rows)['comparisons'])
        rows = self.fixture()
        rows[2]['operands'] = [dict(tag=0, kind='integer', value=0)]
        self.assertFalse(roles.recognize_object_query_roles(rows)['comparisons'])

    def test_nonadjacent_different_capture_and_competing_entries_rejected(self):
        rows = self.fixture(True)
        rows[2]['operands'][1]['value'] = 8
        report = roles.recognize_object_query_roles(rows)
        self.assertEqual(len(report['captures']), 1)
        self.assertFalse(report['comparisons'])
        rows = self.fixture(True)
        rows[2]['name'] = 'SCRIPT_OP_DISCARD'
        self.assertFalse(roles.recognize_object_query_roles(rows)['comparisons'])
        rows = self.fixture()
        extra = dict(offset=0, name='SCRIPT_OP_JUMP', size=6, operands=[], successors=[dict(kind='branch', offset=rows[1]['offset'])])
        self.assertFalse(roles.recognize_object_query_roles([extra] + rows)['comparisons'])
        rows = self.fixture()
        rows[0]['successors'][0]['kind'] = 'fallthrough'
        self.assertFalse(roles.recognize_object_query_roles(rows)['comparisons'])

    def test_duplicate_order_and_slot_width(self):
        rows = self.fixture(True)
        rows[1]['operands'][1]['value'] = 0x100000007
        rows[2]['operands'][1]['value'] = 0x100000007
        report = roles.recognize_object_query_roles(rows)
        self.assertFalse(report['captures'])
        self.assertFalse(report['comparisons'])
        rows = self.fixture()
        self.assertFalse(roles.recognize_object_query_roles(rows + [rows[0]])['comparisons'])
        rows[1]['operands'][0]['value'] = True
        self.assertFalse(roles.recognize_object_query_roles(rows)['comparisons'])

    def test_opcode_spelling_and_required_header_set(self):
        rows = self.fixture()
        rows[2]['opcode'] = rows[0]['opcode']
        self.assertFalse(roles.recognize_object_query_roles(rows)['comparisons'])
        rows = self.fixture()
        rows[2]['opcode'] = True
        self.assertFalse(roles.recognize_object_query_roles(rows)['comparisons'])
        for paths in (set(), {'include/object_types.h'}, set(roles.HEADER_BINDINGS) | {'extra.h'}):
            bindings = {path: roles.HEADER_BINDINGS.get(path, ('EXTRA_', 'unused')) for path in paths}
            with patch.object(roles, 'HEADER_BINDINGS', bindings), patch.object(roles, 'context_snapshot', side_effect=AssertionError('Must fail before snapshot')):
                self.assertEqual(roles.object_query_roles(self.fixture())['status'], 'unresolved')

    def predicate_fixture(self, comparison='NOT_EQUAL'):
        specs = [('LOAD_VARIABLE', [1, 3], None), ('CALL_NATIVE', [103], roles.NATIVE_NAME),
                 ('PUSH_LITERAL', [134], None), ('EQUAL', [], None),
                 ('LOAD_VARIABLE', [1, 3], None), ('CALL_NATIVE', [200], roles.ITEM_STATE_NATIVE),
                 ('PUSH_LITERAL', [0], None), (comparison, [], None),
                 ('LOGICAL_AND', [], None), ('JUMP_IF_ZERO', [1000], None)]
        rows = []
        offset = 100
        names = roles.opcode_names('vm')
        for name, values, native in specs:
            tag = 2 if name in ('CALL_NATIVE', 'JUMP_IF_ZERO') else 0
            size = 1 + len(values) * 5
            edges = [dict(kind='resume_or_fallthrough' if native else 'fallthrough', offset=offset + size)]
            if name == 'JUMP_IF_ZERO':
                edges.append(dict(kind='branch', offset=values[0]))
            row = dict(offset=offset, opcode=next(k for k, v in names.items() if v == 'SCRIPT_OP_' + name),
                       name='SCRIPT_OP_' + name, size=size,
                       operands=[dict(tag=tag, kind='code_reference' if tag == 2 else 'integer', value=v) for v in values], successors=edges)
            if native:
                row['native'] = dict(functions=[native])
            rows.append(row)
            offset += size
        return rows

    def test_eager_item_predicates_have_exact_evidence_and_branch(self):
        for comparison, role in (('EQUAL', 'available_item_with_world_ownership'), ('NOT_EQUAL', 'available_item_with_nonzero_ownership')):
            rows = self.predicate_fixture(comparison)
            report = roles.recognize_object_query_roles(rows)
            self.assertEqual(len(report['item_ownership_predicates']), 1)
            predicate = report['item_ownership_predicates'][0]
            self.assertEqual(predicate['role'], role)
            self.assertEqual(predicate['instruction_evidence'], rows)
            self.assertEqual(predicate['branch_target'], 1000)
            self.assertTrue(predicate['both_queries_evaluated'])
            self.assertFalse(predicate['short_circuit'])
            self.assertFalse(predicate['actual_predicate_result_proven'])

    def test_item_predicate_coordinates_literals_control_and_width_fail_closed(self):
        for index, operand, value in ((4, 1, 4), (4, 1, 0x100000003), (2, 0, 0x100000086), (6, 0, 1), (9, 0, 0x1000003e8)):
            rows = self.predicate_fixture()
            rows[index]['operands'][operand]['value'] = value
            self.assertFalse(roles.recognize_object_query_roles(rows)['item_ownership_predicates'])
        for index in (5, 7, 8, 9):
            rows = self.predicate_fixture()
            rows[index]['size'] += 5
            self.assertFalse(roles.recognize_object_query_roles(rows)['item_ownership_predicates'])
        rows = self.predicate_fixture()
        rows[7]['name'] = 'SCRIPT_OP_GREATER_THAN'
        self.assertFalse(roles.recognize_object_query_roles(rows)['item_ownership_predicates'])
        rows = self.predicate_fixture()
        extra = dict(offset=0, successors=[dict(kind='branch', offset=rows[5]['offset'])])
        self.assertFalse(roles.recognize_object_query_roles([extra] + rows)['item_ownership_predicates'])
        rows = self.predicate_fixture()
        rows[9]['successors'][1]['offset'] = 1001
        self.assertFalse(roles.recognize_object_query_roles(rows)['item_ownership_predicates'])

    def test_stale_item_evidence_leaves_raw_kind_roles_current(self):
        snapshot = self.snapshot()
        item_review = dict(arguments=1, evidence='synthetic item-state review')
        snapshot[0]['active'][roles.ITEM_STATE_NATIVE] = item_review
        with patch.object(roles, 'NATIVE_REVIEW_SHA256', roles._digest(snapshot[0]['active'][roles.NATIVE_NAME])), patch.object(roles, 'ITEM_STATE_REVIEW_SHA256', roles._digest(item_review)):
            current = roles.object_query_roles(self.predicate_fixture(), snapshot=snapshot)
            self.assertEqual(current['item_ownership_predicates_status'], 'conditional_roles')
            self.assertEqual(len(current['item_ownership_predicates']), 1)
            changed = copy.deepcopy(snapshot)
            changed[0]['active'][roles.ITEM_STATE_NATIVE]['arguments'] = 2
            stale = roles.object_query_roles(self.predicate_fixture(), snapshot=changed)
            self.assertEqual(stale['status'], 'conditional_roles')
            self.assertEqual(len(stale['comparisons']), 1)
            self.assertEqual(stale['item_ownership_predicates_status'], 'unresolved')
            self.assertFalse(stale['item_ownership_predicates'])
            with patch.object(roles, 'ITEM_TYPES_SHA256', 'stale'):
                stale = roles.object_query_roles(self.predicate_fixture(), snapshot=snapshot)
                self.assertEqual(stale['status'], 'conditional_roles')
                self.assertEqual(len(stale['comparisons']), 1)
                self.assertEqual(stale['item_ownership_predicates_status'], 'unresolved')

    def snapshot(self):
        review = dict(arguments=1, evidence='synthetic native contract')
        return (dict(active={roles.NATIVE_NAME: review}), dict(context_vm_contract=dict(current=True)))

    def test_source_bindings_and_namespace(self):
        snapshot = self.snapshot()
        with patch.object(roles, 'NATIVE_REVIEW_SHA256', roles._digest(snapshot[0]['active'][roles.NATIVE_NAME])):
            for value, expected in ((134, 'RUNTIME_OBJECT_KIND_ITEM'),
                                    (175, 'RUNTIME_OBJECT_KIND_SCENE_REGION'),
                                    (12, 'ACTOR_KIND_POLIWAG')):
                result = roles.object_query_roles(self.fixture(literal=value), snapshot=snapshot)
                self.assertEqual(result['status'], 'conditional_roles')
                self.assertEqual(result['comparisons'][0]['symbolic_names'][0]['name'], expected)
            for value in (0, 38):
                result = roles.object_query_roles(self.fixture(literal=value), snapshot=snapshot)
                self.assertFalse(result['comparisons'][0]['symbolic_names'])
            changed = copy.deepcopy(snapshot)
            changed[0]['active'][roles.NATIVE_NAME]['arguments'] = True
            self.assertEqual(roles.object_query_roles(self.fixture(), snapshot=changed)['status'], 'unresolved')
            changed = copy.deepcopy(snapshot)
            changed[1]['context_vm_contract']['current'] = False
            self.assertEqual(roles.object_query_roles(self.fixture(), snapshot=changed)['status'], 'unresolved')
            with tempfile.TemporaryDirectory() as directory:
                self.assertEqual(roles.object_query_roles(self.fixture(), snapshot=snapshot, root=Path(directory))['status'], 'unresolved')
        self.assertEqual(roles.object_query_roles(self.fixture(), snapshot=snapshot)['status'], 'unresolved')


if __name__ == '__main__':
    unittest.main()
