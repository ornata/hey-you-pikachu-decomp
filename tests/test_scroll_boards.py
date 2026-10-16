"""Short authored scrolling-board fixtures, without original asset payloads."""

import copy
import unittest
from unittest.mock import patch

from tools.script_decoder import scroll_boards as roles
from tools.script_decoder.context import opcode_names


class ScrollBoardTests(unittest.TestCase):
    def setUp(self):
        self.review = dict(arguments=18)
        self.snapshot = ({'active': {roles.NATIVE_NAME: self.review}}, {'context_vm_contract': {'current': True}})
        pin = patch.object(roles, 'NATIVE_REVIEW_SHA256', roles._digest(self.review))
        pin.start()
        self.addCleanup(pin.stop)

    def fixture(self, specs=None):
        specs = specs or [('PUSH_LITERAL', [i]) for i in range(18)]
        specs = specs + [('CALL_NATIVE', [253])]
        opcodes = {v: k for k, v in opcode_names('vm').items()}
        rows = []
        offset = 100
        for name, values in specs:
            native = name == 'CALL_NATIVE'
            operands = [dict(tag=2 if native else 0, kind='code_reference' if native else 'integer', value=v) for v in values]
            size = 1 + 5 * len(operands)
            row = dict(offset=offset, opcode=opcodes['SCRIPT_OP_' + name], name='SCRIPT_OP_' + name,
                       size=size, operands=operands, successors=[dict(kind='resume_or_fallthrough' if native else 'fallthrough', offset=offset + size)])
            if native:
                row['native'] = dict(address=roles.NATIVE_ADDRESS, functions=[roles.NATIVE_NAME], inline_operands=[])
            rows.append(row)
            offset += size
        return rows

    def run_rows(self, rows):
        return roles.scroll_board_requests(rows, snapshot=self.snapshot)

    def test_exact_push_order_and_conversions(self):
        request = self.run_rows(self.fixture())['requests'][0]
        self.assertEqual(list(request['arguments']), list(roles.ARGUMENT_NAMES))
        self.assertEqual([v['value'] for v in request['arguments'].values()], list(range(18)))
        self.assertEqual(request['argument_conversions']['x'], 'float')
        self.assertEqual(request['argument_conversions']['red'], 'integer')
        self.assertFalse(request['creation_proven'])

    def test_symbolic_float_expression_preserves_original_inputs(self):
        specs = [('PUSH_LITERAL', [i]) for i in range(17)] + [('LOAD_VARIABLE', [1, 7]), ('PUSH_LITERAL', [2]), ('MULTIPLY', [])]
        request = self.run_rows(self.fixture(specs))['requests'][0]
        speed = request['arguments']['speed']
        self.assertEqual(speed['kind'], 'vm_operator')
        self.assertEqual(speed['operands'][0], dict(kind='variable', frame=1, slot=7))
        rows = self.fixture()
        rows[4]['operands'][0].update(tag=1, kind='float', value=0.5)
        self.assertEqual(self.run_rows(rows)['requests'][0]['arguments']['x'], dict(kind='float', value=0.5))

    def test_stale_vm_native_and_missing_producer_pin(self):
        self.snapshot[1]['context_vm_contract']['current'] = 1
        self.assertEqual(self.run_rows(self.fixture())['status'], 'unresolved')
        self.snapshot[1]['context_vm_contract']['current'] = True
        self.review['stale'] = True
        self.assertEqual(self.run_rows(self.fixture())['status'], 'unresolved')
        self.review.pop('stale')
        with patch.object(roles, 'PRODUCER_BINDINGS', {}):
            self.assertEqual(self.run_rows(self.fixture())['status'], 'unresolved')

    def test_rebound_native_malformed_opcode_tags_and_cfg(self):
        for mutate in (
            lambda r: r[-1]['native'].update(address=roles.NATIVE_ADDRESS+4),
            lambda r: r[-1]['native'].update(functions=[roles.NATIVE_NAME, 'alias']),
            lambda r: r[0].update(opcode=99),
            lambda r: r[0]['operands'][0].update(tag=True),
            lambda r: r[0]['successors'][0].update(offset=9999),
        ):
            rows = self.fixture()
            mutate(rows)
            self.assertEqual(self.run_rows(rows)['status'], 'unresolved')

    def test_competing_entry_retains_request_without_arguments(self):
        rows = self.fixture()
        rows.insert(0, dict(offset=1, opcode=28, name='SCRIPT_OP_JUMP', size=6,
                            operands=[dict(tag=2,kind='code_reference',value=rows[5]['offset'])],
                            successors=[dict(kind='branch',offset=rows[5]['offset'])]))
        request = self.run_rows(rows)['requests'][0]
        self.assertIsNone(request['arguments'])

    def test_budget_and_underflow_preserve_unresolved_sites(self):
        for count in (17, 33):
            specs = [('PUSH_LITERAL', [i]) for i in range(count)]
            if count == 33:
                specs = [('PUSH_LITERAL', [0])] * 17 + [('LOAD_VARIABLE', [1, 7])] + [('NEGATE', [])] * 33
            report = self.run_rows(self.fixture(specs))
            self.assertEqual(len(report['requests']), 1)
            self.assertIsNone(report['requests'][0]['arguments'])

    def test_native_producer_requires_fresh_review_and_exact_address(self):
        rows = self.fixture()
        producer = rows[17]
        producer.update(opcode=27, name='SCRIPT_OP_CALL_NATIVE',
                        operands=[dict(tag=2,kind='code_reference',value=92)],
                        successors=[dict(kind='resume_or_fallthrough',offset=rows[-1]['offset'])],
                        native=dict(address=roles.PRODUCER_BINDINGS['actorScriptPushSelfHandle']['address'],
                                    functions=['actorScriptPushSelfHandle'],inline_operands=[]))
        self.assertIsNone(self.run_rows(rows)['requests'][0]['arguments'])
        review = dict(arguments=0)
        self.snapshot[0]['active']['actorScriptPushSelfHandle'] = review
        bindings = copy.deepcopy(roles.PRODUCER_BINDINGS)
        bindings['actorScriptPushSelfHandle']['review_sha256'] = roles._digest(review)
        with patch.object(roles, 'PRODUCER_BINDINGS', bindings):
            self.assertEqual(self.run_rows(rows)['requests'][0]['arguments']['speed']['kind'], 'handle')
            review['stale'] = True
            self.assertIsNone(self.run_rows(rows)['requests'][0]['arguments'])
            review.pop('stale')
            producer['native']['address'] += 4
            self.assertIsNone(self.run_rows(rows)['requests'][0]['arguments'])


if __name__ == '__main__':
    unittest.main()
