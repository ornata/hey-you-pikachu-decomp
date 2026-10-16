"""Source-bound authored fishing measurement phases, not value provenance proofs."""

import hashlib
import json

from .callee_contexts import context_snapshot
from .stack import incoming_edges

REQUIRED_NATIVES = frozenset((
    'actorScriptPushSelfHandle', 'actorScriptGetBoundingRadius', 'actorScriptGetActorKind',
    'actorScriptPassThroughInteger', 'actorScriptPostMessage',
))

NATIVE_BINDINGS = {
    'actorScriptPushSelfHandle': (0, '3b9a4986214d4e50d7080f0ef63dc6b08a12bd0af3836ba1a19df10c3aef8d1f'),
    'actorScriptGetBoundingRadius': (1, 'dc9f6488197be3f13396ca66aa0d1f05c3b736b5b559844a9397be72408a883c'),
    'actorScriptGetActorKind': (1, 'ee5d8dd13bb391df6e0cb37c4c2d85a0262aeaf9f6b231ad891be1ae60dfbddc'),
    'actorScriptPassThroughInteger': (1, '7ce5019c6dafb96114d27892f09383b0f4af354c265b840c4a24c41823eb16d8'),
    'actorScriptPostMessage': (6, '9dac2d5d4c12f0a7f0d0df8721448b4d5cdce77915e6a57ec32966e13333d77f'),
}

OBLIGATIONS = [
    'Root slot54 must still denote the intended producer actor; observing self initialization does not prove it reaches or remains live at a query.',
    'Each phase must execute with valid disjoint live frames and operand storage, sufficient capacity and a separately established caller floor.',
    'The intermediate float box and actor-kind box must reach subsequent phases unchanged; intervening calls, indexed aliases, callbacks, root restarts and binding changes are not discharged here.',
    'Float arithmetic and conversion must be valid; conversion requires a finite quotient representable as signed32 after truncation.',
    'No physical unit, range, path feasibility, definition dominance, message delivery, receiver index bound or saved-value equality is established.',
]


def _digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def _values(row):
    values = row.get('operands', [])
    if any(type(v.get('value')) is not int for v in values):
        return None
    return [v['value'] for v in values]


def _op(row, name, values=None):
    arity = {'LOAD_VARIABLE': 2, 'STORE_VARIABLE': 2, 'PUSH_LITERAL': 1, 'CALL_NATIVE': 1}.get(name, 0)
    operands = row.get('operands', [])
    tag = 2 if name == 'CALL_NATIVE' else 0
    return (row.get('name') == 'SCRIPT_OP_' + name and len(operands) == arity
            and type(row.get('size')) is int and row['size'] == 1 + 5 * arity
            and all(type(v.get('tag')) is int and v['tag'] == tag and v.get('kind') == ('code_reference' if tag == 2 else 'integer') for v in operands)
            and _values(row) is not None and (values is None or _values(row) == values))


def _native(row, name):
    return _op(row, 'CALL_NATIVE') and row.get('native', {}).get('functions') == [name]


