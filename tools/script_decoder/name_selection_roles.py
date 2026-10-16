"""Recognize conditional name-selection storage recipes, not reaching-init proofs."""
import hashlib
import json

from .callee_contexts import context_snapshot
from .stack import incoming_edges

REQUIRED_NATIVES = frozenset(('actorScriptChoosePokemonName', 'actorScriptGetPokemonNameDictionaryGroup', 'actorScriptRandomBelow'))

NATIVE_BINDINGS = {
    'actorScriptChoosePokemonName': dict(arguments=0, review_sha256='1cd50dc27b0537ab6949db067f585fb1d517711bf2ef74fb3b3270c814895f34'),
    'actorScriptGetPokemonNameDictionaryGroup': dict(arguments=1, review_sha256='0f116e4f26833514127c51f75a4696c3106f50a7d3fc733436c2051bcf568199'),
    'actorScriptRandomBelow': dict(arguments=1, review_sha256='45206f67a57091cbca5b17b2e640a6618c518b8c7461fc7899614bf7630e4a8f'),
}

OBLIGATIONS = [
    'Initialization must reach the selected consumer before use; this recognizer does not prove dominance or path feasibility.',
    'The requested count, index and mode remain coherent through waits, calls and repeated activity; bounds and reaching definitions need separate proofs.',
    'Root locals, operand values and frame metadata are live, disjoint and large enough; no other indexed access aliases these arrays.',
    'Any requested initializer count must be separately valid and fit both nonoverlapping arrays without reaching the shared scratch counter; count10 is established only for the recognized setup recipes.',
    'Bedroom catalog and coherent 100-entry history remain initialized; RNG/name selection is serialized and the rejection loop eventually returns.',
    'Native/frame preservation and operand-floor prerequisites remain mandatory. No runtime reachability, uniqueness or uniform randomness is proved here.',
]


def _values(row):
    operands = row.get('operands', [])
    if any(o.get('kind') not in ('integer', 'code_reference') or type(o.get('value')) is not int for o in operands):
        return None
    return [o['value'] for o in operands]


def _op(row, suffix, values=None):
    operands = row.get('operands', [])
    arity = {'ENTER_FRAME': 3, 'LOAD_VARIABLE': 2, 'STORE_VARIABLE': 2,
             'LOAD_ELEMENT': 2, 'STORE_ELEMENT': 2, 'PUSH_LITERAL': 1,
             'CALL': 1, 'CALL_NATIVE': 1, 'JUMP': 1, 'JUMP_IF_ZERO': 1}.get(suffix, 0)
    tag = 2 if suffix in ('CALL', 'CALL_NATIVE', 'JUMP', 'JUMP_IF_ZERO') else 0
    return (row.get('name') == 'SCRIPT_OP_' + suffix and len(operands) == arity and
            type(row.get('size')) is int and row['size'] == 1 + 5 * len(operands) and
            all(type(o.get('tag')) is int and o['tag'] == tag for o in operands) and
            (values is None or _values(row) == values))


def _native(row, function):
    return _op(row, 'CALL_NATIVE') and row.get('native', {}).get('functions') == [function]


def _linear(rows):
    return (all(type(r.get('offset')) is int and type(r.get('size')) is int and r['size'] > 0 for r in rows) and
            all(a['offset'] + a['size'] == b['offset'] for a, b in zip(rows, rows[1:])))


