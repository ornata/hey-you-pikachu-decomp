"""Invented returning functions exercise graph boundaries and incomplete summaries."""

from tools.script_decoder.context import opcode_names
from tools.script_decoder.demo import value
from tools.script_decoder.formats import decode_vm
from tools.script_decoder.functions import function_summary


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
