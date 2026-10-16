"""Source-bound inventories of authored passage-access requests.

Request operands are observations, not proof of a live registered door, successful
transition, reachability or a stable relationship to a nearby object handle.
"""

import hashlib
import json
import math

from .callee_contexts import context_snapshot
from .context import ROOT, opcode_names
from .effects import enum_names
from .formats import VM_OPERANDS
from .stack import incoming_edges, recover_arguments

NATIVE_NAME = 'actorScriptSetDoorAccessState'
NATIVE_ADDRESS = 0x80120408
NATIVE_REVIEW_SHA256 = 'ec82906b443a5a0ab37c9ed4840281c01a10cbea78dec04753eccb7fbe8b71d9'
HEADER = 'include/passage_types.h'
HEADER_SHA256 = '6b51041e29b3db7ba2ffe6005dbf1ff6b6b7a01a0ba7f89307beaa1bf213bcea'
EXPECTED_NAMES = {0: 'PASSAGE_ACCESS_ALLOWED', 1: 'PASSAGE_ACCESS_LOCKED',
                  2: 'PASSAGE_ACCESS_UNAVAILABLE', 3: 'PASSAGE_ACCESS_REQUIRES_PIKACHU',
                  4: 'PASSAGE_ACCESS_BLOCKED_BY_SQUIRTLE'}
PRODUCER_BINDINGS = {'actorScriptGetAreaPathPointCoordinate': {'address': 2148656920,
                                           'arguments': 3,
                                           'review_sha256': '86b5998903dea69b9c71670ac35eef019d4ce209d0e670e6984b5e644644cad5'},
 'actorScriptGetItemKind': {'address': 2148654392,
                            'arguments': 1,
                            'review_sha256': '0a3aba3e1d052d4646c7cfe33d7c6b4f37fa012f076275644b4db0546857499b'},
 'actorScriptGetPlayerCoordinate': {'address': 2148655112,
                                    'arguments': 1,
                                    'review_sha256': 'e35e10efdcabe31324333f82f911f78df26e38aa4727d5444f8342c4344033bc'},
 'actorScriptGetProgressMaximum': {'address': 2148664200,
                                   'arguments': 1,
                                   'review_sha256': 'fc05f68cad900af1f5955c6b93bb309e86228108814a391163fdb1207e1c609a'},
 'actorScriptGetProgressMinimum': {'address': 2148664264,
                                   'arguments': 1,
                                   'review_sha256': '75216fd80f8765106304ac9fd80336c0e1b670f41e149ce972b748dabd26b2ec'},
 'actorScriptGetProgressValue': {'address': 2148664136,
                                 'arguments': 1,
                                 'review_sha256': '8a9f3b4f5d82dd265972330928136d5da8ac1e6edfebed3a4748f858bc182813'},
 'actorScriptGetTargetCoordinate': {'address': 2148651920,
                                    'arguments': 1,
                                    'review_sha256': '54ae646b94cbc82d1f6a23642bf31c1cb718f212f569a092f1afa64bbcf174c1'},
 'actorScriptGetWorldCoordinate': {'address': 2148651992,
                                   'arguments': 1,
                                   'review_sha256': 'c19b99d4917df49aa4d116f2cfe63306fa993158397b8da01d3d0b83efe9380d'},
 'actorScriptPassThroughInteger': {'address': 2148647536,
                                   'arguments': 1,
                                   'review_sha256': '7ce5019c6dafb96114d27892f09383b0f4af354c265b840c4a24c41823eb16d8'},
 'actorScriptPushHeldItemHandle': {'address': 2148660596,
                                   'arguments': 0,
                                   'review_sha256': '8597a859e3feb8f70f77b6a5304a86a849ded5af866731b7cfb892a74425bc4a'},
 'actorScriptPushHeldItemKind': {'address': 2148650980,
                                 'arguments': 0,
                                 'review_sha256': '52d0cc64a3a178b9d529d35b13d2deb5e2280dc24cd3ae4b285f218d62dd12cd'},
 'actorScriptPushPikachuHandle': {'address': 2148656756,
                                  'arguments': 0,
                                  'review_sha256': '0d95a63eeefbd3466229e178543e1d0732fa670db6f3e3cfbae0ff615447f583'},
 'actorScriptPushSelfHandle': {'address': 2148656808,
                               'arguments': 0,
                               'review_sha256': '3b9a4986214d4e50d7080f0ef63dc6b08a12bd0af3836ba1a19df10c3aef8d1f'},
 'actorScriptPushTargetDistanceAllowance': {'address': 2148654028,
                                            'arguments': 0,
                                            'review_sha256': '44b1b5ffd9da55399f98ab1a0dfb68b72e7eec0754913ab93c0c75da000f625e'},
 'actorScriptPushTargetHandle': {'address': 2148654072,
                                 'arguments': 0,
                                 'review_sha256': 'ba53ecbc2dfd1f51620895335c1ba1c5b9ad6a8094820d721c0df2fd6223691e'},
 'actorScriptRandomBelow': {'address': 2148647408,
                            'arguments': 1,
                            'review_sha256': '45206f67a57091cbca5b17b2e640a6618c518b8c7461fc7899614bf7630e4a8f'}}
