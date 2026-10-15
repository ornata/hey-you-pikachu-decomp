"""Invented names and bytecode keep catalog, word and dictionary domains separate."""

import struct

import pytest

from tools.script_decoder.name_catalog import pokemon_name_catalog, name_dictionary_request
from tools.script_decoder.context import opcode_names
from tools.script_decoder.demo import value
from tools.script_decoder.formats import decode_vm


class Image:
    def __init__(self):
        self.allowed = {'.boot', '.ovl'}
        self.groups = bytes([2, 3, 2])
        self.names = [b'Invented A\0', b'Invented B\0', b'Invented C\0']

    def symbol(self, name):
        assert '.scene_bedroom' in self.allowed
        return name, 0x80401000 if name.endswith('Groups') else 0x80402000

    def read_address(self, address, size):
        if address == 0x80401000:
            data = self.groups
        elif address == 0x80402000:
            data = struct.pack('>3I', 0x80403000, 0x80403100, 0x80403200)
        elif 0x80403000 <= address < 0x80403300:
            index, offset = divmod(address - 0x80403000, 256)
            data = self.names[index][offset:]
        else:
            raise ValueError('outside invented scene')
        if len(data) < size:
            raise ValueError('short invented record')
        return data[:size], dict(address=address, section='.scene_bedroom')


def vocabulary():
    return dict(dictionaries=[dict(dictionary_id=19, word_ids=[9, 13]), dict(dictionary_id=20, word_ids=[11])],
                words=[dict(word_id=word, command_type=10, command_value=index + 8, name=dict(ascii='Invented ' + letter))
                       for index, (word, letter) in enumerate(((9, 'A'), (11, 'B'), (13, 'C')))])


def test_name_groups_link_commands_and_memberships_without_merging_ids():
    image = Image()
    original = image.allowed
    result = pokemon_name_catalog(image, vocabulary(), count=3)
    assert image.allowed is original
    assert result['dictionary_group_domain'] == [2, 3]
    assert result['dictionary_id_domain'] == [19, 20]
    row = result['entries'][2]
    assert row['catalog_index'] == 2 and row['word_ids'] == [13]
    assert row['command_value'] == 10 and row['dictionary_id'] == 19
    assert row['display_label_agrees']
    assert 'no live selection' in result['scope']


def test_wrong_group_or_command_is_rejected_while_label_differences_remain_explicit():
    image = Image()
    original = image.allowed
    image.groups = bytes([1, 3, 2])
    with pytest.raises(ValueError, match='descriptor'):
        pokemon_name_catalog(image, vocabulary(), count=3)
    assert image.allowed is original
    image = Image()
    image.names[0] = b'Other name\0'
    result = pokemon_name_catalog(image, vocabulary(), count=3)
    assert not result['entries'][0]['display_label_agrees']
    assert result['entries'][0]['word_ids'] == [9]
    data = vocabulary()
    data['words'][0]['command_value'] = 99
    with pytest.raises(ValueError, match='matching command'):
        pokemon_name_catalog(Image(), data, count=3)
    with pytest.raises(ValueError, match='extent'):
        pokemon_name_catalog(Image(), vocabulary(), count=0)


def request_program():
    load = lambda frame, slot: bytes([25]) + value(frame) + value(slot)
    push = lambda number: bytes([24]) + value(number)
    native = lambda selector: bytes([27]) + value(selector, 2)
    prefix = bytes([32]) + value(1) + value(2) + value(1)
    prefix += load(1, 0) + native(1) + bytes([10]) + value(1) + value(1)
    prefix += load(0, 4) + load(1, 1) + bytes([12])
    body = push(18) + load(1, 1) + bytes([5]) + push(1) + bytes([6]) + native(2)
    program = prefix + bytes([29]) + value(len(prefix) + 6 + len(body) + 1, 2) + body + bytes([31]) + push(0) + bytes([30])
    natives = {1: dict(address=0x80404000, inline_operands=[], functions=['actorScriptGetPokemonNameDictionaryGroup']),
               2: dict(address=0x80405000, inline_operands=[], functions=['actorScriptRequestMicDictionary'])}
    instructions = decode_vm(program, opcode_names('vm'), natives=natives)['instructions']
    index = next(i for i, row in enumerate(instructions) if row.get('native', {}).get('functions') == natives[2]['functions'])
    return instructions, index


def test_name_request_domain_is_conditional_on_valid_catalog_argument():
    rows, index = request_program()
    catalog = pokemon_name_catalog(Image(), vocabulary(), count=3)
    result = name_dictionary_request(rows, index, catalog)
    assert result['dictionary_id_domain'] == [19, 20]
    assert result['precondition']['valid_index_range'] == [0, 2]
    assert 'no caller-argument validity' in result['scope']


def test_competing_entries_wrong_arithmetic_and_group_aliases_remain_unresolved():
    catalog = pokemon_name_catalog(Image(), vocabulary(), count=3)
    rows, index = request_program()
    rows[0]['successors'].append(dict(kind='branch', offset=rows[9]['offset']))
    assert name_dictionary_request(rows, index, catalog) is None
    rows, index = request_program()
    rows[8]['operands'][0]['value'] = 19
    assert name_dictionary_request(rows, index, catalog) is None
    rows, index = request_program()
    rows[4]['operands'][0]['value'] = 1
    assert name_dictionary_request(rows, index, catalog) is None
