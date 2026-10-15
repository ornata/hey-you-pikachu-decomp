"""Invented action resources test ID domains, deferred consumption and bounded reads."""

import struct

from tools.script_decoder.actions import action_calls, action_resources
from tools.script_decoder.demo import value
from tools.script_decoder.formats import decode_vm


def calls(program, selector, function):
    native = {selector: dict(address=0x80405000, functions=[function], inline_operands=[])}
    names = {24: 'SCRIPT_OP_PUSH_LITERAL', 25: 'SCRIPT_OP_LOAD_VARIABLE', 27: 'SCRIPT_OP_CALL_NATIVE', 35: 'SCRIPT_OP_FINISH'}
    return action_calls(decode_vm(program + bytes([27]) + value(selector, 2) + bytes([35]), names, natives=native)['instructions'])


def test_action_wait_preserves_deferred_contract_and_script_id_domain():
    result = calls(bytes([24]) + value(7), 15, 'actorScriptStartActionAndWait')[0]
    assert result['constant_script_id'] == 7
    assert result['contract']['consumption'] == 'deferred'
    assert result['contract']['scheduling'] == 'wait_for_action'
    assert result['contract']['result'] == 'zero_when_sequencer_stops'


def test_timed_action_retains_float_duration_and_variable_id_without_resolving_it():
    program = bytes([25]) + value(0) + value(9) + bytes([24, 1]) + struct.pack('>f', 1.25)
    result = calls(program, 108, 'actorScriptStartTimedAction')[0]
    assert result['arguments']['script_id'] == dict(kind='variable', frame=0, slot=9)
    assert result['arguments']['duration_seconds'] == dict(kind='float', value=1.25)
    assert 'constant_script_id' not in result
    assert result['contract']['consumption'] == 'immediate'


class Image:
    def __init__(self):
        self.stream = struct.pack('>ifii', 0, 0, 3, 18) + struct.pack('>ifii', 4, 1000, 0, 0)

    def symbol(self, name):
        return name, 0x80401000 if name == 'gActionScriptPrograms' else 0x80402000, 8

    def read_address(self, address, size):
        if address in (0x80401000, 0x80401004):
            data = struct.pack('>I', 0x80403000)
        elif address == 0x80402000 + 3 * 44:
            data = struct.pack('>iI', 3, 0x80404000) + b'\0' * 36
        elif 0x80403000 <= address < 0x80403000 + len(self.stream):
            data = self.stream[address - 0x80403000:]
        elif 0x80404000 <= address < 0x80404000 + 14:
            data = b'Invented turn\0'[address - 0x80404000:]
        else:
            raise ValueError('outside synthetic resource')
        assert len(data) >= size
        return data[:size], dict(address=address, section='.boot')


def test_resource_chain_separates_timed_script_and_motion_and_rejects_bad_extents():
    rows = action_resources(Image(), {0, 1, 2})
    assert [row['status'] for row in rows] == ['decoded', 'decoded', 'unresolved']
    motion = rows[0]['motion_requests'][0]
    assert rows[0]['script_id'] == 0 and motion['motion_action_id'] == 3
    assert motion['debug_label'] == 'Invented turn' and motion['inherits_default_kind']
    assert rows[0]['decoded']['instructions'][0]['action_request']['inherits_default_kind']
    assert 'bounded linked table view' in rows[2]['reason']
    row = action_resources(Image(), {0}, record_limit=1)[0]
    assert row['status'] == 'unresolved'
    assert any('END not established' in error for error in row['decoded']['diagnostics'])
    image = Image()
    image.stream = struct.pack('>ifii', 9, 0, 0, 0)
    assert action_resources(image, {0})[0]['status'] == 'unresolved'


def test_symbolic_script_return_keeps_arity_without_inventing_a_constant_result():
    native = {15: dict(address=0x80405000, functions=['actorScriptStartActionAndWait'], inline_operands=[])}
    names = {24: 'SCRIPT_OP_PUSH_LITERAL', 25: 'SCRIPT_OP_LOAD_VARIABLE', 26: 'SCRIPT_OP_CALL',
             27: 'SCRIPT_OP_CALL_NATIVE', 30: 'SCRIPT_OP_RETURN', 32: 'SCRIPT_OP_ENTER_FRAME', 35: 'SCRIPT_OP_FINISH'}
    main = bytes([24]) + value(9) + bytes([26]) + value(19, 2) + bytes([27]) + value(15, 2) + bytes([35])
    callee = bytes([32]) + value(1) + value(1) + value(1) + bytes([25]) + value(1) + value(0) + bytes([30])
    decoded = decode_vm(main + callee, names, natives=native)
    result = action_calls(decoded['instructions'])[0]
    argument = result['arguments']['script_id']
    assert argument['kind'] == 'script_return' and argument['target_offset'] == 19
    assert argument['arguments'] == [dict(kind='integer', value=9)]
    assert 'constant_script_id' not in result
    bad = main + bytes([32]) + value(2) + value(1) + value(1) + callee[16:]
    assert action_calls(decode_vm(bad, names, natives=native)['instructions'])[0]['status'] == 'unresolved'


def test_indexed_random_choice_preserves_unknown_element_value_and_known_index_range():
    names = {24: 'SCRIPT_OP_PUSH_LITERAL', 27: 'SCRIPT_OP_CALL_NATIVE', 33: 'SCRIPT_OP_LOAD_ELEMENT', 35: 'SCRIPT_OP_FINISH'}
    natives = {6: dict(address=0x80405000, functions=['actorScriptRandomBelow'], inline_operands=[]),
               15: dict(address=0x80406000, functions=['actorScriptStartActionAndWait'], inline_operands=[])}
    program = (bytes([24]) + value(3) + bytes([27]) + value(6, 2) + bytes([33]) + value(1) + value(0) +
               bytes([27]) + value(15, 2) + bytes([35]))
    result = action_calls(decode_vm(program, names, natives=natives)['instructions'])[0]
    argument = result['arguments']['script_id']
    assert argument['kind'] == 'array_element' and argument['frame'] == 1 and argument['slot'] == 0
    assert argument['index']['integer_range'] == [0, 2]
    assert 'constant_script_id' not in result


def test_no_program_sentinel_requires_retained_null_backing_and_does_not_allow_other_negative_ids():
    class SentinelImage(Image):
        def read_address(self, address, size):
            if address == 0x80401004:
                return struct.pack('>I', 0x80401000 - 32), dict(address=address)
            if 0x80401000 - 32 <= address < 0x80401000:
                start = address - (0x80401000 - 32)
                return self.stream[start:start + size], dict(address=address)
            return super().read_address(address, size)

    image = SentinelImage()
    rows = action_resources(image, {-2, -1})
    assert rows[0]['status'] == 'unresolved'
    assert rows[1]['status'] == 'no_program' and rows[1]['address'] == 0
    assert rows[1]['recovered_name'] == 'ACTION_SCRIPT_NO_PROGRAM'
    image.stream = image.stream[:-4] + struct.pack('>I', 1)
    assert action_resources(image, {-1})[0]['status'] == 'unresolved'