REQUIRED_PRODUCERS = frozenset(('actorScriptGetAreaPathPointCoordinate', 'actorScriptGetItemKind', 'actorScriptGetPlayerCoordinate', 'actorScriptGetProgressMaximum', 'actorScriptGetProgressMinimum', 'actorScriptGetProgressValue', 'actorScriptGetTargetCoordinate', 'actorScriptGetWorldCoordinate', 'actorScriptPassThroughInteger', 'actorScriptPushHeldItemHandle', 'actorScriptPushHeldItemKind', 'actorScriptPushPikachuHandle', 'actorScriptPushSelfHandle', 'actorScriptPushTargetDistanceAllowance', 'actorScriptPushTargetHandle', 'actorScriptRandomBelow'))

SCOPE = ('Conditional authored requests only. Registry bounds, live initialized PassageState, '
         'integer conversions, protected operand floor and disjoint storage remain preconditions. '
         'No nearby-handle identity, execution, transition or success proof; boxed zero is not success.')


def _digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def _validate(rows):
    names = opcode_names('vm')
    previous = -1
    offsets = {row.get('offset') for row in rows if type(row.get('offset')) is int}
    end = rows[-1].get('offset', 0) + rows[-1].get('size', 0) if rows else 0
    for row in rows:
        offset, opcode, size = (row.get(k) for k in ('offset', 'opcode', 'size'))
        if type(offset) is not int or offset <= previous or offset < 0 or type(opcode) is not int or opcode not in names or names.get(opcode) != row.get('name'):
            raise ValueError('Malformed instruction ordering or opcode.')
        operands = row.get('operands')
        if not isinstance(operands, list) or type(size) is not int:
            raise ValueError('Malformed operands or size.')
        arity = VM_OPERANDS.get(opcode, 0)
        if opcode == 27 and row.get('native', {}).get('inline_operands'):
            arity += len(row['native']['inline_operands'])
        if len(operands) != arity or size != 1 + 5 * arity:
            raise ValueError('Malformed instruction arity.')
        previous = offset + size - 1
        for operand in operands:
            tag, value = operand.get('tag'), operand.get('value')
            kinds = {0: 'integer', 1: 'float', 2: 'code_reference', 3: 'string_index', 4: 'borrowed_string_pointer'}
            if type(tag) is not int or tag not in kinds or operand.get('kind') != kinds.get(tag):
                raise ValueError('Malformed operand tag.')
            if tag == 1:
                if type(value) not in (int, float) or not math.isfinite(value):
                    raise ValueError('Nonfinite numeric operand.')
            elif type(value) is not int or not (-2147483648 <= value <= (4294967295 if tag == 4 else 2147483647)):
                raise ValueError('Malformed integer operand.')
        if opcode in (10, 25, 32, 33, 34) and any(o['tag'] != 0 for o in operands):
            raise ValueError('Frame operands require integer tags.')
        if opcode in (10, 25, 33, 34) and not (-1 <= operands[0]['value'] < 32 and operands[1]['value'] >= 0):
            raise ValueError('Invalid frame coordinate.')
        if opcode == 32 and not (0 <= operands[0]['value'] <= operands[1]['value'] and 0 <= operands[2]['value'] < 32):
            raise ValueError('Invalid frame entry.')
        if opcode in (26, 27, 28, 29) and (operands[0]['tag'] != 2 or operands[0]['value'] < 0):
            raise ValueError('Dispatch/branch requires code-reference tag.')
        edges = row.get('successors')
        if not isinstance(edges, list) or any(type(e.get('offset')) is not int or e['offset'] < 0 or not isinstance(e.get('kind'), str) for e in edges):
            raise ValueError('Malformed control-flow edges.')
        if any(e['offset'] not in offsets and not (e['offset'] == end and e['kind'] in ('fallthrough', 'resume_or_fallthrough', 'return_continuation')) for e in edges):
            raise ValueError('Incomplete control-flow graph.')
        following = offset + size
        if opcode == 27:
            expected = [dict(kind='resume_or_fallthrough', offset=following)]
        elif opcode == 26:
            expected = [dict(kind='return_continuation', offset=following), dict(kind='call', offset=operands[0]['value'])]
        elif opcode == 28:
            expected = [dict(kind='branch', offset=operands[0]['value'])]
        elif opcode == 29:
            expected = [dict(kind='fallthrough', offset=following), dict(kind='branch', offset=operands[0]['value'])]
        elif opcode == 31:  # DISCARD, not RETURN.
            expected = [dict(kind='fallthrough', offset=following)]
        elif row['name'] in ('SCRIPT_OP_RETURN', 'SCRIPT_OP_FINISH'):
            expected = []
        else:
            expected = [dict(kind='fallthrough', offset=following)]
        if edges != expected:
            raise ValueError(f'Unsupported control-flow shape at {offset}: {edges!r}.')


