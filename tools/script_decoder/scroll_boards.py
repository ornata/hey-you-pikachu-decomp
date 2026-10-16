"""Source-bound inventories of authored scrolling-board creation requests.

Raw expressions retain their write epochs. This does not prove allocation,
rendering, resource validity, reachability or retained ownership/lifetime.
"""

from .callee_contexts import context_snapshot
from .request_arguments import (PRODUCER_BINDINGS, REQUIRED_PRODUCERS, _digest,
                                validate_instructions, native_producer_validator)
from .stack import incoming_edges, recover_arguments

NATIVE_NAME = 'actorScriptCreateScrollBoard'
NATIVE_ADDRESS = 0x801209E8
NATIVE_REVIEW_SHA256 = '1a38c22efae0945458c734de71fa68f8972cf03b5c5d3ced1dfe94f543d8234b'
ARGUMENT_NAMES = ('preset', 'style', 'columns', 'lines', 'x', 'y', 'z',
                  'rotation_x', 'rotation_y', 'rotation_z', 'red', 'green', 'blue',
                  'alpha', 'leading_outline', 'trailing_outline', 'scale', 'speed')
FLOAT_ARGUMENTS = frozenset(('x', 'y', 'z', 'rotation_x', 'rotation_y', 'rotation_z', 'scale', 'speed'))
SCOPE = ('Conditional authored creation requests only. Operand floor, VM conversions, '
         'initialized resource tables, dimensions, allocation, disjoint storage and live '
         'ownership remain prerequisites. No execution, successful creation, rendering, '
         'reachability or later lifetime proof. Variables are original inputs, not substituted values.')


def scroll_board_requests(instructions, *, snapshot=None):
    """Preserve eighteen pushed expressions under one fresh source snapshot."""
    unresolved = dict(status='unresolved', requests=[], scope=SCOPE)
    try:
        reviews, preservation = context_snapshot() if snapshot is None else snapshot
        vm = preservation.get('context_vm_contract', {})
        review = reviews.get('active', {}).get(NATIVE_NAME)
        if preservation.get('stale_vm_functions') or vm.get('current') is not True:
            raise ValueError('Stale VM context contract.')
        if (review is None or _digest(review) != NATIVE_REVIEW_SHA256
                or type(review.get('arguments')) is not int or review['arguments'] != 18):
            raise ValueError('Stale or unsupported scroll-board native contract.')
        validate_instructions(instructions)
        incoming = incoming_edges(instructions)
        validator = native_producer_validator(reviews, bindings=PRODUCER_BINDINGS, required=REQUIRED_PRODUCERS)
        requests = []
        for index, row in enumerate(instructions):
            native = row.get('native', {})
            if NATIVE_NAME not in native.get('functions', []):
                continue
            if (row['name'] != 'SCRIPT_OP_CALL_NATIVE' or native.get('functions') != [NATIVE_NAME]
                    or type(native.get('address')) is not int or native['address'] != NATIVE_ADDRESS
                    or row['size'] != 6 or native.get('inline_operands', []) != []):
                raise ValueError('Rebound or malformed scroll-board native request.')
            observed = dict(instruction_offset=row['offset'], native_selector=row['operands'][0]['value'],
                            native_address=NATIVE_ADDRESS, arguments=None,
                            argument_conversions={name: 'float' if name in FLOAT_ARGUMENTS else 'integer' for name in ARGUMENT_NAMES},
                            creation_proven=False, result_semantics='Integer hierarchy handle on a valid returning path; no successful allocation or later lifetime proof.')
            try:
                arguments = recover_arguments(instructions, index, 18, incoming=incoming, allow_float=True,
                                              allow_symbolic=True, native_validator=validator)
                observed['arguments'] = dict(zip(ARGUMENT_NAMES, arguments))
            except (ValueError, KeyError, IndexError, TypeError) as error:
                observed['argument_recovery_reason'] = str(error)
            requests.append(observed)
        return dict(status='conditional_requests', requests=requests, scope=SCOPE,
                    native_review_sha256=NATIVE_REVIEW_SHA256, vm_implementation_binding=vm)
    except (ValueError, KeyError, IndexError, TypeError, OSError) as error:
        return dict(unresolved, reason=str(error))
