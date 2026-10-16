"""Source-bound observations of runtime object-kind captures and comparisons.

These roles concern RuntimeObjectPrefix.kind, not ItemKind catalog identifiers or
ObjectScriptCategory. They do not establish handle availability or query outcomes.
"""

import hashlib
import json

from .callee_contexts import context_snapshot
from .context import ROOT, opcode_names
from .effects import enum_names
from .stack import incoming_edges

NATIVE_NAME = 'actorScriptGetObjectType'
NATIVE_REVIEW_SHA256 = '18540a9de22e7fb8c6748ba98310c2931628b7e4669e6bd4adcff7edc88edcd1'
ITEM_STATE_NATIVE = 'actorScriptGetItemState'
ITEM_STATE_REVIEW_SHA256 = '2b82e3bf0b97bc624ddee4b36f9d5838d388c29282969c92778c01213398f009'
ITEM_TYPES_SHA256 = '6d67f534c838307b1ab917eae0793f2c17ff93eb5f0d0a2e01f9d46154221c58'

REQUIRED_HEADERS = frozenset(('include/object_types.h', 'include/actor_kind.h'))
HEADER_BINDINGS = {
    'include/object_types.h': ('RUNTIME_OBJECT_KIND_', '30ce7811d84b38295e33c68e500ce4e6d1c40c14a524d7edbc1859560a5c4f64'),
    'include/actor_kind.h': ('ACTOR_KIND_', 'f614803c6f8da926f786cff2ff5a0a57fee179562fa7af6e2854a5ca581b9d63'),
}


def _digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def _op(row, name, *, native_name=NATIVE_NAME):
    arity = {'CALL_NATIVE': 1, 'STORE_VARIABLE': 2, 'LOAD_VARIABLE': 2, 'PUSH_LITERAL': 1,
             'EQUAL': 0, 'NOT_EQUAL': 0, 'LOGICAL_AND': 0, 'JUMP_IF_ZERO': 1}.get(name)
    if arity is None or row.get('name') != 'SCRIPT_OP_' + name:
        return False
    operands = row.get('operands', [])
    if type(row.get('size')) is not int or row['size'] != 1 + 5 * arity or len(operands) != arity:
        return False
    tag, kind = (2, 'code_reference') if name in ('CALL_NATIVE', 'JUMP_IF_ZERO') else (0, 'integer')
    if any(type(o.get('tag')) is not int or o['tag'] != tag or o.get('kind') != kind or type(o.get('value')) is not int for o in operands):
        return False
    values = [o['value'] for o in operands]
    if name in ('STORE_VARIABLE', 'LOAD_VARIABLE'):
        return -1 <= values[0] < 32 and 0 <= values[1] <= 2147483647
    if name == 'PUSH_LITERAL':
        return -2147483648 <= values[0] <= 2147483647
    if name == 'CALL_NATIVE':
        return 0 <= values[0] <= 2147483647 and row.get('native', {}).get('functions') == [native_name]
    if name == 'JUMP_IF_ZERO':
        return 0 <= values[0] <= 2147483647
    return True