def passage_access_requests(instructions, *, snapshot=None):
    """Inventory exact native requests under one fresh source-validation snapshot."""
    unresolved = dict(status='unresolved', requests=[], scope=SCOPE)
    try:
        reviews, preservation = context_snapshot() if snapshot is None else snapshot
        vm = preservation.get('context_vm_contract', {})
        review = reviews.get('active', {}).get(NATIVE_NAME)
        if preservation.get('stale_vm_functions') or vm.get('current') is not True:
            raise ValueError('Stale VM context contract.')
        if review is None or _digest(review) != NATIVE_REVIEW_SHA256 or type(review.get('arguments')) is not int or review['arguments'] != 2:
            raise ValueError('Stale or unsupported passage native contract.')
        header = ROOT / HEADER
        if hashlib.sha256(header.read_bytes()).hexdigest() != HEADER_SHA256 or enum_names(header, 'PASSAGE_ACCESS_') != EXPECTED_NAMES:
            raise ValueError('Stale or unsupported passage enum.')
        _validate(instructions)
        incoming = incoming_edges(instructions)
        if set(PRODUCER_BINDINGS) != REQUIRED_PRODUCERS:
            raise ValueError('Incomplete or unexpected native producer binding set.')

        def native_validator(producer):
            native = producer.get('native', {})
            functions = native.get('functions', [])
            if len(functions) != 1 or functions[0] not in PRODUCER_BINDINGS:
                return False
            binding = PRODUCER_BINDINGS[functions[0]]
            active = reviews.get('active', {}).get(functions[0])
            return (type(native.get('address')) is int and native['address'] == binding['address']
                    and native.get('inline_operands', []) == [] and producer['size'] == 6
                    and active is not None and type(active.get('arguments')) is int
                    and active['arguments'] == binding['arguments']
                    and _digest(active) == binding['review_sha256'])

        requests = []
        for index, row in enumerate(instructions):
            native = row.get('native', {})
            if NATIVE_NAME not in native.get('functions', []):
                continue
            if row['name'] != 'SCRIPT_OP_CALL_NATIVE' or native.get('functions') != [NATIVE_NAME] or (type(native.get('address')) is not int or native['address'] != NATIVE_ADDRESS) or row['size'] != 6:
                raise ValueError('Rebound or malformed passage native request.')
            observed = dict(instruction_offset=row['offset'], native_selector=row['operands'][0]['value'],
                            native_address=NATIVE_ADDRESS, arguments=None, access_state_name=None,
                            registry_bounds_proven=False, actual_write_proven=False, result_semantics='boxed integer zero; not success')
            try:
                arguments = recover_arguments(instructions, index, 2, incoming=incoming, allow_float=True, allow_symbolic=True, native_validator=native_validator)
                observed['arguments'] = dict(created_door_index=arguments[0], access_state=arguments[1])
                # Only an original direct integer literal receives an enum label.
                prior = instructions[index - 1] if index else {}
                if prior.get('name') == 'SCRIPT_OP_PUSH_LITERAL' and prior['operands'][0]['tag'] == 0:
                    observed['access_state_name'] = EXPECTED_NAMES.get(prior['operands'][0]['value'])
            except (ValueError, KeyError, IndexError, TypeError) as error:
                observed['argument_recovery_reason'] = str(error)
            requests.append(observed)
        return dict(status='conditional_requests', requests=requests, scope=SCOPE,
                    native_review_sha256=NATIVE_REVIEW_SHA256, header_sha256={HEADER: HEADER_SHA256}, vm_implementation_binding=vm)
    except (ValueError, KeyError, IndexError, TypeError, OSError) as error:
        return dict(unresolved, reason=str(error))
