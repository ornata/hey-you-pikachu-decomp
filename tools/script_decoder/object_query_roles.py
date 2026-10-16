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
NATIVE_REVIEW_SHA256 = '978e95b72d62f43ff396f93adb5f9a939d115351be5907b7d5302f4f4b56ddb8'
REQUIRED_HEADERS = frozenset(('include/object_types.h', 'include/actor_kind.h'))
HEADER_BINDINGS = {
    'include/object_types.h': ('RUNTIME_OBJECT_KIND_', '932003acb7daf207ca52e3d8645431086f285daa4f7519ae75efaecff44fe64c'),
    'include/actor_kind.h': ('ACTOR_KIND_', 'f614803c6f8da926f786cff2ff5a0a57fee179562fa7af6e2854a5ca581b9d63'),
}


def _digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def _op(row, name):
    arity = {'CALL_NATIVE': 1, 'STORE_VARIABLE': 2, 'LOAD_VARIABLE': 2, 'PUSH_LITERAL': 1,
             'EQUAL': 0, 'NOT_EQUAL': 0}.get(name)
    if arity is None or row.get('name') != 'SCRIPT_OP_' + name:
        return False
    operands = row.get('operands', [])
    if type(row.get('size')) is not int or row['size'] != 1 + 5 * arity or len(operands) != arity:
        return False
    tag, kind = (2, 'code_reference') if name == 'CALL_NATIVE' else (0, 'integer')
    if any(type(o.get('tag')) is not int or o['tag'] != tag or o.get('kind') != kind or type(o.get('value')) is not int for o in operands):
        return False
    values = [o['value'] for o in operands]
    if name in ('STORE_VARIABLE', 'LOAD_VARIABLE'):
        return -1 <= values[0] < 32 and 0 <= values[1] <= 2147483647
    if name == 'PUSH_LITERAL':
        return -2147483648 <= values[0] <= 2147483647
    if name == 'CALL_NATIVE':
        return 0 <= values[0] <= 2147483647 and row.get('native', {}).get('functions') == [NATIVE_NAME]
    return True


def recognize_object_query_roles(instructions, *, symbolic_names=None):
    """Recognize only exclusive adjacent authored paths; retain raw operands.

    The optional name map is display metadata supplied by the source-bound wrapper.
    No observation is a persistent variable rename or cross-operation value proof.
    """
    result = dict(captures=[], comparisons=[], scope='Conditional adjacent runtime-object-kind observations only; no actual result, availability, reachability or persistent variable role.')
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
            if row.get('successors') != [dict(kind=kind, offset=following)]:
                return False
            if i and (rows[i - 1]['offset'] + rows[i - 1]['size'] != row['offset'] or incoming.get(row['offset'], set()) != {rows[i - 1]['offset']}):
                return False
        return True

    authoritative_names = opcode_names('vm')

    def valid(row, name):
        return (_op(row, name) and type(row.get('opcode')) is int
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
    return dict(status='conditional_roles', native_review_sha256=NATIVE_REVIEW_SHA256,
                header_sha256={p: b[1] for p, b in HEADER_BINDINGS.items()}, vm_implementation_binding=vm,
                **recognize_object_query_roles(instructions, symbolic_names=names))
