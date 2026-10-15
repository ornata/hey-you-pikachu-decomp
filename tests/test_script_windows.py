"""Invented operands distinguish window settings from successful window creation."""

import struct

from tools.script_decoder.demo import value
from tools.script_decoder.formats import decode_vm
from tools.script_decoder.windows import text_window_requests


def test_window_settings_keep_push_order_conversion_and_unresolved_producers():
    names = {24: 'SCRIPT_OP_PUSH_LITERAL', 25: 'SCRIPT_OP_LOAD_VARIABLE', 27: 'SCRIPT_OP_CALL_NATIVE'}
    natives = {0: dict(address=0x80403000, functions=['actorScriptStartTextWindow'], inline_operands=[])}
    call = bytes([27]) + value(0, 2)
    prefixes = [bytes([24]) + value(2) + bytes([24]) + value(7, 2) + bytes([24, 1]) + struct.pack('>f', 0.75),
                bytes([24]) + value(2) + bytes([24]) + value(7) + bytes([25]) + value(1) + value(5),
                b'']
    literal, symbolic, unresolved = [text_window_requests(decode_vm(prefix + call, names, natives=natives)['instructions'])[0]
                                     for prefix in prefixes]
    assert literal['constant_settings'] == dict(indicator=2, style=7, scene_view_mode=0)
    assert literal['arguments']['scene_view_mode'] == dict(kind='float', value=0.75)
    assert literal['requests_scene_updates'] is False
    assert literal['consumption'] == 'deferred_until_outside_text_window_mode'
    assert literal['completion_result'] == 'integer_zero_even_if_window_open_is_rejected'
    assert symbolic['arguments']['scene_view_mode'] == dict(kind='variable', frame=1, slot=5)
    assert 'requests_scene_updates' not in symbolic
    assert unresolved['status'] == 'unresolved' and 'arguments' not in unresolved
