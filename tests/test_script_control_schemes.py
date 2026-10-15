"""Invented table views and operands guard control-scheme interpretation."""

import struct

import pytest

from tools.script_decoder.control_schemes import control_scheme_catalog, control_scheme_requests
from tools.script_decoder.demo import value
from tools.script_decoder.formats import decode_vm


class Image:
    def __init__(self, bad_mode=False):
        self.bad_mode = bad_mode

    def symbol(self, name):
        return name, 0x80401000 if name == 'gPlayerModes' else 0x80402000

    def functions(self, address):
        return {0x80403000: ['inventedUpdate'], 0x80403004: ['otherUpdate'],
                0x80403008: ['inventedLeave']}.get(address, [])

    def read_address(self, address, size):
        records = {0x80401000: struct.pack('>III', 0x80403000, 0, 0x8000),
                   0x8040100C: struct.pack('>III', 0x80403004, 0x80403008, 0x200),
                   0x80402000: struct.pack('>iiH2x', 32 if self.bad_mode else 1, 0, 0x8040),
                   0x8040200C: struct.pack('>iiH2x', 0, -1, 0x80)}
        assert size == 12
        return records[address], dict(address=address, section='.ovl')


def test_table_links_preserve_input_domains_modes_and_leave_frontier():
    result = control_scheme_catalog(Image(), 2, 2)
    assert result['schemes'][0]['retained_inputs'] == dict(buttons=['A_BUTTON'], stick_axes=['X'])
    assert result['schemes'][1]['retained_inputs'] == dict(buttons=[], stick_axes=['Y'])
    assert result['modes'][0]['permission_mask'] == 0x8000
    assert result['leave_callback_frontier'] == ['inventedLeave']
    with pytest.raises(ValueError, match='invalid mode'):
        control_scheme_catalog(Image(True), 2, 2)


def test_requests_truncate_numeric_indices_but_keep_dynamic_and_invalid_values_explicit():
    names = {24: 'SCRIPT_OP_PUSH_LITERAL', 25: 'SCRIPT_OP_LOAD_VARIABLE', 27: 'SCRIPT_OP_CALL_NATIVE'}
    native = {0: dict(address=0x80403000, inline_operands=[], functions=['actorScriptStartControlSchemeChange'])}
    prefixes = [bytes([24, 1]) + struct.pack('>f', 27.75), bytes([24]) + value(99),
                bytes([25]) + value(0) + value(5)]
    results = []
    for prefix in prefixes:
        rows = decode_vm(prefix + bytes([27]) + value(0, 2), names, natives=native)['instructions']
        results.append(control_scheme_requests(rows, [dict(scheme=27, mode=31)])[0])
    assert results[0]['constant_scheme_id'] == 27 and results[0]['selected_mode'] == 31
    assert results[0]['consumption'] == 'deferred_until_outside_text_window_mode'
    assert results[1]['constant_scheme_id'] == 99 and not results[1]['valid_scheme_index']
    assert results[2]['scheme_expression'] == dict(kind='variable', frame=0, slot=5)
    assert 'constant_scheme_id' not in results[2]
