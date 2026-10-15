"""Invented frames distinguish shadowing, caller writes and opaque native effects."""

from tools.script_decoder.context import opcode_names
from tools.script_decoder.demo import value
from tools.script_decoder.formats import decode_vm
from tools.script_decoder.saved_dictionary import saved_dictionary_analysis


def example(child_frame=1, root_write=False, opaque=False, indexed=False):
    frame = lambda number: bytes([32]) + value(0) + value(1) + value(number)
    native = lambda selector: bytes([27]) + value(selector, 2)
    store = bytes([10]) + value(1) + value(0)
    push = lambda number: bytes([24]) + value(number)
    prefix = frame(1) + native(1) + store
    tail = bytes([31])
    if root_write:
        tail += push(55) + store
    tail += bytes([25]) + value(1) + value(0) + native(2) + bytes([30])
    root = prefix + bytes([26]) + value(len(prefix) + 6 + len(tail), 2) + tail
    child = frame(child_frame)
    if indexed:
        child += push(0) + push(999) + bytes([34]) + value(1) + value(0)
    else:
        child += push(999) + store
    if opaque:
        child += native(3)
    child += push(0) + bytes([30])
    natives = {1: dict(address=0x80404000, inline_operands=[], functions=['actorScriptQueryRequestedMicDictionary']),
               2: dict(address=0x80405000, inline_operands=[], functions=['actorScriptRequestMicDictionary']),
               3: dict(address=0x80406000, inline_operands=[], functions=['inventedMutator'])}
    rows = decode_vm(root + child, opcode_names('vm'), natives=natives)['instructions']
    request = next(row['offset'] for row in rows if row.get('native', {}).get('functions') == natives[2]['functions'])
    return rows, request


def analyze(rows, request, **arguments):
    return saved_dictionary_analysis(rows, request, dict(kind='variable', frame=1, slot=0), **arguments)


def test_nested_same_frame_shadows_saved_local_but_numeric_value_remains_unknown():
    rows, request = example()
    result = analyze(rows, request)
    assert result['save_dominates_request']
    assert result['bytecode_preserves_saved_local']
    assert result['script_function_views'] == 2
    assert result['write_hazards'] == []
    assert 'numeric ID remain unproven' in result['scope']


def test_different_frame_can_write_callers_saved_local_and_root_overwrite_is_detected():
    for arguments in (dict(child_frame=2), dict(root_write=True), dict(child_frame=2, indexed=True)):
        rows, request = example(**arguments)
        result = analyze(rows, request)
        assert not result['bytecode_preserves_saved_local']
        assert any('saved caller local' in row['reason'] for row in result['write_hazards'])


def test_bypass_budget_and_unresolved_callee_do_not_prove_preservation():
    rows, request = example()
    rows[0]['successors'].append(dict(kind='branch', offset=request))
    assert analyze(rows, request) is None
    rows, request = example()
    assert analyze(rows, request, budget=1) is None
    call = next(row for row in rows if row['name'] == 'SCRIPT_OP_CALL')
    call['operands'][0]['value'] = 123456
    result = analyze(rows, request)
    assert not result['bytecode_preserves_saved_local'] and result['diagnostics']


def test_native_alias_effects_are_an_explicit_frontier_even_with_safe_bytecode():
    rows, request = example(opaque=True)
    result = analyze(rows, request)
    assert result['bytecode_preserves_saved_local']
    assert 'inventedMutator' in result['native_effect_frontier']
    assert 'native effects' in result['scope']


def test_provided_review_clears_only_that_callback_and_missing_origin_review_stays_explicit():
    rows, request = example(opaque=True)
    result = analyze(rows, request, native_reviews={'actorScriptQueryRequestedMicDictionary': {'effect': 'append_scalar'}})
    assert result['origin_effect_reviewed']
    assert 'actorScriptQueryRequestedMicDictionary' not in result['native_effect_frontier']
    assert 'inventedMutator' in result['native_effect_frontier']
    result = analyze(rows, request)
    assert not result['origin_effect_reviewed']
    assert 'actorScriptQueryRequestedMicDictionary' in result['native_effect_frontier']