def recognize_name_selection_roles(instructions):
    """Find exact semantic shapes in already authenticated decoded instructions.

    This pure shape recognizer is independently testable. Catalog integration must
    use name_selection_roles, which additionally binds the current source meanings.
    """
    initializers, counts, consumers, setups = [], [], [], []
    if len({r.get('offset') for r in instructions}) != len(instructions):
        return dict(initializers=[], count_helpers=[], setup_recipes=[], consumers=[], obligations=OBLIGATIONS,
                    indexed_producer_provenance_proven=False, reason='Duplicate instruction offsets.')
    incoming = incoming_edges(instructions)

    def closed(rows):
        if not _linear(rows):
            return False
        internal = {r['offset']: set() for r in rows}
        for row in rows:
            next_offset = row['offset'] + row['size']
            name, values = row['name'], _values(row)
            if name in ('SCRIPT_OP_JUMP', 'SCRIPT_OP_JUMP_IF_ZERO', 'SCRIPT_OP_CALL') and (values is None or len(values) != 1):
                return False
            if name == 'SCRIPT_OP_RETURN':
                edges = []
            elif name == 'SCRIPT_OP_JUMP':
                edges = [dict(kind='branch', offset=values[0])]
            elif name == 'SCRIPT_OP_JUMP_IF_ZERO':
                edges = [dict(kind='fallthrough', offset=next_offset), dict(kind='branch', offset=values[0])]
            elif name == 'SCRIPT_OP_CALL':
                edges = [dict(kind='return_continuation', offset=next_offset), dict(kind='call', offset=values[0])]
            else:
                edges = [dict(kind='resume_or_fallthrough' if name == 'SCRIPT_OP_CALL_NATIVE' else 'fallthrough', offset=next_offset)]
            if row.get('successors') != edges:
                return False
            for edge in edges:
                if edge['offset'] in internal:
                    internal[edge['offset']].add(row['offset'])
        return all(incoming.get(r['offset'], set()) == internal[r['offset']] for r in rows[1:])
    for i, row in enumerate(instructions):
        if not _op(row, 'ENTER_FRAME', [1, 2, 1]):
            continue
        q = instructions[i:i + 21]
        if len(q) != 21 or not _linear(q) or not _op(q[0], 'ENTER_FRAME', [1, 2, 1]):
            continue
        scratch = _values(q[2]); names = _values(q[9]); samples = _values(q[13])
        if any(v is None or len(v) != 2 or v[0] != 0 or not 0 <= v[1] <= 2147483647 for v in (scratch, names, samples)):
            continue
        if not all((
            _op(q[1], 'PUSH_LITERAL', [0]), _op(q[2], 'STORE_VARIABLE', scratch),
            _op(q[3], 'LOAD_VARIABLE', scratch), _op(q[4], 'LOAD_VARIABLE', [1, 0]),
            _op(q[5], 'LESS_THAN'), _op(q[6], 'JUMP_IF_ZERO', [q[19]['offset']]),
            _op(q[7], 'LOAD_VARIABLE', scratch), _native(q[8], 'actorScriptChoosePokemonName'),
            _op(q[9], 'STORE_ELEMENT', names), _op(q[10], 'LOAD_VARIABLE', scratch),
            _op(q[11], 'PUSH_LITERAL', [8]), _native(q[12], 'actorScriptRandomBelow'),
            _op(q[13], 'STORE_ELEMENT', samples), _op(q[14], 'LOAD_VARIABLE', scratch),
            _op(q[15], 'PUSH_LITERAL', [1]), _op(q[16], 'ADD'),
            _op(q[17], 'STORE_VARIABLE', scratch), _op(q[18], 'JUMP', [q[3]['offset']]),
            _op(q[19], 'PUSH_LITERAL', [0]), _op(q[20], 'RETURN'))):
            continue
        if not closed(q) or len({scratch[1], names[1], samples[1]}) != 3:
            continue
        initializers.append(dict(entry=row['offset'], role='fill_selected_name_catalog_indices_and_random_samples',
                                 parameter=dict(position=0, role='requested_count', scope='Incoming boxed value consumed by the original comparison; count10 applies only to a recognized setup recipe, not arbitrary arguments.'),
                                 frame_contract=dict(arguments=1, total_local_cells=2, local_count_includes_arguments=True, additional_local_cells=1),
                                 result=dict(tag=0, value=0, scope='Boxed integer zero on the returning path; no termination promise.'),
                                 scratch=scratch, name_array=names,
                                 sample_array=samples, candidate_domain=[0, 152], sample_domain=[0, 7],
                                 writes=[dict(role='loop_index', variable=scratch), dict(role='selected_name_indices', array_base=names),
                                         dict(role='random_samples', array_base=samples)],
                                 evidence=q, scope='Writes shared root storage; not pure and not a proof that protected root locals are preserved.'))
    for i in range(len(instructions) - 2):
        q = instructions[i:i + 3]
        if _op(q[0], 'ENTER_FRAME', [0, 0, 1]) and closed(q) and _op(q[1], 'PUSH_LITERAL', [10]) and _op(q[2], 'RETURN'):
            counts.append(dict(entry=q[0]['offset'], role='requested_name_batch_count', arguments=0,
                               result=dict(tag=0, value=10), evidence=q))
    count_entries = {x['entry'] for x in counts}
    for init in initializers:
        for i, row in enumerate(instructions):
            if not _op(row, 'CALL', [init['entry']]) or i < 1:
                continue
            count_variable = _values(instructions[i - 1])
            if not _op(instructions[i - 1], 'LOAD_VARIABLE') or count_variable is None or count_variable[0] != 0:
                continue
            # Describe an authored setup recipe only; intervening calls can yield.
            start = max(0, i - 40)
            for j in range(start, i - 1):
                q = instructions[j:i + 1]
                if not _op(q[0], 'CALL') or _values(q[0]) is None or _values(q[0])[0] not in count_entries or not _op(q[1], 'STORE_VARIABLE', count_variable):
                    continue
                reset = [(q[k + 1]['offset'], _values(q[k + 1])) for k in range(2, len(q) - 1)
                         if _op(q[k], 'PUSH_LITERAL', [0]) and _op(q[k + 1], 'STORE_VARIABLE')
                         and _values(q[k + 1]) is not None and _values(q[k + 1])[0] == 0]
                if any(_op(r, 'STORE_VARIABLE', count_variable) for r in q[2:]):
                    continue
                name_base, sample_base, scratch_slot = init['name_array'][1], init['sample_array'][1], init['scratch'][1]
                if (max(name_base, sample_base) > 2147483647 - 9 or
                        not (name_base + 9 < sample_base or sample_base + 9 < name_base) or
                        name_base <= scratch_slot <= name_base + 9 or
                        sample_base <= scratch_slot <= sample_base + 9):
                    continue
                if not reset or not closed(q):
                    continue
                guard = instructions[max(0, j - 4):j]
                guard_fact = None
                if (len(guard) == 4 and _linear(guard + q[:1]) and closed(guard + q[:1]) and
                        _op(guard[0], 'LOAD_VARIABLE') and
                        _op(guard[1], 'PUSH_LITERAL') and _values(guard[1]) is not None and
                        _op(guard[2], 'EQUAL') and _op(guard[3], 'JUMP_IF_ZERO')):
                    guard_fact = dict(variable=_values(guard[0]), value=_values(guard[1])[0], evidence=guard)
                assignments = [dict(variable=_values(q[k + 1]), value=_values(q[k])[0], offset=q[k + 1]['offset'])
                               for k in range(len(q) - 1) if _op(q[k], 'PUSH_LITERAL') and
                               _values(q[k]) is not None and _op(q[k + 1], 'STORE_VARIABLE')]
                setups.append(dict(initializer=init['entry'], fill_call=row['offset'], count_variable=count_variable,
                                   count=10, count_helper_call=q[0]['offset'], count_helper_entry=_values(q[0])[0], root_zero_assignments=reset,
                                   preceding_guard=guard_fact, literal_assignments=assignments,
                                   name_slots=[init['name_array'][1], init['name_array'][1] + 9],
                                   sample_slots=[init['sample_array'][1], init['sample_array'][1] + 9],
                                   evidence=q, reaching_initialization_proven=False, index_bounds_proven=False))
        for i in range(len(instructions) - 4):
            q = instructions[i:i + 5]
            index, capture = _values(q[0]), _values(q[2])
            if not closed(q) or index is None or capture is None:
                continue
            if not (_op(q[0], 'LOAD_VARIABLE') and _op(q[1], 'LOAD_ELEMENT', init['name_array']) and
                    _op(q[2], 'STORE_VARIABLE') and _op(q[3], 'LOAD_VARIABLE', capture) and _op(q[4], 'CALL')):
                continue
            target = _values(q[4])[0]
            k = next((n for n, x in enumerate(instructions) if x['offset'] == target), None)
            if k is None:
                continue
            callee = instructions[k:k + 3]
            if len(callee) != 3 or not closed(callee) or not (_op(callee[0], 'ENTER_FRAME', [1, 2, 1]) and
                    _op(callee[1], 'LOAD_VARIABLE', [1, 0]) and _native(callee[2], 'actorScriptGetPokemonNameDictionaryGroup')):
                continue
            consumers.append(dict(initializer=init['entry'], array_base=init['name_array'], index=index,
                                  capture_variable=capture, capture_write_epoch=q[2]['offset'],
                                  array_load=q[1]['offset'], call=q[4]['offset'], first_native=callee[2]['offset'],
                                  role='selected_pokemon_name_catalog_index', evidence=q + callee,
                                  indexed_producer_provenance_proven=False, index_bounds_proven=False,
                                  scope='This capture epoch only; no global semantic rename of the reused variable.'))
    linked_counts = {setup['count_helper_entry'] for setup in setups}
    counts = [row for row in counts if row['entry'] in linked_counts]
    for setup in setups:
        setup['consumer_index_resets'] = [dict(offset=offset, variable=variable)
                                          for offset, variable in setup['root_zero_assignments']
                                          if any(c['initializer'] == setup['initializer'] and c['index'] == variable for c in consumers)]
    roots = [dict(entry=row['offset'], local_count=_values(row)[1], evidence=row, scope='Generic root frame declaration; not itself a name-selection role.') for row in instructions
             if _op(row, 'ENTER_FRAME') and _values(row) is not None and len(_values(row)) == 3
             and _values(row)[0] == 0 and _values(row)[2] == 0]
    return dict(root_frame_declarations=roots, initializers=initializers, count_helpers=counts, setup_recipes=setups, consumers=consumers,
                obligations=OBLIGATIONS, indexed_producer_provenance_proven=False,
                scope='Conditional source-bound storage roles, not reaching-initialization, bounds or path proofs.')


