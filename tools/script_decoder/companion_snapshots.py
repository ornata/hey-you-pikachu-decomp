"""Recognize complete source-bound companion snapshot producer prefixes.

A full canonical producer and each actual absolute-value helper are required.
These are sampled script predicates, not object flags, coherent atomic state,
retail reachability or a claim about later scene-specific extensions.
"""

import hashlib
import json
import math
import struct

from .callee_contexts import context_snapshot
from .context import ROOT, opcode_names
from .formats import VM_OPERANDS
from .native_frames import fingerprint, function_source
from .stack import incoming_edges
from .vocabulary import _reviewed_enum
from tools.source_quality import code_only

PREFIX_INSTRUCTIONS = 329
PREFIX_BYTES = 2204
PREFIX_SHA256 = '9756059b2998dd64b7c096c3174a5739daab736c14edb79d54b2dda2650b2d1d'
FLAG_MAPPING_SHA256 = 'f5632d42875733843461d960fd67b5757dcec27d312f849ebafd07d7c2a6b868'
NATIVE_BINDINGS = {'actorScriptPushOriginDistance': {'address': 2148651528,
                                   'file': 'src/ovl/actor_script_messages_items.c',
                                   'function': 'actorScriptPushOriginDistance',
                                   'sha256': '4672773a21a8bfa5c404b54439f348aaad5d4c2fd6ee9d9148697c7454c892a9',
                                   'review_sha256': '5de5aaff6eaf396d357e313d1bf9812cb37c067abf1d5e85365b03f1242a4547',
                                   'arguments': 0},
 'actorScriptPushPikachuSecondaryMeter': {'address': 2148659644,
                                          'file': 'src/ovl/actor_script_player_world.c',
                                          'function': 'actorScriptPushPikachuSecondaryMeter',
                                          'sha256': '8721fbd34faadde2ad86189c39af9ac61742710dd84c55cc14671ca30b3ed7f1',
                                          'review_sha256': 'e3bf7c4df058f3ed0214c3ffa1824b9c918405c410bd1c35b1c63d118b038f0d',
                                          'arguments': 0},
 'actorScriptPushMovementCollisionFlags': {'address': 2148652304,
                                           'file': 'src/ovl/actor_script_messages_items.c',
                                           'function': 'actorScriptPushMovementCollisionFlags',
                                           'sha256': '908c455a53c6c27f5cd988061a99d5296d35be186b6f1ace3d48949a137e26f6',
                                           'review_sha256': 'b36f7833225c002e78d42d53e87ca73f66fd89d6bbc4824e90c28757d0869ded',
                                           'arguments': 0},
 'actorScriptAcknowledgeMessage': {'address': 2148649036,
                                   'file': 'src/ovl/actor_script_messages_items.c',
                                   'function': 'actorScriptAcknowledgeMessage',
                                   'sha256': '6c85f11d907f7f018a366a6b18dde1d3a5d6045a2c6f03b1fbbe91494aff5df1',
                                   'review_sha256': 'f1e0a7a9aa429c606db2b16cbed462886de689626eaa1d22937811c6ab194327',
                                   'arguments': 0},
 'actorScriptPushMessageType': {'address': 2148649136,
                                'file': 'src/ovl/actor_script_messages_items.c',
                                'function': 'actorScriptPushMessageType',
                                'sha256': '4274df9247dcec29dacecb50255c60faaadd59acef1e294435ae4aa1fad5b2c0',
                                'review_sha256': 'f919c0adf4c9c066445ab5a4157d5298e28016445537e1dbb6416541bbbbaf92',
                                'arguments': 0},
 'actorScriptPushMessageArgument0': {'address': 2148649180,
                                     'file': 'src/ovl/actor_script_messages_items.c',
                                     'function': 'actorScriptPushMessageArgument0',
                                     'sha256': '9d10e6aaf747218938fd3fc296940d354a8cdef41e96037100de6499df2c86df',
                                     'review_sha256': '2b4fe1b6cd3c0472e1e8f536138b736b1871541c7bf319c2ec058c56fef7b087',
                                     'arguments': 0},
 'actorScriptPushMessageArgument2': {'address': 2148649224,
                                     'file': 'src/ovl/actor_script_messages_items.c',
                                     'function': 'actorScriptPushMessageArgument2',
                                     'sha256': '2cfc1f3c4bbec5cfde02713e288bb86e2fa895b7414599adb9bd28bb03eb8cd4',
                                     'review_sha256': 'b175f4f2bb895e08420d7580891ced0bb82aa8e1cdea2826e65c5547e598fef5',
                                     'arguments': 0},
 'actorScriptPushMessageArgument3': {'address': 2148649268,
                                     'file': 'src/ovl/actor_script_messages_items.c',
                                     'function': 'actorScriptPushMessageArgument3',
                                     'sha256': 'c62b957abc2b69d040ffd82a2faaa1cc536e08feb9087d831f4c26f0c9f1d250',
                                     'review_sha256': '8895aa87b2793929fd4e28001999e7452c3dbce7898f57d81ed13153506a0f81',
                                     'arguments': 0},
 'actorScriptPushMessageArgument4': {'address': 2148649312,
                                     'file': 'src/ovl/actor_script_messages_items.c',
                                     'function': 'actorScriptPushMessageArgument4',
                                     'sha256': '00e259295b5d51a9734f5326fff313627d1653e0effc3102a216b7d59086ba67',
                                     'review_sha256': '70c6e256b4b24b1647c574b84727054b6b0d4384a9372e422e6109c0155407c7',
                                     'arguments': 0},
 'actorScriptCreateAttachedEffect': {'address': 2148649356,
                                     'file': 'src/ovl/actor_script_messages_items.c',
                                     'function': 'actorScriptCreateAttachedEffect',
                                     'sha256': '05d75e3e0665b0db556ab7884ea58b5c229c049e2b54265bca2474b3a4c77358',
                                     'review_sha256': '7e71c56f22afd6eaf160e0572c437536e6c3f1bd0806a8fa04bf867610093043',
                                     'arguments': 2},
 'actorScriptPushTargetApproachDistance': {'address': 2148648564,
                                           'file': 'src/ovl/actor_script_action_commands.c',
                                           'function': 'actorScriptPushTargetApproachDistance',
                                           'sha256': 'ee7652cc24b047c50e99df055d29403c57dacadc495dea7b45c092c76814bf2d',
                                           'review_sha256': '117f9156e3e996b2e05ca2b2957f4bb7fb9cefdbbb67d9b10c13c66c665245b1',
                                           'arguments': 0},
 'actorScriptPushPlayerDistance': {'address': 2148649784,
                                   'file': 'src/ovl/actor_script_messages_items.c',
                                   'function': 'actorScriptPushPlayerDistance',
                                   'sha256': '7bab5626dfb234da4cc7bd194744ff79c21026841cbd257d1dd47bdc94d2b3ea',
                                   'review_sha256': 'ad540217e7ef337c7ee243558874f909aa875f9912c3f4fcc41d5e1aa8d7da06',
                                   'arguments': 0},
 'actorScriptPushPlayerHeldItem': {'address': 2148654960,
                                   'file': 'src/ovl/actor_script_messages_items.c',
                                   'function': 'actorScriptPushPlayerHeldItem',
                                   'sha256': 'dd2d08d9b166f5c009c7ee9a0752a92d1c53c4b82faca3ee4fc5c884b4359d53',
                                   'review_sha256': 'a01f9b4c4ea273e526db44321c05e0f0a9d7b7d74508d2a9c91509b7f417c606',
                                   'arguments': 0},
 'actorScriptGetItemKind': {'address': 2148654392,
                            'file': 'src/ovl/actor_script_messages_items.c',
                            'function': 'actorScriptGetItemKind',
                            'sha256': '5aebe19cf210bd9db8b4d9abae32d37c65f13c4c79fe51a3b73d1a368908331d',
                            'review_sha256': '56d20013f5ff9c2683abc19d163c934d290ac4c9b197648ef7240c1c3ff5bcbc',
                            'arguments': 1},
 'actorScriptFindItemInFacingCone': {'address': 2148653400,
                                     'file': 'src/ovl/actor_script_messages_items.c',
                                     'function': 'actorScriptFindItemInFacingCone',
                                     'sha256': 'e75f34264337661a0958a0e3828027ec53ed841b49ee52ceda710a0463ecb7eb',
                                     'review_sha256': '4d72bd4ae15d2eeb5b33406a0df8f5afa8653f43d17df55589a55f145c520b2a',
                                     'arguments': 6},
 'actorScriptGetObjectHeightDifference': {'address': 2148651656,
                                          'file': 'src/ovl/actor_script_messages_items.c',
                                          'function': 'actorScriptGetObjectHeightDifference',
                                          'sha256': '93c7499f0af6aa253cfaacafcc6e812ccb9e1965fda8254b573b00a61a97ce9b',
                                          'review_sha256': '6c32b502289492d36e71970a85adee73f2cb30a088914e8da5998029416a3963',
                                          'arguments': 1},
 'actorScriptFindObjectInFacingCone': {'address': 2148657376,
                                       'file': 'src/ovl/actor_script_player_world.c',
                                       'function': 'actorScriptFindObjectInFacingCone',
                                       'sha256': 'bc1ee2a91eea11ab2021598709aec5c37be9bccacf272c5a8ce9629196fc3f8c',
                                       'review_sha256': '8405920f5aafdcdcb9e718b0755db2aa5f77d64e1e6ea01da3b3600fc763ac11',
                                       'arguments': 4},
 'actorScriptGetVoiceReactiveSpinnerSpeed': {'address': 2148657820,
                                             'file': 'src/ovl/actor_script_player_world.c',
                                             'function': 'actorScriptGetVoiceReactiveSpinnerSpeed',
                                             'sha256': '184397a18445b45542825a99453bc3429912f85af9066386edbe4937b0fa4f1c',
                                             'review_sha256': '0245eb73b32063ca0982ba61e77ca3c884684fcfbe91da62d524d3004315af60',
                                             'arguments': 1},
 'actorScriptGetObjectType': {'address': 2148654520,
                              'file': 'src/ovl/actor_script_messages_items.c',
                              'function': 'actorScriptGetObjectType',
                              'sha256': '165df538ce6c8e8b54243bd7461f89eaefc45d47b120b530eab7faa6c53adaf6',
                              'review_sha256': '978e95b72d62f43ff396f93adb5f9a939d115351be5907b7d5302f4f4b56ddb8',
                              'arguments': 1},
 'actorScriptFindItemFromPlayerView': {'address': 2148667188,
                                       'file': 'src/ovl/actor_script_collection_profile.c',
                                       'function': 'actorScriptFindItemFromPlayerView',
                                       'sha256': '540c4ab2703cb88c22071fb4cbc4dea71a880e3940cdf54b86d9bd3716115ddb',
                                       'review_sha256': 'ff58bbca23d348283019e5dbfd16c7ea4df564c6af945b25db51a8c8222dc763',
                                       'arguments': 6},
 'actorScriptPushTotalElapsedTime': {'address': 2148661616,
                                     'file': 'src/ovl/actor_script_player_world.c',
                                     'function': 'actorScriptPushTotalElapsedTime',
                                     'sha256': '1d97f7fb3a089fcb674302974023554992ed787a42ec3e3ce9c47ddedeecbf16',
                                     'review_sha256': '554fe92f02256193649ee7f2e71141a68a5c3f4b5c39977d5afae6a7fbbd3e89',
                                     'arguments': 0},
 'actorScriptEnableElapsedTimeTint': {'address': 2148662220,
                                      'file': 'src/ovl/actor_script_collection_profile.c',
                                      'function': 'actorScriptEnableElapsedTimeTint',
                                      'sha256': 'c6591122cc84e3e1b825e99da7022485c858c386e1160d94fd8c55a27d0578f8',
                                      'review_sha256': '545e650d76fed2fe93967c03926dc36a35ac1a1f2968feace1b7b5f88d7a7502',
                                      'arguments': 1},
 'actorScriptPlaySoundEffect': {'address': 2148653104,
                                'file': 'src/ovl/actor_script_messages_items.c',
                                'function': 'actorScriptPlaySoundEffect',
                                'sha256': '36f71810a83e110aa7704d4921fd3718db1fcc683d6963acc01d5b24b1601c7a',
                                'review_sha256': 'c63cb7a7521377a5161362dae638a2972bddfb3d419b727fdd93799fe31e60c4',
                                'arguments': 1},
 'actorScriptSetSoundEffectVolume': {'address': 2148653248,
                                     'file': 'src/ovl/actor_script_messages_items.c',
                                     'function': 'actorScriptSetSoundEffectVolume',
                                     'sha256': '4af13738718296136991459bf9cd7f48bdc2bcd10b3ae0f502908ca802e8778a',
                                     'review_sha256': 'b575c5a5f70184b3aaf43b03fd3b53ef7c3d9789fd0a4b0f6ba3ed1908b5fc96',
                                     'arguments': 2},
 'actorScriptSetCueRate': {'address': 2148652904,
                           'file': 'src/ovl/actor_script_messages_items.c',
                           'function': 'actorScriptSetCueRate',
                           'sha256': 'cbce9f004019421ae514cedf03abe6a88f1de8f624b4ab98d8ca8f3965b23900',
                           'review_sha256': '1fca6231b8900c18cd03deb37b0316060d4214679848d57a39bf1b274f2626ba',
                           'arguments': 1}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptPushOriginDistance', 'actorScriptPushPikachuSecondaryMeter', 'actorScriptPushMovementCollisionFlags', 'actorScriptAcknowledgeMessage', 'actorScriptPushMessageType', 'actorScriptPushMessageArgument0', 'actorScriptPushMessageArgument2', 'actorScriptPushMessageArgument3', 'actorScriptPushMessageArgument4', 'actorScriptCreateAttachedEffect', 'actorScriptPushTargetApproachDistance', 'actorScriptPushPlayerDistance', 'actorScriptPushPlayerHeldItem', 'actorScriptGetItemKind', 'actorScriptFindItemInFacingCone', 'actorScriptGetObjectHeightDifference', 'actorScriptFindObjectInFacingCone', 'actorScriptGetVoiceReactiveSpinnerSpeed', 'actorScriptGetObjectType', 'actorScriptFindItemFromPlayerView', 'actorScriptPushTotalElapsedTime', 'actorScriptEnableElapsedTimeTint', 'actorScriptPlaySoundEffect', 'actorScriptSetSoundEffectVolume', 'actorScriptSetCueRate'))
SUPPORT_BINDINGS = {'objectGetOriginDistance': {'file': 'src/ovl/actor_script_queries.c',
                             'sha256': 'aadbdf9948421a0f009dc078929ae7ab7c946a9ec8c81c6af25ab2fa76cc3501'},
 'objectGetPlayerDistance': {'file': 'src/ovl/actor_script_queries.c',
                             'sha256': '9996e202ab844d49dc5d8a5c963d6bcd60ecfa50a815d0af64e49ef34f3d9aaa'},
 'actorGetMovementCollisionFlags': {'file': 'src/ovl/actor_script_queries.c',
                                    'sha256': '87240bf69973e625afe723b2239084ed2c26c14ad5d666ff0e79166c48aeb0fd'},
 'sceneRegionIsHighlightedByHandle': {'file': 'src/ovl/actor_script_queries.c',
                                      'sha256': 'cb9996b98596346ad09f73c36e3dbeeebd6bca1e863af33661ba30eb85711e78'},
 'actorScriptPushTargetApproachDistance': {'file': 'src/ovl/actor_script_action_commands.c',
                                           'sha256': 'ee7652cc24b047c50e99df055d29403c57dacadc495dea7b45c092c76814bf2d'},
 'actorScriptGetVoiceReactiveSpinnerSpeed': {'file': 'src/ovl/actor_script_player_world.c',
                                             'sha256': '184397a18445b45542825a99453bc3429912f85af9066386edbe4937b0fa4f1c'}}
REQUIRED_SUPPORT_NAMES = frozenset(('objectGetOriginDistance', 'objectGetPlayerDistance', 'actorGetMovementCollisionFlags', 'sceneRegionIsHighlightedByHandle', 'actorScriptPushTargetApproachDistance', 'actorScriptGetVoiceReactiveSpinnerSpeed'))

FLAG_PREDICATES = {
    1: 'Sampled movement collision flags are nonzero.',
    2: 'Acknowledged pending value is exactly one; captured category is outside 13, 14 and 15.',
    4: 'A gated collision accumulator check records more than two nearly unchanged origin-radius samples; this does not prove immobility.',
    8: 'Sampled remaining target approach distance is below 5.',
    16: 'A separate remaining target approach-distance sample is below 15.',
    32: 'Sampled approximate player distance is below 15.',
    64: 'A separate approximate player-distance sample is below 30.',
    128: 'Sampled player-held kind is nonzero and another player-distance sample is below 30; kind and handle samples need not be atomic.',
    256: 'Sampled approximate player distance is at least 60.',
    512: 'Actor-origin weighted world-item search returns a handle and absolute actor-relative height difference is at most 10; search can reset item priority.',
    1024: 'Nearby spinner search and speed query report positive speed, under the native numeric and handle contracts.',
    2048: 'The integer counter in mapped root role 21 is greater than 50; gameplay meaning remains unresolved.',
    4096: 'The integer counter in mapped root role 22 is less than 10; gameplay meaning remains unresolved.',
    8192: 'Acknowledged pending value is exactly one; captured category is 13, 14 or 15.',
    16384: 'Player-view world-item search returns a handle and absolute actor-relative height difference is at most 10.',
    32768: 'First elapsed-limit enable is nonzero and sampled elapsed time is strictly greater than its limit; this path can latch warning, tint, sound and rate effects.',
    65536: 'Second elapsed-limit enable is nonzero and sampled elapsed time is strictly greater than its limit.',
}
KNOWN_FLAG_MASKS = frozenset(FLAG_PREDICATES) - {2048, 4096}


def _digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def _operands(row, names):
    """Validate decoded widths and metadata before any semantic normalization."""
    opcode = row.get('opcode')
    if type(opcode) is not int or names.get(opcode) != row.get('name'):
        raise ValueError('Opcode and authoritative source spelling disagree.')
    operands = row.get('operands', [])
    if len(operands) != VM_OPERANDS.get(opcode, 0) or type(row.get('size')) is not int or row['size'] != 1 + 5 * len(operands):
        raise ValueError('Decoded arity or byte size disagrees with the VM format.')
    values, tags = [], []
    for operand in operands:
        tag, value = operand.get('tag'), operand.get('value')
        if type(tag) is not int or tag not in (0, 1, 2):
            raise ValueError('Unsupported boxed tag in snapshot producer.')
        if tag == 1:
            if operand.get('kind') != 'float' or type(value) is not float or not math.isfinite(value):
                raise ValueError('Invalid finite floating operand.')
            try:
                raw = struct.pack('>f', value)
            except (OverflowError, struct.error) as error:
                raise ValueError('Floating operand exceeds binary32.') from error
            if struct.unpack('>f', raw)[0] != value:
                raise ValueError('Floating operand is not a decoded binary32 value.')
        else:
            if operand.get('kind') != ('integer' if tag == 0 else 'code_reference') or type(value) is not int or not -2147483648 <= value <= 2147483647:
                raise ValueError('Invalid signed32 encoded operand.')
            raw = struct.pack('>i', value)
        if 'payload_hex' in operand and operand['payload_hex'] != raw.hex():
            raise ValueError('Decoded value and original operand bits disagree.')
        values.append(value)
        tags.append(tag)
    name = row['name']
    if name in ('SCRIPT_OP_CALL', 'SCRIPT_OP_CALL_NATIVE', 'SCRIPT_OP_JUMP', 'SCRIPT_OP_JUMP_IF_ZERO'):
        if tags != [2] or values[0] < 0:
            raise ValueError('Control/native selector requires one nonnegative encoded reference.')
    elif name in ('SCRIPT_OP_LOAD_VARIABLE', 'SCRIPT_OP_STORE_VARIABLE', 'SCRIPT_OP_LOAD_ELEMENT', 'SCRIPT_OP_STORE_ELEMENT'):
        if tags != [0, 0] or not -1 <= values[0] < 32 or values[1] < 0:
            raise ValueError('Invalid frame/slot coordinates.')
    elif name == 'SCRIPT_OP_ENTER_FRAME' and tags != [0, 0, 0]:
        raise ValueError('Frame declaration requires integer operands.')
    if name == 'SCRIPT_OP_CALL_NATIVE':
        native = row.get('native', {})
        functions = native.get('functions')
        if (type(row.get('native_selector')) is not int or row['native_selector'] != values[0] or
                not isinstance(functions, list) or len(functions) != 1 or not isinstance(functions[0], str) or
                type(native.get('address')) is not int or not 0 <= native['address'] <= 0xFFFFFFFF or
                native.get('inline_operands') != []):
            raise ValueError('Native metadata and selector disagree.')
    return values, tags


def _edges(row, values):
    following = row['offset'] + row['size']
    name = row['name']
    if name == 'SCRIPT_OP_RETURN':
        return []
    if name == 'SCRIPT_OP_JUMP':
        return [dict(kind='branch', offset=values[0])]
    if name == 'SCRIPT_OP_JUMP_IF_ZERO':
        return [dict(kind='fallthrough', offset=following), dict(kind='branch', offset=values[0])]
    if name == 'SCRIPT_OP_CALL':
        return [dict(kind='return_continuation', offset=following), dict(kind='call', offset=values[0])]
    return [dict(kind='resume_or_fallthrough' if name == 'SCRIPT_OP_CALL_NATIVE' else 'fallthrough', offset=following)]


def _validate_sequence(rows, incoming, names):
    if not rows or any(type(row.get('offset')) is not int or not 0 <= row['offset'] <= 2147483647 for row in rows):
        raise ValueError('Invalid instruction offsets.')
    expected_incoming, operands = {}, []
    for index, row in enumerate(rows):
        values, tags = _operands(row, names)
        if index and rows[index - 1]['offset'] + rows[index - 1]['size'] != row['offset']:
            raise ValueError('Instruction span is not contiguous.')
        edges = _edges(row, values)
        if row.get('successors') != edges:
            raise ValueError('Decoded control-flow edges disagree with the complete instruction.')
        for edge in edges:
            expected_incoming.setdefault(edge['offset'], set()).add(row['offset'])
        operands.append((values, tags))
    for row in rows[1:]:
        if incoming.get(row['offset'], set()) != expected_incoming.get(row['offset'], set()):
            raise ValueError('Competing control-flow entry into the reviewed interior.')
    return operands


def _absolute_helper(target, all_rows, by, incoming, names):
    """Independently validate each actual 65-byte signed numeric absolute helper."""
    if target not in by:
        raise ValueError('Absolute-value helper is missing.')
    rows = all_rows[by[target]:by[target] + 10]
    operands = _validate_sequence(rows, incoming, names)
    expected_names = ('ENTER_FRAME', 'LOAD_VARIABLE', 'PUSH_LITERAL', 'LESS_THAN', 'JUMP_IF_ZERO',
                      'LOAD_VARIABLE', 'NEGATE', 'RETURN', 'LOAD_VARIABLE', 'RETURN')
    if len(rows) != 10 or [r['name'] for r in rows] != ['SCRIPT_OP_' + n for n in expected_names]:
        raise ValueError('CALL does not name the complete absolute-value helper.')
    expected = [[1, 1, 1], [1, 0], [0], [], [rows[8]['offset']], [1, 0], [], [], [1, 0], []]
    if [values for values, _ in operands] != expected or rows[-1]['offset'] + rows[-1]['size'] - target != 65:
        raise ValueError('Absolute-value helper operands or extent differ.')
    if any(tags != ([2] if i == 4 else [0] * len(expected[i])) for i, (_, tags) in enumerate(operands)):
        raise ValueError('Absolute-value helper boxed tags differ.')
    return dict(entry=target, bytes=65, instruction_offsets=[r['offset'] for r in rows],
                scope='Numeric input must satisfy the original comparison/conversion contract; integer NEGATE excludes INT_MIN. No termination, numeric type or caller-floor proof.')


def _normalize_prefix(rows, all_rows, *, native_bindings=None):
    """Normalize a complete candidate without relaxing aliases or nonroot values.

    This helper accepts small authored fixtures for testing the normalizer. Only
    the public recognizer compares the full production extent and frozen digest.
    """
    native_bindings = NATIVE_BINDINGS if native_bindings is None else native_bindings
    if any(type(r.get('offset')) is not int for r in all_rows):
        raise ValueError('Invalid decoded instruction offset.')
    by = {r['offset']: index for index, r in enumerate(all_rows)}
    if len(by) != len(all_rows) or any(a['offset'] >= b['offset'] for a, b in zip(all_rows, all_rows[1:])):
        raise ValueError('Duplicate or unordered decoded instructions.')
    incoming = incoming_edges(all_rows)
    names = opcode_names('vm')
    operands = _validate_sequence(rows, incoming, names)
    end = rows[-1]['offset'] + rows[-1]['size']
    indices = {r['offset']: i for i, r in enumerate(rows)}
    indices[end] = len(rows)
    if {r['offset'] for r in all_rows if rows[0]['offset'] <= r['offset'] < end} != set(indices) - {end}:
        raise ValueError('Competing overlapping decoded instruction.')
    roots, normalized, helpers, selectors = {}, [], {}, {}
    for row, (values, tags) in zip(rows, operands):
        name = row['name']
        encoded = values[:]
        if name in ('SCRIPT_OP_LOAD_VARIABLE', 'SCRIPT_OP_STORE_VARIABLE', 'SCRIPT_OP_LOAD_ELEMENT', 'SCRIPT_OP_STORE_ELEMENT') and values[0] == 0:
            roots.setdefault(values[1], len(roots))
            encoded = [0, 'root-role-' + str(roots[values[1]])]
        elif name in ('SCRIPT_OP_JUMP', 'SCRIPT_OP_JUMP_IF_ZERO'):
            if values[0] not in indices:
                raise ValueError('Branch leaves the reviewed prefix or enters an operand.')
            encoded = ['instruction', indices[values[0]]]
        elif name == 'SCRIPT_OP_CALL':
            helpers[values[0]] = _absolute_helper(values[0], all_rows, by, incoming, names)
            encoded = ['independently_checked_abs_helper']
        elif name == 'SCRIPT_OP_CALL_NATIVE':
            native = row['native']
            function, address = native['functions'][0], native['address']
            binding = native_bindings.get(function)
            if binding is None or binding['address'] != address:
                raise ValueError('Native name/address is outside the reviewed binding set.')
            if values[0] in selectors and selectors[values[0]] != (function, address):
                raise ValueError('One native selector resolves to inconsistent bindings.')
            selectors[values[0]] = (function, address)
            encoded = [function, address, *values[1:]]
        normalized.append([name, encoded, tags])
    digest = hashlib.sha256(json.dumps(normalized, separators=(',', ':')).encode()).hexdigest()
    return dict(sha256=digest, root_role_mapping=roots, absolute_helpers=list(helpers.values()),
                normalized=normalized, end_exclusive=end)


def _flag_names(root):
    code = code_only((root / 'include/companion_snapshot_flags.h').read_text())
    names = _reviewed_enum(code, 'CompanionSnapshotFlag', 'COMPANION_SNAPSHOT_', FLAG_MAPPING_SHA256)
    if set(names) != KNOWN_FLAG_MASKS:
        raise ValueError('Incomplete or unexpected partial snapshot flag domain.')
    return names


def _source_bindings(root, snapshot):
    """Require fresh expanded native dependencies, VM and independently read source."""
    errors, files = [], {}
    if set(NATIVE_BINDINGS) != REQUIRED_NATIVE_NAMES or set(SUPPORT_BINDINGS) != REQUIRED_SUPPORT_NAMES:
        return dict(status='unresolved', reasons=['Incomplete source binding set.'])
    reviews, preservation = context_snapshot(root=root) if snapshot is None else snapshot
    vm = preservation.get('context_vm_contract', {})
    if preservation.get('stale_vm_functions') or not vm.get('current'):
        errors.append('Stale full VM source contract.')
    for name, binding in NATIVE_BINDINGS.items():
        review = reviews.get('active', {}).get(name)
        if review is None or _digest(review) != binding['review_sha256'] or review.get('arguments') != binding['arguments']:
            errors.append('Stale expanded native dependencies: ' + name)
    for name, binding in list(NATIVE_BINDINGS.items()) + list(SUPPORT_BINDINGS.items()):
        path = binding['file']
        try:
            if path not in files:
                files[path] = (root / path).read_text()
            if fingerprint(function_source(files[path], name)) != binding['sha256']:
                errors.append('Stale source function: ' + name)
        except (OSError, ValueError):
            errors.append('Missing source function: ' + name)
    return dict(status='unresolved' if errors else 'current', reasons=errors,
                native_review_sha256={name: binding['review_sha256'] for name, binding in NATIVE_BINDINGS.items()},
                function_sha256={name: binding['sha256'] for name, binding in list(NATIVE_BINDINGS.items()) + list(SUPPORT_BINDINGS.items())},
                vm_implementation_binding=vm)


def companion_snapshots(instructions, *, snapshot=None, root=ROOT):
    """Report only complete canonical prefixes under an ephemeral current source snapshot.

    Catalog callers may reuse their already validated native/VM snapshot within
    one analysis, never across source mutations. Source functions and the partial
    enum are checked against this root on every call; no cross-root cache is kept.
    """
    scope = ('Complete authored companion snapshot prefix only. Repeated samples are not atomic; '
             'mailbox acknowledgement, weighted item search and timer warning paths have effects. '
             'Later extensions, including 0x1000000, remain outside this recognition.')
    bindings = _source_bindings(root, snapshot)
    result = dict(status='unresolved', snapshots=[], flags=[], source_bindings=bindings, scope=scope)
    if bindings['status'] != 'current':
        return result
    try:
        names = _flag_names(root)
    except (OSError, ValueError) as error:
        result['reason'] = str(error)
        return result
    result['status'] = 'conditional_snapshots'
    result['flag_mapping_sha256'] = FLAG_MAPPING_SHA256
    result['flags'] = [dict(mask=mask, enum_name=names.get(mask), predicate=predicate,
                           naming_status='named' if mask in names else 'unresolved_counter_role')
                       for mask, predicate in sorted(FLAG_PREDICATES.items())]
    for index, row in enumerate(instructions):
        if row.get('name') != 'SCRIPT_OP_ENTER_FRAME':
            continue
        rows = instructions[index:index + PREFIX_INSTRUCTIONS]
        if len(rows) != PREFIX_INSTRUCTIONS:
            continue
        try:
            if [o.get('value') for o in rows[0].get('operands', [])] != [0, 4, 1]:
                continue
            if rows[-1].get('name') != 'SCRIPT_OP_STORE_VARIABLE' or rows[-1]['offset'] + rows[-1]['size'] - rows[0]['offset'] != PREFIX_BYTES:
                continue
            normalized = _normalize_prefix(rows, instructions)
            if normalized['sha256'] != PREFIX_SHA256:
                continue
        except (ValueError, KeyError, TypeError, IndexError, OverflowError, struct.error):
            continue
        roles = {role: slot for slot, role in normalized['root_role_mapping'].items()}
        writes = []
        for a, b, c, d in zip(rows, rows[1:], rows[2:], rows[3:]):
            if (a['name'] == 'SCRIPT_OP_LOAD_VARIABLE' and [o['value'] for o in a['operands']] == [0, roles[0]] and
                    b['name'] == 'SCRIPT_OP_PUSH_LITERAL' and b['operands'][0]['tag'] == 0 and b['operands'][0]['value'] in FLAG_PREDICATES and
                    c['name'] == 'SCRIPT_OP_ADD' and d['name'] == 'SCRIPT_OP_STORE_VARIABLE' and
                    [o['value'] for o in d['operands']] == [0, roles[0]]):
                writes.append(dict(mask=b['operands'][0]['value'], store_offset=d['offset']))
        result['snapshots'].append(dict(
            entry_offset=rows[0]['offset'], prefix_end_exclusive=normalized['end_exclusive'],
            instruction_count=PREFIX_INSTRUCTIONS, bytes=PREFIX_BYTES, canonical_sha256=PREFIX_SHA256,
            frame_contract=dict(arguments=0, total_local_cells=4, frame_index=1, local_count_includes_arguments=True),
            root_role_mapping=normalized['root_role_mapping'], status_variable=[0, roles[0]],
            unresolved_counter_predicates=[dict(mask=2048, variable=[0, roles[21]], comparison='greater_than', threshold=50),
                                           dict(mask=4096, variable=[0, roles[22]], comparison='less_than', threshold=10)],
            flag_write_sites=writes, absolute_helpers=normalized['absolute_helpers'],
            instruction_offsets=[r['offset'] for r in rows],
            preconditions=['Normal entry, valid initialized root locals, live frames/tables and enough nonwrapping operand storage with separately established caller floor.',
                'Source/root/own-frame/native workspaces and saved metadata are disjoint; native dependency and resource/handle/lifetime contracts all apply.',
                'Numeric tags and conversions/comparisons/arithmetic are valid and finite where required; integer absolute-value NEGATE excludes INT_MIN.',
                'No asynchronous mutation, callback rebinding or root restart invalidates the sampled storage. Separate native samples need not agree or extend handle lifetimes.',
                'The distinct low masks are added at most once by this producer under its integer/storage contracts; this does not authorize replacing arbitrary addition with bitwise OR.'],
            scope=scope))
    return result