def recognize_object_query_roles(instructions, *, symbolic_names=None):
    """Recognize only exclusive adjacent authored paths; retain raw operands.

    The optional name map is display metadata supplied by the source-bound wrapper.
    No observation is a persistent variable rename or cross-operation value proof.
    """
    result = dict(captures=[], comparisons=[], item_ownership_predicates=[], scope='Conditional adjacent runtime-object-kind observations only; no actual result, availability, reachability or persistent variable role.')
    if (any(type(r.get('offset')) is not int for r in instructions) or
            any(a['offset'] >= b['offset'] for a, b in zip(instructions, instructions[1:]))):
        return dict(result, reason='Invalid or duplicate instruction ordering.')
    try:
        incoming = incoming_edges(instructions)
    except (KeyError, TypeError):
        return dict(result, reason='Malformed control-flow edges.')

    def closed(rows):
        for i, row in enumerate(rows):
            if type(row.get('size')) is not int or row['size'] <= 0:
                return False
            following = row['offset'] + row['size']
            kind = 'resume_or_fallthrough' if row.get('name') == 'SCRIPT_OP_CALL_NATIVE' else 'fallthrough'
            edges = [dict(kind=kind, offset=following)]
            if i == len(rows) - 1 and row.get('name') == 'SCRIPT_OP_JUMP_IF_ZERO':
                edges = [dict(kind='fallthrough', offset=following), dict(kind='branch', offset=row['operands'][0]['value'])]
            if row.get('successors') != edges:
                return False
            if i and (rows[i - 1]['offset'] + rows[i - 1]['size'] != row['offset'] or incoming.get(row['offset'], set()) != {rows[i - 1]['offset']}):
                return False
        return True

    authoritative_names = opcode_names('vm')

    def valid(row, name, *, native_name=NATIVE_NAME):
        return (_op(row, name, native_name=native_name) and type(row.get('opcode')) is int
                and authoritative_names.get(row['opcode']) == row.get('name'))

    for i, native in enumerate(instructions):
        if not valid(native, 'CALL_NATIVE'):
            continue
        capture = instructions[i:i + 2]
        if len(capture) == 2 and valid(capture[1], 'STORE_VARIABLE') and closed(capture):
            result['captures'].append(dict(native_offset=native['offset'], store_offset=capture[1]['offset'],
                variable=[o['value'] for o in capture[1]['operands']], role='runtime_object_kind_u16_widened_integer',
                instruction_evidence=capture, scope='This adjacent capture epoch only. The getter returns zero on an unavailable handle; no nonzero or availability claim.'))
        for via_capture, length in ((False, 3), (True, 5)):
            rows = instructions[i:i + length]
            if len(rows) != length:
                continue
            if via_capture:
                if not (valid(rows[1], 'STORE_VARIABLE') and valid(rows[2], 'LOAD_VARIABLE') and rows[1]['operands'] == rows[2]['operands']):
                    continue
            literal, comparison = rows[-2:]
            if not (valid(literal, 'PUSH_LITERAL') and (valid(comparison, 'EQUAL') or valid(comparison, 'NOT_EQUAL')) and closed(rows)):
                continue
            value = literal['operands'][0]['value']
            result['comparisons'].append(dict(native_offset=native['offset'], comparison_offset=comparison['offset'],
                comparison=comparison['name'], literal=value, literal_operand=literal['operands'][0],
                symbolic_names=(symbolic_names or {}).get(value, []), via_adjacent_capture=via_capture,
                role='runtime_object_kind_comparison_literal', instruction_evidence=rows,
                result_proven=False, scope='Comparison operand role only; literal widths are preserved without u16 truncation. Immediate STORE/LOAD requires valid disjoint storage and native/frame/operand-floor contracts; no claim across intervening operations.'))
    for i in range(len(instructions) - 9):
        rows = instructions[i:i + 10]
        if not (valid(rows[0], 'LOAD_VARIABLE') and valid(rows[1], 'CALL_NATIVE')
                and valid(rows[2], 'PUSH_LITERAL') and rows[2]['operands'][0]['value'] == 134
                and valid(rows[3], 'EQUAL') and valid(rows[4], 'LOAD_VARIABLE')
                and rows[0]['operands'] == rows[4]['operands']
                and valid(rows[5], 'CALL_NATIVE', native_name=ITEM_STATE_NATIVE)
                and valid(rows[6], 'PUSH_LITERAL') and rows[6]['operands'][0]['value'] == 0
                and (valid(rows[7], 'EQUAL') or valid(rows[7], 'NOT_EQUAL'))
                and valid(rows[8], 'LOGICAL_AND') and valid(rows[9], 'JUMP_IF_ZERO')
                and closed(rows)):
            continue
        world = rows[7]['name'] == 'SCRIPT_OP_EQUAL'
        result['item_ownership_predicates'].append(dict(
            offset=rows[0]['offset'], variable=[o['value'] for o in rows[0]['operands']],
            object_query_offset=rows[1]['offset'], item_state_query_offset=rows[5]['offset'],
            branch_target=rows[9]['operands'][0]['value'], fallthrough_offset=rows[9]['offset'] + rows[9]['size'],
            role='available_item_with_world_ownership' if world else 'available_item_with_nonzero_ownership',
            object_kind_literal=134, ownership_literal=0, ownership_comparison=rows[7]['name'],
            ownership_zero_name='ITEM_OWNERSHIP_WORLD', both_queries_evaluated=True, short_circuit=False,
            ownership_comparison_scope=('The equality comparison accepts only signed ownership state0.' if world else 'The nonzero comparison accepts every nonzero signed ownership encoding, including TOOLBOX4, SUSPENDED5 and negative encodings; it does not mean exclusively held.'),
            instruction_evidence=rows, actual_predicate_result_proven=False,
            scope='Both queries are evaluated before LOGICAL_AND; the kind comparison is not a safety guard. Under valid disjoint frame/operand storage, coherent unchanged handle/object bindings and all native preconditions, the authored conjunction compares runtime kind ITEM and signed ownership state against zero. A coherent toolbox item fails the available-object kind query, so the conjunction is false for it; suspended state5 is not itself excluded by that resolver, but still requires a live valid handle. This is not an exclusively-held, lifecycle-validity or branch-reachability test. No whole-function or return role.'))
    return result


