"""Authored whole-CFG fixtures expose the conflicting noun-group guard."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder import item_noun_helpers as helpers


class ItemNounHelperTests(unittest.TestCase):
    def fixture(self, start=1000, category_slot=4, value_slot=5):
        template, labels, _ = helpers._template()
        spellings = helpers.opcode_names('vm')
        rows = []
        for spec in template:
            name = spec['name']
            offset = start + spec['relative_offset']
            operands = []
            for wanted in spec['args']:
                if wanted == 'type_slot':
                    value = category_slot
                elif wanted == 'value_slot':
                    value = value_slot
                elif wanted == helpers.NATIVE_NAME:
                    value = 89
                elif isinstance(wanted, str):
                    value = start + labels[wanted]
                else:
                    value = wanted
                tag = 2 if name in ('JUMP', 'JUMP_IF_ZERO', 'CALL_NATIVE') else 1 if spec['float_literal'] else 0
                operands.append(dict(tag=tag, kind='code_reference' if tag == 2 else 'float' if tag == 1 else 'integer', value=value))
            following = offset + spec['size']
            if name == 'RETURN':
                edges = []
            elif name == 'JUMP':
                edges = [dict(kind='branch', offset=operands[0]['value'])]
            elif name == 'JUMP_IF_ZERO':
                edges = [dict(kind='fallthrough', offset=following), dict(kind='branch', offset=operands[0]['value'])]
            else:
                edges = [dict(kind='resume_or_fallthrough' if name == 'CALL_NATIVE' else 'fallthrough', offset=following)]
            row = dict(offset=offset, opcode=next(k for k, v in spellings.items() if v == 'SCRIPT_OP_' + name),
                       name='SCRIPT_OP_' + name, size=spec['size'], operands=operands, successors=edges)
            if name == 'CALL_NATIVE':
                row['native'] = dict(functions=[helpers.NATIVE_NAME], inline_operands=[])
            rows.append(row)
        return rows

    def test_whole_definition_frame_and_conflict_not_translation(self):
        rows = self.fixture()
        result = helpers.recognize_item_noun_helpers(rows)[0]
        self.assertEqual(result['definition_bytes'], 791)
        self.assertEqual(result['instruction_count'], 116)
        self.assertEqual(result['instruction_evidence'], rows)
        self.assertEqual(result['frame_contract'], dict(arguments=1, total_local_cells=2, frame_index=1, additional_local_cells=1, local_count_includes_arguments=True))
        self.assertEqual(len(result['authored_group_arms']), 7)
        self.assertTrue(all(not arm['feasible_under_stable_signed_integer_guard'] for arm in result['authored_group_arms']))
        self.assertEqual(result['root_value_variable'], [0, 5])
        self.assertIn('only values1..164', result['guard_conflict'])

    def test_root_coordinates_parameterized_but_must_remain_coherent(self):
        self.assertEqual(len(helpers.recognize_item_noun_helpers(self.fixture(category_slot=100, value_slot=101))), 1)
        self.assertFalse(helpers.recognize_item_noun_helpers(self.fixture(category_slot=5, value_slot=5)))
        rows = self.fixture()
        rows[6]['operands'][1]['value'] += 1
        self.assertFalse(helpers.recognize_item_noun_helpers(rows))
        self.assertFalse(helpers.recognize_item_noun_helpers(self.fixture(value_slot=0x100000005)))

    def test_entire_cfg_and_exclusive_entries_required(self):
        rows = self.fixture()
        self.assertFalse(helpers.recognize_item_noun_helpers(rows[:-1]))
        rows[-1]['successors'] = [dict(kind='fallthrough', offset=rows[-1]['offset'] + 1)]
        self.assertFalse(helpers.recognize_item_noun_helpers(rows))
        rows = self.fixture()
        rows[14]['successors'][1]['offset'] += 1
        self.assertFalse(helpers.recognize_item_noun_helpers(rows))
        rows = self.fixture()
        extra = dict(offset=0, successors=[dict(kind='branch', offset=rows[20]['offset'])])
        self.assertFalse(helpers.recognize_item_noun_helpers([extra] + rows))
        self.assertFalse(helpers.recognize_item_noun_helpers(rows + [rows[0]]))

    def test_guard_native_float_tag_and_full_width_tampering_rejected(self):
        for index, value in ((4, 7), (11, 180), (16, 165)):
            rows = self.fixture()
            rows[index]['operands'][0]['value'] = value
            self.assertFalse(helpers.recognize_item_noun_helpers(rows))
        for change in ('ambiguous', 'inline', 'opcode', 'floatbits'):
            rows = self.fixture()
            native = next(r for r in rows if r['name'] == 'SCRIPT_OP_CALL_NATIVE')
            if change == 'ambiguous':
                native['native']['functions'].append('otherNative')
            elif change == 'inline':
                native['native']['inline_operands'].append('count')
            elif change == 'opcode':
                native['opcode'] = True
            else:
                floating = next(r for r in rows if r['operands'] and r['operands'][0]['tag'] == 1)
                floating['operands'][0]['payload_hex'] = '00000000'
            self.assertFalse(helpers.recognize_item_noun_helpers(rows))

    def test_fresh_source_pins_and_header_gate(self):
        review = dict(arguments=6, evidence='synthetic reviewed native')
        snapshot = (dict(active={helpers.NATIVE_NAME: review}), dict(context_vm_contract=dict(current=True)))
        with patch.object(helpers, 'NATIVE_REVIEW_SHA256', helpers._digest(review)), patch.object(helpers, 'item_noun_group_names', return_value={}):
            self.assertEqual(len(helpers.item_noun_helper_roles(self.fixture(), snapshot=snapshot)), 1)
            stale = copy.deepcopy(snapshot)
            stale[0]['active'][helpers.NATIVE_NAME]['arguments'] = True
            self.assertFalse(helpers.item_noun_helper_roles(self.fixture(), snapshot=stale))
            stale = copy.deepcopy(snapshot)
            stale[1]['context_vm_contract']['current'] = False
            self.assertFalse(helpers.item_noun_helper_roles(self.fixture(), snapshot=stale))
            with patch.object(helpers, 'item_noun_group_names', side_effect=ValueError('stale enum')):
                self.assertFalse(helpers.item_noun_helper_roles(self.fixture(), snapshot=snapshot))


if __name__ == '__main__':
    unittest.main()