def recognize_fishing_measurement_roles(instructions):
    """Find independently closed local shapes in authenticated decoded instructions.

    Matching phases share authored coordinates only. This pure recognizer never
    upgrades those correspondences into a reaching-definition or whole-formula proof.
    """
    result = dict(phases=[], correspondences=[], incomplete_message_requests=[], whole_formula_proven=False,
                  obligations=OBLIGATIONS, scope='Conditional authored phases only; no actual-value provenance.')
    if (any(type(r.get('offset')) is not int for r in instructions) or
            any(a['offset'] >= b['offset'] for a, b in zip(instructions, instructions[1:]))):
        return dict(result, reason='Invalid or duplicate instruction offsets.')
    incoming = incoming_edges(instructions)

    def closed(rows):
        for i, row in enumerate(rows):
            if type(row.get('size')) is not int or row['size'] <= 0:
                return False
            following = row['offset'] + row['size']
            kind = 'resume_or_fallthrough' if row.get('name') == 'SCRIPT_OP_CALL_NATIVE' else 'fallthrough'
            if row.get('successors') != [dict(kind=kind, offset=following)]:
                return False
            if i and (rows[i - 1]['offset'] + rows[i - 1]['size'] != row['offset'] or incoming.get(row['offset'], set()) != {rows[i - 1]['offset']}):
                return False
        return True

    owner = None
    owners = {}
    for row in instructions:
        if row.get('name') == 'SCRIPT_OP_ENTER_FRAME':
            owner = row['offset']
        owners[row['offset']] = owner

    def add(role, rows, **fields):
        if closed(rows):
            result['phases'].append(dict(role=role, offset=rows[0]['offset'],
                authored_definition_entry=owners[rows[0]['offset']], instruction_evidence=rows,
                reaching_definition_proven=False, **fields))

    for i in range(len(instructions)):
        q = instructions[i:i + 2]
        if len(q) == 2 and _native(q[0], 'actorScriptPushSelfHandle') and _op(q[1], 'STORE_VARIABLE', [0, 54]):
            add('producer_self_handle_initialization', q, variable=[0, 54])
        q = instructions[i:i + 3]
        if len(q) == 3 and _op(q[0], 'LOAD_VARIABLE', [0, 54]) and _native(q[1], 'actorScriptGetActorKind') and _op(q[2], 'STORE_VARIABLE', [1, 9]):
            add('producer_kind_capture', q, variable=[1, 9], handle_source=[0, 54])
        q = instructions[i:i + 5]
        if len(q) == 5 and all((_op(q[0], 'LOAD_VARIABLE', [0, 54]), _native(q[1], 'actorScriptGetBoundingRadius'), _op(q[2], 'PUSH_LITERAL', [50]), _op(q[3], 'MULTIPLY'), _op(q[4], 'STORE_VARIABLE', [1, 1]))):
            add('scaled_bounding_radius_float_capture', q, variable=[1, 1], handle_source=[0, 54], numeric_contract='Float-box capture of f32(radius * 50), under the native and VM contracts.')
        if len(q) == 5 and all((_op(q[0], 'LOAD_VARIABLE', [1, 1]), _op(q[1], 'PUSH_LITERAL', [3]), _op(q[2], 'DIVIDE'), _native(q[3], 'actorScriptPassThroughInteger'), _op(q[4], 'STORE_VARIABLE', [1, 1]))):
            add('measurement_integer_conversion', q, variable=[1, 1], numeric_contract='If the loaded box is the preceding float intermediate: trunc_s32(f32(intermediate / 3)). The shape alone does not prove the loaded tag or value.')
        q = instructions[i:i + 7]
        if len(q) == 7 and all((_op(q[0], 'LOAD_VARIABLE', [0, 55]), _op(q[1], 'LOAD_VARIABLE', [1, 9]), _op(q[2], 'LOAD_VARIABLE', [1, 1]), _op(q[3], 'PUSH_LITERAL', [14]), _op(q[4], 'PUSH_LITERAL', [0]), _op(q[5], 'PUSH_LITERAL', [0]), _native(q[6], 'actorScriptPostMessage'))):
            add('fishing_measurement_message_request', q, recipient_source=[0, 55], type_source=[1, 9], argument0_source=[1, 1], argument2=14, argument3=0, argument4=0)
    for post in [p for p in result['phases'] if p['role'] == 'fishing_measurement_message_request']:
        linked = [p for p in result['phases'] if p['authored_definition_entry'] == post['authored_definition_entry'] and p['role'] in ('producer_kind_capture', 'scaled_bounding_radius_float_capture', 'measurement_integer_conversion')]
        initializers = [p for p in result['phases'] if p['role'] == 'producer_self_handle_initialization']
        required_roles = ('producer_kind_capture', 'scaled_bounding_radius_float_capture', 'measurement_integer_conversion')
        missing = [role for role in required_roles if not any(p['role'] == role for p in linked)]
        ambiguous = [role for role in required_roles if sum(p['role'] == role for p in linked) > 1]
        if len(initializers) != 1:
            (missing if not initializers else ambiguous).append('producer_self_handle_initialization')
        if missing or ambiguous:
            result['incomplete_message_requests'].append(dict(
                message_request=post['offset'], status='incomplete_authored_phase_correspondence',
                missing_phases=missing, ambiguous_phases=ambiguous, whole_formula_proven=False))
            continue
        scalar_writes = [dict(offset=r['offset'], variable=_values(r)) for r in instructions
                         if owners[r['offset']] == post['authored_definition_entry'] and _op(r, 'STORE_VARIABLE') and _values(r) in ([1, 1], [1, 9])]
        indexed_writes = [dict(offset=r['offset'], operands=r.get('operands')) for r in instructions if r.get('name') == 'SCRIPT_OP_STORE_ELEMENT']
        root_writes = [dict(offset=r['offset'], operands=r.get('operands')) for r in instructions if r.get('name') == 'SCRIPT_OP_STORE_VARIABLE' and _values(r) == [0, 54]]
        result['correspondences'].append(dict(message_request=post['offset'],
            phase_offsets=[p['offset'] for p in linked], self_initialization_offsets=[p['offset'] for p in initializers],
            status='complete_authored_phase_correspondence_not_provenance',
            same_authored_local_coordinates=True, whole_formula_proven=False,
            conditional_formula='trunc_s32(f32(f32(radius * 50) / 3)) only if the float intermediate reaches conversion, the converted box reaches posting, and the actor/kind bindings remain coherent.',
            scalar_write_sites_in_authored_definition=scalar_writes, root54_scalar_write_sites=root_writes,
            full_program_indexed_write_candidates=indexed_writes,
            write_audit_scope='All supplied instructions scanned. Indexed writes retain full operand widths; their runtime index/aliasing and transitive native/CALL writes are unresolved. Local coordinates reused by other definitions are not shared reaching definitions.'))
    return result


def fishing_measurement_roles(instructions, *, snapshot=None):
    """Require fresh full VM and exact native contracts before labeling phases."""
    if set(NATIVE_BINDINGS) != REQUIRED_NATIVES:
        return dict(status='unresolved', reason='Incomplete or unexpected required native binding set.', phases=[], correspondences=[])
    reviews, preservation = context_snapshot() if snapshot is None else snapshot
    vm = preservation.get('context_vm_contract', {})
    if preservation.get('stale_vm_functions') or not vm.get('current'):
        return dict(status='unresolved', reason='Stale VM context contract.', phases=[], correspondences=[])
    for name, (arguments, digest) in NATIVE_BINDINGS.items():
        review = reviews.get('active', {}).get(name)
        if review is None or type(review.get('arguments')) is not int or review['arguments'] != arguments or _digest(review) != digest:
            return dict(status='unresolved', reason=f'Stale or unsupported native binding: {name}', phases=[], correspondences=[])
    return dict(status='conditional_roles', native_review_sha256={n: b[1] for n, b in NATIVE_BINDINGS.items()}, vm_implementation_binding=vm, **recognize_fishing_measurement_roles(instructions))
