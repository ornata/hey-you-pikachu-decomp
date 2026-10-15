"""Invented requests distinguish static resource links from asynchronous selection."""

import struct

from tools.script_decoder.dictionaries import dictionary_requests, source_dictionary_requests, attach_dictionary_usage
from tools.script_decoder.demo import value
from tools.script_decoder.context import opcode_names
from tools.script_decoder.formats import decode_vm


def requests(program):
    native = {7: dict(address=0x80405000, functions=['actorScriptRequestMicDictionary'], inline_operands=[])}
    rows = decode_vm(program + bytes([27]) + value(7, 2) + bytes([35]), opcode_names('vm'), natives=native)['instructions']
    return dictionary_requests(rows, [dict(dictionary_id=3, count=2, location=dict(address=0x80407000),
                                          recovered_name='INVENTED_DICTIONARY')])[0]


def test_literal_request_has_resource_and_stack_contract_without_upload_claim():
    result = requests(bytes([24]) + value(3))
    assert result['constant_dictionary_id'] == 3
    assert result['dictionary']['recovered_name'] == 'INVENTED_DICTIONARY'
    assert result['contract']['stack_result'] == 0
    assert result['contract']['scheduling'] == 'yield'
    assert result['contract']['consumption'] == 'immediate'
    assert 'no upload' in result['scope']


def test_float_id_truncates_but_variable_and_unreviewed_ids_remain_explicit():
    result = requests(bytes([24, 1]) + struct.pack('>f', 3.75))
    assert result['constant_dictionary_id'] == 3
    assert result['dictionary_id_expression']['value'] == 3.75
    result = requests(bytes([25]) + value(0) + value(9))
    assert result['dictionary_id_expression'] == dict(kind='variable', frame=0, slot=9)
    assert 'dictionary' not in result and 'constant_dictionary_id' not in result
    result = requests(bytes([24]) + value(-1))
    assert result['constant_dictionary_id'] == -1
    assert 'unresolved_resource' in result and 'dictionary' not in result


def test_unknown_producer_and_invalid_float_conversion_do_not_invent_ids():
    assert requests(bytes([31]))['status'] == 'unresolved'
    result = requests(bytes([24, 1]) + struct.pack('>f', float('inf')))
    assert 'constant_dictionary_id' not in result
    assert 'dictionary' not in result


def test_source_inventory_excludes_declarations_and_keeps_nested_dynamic_calls():
    source = {'invented.c': '''void micRequestDictionary(s32 id);
void f(void) {
 micRequestDictionary(VOICE_DICTIONARY_DEFAULT);
 micRequestDictionary(0x1F);
 micRequestDictionary(pop(convert(1)));
}'''}
    rows = source_dictionary_requests(source, [dict(dictionary_id=0, count=2, recovered_name='VOICE_DICTIONARY_DEFAULT')])
    assert len(rows) == 3
    assert rows[0]['dictionary']['recovered_name'] == 'VOICE_DICTIONARY_DEFAULT'
    assert rows[1]['constant_dictionary_id'] == 31 and 'dictionary' not in rows[1]
    assert rows[2]['expression'] == 'pop(convert(1))'
    assert 'constant_dictionary_id' not in rows[2]


def test_usage_preserves_scene_bindings_without_assigning_dictionary_ownership():
    vocabulary = dict(dictionaries=[dict(dictionary_id=3), dict(dictionary_id=4)])
    configs = [dict(address=0x80401000, section='.scene_invented', listing='invented.json',
                    direct_creation_sites=[], assigned_creation_sites=[],
                    companion_scenario_sites=[dict(area_index=7, scenario=2)],
                    voice_dictionary_requests=[dict(constant_dictionary_id=3, instruction_offset=18),
                                               dict(dictionary_id_expression=dict(kind='variable', frame=0, slot=2))])]
    attach_dictionary_usage(vocabulary, configs, [dict(file='invented.c', constant_dictionary_id=4)])
    descriptor = vocabulary['dictionaries'][0]
    assert len(descriptor['script_requests']) == 1
    assert descriptor['script_requests'][0]['companion_scenario_sites'] == [dict(area_index=7, scenario=2)]
    assert descriptor['source_requests'] == []
    assert 'no scene ownership' in descriptor['usage_scope']
    assert vocabulary['dictionaries'][1]['script_requests'] == []
    assert vocabulary['dictionaries'][1]['source_requests'][0]['file'] == 'invented.c'