def name_selection_roles(instructions, *, snapshot=None):
    """Recognize roles under an ephemeral fresh full-native/VM evidence snapshot."""
    if set(NATIVE_BINDINGS) != REQUIRED_NATIVES:
        return dict(status='unresolved', reason='Incomplete required native binding set.', initializers=[], consumers=[], setup_recipes=[])
    if snapshot is None:
        snapshot = context_snapshot()
    reviews, preservation = snapshot
    vm = preservation.get('context_vm_contract', {})
    if preservation.get('stale_vm_functions') or not vm.get('current'):
        return dict(status='unresolved', reason='Stale VM context contract.', initializers=[], consumers=[], setup_recipes=[])
    for name, binding in NATIVE_BINDINGS.items():
        review = reviews.get('active', {}).get(name)
        digest = hashlib.sha256(json.dumps(review, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
        if (review is None or digest != binding['review_sha256'] or
                type(review.get('arguments')) is not int or review['arguments'] != binding['arguments']):
            return dict(status='unresolved', reason=f'Stale or unsupported native binding: {name}',
                        initializers=[], consumers=[], setup_recipes=[])
    return dict(status='conditional_roles', native_review_sha256={n: b['review_sha256'] for n, b in NATIVE_BINDINGS.items()},
                vm_implementation_binding=vm, **recognize_name_selection_roles(instructions))
