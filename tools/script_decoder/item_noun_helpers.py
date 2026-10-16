"""Complete authored item-noun search interfaces, including conflicting guards.

Group branches describe retained requests; their selectors exceed the outer guard.
They are not a repaired group translator or evidence of retail path reachability.
"""

import hashlib
import json
import struct

from .callee_contexts import context_snapshot
from .context import opcode_names
from .stack import incoming_edges
from .vocabulary import item_noun_group_names, ITEM_NOUN_GROUP_MAPPING_SHA256

NATIVE_NAME = 'actorScriptFindItemInFacingCone'
NATIVE_REVIEW_SHA256 = '477c1e6fd43eca0066d0ba0762b77c83de24b9b796f0496904a8a75a5e73e21b'


def _digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def _template():
    """Describe the reviewed boxed operands, search arms and complete branch shape."""
    rows = []
    def emit(name, *args, label=None, float_literal=False):
        rows.append(dict(name=name, args=list(args), label=label, float_literal=float_literal))
    def push(value):
        emit('PUSH_LITERAL', value)
    def root(slot, label=None):
        emit('LOAD_VARIABLE', 0, slot, label=label)
    def search(minimum=None, maximum=None):
        if minimum is None:
            root('value_slot'); root('value_slot')
        else:
            push(minimum); push(maximum)
        emit('LOAD_VARIABLE', 1, 0)
        emit('PUSH_LITERAL', 360.0, float_literal=True)
        push(0); push(0)
        emit('CALL_NATIVE', NATIVE_NAME)
        emit('STORE_VARIABLE', 1, 1)
    emit('ENTER_FRAME', 1, 2, 1)
    push(0); emit('STORE_VARIABLE', 1, 1)
    root('type_slot'); push(6); emit('EQUAL')
    root('value_slot'); push(0); emit('GREATER_THAN'); emit('LOGICAL_AND')
    root('value_slot'); push(165); emit('LESS_THAN'); emit('LOGICAL_AND')
    emit('JUMP_IF_ZERO', 'return')
    arms = ((166, 1, 8), (167, 1, 4), (168, 9, 19), (169, 20, 36),
            (171, 30, 36), (172, 37, 49), (170, 51, 60))
    for index, (selector, minimum, maximum) in enumerate(arms):
        root('value_slot', label=f'arm{index}')
        push(selector); emit('EQUAL'); emit('JUMP_IF_ZERO', f'arm{index + 1}' if index < 6 else 'direct')
        search(minimum, maximum); emit('JUMP', 'return')
    root('value_slot', label='direct'); root('value_slot')
    emit('LOAD_VARIABLE', 1, 0); emit('PUSH_LITERAL', 360.0, float_literal=True)
    push(0); push(0); emit('CALL_NATIVE', NATIVE_NAME); emit('STORE_VARIABLE', 1, 1)
    emit('LOAD_VARIABLE', 1, 1, label='return'); emit('RETURN')
    offset, labels = 0, {}
    for row in rows:
        row['relative_offset'] = offset
        row['size'] = 1 + 5 * len(row['args'])
        if row['label']:
            labels[row['label']] = offset
        offset += row['size']
    assert len(rows) == 116 and offset == 791
    return rows, labels, arms


