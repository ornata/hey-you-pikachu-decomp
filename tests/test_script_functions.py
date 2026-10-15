"""Invented returning functions exercise graph boundaries and incomplete summaries."""

from tools.script_decoder.context import opcode_names
from tools.script_decoder.demo import value
from tools.script_decoder.formats import decode_vm
from tools.script_decoder.functions import function_summary
from tools.script_decoder.stack import recover_arguments


def frame(arguments=0, locals_count=0):
    return bytes([32]) + value(arguments) + value(locals_count) + value(1)


def decode(program):
    return decode_vm(program, opcode_names('vm'))['instructions']


def test_conditional_returns_keep_finite_overapproximation_without_termination_claim():
    program = frame() + bytes([24]) + value(1) + bytes([29]) + value(35, 2)
    program += bytes([24]) + value(7) + bytes([30, 24]) + value(9) + bytes([30])
    result = function_summary(decode(program), 0)
    assert result['integer_return_domain'] == [7, 9]
    assert result['static_return_domain_complete']
    assert 'no termination' in result['scope']


def test_callee_return_is_not_mistaken_for_callers_return_domain():
    program = frame() + bytes([26]) + value(30, 2) + bytes([31, 24]) + value(7) + bytes([30])
    program += frame() + bytes([24]) + value(999) + bytes([30])
    instructions = decode(program)
    assert function_summary(instructions, 0)['integer_return_domain'] == [7]
    assert function_summary(instructions, 30)['integer_return_domain'] == [999]


def test_unknown_values_other_frame_entries_and_budget_limits_stay_incomplete():
    program = frame(0, 1) + bytes([25]) + value(1) + value(0) + bytes([30])
    result = function_summary(decode(program), 0)
    assert not result['static_return_domain_complete']
    assert result['returns'][0]['expression']['kind'] == 'variable'
    assert not function_summary(decode(program), 0, limit=1)['static_return_domain_complete']
    program = frame() + bytes([28]) + value(22, 2) + frame() + bytes([24]) + value(7) + bytes([30])
    result = function_summary(decode(program), 0)
    assert not result['static_return_domain_complete']
    assert any('another frame' in error for error in result['diagnostics'])


def table_program(indices=(0, 1, 2), low=7, high=9, array_frame=1, locals_count=4, variable=5):
    """Invent an inclusive guarded table with a distinct fallback return."""
    load = lambda slot: bytes([25]) + value(0) + value(slot)
    push = lambda number: bytes([24]) + value(number)
    guard = load(variable) + push(low) + bytes([15]) + load(variable) + push(high) + bytes([16, 18])
    body = b''.join(push(i) + push(101 * (i + 1)) + bytes([34]) + value(array_frame) + value(1) for i in indices)
    body += load(variable) + push(low) + bytes([6, 33]) + value(array_frame) + value(1) + bytes([30])
    prefix = frame(0, locals_count) + guard
    jump = bytes([29]) + value(len(prefix) + 6 + len(body), 2)
    return prefix + jump + body + push(-1) + bytes([30])


def test_guarded_local_table_return_has_complete_values_and_write_provenance():
    result = function_summary(decode(table_program()), 0)
    assert result['static_return_domain_complete']
    assert result['integer_return_domain'] == [-1, 101, 202, 303]
    lookup = result['returns'][0]['resolved_lookup']
    assert lookup['variable_range'] == [7, 9]
    assert [row['index'] for row in lookup['initialized_elements']] == [0, 1, 2]


def test_missing_duplicate_out_of_frame_or_other_frame_writes_remain_unresolved():
    for arguments in (dict(indices=(0, 2)), dict(indices=(0, 1, 1)), dict(locals_count=3),
                      dict(array_frame=2), dict(low=10, high=9)):
        result = function_summary(decode(table_program(**arguments)), 0)
        assert not result['static_return_domain_complete']
        assert 'resolved_lookup' not in result['returns'][0]


def test_guard_bypass_other_variable_and_intervening_mutation_are_not_accepted():
    instructions = decode(table_program())
    store = next(row for row in instructions if row['name'] == 'SCRIPT_OP_STORE_ELEMENT')
    instructions[0]['successors'].append(dict(kind='branch', offset=store['offset']))
    assert not function_summary(instructions, 0)['static_return_domain_complete']
    instructions = decode(table_program())
    reads = [row for row in instructions if row['name'] == 'SCRIPT_OP_LOAD_VARIABLE']
    reads[1]['operands'][1]['value'] = 6
    assert not function_summary(instructions, 0)['static_return_domain_complete']
    instructions = decode(table_program())
    for row in instructions:
        if row['name'] == 'SCRIPT_OP_LOAD_VARIABLE':
            row['operands'][0]['value'] = 1
    assert not function_summary(instructions, 0)['static_return_domain_complete']
    instructions = decode(table_program())
    stores = [row for row in instructions if row['name'] == 'SCRIPT_OP_STORE_ELEMENT']
    stores[1]['name'] = 'SCRIPT_OP_STORE_VARIABLE'
    assert not function_summary(instructions, 0)['static_return_domain_complete']


def test_subtraction_keeps_operand_order_and_refuses_overflow():
    import pytest

    def expression(left, right):
        program = bytes([24]) + value(left) + bytes([24]) + value(right) + bytes([6, 30])
        rows = decode(program)
        return recover_arguments(rows, len(rows) - 1, 1, allow_symbolic=True)[0]

    assert expression(3, 9)['value'] == -6
    with pytest.raises(ValueError, match='overflowing'):
        expression(-2147483648, 1)
    result = function_summary(decode(table_program()), 0)['returns'][0]
    index = result['expression']['index']
    assert index['operator'] == 'subtract'
    assert index['operands'][0] == dict(kind='variable', frame=0, slot=5)
    assert index['operands'][1]['value'] == 7
