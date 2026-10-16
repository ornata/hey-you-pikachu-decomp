"""Authored passage request fixtures; no retail program data."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder import passage_access as roles
from tools.script_decoder.context import opcode_names


class PassageAccessTests(unittest.TestCase):
    def setUp(self):
        self.review = dict(arguments=2)
        self.snapshot = ({'active': {roles.NATIVE_NAME: self.review}},
                         {'context_vm_contract': {'current': True}})
        self.pin = patch.object(roles, 'NATIVE_REVIEW_SHA256', roles._digest(self.review))
        self.pin.start()
        self.addCleanup(self.pin.stop)

    def fixture(self, state=3, symbolic=False):
        names = {v: k for k, v in opcode_names('vm').items()}
        rows = []
        offset = 100
        for name, values in [('PUSH_LITERAL', [0]),
                             ('LOAD_VARIABLE' if symbolic else 'PUSH_LITERAL', [0, 7] if symbolic else [state]),
                             ('CALL_NATIVE', [53])]:
            native = name == 'CALL_NATIVE'
            operands = [dict(tag=2 if native else 0, kind='code_reference' if native else 'integer', value=v) for v in values]
            size = 1 + 5 * len(operands)
            row = dict(offset=offset, opcode=names['SCRIPT_OP_' + name], name='SCRIPT_OP_' + name,
                       size=size, operands=operands,
                       successors=[dict(kind='resume_or_fallthrough' if native else 'fallthrough', offset=offset + size)])
            if native:
                row['native'] = dict(address=roles.NATIVE_ADDRESS, functions=[roles.NATIVE_NAME], inline_operands=[])
            rows.append(row)
            offset += size
        return rows

    def run_rows(self, rows):
        return roles.passage_access_requests(rows, snapshot=self.snapshot)

    def test_literal_labels_unknown_values_and_boxed_result(self):
        for state in (-1, 0, 1, 2, 3, 4, 5, 65539):
            report = self.run_rows(self.fixture(state))
            self.assertEqual(report['status'], 'conditional_requests')
            request = report['requests'][0]
            self.assertEqual(request['arguments']['access_state']['value'], state)
            self.assertEqual(request['access_state_name'], roles.EXPECTED_NAMES.get(state))
            self.assertFalse(request['registry_bounds_proven'])
            self.assertFalse(request['actual_write_proven'])

    def test_symbolic_input_preserved_without_label(self):
        request = self.run_rows(self.fixture(symbolic=True))['requests'][0]
        self.assertEqual(request['arguments']['access_state'], dict(kind='variable', frame=0, slot=7))
        self.assertIsNone(request['access_state_name'])

    def test_stale_native_vm_and_header_fail_closed(self):
        self.snapshot[1]['context_vm_contract']['current'] = 1
        self.assertEqual(self.run_rows(self.fixture())['status'], 'unresolved')
        self.snapshot[1]['context_vm_contract']['current'] = True
        self.review['arguments'] = 3
        self.assertEqual(self.run_rows(self.fixture())['status'], 'unresolved')
        self.review['arguments'] = 2
        with patch.object(roles, 'HEADER_SHA256', 'stale'):
            self.assertEqual(self.run_rows(self.fixture())['status'], 'unresolved')
        with patch.object(roles, 'EXPECTED_NAMES', {0: 'PASSAGE_ACCESS_ALLOWED'}):
            self.assertEqual(self.run_rows(self.fixture())['status'], 'unresolved')

    def test_malformed_opcode_operands_native_and_edges_fail_closed(self):
        for mutate in (
            lambda r: r[0].update(opcode=99),
            lambda r: r[0].update(size=1),
            lambda r: r[0]['operands'][0].update(tag=True),
            lambda r: r[1]['operands'][0].update(value=True),
            lambda r: r[2]['native'].update(address=roles.NATIVE_ADDRESS + 4),
            lambda r: r[2]['native'].update(functions=[roles.NATIVE_NAME, 'alias']),
            lambda r: r[0]['successors'][0].update(kind='branch'),
            lambda r: r[2]['operands'][0].update(tag=0, kind='integer'),
        ):
            rows = self.fixture()
            mutate(rows)
            self.assertEqual(self.run_rows(rows)['status'], 'unresolved')

    def test_competing_entry_preserves_site_but_not_argument_claim(self):
        rows = self.fixture()
        extra = copy.deepcopy(rows[0])
        extra.update(offset=1, opcode=28, name='SCRIPT_OP_JUMP', operands=[dict(tag=2,kind='code_reference',value=rows[1]['offset'])],
                     successors=[dict(kind='branch', offset=rows[1]['offset'])])
        report = self.run_rows([extra] + rows)
        self.assertEqual(len(report['requests']), 1)
        self.assertIsNone(report['requests'][0]['arguments'])

    def test_bound_native_producers_and_stale_or_rebound_rejection(self):
        for name in ('actorScriptPassThroughInteger', 'actorScriptPushSelfHandle'):
            rows = self.fixture()
            native = copy.deepcopy(rows[-1])
            native['native'] = dict(address=roles.PRODUCER_BINDINGS[name]['address'], functions=[name], inline_operands=[])
            if name == 'actorScriptPassThroughInteger':
                native['offset'] = rows[-1]['offset']
                rows[-1]['offset'] += 6
                rows[-1]['successors'][0]['offset'] += 6
                native['successors'] = [dict(kind='resume_or_fallthrough', offset=rows[-1]['offset'])]
                rows.insert(-1, native)
            else:
                native['offset'] = rows[1]['offset']
                native['successors'] = [dict(kind='resume_or_fallthrough', offset=rows[-1]['offset'])]
                rows[1] = native
            active = dict(arguments=roles.PRODUCER_BINDINGS[name]['arguments'])
            self.snapshot[0]['active'][name] = active
            bindings = copy.deepcopy(roles.PRODUCER_BINDINGS)
            bindings[name]['review_sha256'] = roles._digest(active)
            with patch.object(roles, 'PRODUCER_BINDINGS', bindings):
                request = self.run_rows(rows)['requests'][0]
                self.assertIsNotNone(request['arguments'])
                self.assertIsNone(request['access_state_name'])
                active['stale'] = True
                self.assertIsNone(self.run_rows(rows)['requests'][0]['arguments'])
                active.pop('stale')
                native['native']['address'] += 4
                self.assertIsNone(self.run_rows(rows)['requests'][0]['arguments'])
                native['native']['address'] -= 4
                native['native']['functions'].append('alias')
                self.assertIsNone(self.run_rows(rows)['requests'][0]['arguments'])

    def test_missing_required_producer_binding_fails_closed(self):
        bindings = dict(roles.PRODUCER_BINDINGS)
        bindings.pop('actorScriptPassThroughInteger')
        with patch.object(roles, 'PRODUCER_BINDINGS', bindings):
            self.assertEqual(self.run_rows(self.fixture())['status'], 'unresolved')

    def test_finish_terminal_and_invalid_terminal_edges(self):
        rows = self.fixture()
        offset = rows[-1]['offset'] + rows[-1]['size']
        rows.append(dict(offset=offset, opcode=35, name='SCRIPT_OP_FINISH', size=1, operands=[], successors=[]))
        self.assertEqual(len(self.run_rows(rows)['requests']), 1)
        rows[-1]['successors'] = [dict(kind='fallthrough', offset=offset + 1)]
        self.assertEqual(self.run_rows(rows)['status'], 'unresolved')

    def test_floats_preserved_without_truncation_or_enum_label(self):
        for value in (0.5, 3.0):
            rows = self.fixture()
            rows[1]['operands'][0].update(tag=1, kind='float', value=value)
            request = self.run_rows(rows)['requests'][0]
            self.assertEqual(request['arguments']['access_state'], dict(kind='float', value=value))
            self.assertIsNone(request['access_state_name'])
        rows = self.fixture()
        rows[1]['operands'][0].update(tag=99)
        rows[1]['operands'][0].pop('kind')
        self.assertEqual(self.run_rows(rows)['status'], 'unresolved')

    def test_incomplete_cfg_and_invalid_frame_coordinates(self):
        rows = self.fixture(symbolic=True)
        rows[1]['operands'][0]['value'] = 32
        self.assertEqual(self.run_rows(rows)['status'], 'unresolved')
        rows = self.fixture()
        rows[0]['successors'][0]['offset'] = 9999
        self.assertEqual(self.run_rows(rows)['status'], 'unresolved')
        rows = self.fixture()
        rows[2]['native']['address'] = True
        self.assertEqual(self.run_rows(rows)['status'], 'unresolved')

    def test_code_reference_literal_kept_numeric_not_named(self):
        rows = self.fixture()
        rows[1]['operands'][0].update(tag=2,kind='code_reference')
        request = self.run_rows(rows)['requests'][0]
        self.assertEqual(request['arguments']['access_state']['value'], 3)
        self.assertIsNone(request['access_state_name'])


if __name__ == '__main__':
    unittest.main()
