"""Source-bound inventories of authored passage-access requests.

Request operands are observations, not proof of a live registered door, successful
transition, reachability or a stable relationship to a nearby object handle.
"""

import hashlib

from .callee_contexts import context_snapshot
from .context import ROOT
from .effects import enum_names
from .stack import incoming_edges, recover_arguments
from .request_arguments import (PRODUCER_BINDINGS, REQUIRED_PRODUCERS, _digest,
                                validate_instructions as _validate, native_producer_validator)

NATIVE_NAME = 'actorScriptSetDoorAccessState'
NATIVE_ADDRESS = 0x80120408
NATIVE_REVIEW_SHA256 = 'ec82906b443a5a0ab37c9ed4840281c01a10cbea78dec04753eccb7fbe8b71d9'
HEADER = 'include/passage_types.h'
HEADER_SHA256 = '6b51041e29b3db7ba2ffe6005dbf1ff6b6b7a01a0ba7f89307beaa1bf213bcea'
EXPECTED_NAMES = {0: 'PASSAGE_ACCESS_ALLOWED', 1: 'PASSAGE_ACCESS_LOCKED',
                  2: 'PASSAGE_ACCESS_UNAVAILABLE', 3: 'PASSAGE_ACCESS_REQUIRES_PIKACHU',
                  4: 'PASSAGE_ACCESS_BLOCKED_BY_SQUIRTLE'}
SCOPE = ('Conditional authored requests only. Registry bounds, live initialized PassageState, '
         'integer conversions, protected operand floor and disjoint storage remain preconditions. '
         'No nearby-handle identity, execution, transition or success proof; boxed zero is not success.')


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
        native_validator = native_producer_validator(
            reviews, bindings=PRODUCER_BINDINGS, required=REQUIRED_PRODUCERS)

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
