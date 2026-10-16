"""Handwritten synthetic name-selection bytecode shapes, with no ROM fixtures."""
import copy
import unittest
from unittest.mock import patch

from tools.script_decoder.name_selection_roles import (
    NATIVE_BINDINGS, name_selection_roles, recognize_name_selection_roles,
)


def program():
    rows = []
    def add(op, values=(), function=None):
        tag = 2 if op in ('CALL', 'CALL_NATIVE', 'JUMP', 'JUMP_IF_ZERO') else 0
        offset = sum(x['size'] for x in rows)
        row = dict(offset=offset, size=1 + 5 * len(values), name='SCRIPT_OP_' + op,
                   operands=[dict(tag=tag, kind='code_reference' if tag == 2 else 'integer', value=v) for v in values])
        if function:
            row['native'] = dict(functions=[function])
        rows.append(row)
    add('ENTER_FRAME', [1, 2, 1]); add('PUSH_LITERAL', [0]); add('STORE_VARIABLE', [0, 4])
    add('LOAD_VARIABLE', [0, 4]); add('LOAD_VARIABLE', [1, 0]); add('LESS_THAN')
    add('JUMP_IF_ZERO', [0]); add('LOAD_VARIABLE', [0, 4])
    add('CALL_NATIVE', [70], 'actorScriptChoosePokemonName'); add('STORE_ELEMENT', [0, 20])
    add('LOAD_VARIABLE', [0, 4]); add('PUSH_LITERAL', [8]); add('CALL_NATIVE', [71], 'actorScriptRandomBelow')
    add('STORE_ELEMENT', [0, 30]); add('LOAD_VARIABLE', [0, 4]); add('PUSH_LITERAL', [1])
    add('ADD'); add('STORE_VARIABLE', [0, 4]); add('JUMP', [rows[3]['offset']])
    add('PUSH_LITERAL', [0]); add('RETURN')
    rows[6]['operands'][0]['value'] = rows[19]['offset']
    add('ENTER_FRAME', [0, 0, 1]); add('PUSH_LITERAL', [10]); add('RETURN')
    count_entry = rows[21]['offset']
    add('CALL', [count_entry]); add('STORE_VARIABLE', [0, 8]); add('PUSH_LITERAL', [0])
    add('STORE_VARIABLE', [0, 7]); add('LOAD_VARIABLE', [0, 8]); add('CALL', [0])
    add('LOAD_VARIABLE', [0, 7]); add('LOAD_ELEMENT', [0, 20]); add('STORE_VARIABLE', [0, 6])
    add('LOAD_VARIABLE', [0, 6]); add('CALL', [0]); add('RETURN')
    wrapper = sum(x['size'] for x in rows)
    rows[34]['operands'][0]['value'] = wrapper
    add('ENTER_FRAME', [1, 2, 1]); add('LOAD_VARIABLE', [1, 0])
    add('CALL_NATIVE', [72], 'actorScriptGetPokemonNameDictionaryGroup'); add('STORE_VARIABLE', [1, 1])
    for row in rows:
        op = row['name'].removeprefix('SCRIPT_OP_'); nxt = row['offset'] + row['size']
        if op == 'RETURN': edges = []
        elif op == 'JUMP': edges = [dict(kind='branch', offset=row['operands'][0]['value'])]
        elif op == 'JUMP_IF_ZERO': edges = [dict(kind='fallthrough', offset=nxt), dict(kind='branch', offset=row['operands'][0]['value'])]
        elif op == 'CALL': edges = [dict(kind='return_continuation', offset=nxt), dict(kind='call', offset=row['operands'][0]['value'])]
        else: edges = [dict(kind='resume_or_fallthrough' if op == 'CALL_NATIVE' else 'fallthrough', offset=nxt)]
        row['successors'] = edges
    return rows


