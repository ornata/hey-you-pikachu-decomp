"""Link the bedroom name activity's catalog to its recognition dictionaries.

Original names and tables are read only from the user's authenticated local ROM.
Catalog indices, dictionary groups and voice word IDs remain distinct domains.
"""

import re
import struct

from .context import ROOT
from .formats import integer_operand
from .stack import incoming_edges
from tools.source_quality import code_only


def pokemon_name_catalog(image, vocabulary, count=None):
    """Authenticate bounded bedroom names, group codes and vocabulary memberships."""
    if count is None:
        code = code_only((ROOT / 'src/ovl/actor_script_collection_profile.c').read_text())
        match = re.search(r'\bBEDROOM_POKEMON_NAME_COUNT\s*=\s*(\d+)', code)
        if match is None:
            raise ValueError('missing reviewed bedroom name-catalog extent')
        count = int(match[1])
    if not 1 <= count <= 256:
        raise ValueError('name catalog extent must be between 1 and 256')
    allowed = image.allowed
    image.allowed = {'.boot', '.ovl', '.scene_bedroom'}
    try:
        groups_address = image.symbol('gBedroomPokemonNameDictionaryGroups')[1]
        names_address = image.symbol('gBedroomPokemonNames')[1]
        groups, group_location = image.read_address(groups_address, count)
        names, name_location = image.read_address(names_address, count * 4)
        dictionaries = {row['dictionary_id']: row for row in vocabulary['dictionaries']}
        words = {row['word_id']: row for row in vocabulary['words']}
        rows = []
        for index, group in enumerate(groups):
            address = struct.unpack_from('>I', names, index * 4)[0]
            text = bytearray()
            if not address:
                raise ValueError('null bedroom catalog name')
            for offset in range(256):
                byte, _ = image.read_address(address + offset, 1)
                if byte == b'\0':
                    break
                text.extend(byte)
            else:
                raise ValueError('bedroom catalog name exceeds the bounded read')
            name = text.decode('ascii')
            dictionary = dictionaries.get(group + 17)
            if dictionary is None:
                raise ValueError('name dictionary group has no reviewed vocabulary descriptor')
            matches = [word for word in dictionary['word_ids']
                       if words[word]['command_type'] == 10 and words[word]['command_value'] == index + 8]
            if not matches:
                raise ValueError('catalog name has no matching command and dictionary membership')
            rows.append(dict(catalog_index=index, name=name, name_address=address, dictionary_group=group,
                             dictionary_id=group + 17, command_type=10, command_value=index + 8, word_ids=matches,
                             display_label_agrees=any(words[word]['name']['ascii'] == name for word in matches)))
        return dict(section='.scene_bedroom', count=count, group_location=group_location, name_location=name_location,
                    entries=rows, dictionary_group_domain=sorted(set(groups)),
                    dictionary_id_domain=sorted({row['dictionary_id'] for row in rows}),
                    scope='Bedroom catalog and vocabulary links under the reviewed script formula; no live selection, recognition or reachability proof.')
    finally:
        image.allowed = allowed


def name_dictionary_request(instructions, index, catalog):
    """Recognize the reviewed guarded group-to-dictionary conversion, or None."""
    if index < 13 or catalog is None:
        return None
    rows = instructions[index - 13:index + 1]
    names = [row['name'].removeprefix('SCRIPT_OP_') for row in rows]
    if names != ['ENTER_FRAME', 'LOAD_VARIABLE', 'CALL_NATIVE', 'STORE_VARIABLE', 'LOAD_VARIABLE',
                 'LOAD_VARIABLE', 'NOT_EQUAL', 'JUMP_IF_ZERO', 'PUSH_LITERAL', 'LOAD_VARIABLE',
                 'ADD', 'PUSH_LITERAL', 'SUBTRACT', 'CALL_NATIVE']:
        return None
    def operands(row):
        return tuple(map(integer_operand, row['operands']))
    argument_count, local_count, frame = operands(rows[0])
    if argument_count != 1 or local_count != 2 or frame is None or not 0 <= frame < 32:
        return None
    if rows[2].get('native', {}).get('functions') != ['actorScriptGetPokemonNameDictionaryGroup']:
        return None
    if rows[-1].get('native', {}).get('functions') != ['actorScriptRequestMicDictionary']:
        return None
    if operands(rows[1]) != (frame, 0) or any(operands(rows[i]) != (frame, 1) for i in (3, 5, 9)):
        return None
    cached_frame, cached_slot = operands(rows[4])
    if cached_frame is None or cached_slot is None or not -1 <= cached_frame < 32 or cached_slot < 0:
        return None
    if cached_frame == frame:
        return None  # The cached comparison input must not alias the new group.
    if operands(rows[8]) != (18,) or operands(rows[11]) != (1,):
        return None
    if rows[8]['operands'][0]['tag'] != 0 or rows[11]['operands'][0]['tag'] != 0:
        return None
    incoming = incoming_edges(instructions)
    if any(left['offset'] + left['size'] != right['offset'] or
           incoming.get(right['offset'], set()) != {left['offset']} for left, right in zip(rows, rows[1:])):
        return None
    target = integer_operand(rows[7]['operands'][0])
    if target is None or rows[0]['offset'] <= target <= rows[-1]['offset']:
        return None
    return dict(kind='pokemon_name_dictionary_group', function_entry=rows[0]['offset'],
                producer_offset=rows[2]['offset'], group_local=dict(frame=frame, slot=1),
                dictionary_id_domain=catalog['dictionary_id_domain'],
                precondition=dict(catalog_argument=dict(frame=frame, slot=0), valid_index_range=[0, catalog['count'] - 1]),
                scope='Conditional static domain for valid bedroom catalog indices and intact nonoverlapping frames; no caller-argument validity, upload or reachability proof.')