def recognize_item_noun_helpers(instructions, *, group_names=None):
    """Recognize the whole exclusive CFG, without promoting group-arm feasibility."""
    if any(type(r.get('offset')) is not int for r in instructions):
        return []
    by = {r['offset']: r for r in instructions}
    if len(by) != len(instructions):
        return []
    try:
        incoming = incoming_edges(instructions)
    except (KeyError, TypeError):
        return []
    template, labels, arms = _template()
    authoritative = opcode_names('vm')
    results = []
    for entry in instructions:
        if entry.get('name') != 'SCRIPT_OP_ENTER_FRAME':
            continue
        start = entry['offset']
        if not 0 <= start <= 2147482856:
            continue
        slots, selected, expected_incoming = {}, [], {}
        try:
            for spec in template:
                offset = start + spec['relative_offset']
                row = by[offset]
                name = spec['name']
                if (row.get('name') != 'SCRIPT_OP_' + name or type(row.get('opcode')) is not int or
                        authoritative.get(row['opcode']) != row['name'] or type(row.get('size')) is not int or
                        row['size'] != spec['size'] or len(row.get('operands', [])) != len(spec['args'])):
                    raise ValueError('different complete instruction shape')
                for index, (operand, wanted) in enumerate(zip(row['operands'], spec['args'])):
                    tag = 2 if name in ('JUMP', 'JUMP_IF_ZERO', 'CALL_NATIVE') else 1 if spec['float_literal'] else 0
                    kind = 'code_reference' if tag == 2 else 'float' if tag == 1 else 'integer'
                    value = operand.get('value')
                    if type(operand.get('tag')) is not int or operand['tag'] != tag or operand.get('kind') != kind:
                        raise ValueError('different boxed operand tag')
                    if tag == 1:
                        if type(value) is not float or value != wanted:
                            raise ValueError('different float operand')
                        if 'payload_hex' in operand and operand['payload_hex'] != struct.pack('>f', wanted).hex():
                            raise ValueError('inconsistent float bits')
                    elif type(value) is not int:
                        raise ValueError('noninteger encoded operand')
                    elif name == 'CALL_NATIVE':
                        if not 0 <= value <= 2147483647 or row.get('native', {}).get('functions') != [NATIVE_NAME] or row['native'].get('inline_operands') != []:
                            raise ValueError('different or ambiguous native')
                    elif wanted in ('type_slot', 'value_slot'):
                        if not 0 <= value <= 2147483647 or wanted in slots and slots[wanted] != value:
                            raise ValueError('incoherent root coordinates')
                        slots[wanted] = value
                    elif isinstance(wanted, str):
                        if value != start + labels[wanted]:
                            raise ValueError('different branch destination')
                    elif value != wanted:
                        raise ValueError('different frame or literal')
                following = offset + row['size']
                if name == 'RETURN':
                    edges = []
                elif name == 'JUMP':
                    edges = [dict(kind='branch', offset=start + labels[spec['args'][0]])]
                elif name == 'JUMP_IF_ZERO':
                    edges = [dict(kind='fallthrough', offset=following), dict(kind='branch', offset=start + labels[spec['args'][0]])]
                else:
                    edges = [dict(kind='resume_or_fallthrough' if name == 'CALL_NATIVE' else 'fallthrough', offset=following)]
                if row.get('successors') != edges:
                    raise ValueError('different complete CFG')
                for edge in edges:
                    expected_incoming.setdefault(edge['offset'], set()).add(offset)
                selected.append(row)
            if slots['type_slot'] == slots['value_slot']:
                raise ValueError('category and value coordinates alias')
            if any(incoming.get(r['offset'], set()) != expected_incoming.get(r['offset'], set()) for r in selected[1:]):
                raise ValueError('competing internal entry')
            if {r['offset'] for r in instructions if start <= r['offset'] < start + 791} != {r['offset'] for r in selected}:
                raise ValueError('overlapping decoded instructions')
        except (ValueError, KeyError, TypeError, IndexError):
            continue
        results.append(dict(entry_offset=start, definition_bytes=791, instruction_count=116,
            role='guarded_item_noun_direct_kind_search_with_conflicting_authored_group_arms',
            frame_contract=dict(arguments=1, total_local_cells=2, frame_index=1, additional_local_cells=1, local_count_includes_arguments=True),
            root_category_variable=[0, slots['type_slot']], root_value_variable=[0, slots['value_slot']],
            guard=dict(category_equals=6, value_greater_than=0, value_less_than=165),
            authored_group_arms=[dict(selector=value, recovered_name=(group_names or {}).get(value), minimum_kind=minimum, maximum_kind=maximum,
                                     feasible_under_stable_signed_integer_guard=False) for value, minimum, maximum in arms],
            search_arguments=dict(minimum_kind='root value for the admitted direct arm', maximum_kind='same root value',
                                  distance='incoming formal0 converted by native to float', cone_degrees=360.0, item_state_filter=0, weighted_preference=0),
            result='On a valid returning execution, boxed integer zero when the outer guard fails; otherwise the direct exact-kind search result under stable signed-integer root values. No nonzero result or successful ownership/lifetime claim.',
            guard_conflict='All seven authored group selectors166..172 exceed the outer strict upper bound165. Under normal entry and stable signed-integer root values, none of those group arms can run; only values1..164 issue the direct search. This is not a working noun-group translator or a repaired script.',
            instruction_evidence=selected,
            preconditions=['Canonical authenticated complete definition and normal entry at ENTER_FRAME; arbitrary interior entries are outside this contract.',
                'Valid live nonwrapping root/frame tables, saved metadata and operand storage with sufficient capacity and independently proved caller floor; source cells, own locals and native/search storage are physically disjoint.',
                'Root category/value remain stable signed integer boxes across comparisons, search and return; no callback, asynchronous mutation, root restart, frame rebinding or object-manager mutation invalidates the interpretation.',
                'Incoming search distance satisfies the original float conversion and search preconditions; all native handle, item-registry, RNG, ownership-filter and helper lifetime obligations remain applicable.'],
            scope='Complete authored request interface and conditional guard consequence, not retail reachability, caller initialization, actual selector values or guaranteed successful search.'))
    return results


def item_noun_helper_roles(instructions, *, snapshot=None):
    """Bind the complete shape to current VM/native and reviewed noun declarations."""
    reviews, preservation = context_snapshot() if snapshot is None else snapshot
    vm = preservation.get('context_vm_contract', {})
    review = reviews.get('active', {}).get(NATIVE_NAME)
    if (preservation.get('stale_vm_functions') or not vm.get('current') or review is None or
            type(review.get('arguments')) is not int or review['arguments'] != 6 or _digest(review) != NATIVE_REVIEW_SHA256):
        return []
    try:
        names = item_noun_group_names()
    except (OSError, ValueError):
        return []
    rows = recognize_item_noun_helpers(instructions, group_names=names)
    for row in rows:
        row['native_review_sha256'] = NATIVE_REVIEW_SHA256
        row['noun_mapping_sha256'] = ITEM_NOUN_GROUP_MAPPING_SHA256
        row['vm_implementation_binding'] = vm
    return rows