class NameSelectionRoleTests(unittest.TestCase):
    def test_shifted_coordinates_and_conditional_capture(self):
        result = recognize_name_selection_roles(program())
        self.assertEqual(len(result['initializers']), 1)
        self.assertEqual(result['initializers'][0]['name_array'], [0, 20])
        self.assertEqual(result['initializers'][0]['candidate_domain'], [0, 152])
        self.assertEqual(len(result['setup_recipes']), 1)
        self.assertEqual(result['setup_recipes'][0]['name_slots'], [20, 29])
        self.assertEqual(len(result['consumers']), 1)
        self.assertEqual(result['consumers'][0]['capture_variable'], [0, 6])
        self.assertFalse(result['indexed_producer_provenance_proven'])
        self.assertFalse(result['consumers'][0]['index_bounds_proven'])

    def test_wrong_native_bound_and_boxing_rejected(self):
        for change in ('native', 'bound', 'tag', 'size'):
            rows = program()
            if change == 'native': rows[8]['native']['functions'].append('ambiguous')
            if change == 'bound': rows[11]['operands'][0]['value'] = 9
            if change == 'tag': rows[9]['operands'][0]['tag'] = 2
            if change == 'size': rows[9]['size'] += 1
            self.assertEqual(recognize_name_selection_roles(rows)['initializers'], [])

    def test_competing_entry_to_loop_body_rejected(self):
        rows = program()
        rows[-1]['successors'] = [dict(kind='branch', offset=rows[8]['offset'])]
        self.assertEqual(recognize_name_selection_roles(rows)['initializers'], [])

    def test_changed_array_and_capture_no_match(self):
        rows = program(); rows[31]['operands'][1]['value'] = 21
        self.assertEqual(recognize_name_selection_roles(rows)['consumers'], [])
        rows = program(); rows[33]['operands'][1]['value'] = 9
        self.assertEqual(recognize_name_selection_roles(rows)['consumers'], [])

    def test_duplicate_offsets_rejected(self):
        rows = program(); rows.append(copy.deepcopy(rows[0]))
        self.assertEqual(recognize_name_selection_roles(rows)['initializers'], [])

    def test_snapshot_stale_fails_closed(self):
        for snapshot in [({}, {'context_vm_contract': {'current': False}}),
                         ({'active': {}}, {'context_vm_contract': {'current': True}})]:
            self.assertEqual(name_selection_roles(program(), snapshot=snapshot)['status'], 'unresolved')

    def test_unrelated_constant_ten_is_not_a_name_role(self):
        rows = copy.deepcopy(program()[21:24])
        self.assertEqual(recognize_name_selection_roles(rows)['count_helpers'], [])
        recognized = recognize_name_selection_roles(program())
        self.assertEqual(len(recognized['count_helpers']), 1)
        self.assertEqual(recognized['count_helpers'][0]['entry'],
                         recognized['setup_recipes'][0]['count_helper_entry'])

    def test_missing_required_pin_fails_closed(self):
        for removed in NATIVE_BINDINGS:
            pins = {name: binding for name, binding in NATIVE_BINDINGS.items() if name != removed}
            with patch('tools.script_decoder.name_selection_roles.NATIVE_BINDINGS', pins):
                result = name_selection_roles(program(), snapshot=({}, {}))
                self.assertEqual(result['status'], 'unresolved')
                self.assertIn('required native', result['reason'])

    def test_count_overwrite_and_array_overlap_reject_setup(self):
        rows = program()
        # The count helper result is directly overwritten before LOAD/count CALL/fill.
        rows[27]['operands'][1]['value'] = 8
        self.assertEqual(recognize_name_selection_roles(rows)['setup_recipes'], [])
        for name_base, sample_base, scratch in [(20, 25, 4), (20, 30, 22), (2147483640, 30, 4)]:
            rows = program()
            rows[9]['operands'][1]['value'] = name_base
            rows[13]['operands'][1]['value'] = sample_base
            for index in (2, 3, 7, 10, 14, 17):
                rows[index]['operands'][1]['value'] = scratch
            self.assertEqual(len(recognize_name_selection_roles(rows)['initializers']), 1)
            self.assertEqual(recognize_name_selection_roles(rows)['setup_recipes'], [])

    def test_guard_metadata_requires_real_adjacent_fallthrough(self):
        rows = program()
        guard = [copy.deepcopy(rows[30]), copy.deepcopy(rows[22]),
                 copy.deepcopy(rows[5]), copy.deepcopy(rows[6])]
        guard[1]['operands'][0]['value'] = 3
        guard[2]['name'] = 'SCRIPT_OP_EQUAL'
        guard[3]['operands'][0]['value'] = rows[30]['offset']
        for i, row in enumerate(guard):
            row['offset'] = -100 + i
        rows[24:24] = guard
        mapping = {}; offset = 0
        for row in rows:
            mapping[row['offset']] = offset
            offset += row['size']
        for row in rows:
            old = row['offset']; row['offset'] = mapping[old]
            if row['name'] in ('SCRIPT_OP_CALL', 'SCRIPT_OP_JUMP', 'SCRIPT_OP_JUMP_IF_ZERO'):
                row['operands'][0]['value'] = mapping[row['operands'][0]['value']]
            nxt = row['offset'] + row['size']; op = row['name']
            if op == 'SCRIPT_OP_RETURN': edges = []
            elif op == 'SCRIPT_OP_JUMP': edges = [dict(kind='branch', offset=row['operands'][0]['value'])]
            elif op == 'SCRIPT_OP_JUMP_IF_ZERO': edges = [dict(kind='fallthrough', offset=nxt), dict(kind='branch', offset=row['operands'][0]['value'])]
            elif op == 'SCRIPT_OP_CALL': edges = [dict(kind='return_continuation', offset=nxt), dict(kind='call', offset=row['operands'][0]['value'])]
            else: edges = [dict(kind='resume_or_fallthrough' if op == 'SCRIPT_OP_CALL_NATIVE' else 'fallthrough', offset=nxt)]
            row['successors'] = edges
        recipes = recognize_name_selection_roles(rows)['setup_recipes']
        self.assertEqual(recipes[0]['preceding_guard']['value'], 3)
        rows[27]['successors'][0]['offset'] -= 1
        recipes = recognize_name_selection_roles(rows)['setup_recipes']
        self.assertIsNone(recipes[0]['preceding_guard'])

    def test_exact_review_snapshot_required(self):
        import hashlib
        import json
        active = {name: dict(arguments=binding['arguments'], synthetic=True) for name, binding in NATIVE_BINDINGS.items()}
        pins = {name: dict(arguments=row['arguments'], review_sha256=hashlib.sha256(json.dumps(row,sort_keys=True,separators=(',', ':')).encode()).hexdigest()) for name,row in active.items()}
        snapshot = ({'active': active}, {'context_vm_contract': {'current': True}})
        with patch('tools.script_decoder.name_selection_roles.NATIVE_BINDINGS', pins):
            self.assertEqual(name_selection_roles(program(), snapshot=snapshot)['status'], 'conditional_roles')
            active['actorScriptChoosePokemonName']['synthetic'] = False
            self.assertEqual(name_selection_roles(program(), snapshot=snapshot)['status'], 'unresolved')


if __name__ == '__main__':
    unittest.main()
