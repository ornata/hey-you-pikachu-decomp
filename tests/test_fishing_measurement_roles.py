"""Authored synthetic phases test recognition without game program payloads."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder import fishing_measurement_roles as roles


class FishingMeasurementRolesTests(unittest.TestCase):
    def fixture(self):
        rows = []
        def add(name, values=(), native=None):
            offset = sum(r['size'] for r in rows)
            tag = 2 if native else 0
            operands = [dict(tag=tag, kind='code_reference' if native else 'integer', value=v) for v in values]
            row = dict(offset=offset, name='SCRIPT_OP_' + name, size=1 + 5 * len(values), operands=operands,
                       successors=[dict(kind='resume_or_fallthrough' if native else 'fallthrough', offset=offset + 1 + 5 * len(values))])
            if native:
                row['native'] = dict(functions=[native])
            rows.append(row)
        add('ENTER_FRAME', [0, 68, 0])
        add('CALL_NATIVE', [185], 'actorScriptPushSelfHandle')
        add('STORE_VARIABLE', [0, 54])
        add('ENTER_FRAME', [0, 15, 1])
        add('LOAD_VARIABLE', [0, 54])
        add('CALL_NATIVE', [102], 'actorScriptGetActorKind')
        add('STORE_VARIABLE', [1, 9])
        add('LOAD_VARIABLE', [0, 54])
        add('CALL_NATIVE', [104], 'actorScriptGetBoundingRadius')
        add('PUSH_LITERAL', [50])
        add('MULTIPLY')
        add('STORE_VARIABLE', [1, 1])
        add('LOAD_VARIABLE', [1, 1])
        add('PUSH_LITERAL', [3])
        add('DIVIDE')
        add('CALL_NATIVE', [7], 'actorScriptPassThroughInteger')
        add('STORE_VARIABLE', [1, 1])
        add('LOAD_VARIABLE', [0, 55])
        add('LOAD_VARIABLE', [1, 9])
        add('LOAD_VARIABLE', [1, 1])
        add('PUSH_LITERAL', [14])
        add('PUSH_LITERAL', [0])
        add('PUSH_LITERAL', [0])
        add('CALL_NATIVE', [181], 'actorScriptPostMessage')
        return rows

    def test_correspondence_never_claims_formula_or_dominance(self):
        report = roles.recognize_fishing_measurement_roles(self.fixture())
        self.assertEqual(len(report['phases']), 5)
        self.assertEqual(len(report['correspondences']), 1)
        self.assertFalse(report['whole_formula_proven'])
        self.assertFalse(report['correspondences'][0]['whole_formula_proven'])
        self.assertTrue(all(not p['reaching_definition_proven'] for p in report['phases']))
        self.assertEqual(len(report['correspondences'][0]['scalar_write_sites_in_authored_definition']), 3)

    def test_changed_literal_and_full_width_slot_do_not_match(self):
        rows = self.fixture()
        rows[9]['operands'][0]['value'] = 49
        self.assertNotIn('scaled_bounding_radius_float_capture', [p['role'] for p in roles.recognize_fishing_measurement_roles(rows)['phases']])
        rows = self.fixture()
        rows[16]['operands'][1]['value'] = 0x100000001
        self.assertNotIn('measurement_integer_conversion', [p['role'] for p in roles.recognize_fishing_measurement_roles(rows)['phases']])

    def test_competing_entry_and_tampered_edges_rejected(self):
        rows = self.fixture()
        rows[1]['successors'].append(dict(kind='branch', offset=rows[10]['offset']))
        report = roles.recognize_fishing_measurement_roles(rows)
        self.assertNotIn('scaled_bounding_radius_float_capture', [p['role'] for p in report['phases']])
        rows = self.fixture()
        rows[14]['successors'][0]['kind'] = 'call'
        self.assertNotIn('measurement_integer_conversion', [p['role'] for p in roles.recognize_fishing_measurement_roles(rows)['phases']])

    def test_other_writers_remain_evidence_not_provenance(self):
        rows = self.fixture()
        extra = copy.deepcopy(rows[-1])
        extra.update(offset=rows[-1]['offset'] + rows[-1]['size'], name='SCRIPT_OP_STORE_ELEMENT', size=11,
                     operands=[dict(tag=0, kind='integer', value=1), dict(tag=0, kind='integer', value=0x100000001)], successors=[])
        rows.append(extra)
        report = roles.recognize_fishing_measurement_roles(rows)
        self.assertEqual(report['correspondences'][0]['full_program_indexed_write_candidates'][0]['operands'][1]['value'], 0x100000001)
        self.assertFalse(report['correspondences'][0]['whole_formula_proven'])

    def test_duplicate_unsorted_and_wrong_tags(self):
        rows = self.fixture()
        self.assertFalse(roles.recognize_fishing_measurement_roles(rows + [rows[0]])['phases'])
        rows[9]['operands'][0]['tag'] = 1
        self.assertNotIn('scaled_bounding_radius_float_capture', [p['role'] for p in roles.recognize_fishing_measurement_roles(rows)['phases']])

    def test_missing_phase_retains_observation_without_formula_correspondence(self):
        for index in (1, 5, 9, 13):
            rows = self.fixture()
            rows[index]['operands'][0]['value'] += 1
            if index in (1, 5):
                rows[index]['native']['functions'] = ['unsupportedNative']
            report = roles.recognize_fishing_measurement_roles(rows)
            self.assertFalse(report['correspondences'])
            self.assertEqual(report['incomplete_message_requests'][0]['status'], 'incomplete_authored_phase_correspondence')
            self.assertTrue(report['incomplete_message_requests'][0]['missing_phases'])
            self.assertNotIn('conditional_formula', report['incomplete_message_requests'][0])

    def test_required_native_binding_keyset_fails_closed(self):
        for names in (set(), set(roles.NATIVE_BINDINGS) - {'actorScriptGetActorKind'}, set(roles.NATIVE_BINDINGS) | {'unexpectedNative'}):
            bindings = {n: roles.NATIVE_BINDINGS.get(n, (0, 'unused')) for n in names}
            with patch.object(roles, 'NATIVE_BINDINGS', bindings), patch.object(roles, 'context_snapshot', side_effect=AssertionError('Must fail before loading snapshot')):
                self.assertEqual(roles.fishing_measurement_roles(self.fixture())['status'], 'unresolved')

    def test_fresh_source_gate_and_native_tampering(self):
        active = {n: dict(arguments=a, source='authored fixture') for n, (a, _) in roles.NATIVE_BINDINGS.items()}
        bindings = {n: (r['arguments'], roles._digest(r)) for n, r in active.items()}
        snapshot = (dict(active=active), dict(context_vm_contract=dict(current=True)))
        with patch.object(roles, 'NATIVE_BINDINGS', bindings):
            self.assertEqual(roles.fishing_measurement_roles(self.fixture(), snapshot=snapshot)['status'], 'conditional_roles')
            stale = copy.deepcopy(snapshot)
            stale[0]['active']['actorScriptGetBoundingRadius']['source'] = 'changed'
            self.assertEqual(roles.fishing_measurement_roles(self.fixture(), snapshot=stale)['status'], 'unresolved')
            stale = copy.deepcopy(snapshot)
            stale[1]['context_vm_contract']['current'] = False
            self.assertEqual(roles.fishing_measurement_roles(self.fixture(), snapshot=stale)['status'], 'unresolved')


if __name__ == '__main__':
    unittest.main()