def object_query_roles(instructions, *, snapshot=None, root=ROOT):
    """Bind roles and optional enum spellings to fresh reviewed source contracts."""
    if set(HEADER_BINDINGS) != REQUIRED_HEADERS:
        return dict(status='unresolved', reason='Incomplete or unexpected enum header binding set.', captures=[], comparisons=[])
    reviews, preservation = context_snapshot(root=root) if snapshot is None else snapshot
    vm = preservation.get('context_vm_contract', {})
    review = reviews.get('active', {}).get(NATIVE_NAME)
    if preservation.get('stale_vm_functions') or not vm.get('current'):
        return dict(status='unresolved', reason='Stale VM context contract.', captures=[], comparisons=[])
    if review is None or type(review.get('arguments')) is not int or review['arguments'] != 1 or _digest(review) != NATIVE_REVIEW_SHA256:
        return dict(status='unresolved', reason='Stale or unsupported object-type native contract.', captures=[], comparisons=[])
    names = {}
    for path, (prefix, digest) in HEADER_BINDINGS.items():
        try:
            source = root / path
            if hashlib.sha256(source.read_bytes()).hexdigest() != digest:
                raise ValueError('stale header')
            for value, name in enum_names(source, prefix).items():
                names.setdefault(value, []).append(dict(name=name, source=path))
        except (OSError, ValueError):
            return dict(status='unresolved', reason=f'Stale or unsupported enum source: {path}', captures=[], comparisons=[])
    observations = recognize_object_query_roles(instructions, symbolic_names=names)
    item_review = reviews.get('active', {}).get(ITEM_STATE_NATIVE)
    item_reason = None
    if (ITEM_STATE_REVIEW_SHA256 is None or item_review is None or
            type(item_review.get('arguments')) is not int or item_review['arguments'] != 1 or
            _digest(item_review) != ITEM_STATE_REVIEW_SHA256):
        item_reason = 'Stale or unsupported item-state native contract.'
    try:
        item_header = root / 'include/item_types.h'
        if hashlib.sha256(item_header.read_bytes()).hexdigest() != ITEM_TYPES_SHA256:
            raise ValueError('stale header')
        ownership_names = enum_names(item_header, 'ITEM_OWNERSHIP_')
        if ownership_names.get(0) != 'ITEM_OWNERSHIP_WORLD':
            raise ValueError('unsupported ownership zero')
    except (OSError, ValueError):
        item_reason = 'Stale or unsupported item ownership enum source.'
    if item_reason:
        observations['item_ownership_predicates'] = []
    observations['item_ownership_predicates_status'] = 'unresolved' if item_reason else 'conditional_roles'
    observations['item_ownership_predicates_reason'] = item_reason
    if not item_reason:
        observations['item_ownership_predicate_bindings'] = dict(
            native_review_sha256=ITEM_STATE_REVIEW_SHA256, header_sha256={'include/item_types.h': ITEM_TYPES_SHA256})
    return dict(status='conditional_roles', native_review_sha256=NATIVE_REVIEW_SHA256,
                header_sha256={p: b[1] for p, b in HEADER_BINDINGS.items()}, vm_implementation_binding=vm,
                **observations)
