"""Conservative forward evaluation-stack floors under active source contracts.

This proves relative depth only. Valid variable/array addressing, unchanged frame
metadata, allocation capacity, operand types, resource lifetimes and noninterference
remain obligations. Active reviews must come from native_frame_reviews(), never
from an unchecked manifest. Input must be a complete, diagnostic-free decoded CFG.
Loops need not terminate; only returning call paths
are summarized. No saved-dictionary preservation claim is upgraded here.
"""
from collections import deque

from .formats import integer_operand

# Each separately source-bound protocol consumes A and leaves one result before
# VM continuation, including early rejection and result-before-followup paths.
_DEFERRED = {
    'start_program_then_conditional_zero_append': 1,
    'loop_exit_then_conditional_zero_append': 0,
    'start_program_then_message_or_sequencer_result': 1,
    'held_handle_gate_start_program_then_sequencer_result': 1,
    'acquisition_settings_then_handle_gate_or_full_action_poll': 3,
    'eat_with_early_rejection_or_result_before_followup': 1,
    'costed_program_then_conditional_result_append': 2,
    'poll_game_mode_then_replace_scheme_argument': 1,
    'poll_game_mode_then_replace_text_window_arguments': 3,
    'show_area_label_then_handoff_and_wait_secondary_overlay': 0,
    'transform_kind_after_timed_effect': 1,
}


def analyze_caller_frames(instructions, native_reviews, deferred_reviews=None, *, entries=None, budget=100000):
    """Return conditional frame summaries and per-instruction depth obligations.

    Roots require ENTER_FRAME. An exact callee summary is used at CALL's return
    continuation; its separate navigation edge is not an ordinary CFG successor.
    The finite instruction-work budget is shared across all roots and recursive
    summaries in this invocation; exhausting it leaves affected proofs incomplete.
    Recursion, unequal joins and unsupported effects stop the enclosing proof.
    Callers must reject decode diagnostics, including unresolved branch targets.
    """
    if type(budget) is not int or budget <= 0:
        raise ValueError('caller-frame instruction budget must be a positive integer')
    deferred_reviews = deferred_reviews or {}
    by = {row['offset']: row for row in instructions}
    entries = list(entries) if entries is not None else [o for o, r in by.items() if r['opcode'] == 32]
    summaries, visiting = {}, set()
    remaining = budget
    assumptions = [
        'Each independent entry receives its declared arguments above the caller floor and has a valid return continuation when returning.',
        'Variable/array accesses stay within valid data ranges and preserve frame metadata; cross-frame local writes remain a separate preservation question.',
        'Actor/context/operand storage remains stable and nonoverlapping throughout native waits and helper calls.',
        'Native types, resource, alias, kernel, lifetime and scheduling preconditions are not proved by stack depths.',
        'Reviewed native footprints have no transient value-stack peak above the greater of entry and final depth; future protocols need a separate peak review.',
        'Runtime value capacity, signed byte-offset arithmetic and initial saved-depth/return-stack capacities satisfy reported bounds.',
    ]

    def native_shape(row):
        names = row.get('native', {}).get('functions', [])
        if not names:
            raise ValueError('unresolved native identity')
        shapes = []
        for name in names:
            review = native_reviews.get(name)
            if review is None:
                raise ValueError('native has no active source-bound review: ' + name)
            effect = review.get('effect')
            obligation = dict(native=name, effect=effect, preconditions=review.get('preconditions', []))
            if effect == 'select_action_handler':
                deferred = deferred_reviews.get(name)
                if deferred is None or deferred.get('effect') not in _DEFERRED or type(deferred.get('arguments')) is not int or _DEFERRED[deferred['effect']] != deferred['arguments']:
                    raise ValueError('missing or unsupported active deferred protocol: ' + name)
                args = deferred['arguments']
                phases = deferred.get('phases', [])
                for phase in phases:
                    consumed = phase.get('consumed_arguments', 0)
                    delta = phase.get('stack_delta')
                    if type(consumed) is not int or not 0 <= consumed <= args or type(delta) is not int or not -args <= delta <= 1:
                        raise ValueError('unsupported deferred transient footprint: ' + name)
                obligation.update(deferred_effect=deferred['effect'],
                                  deferred_preconditions=deferred.get('preconditions', []),
                                  phase_scope='Conditional returning paths only; waits may not complete. Operands protected until actual consumption.')
            elif effect == 'append_original_target_integer':
                if (review.get('arguments') != 0 or review.get('stack_delta') != 1 or
                        review.get('semantic_scope') != 'original_instructions_only'):
                    raise ValueError('invalid original-target-only footprint')
                args = 0
                obligation.update(semantic_scope=review['semantic_scope'],
                                  c_result_definedness=review['c_result_definedness'],
                                  caller_obligation=review['caller_obligation'])
            elif effect == 'inline_count_strings_then_append_scalar':
                operands = dict(zip(row.get('operand_roles', []), row.get('operands', [])))
                count = operands.get('string_count') or operands.get('count')
                if count is None and len(row.get('operands', [])) == 2:
                    count = row['operands'][1]
                args = integer_operand(count) if count is not None else None
                if args is None or args < 0:
                    raise ValueError('unresolved or negative inline string count')
                obligation.update(inline_count=args, instruction_offset_delta=5)
            elif effect in ('append_scalar', 'replace_argument_with_scalar', 'replace_arguments_with_scalar',
                            'append_borrowed_string_pointer', 'replace_argument_with_borrowed_string_pointer',
                            'replace_arguments_with_borrowed_string_pointer'):
                args = review.get('arguments')
                if type(args) is not int or args < 0 or review.get('stack_delta') != 1 - args:
                    raise ValueError('invalid active scalar shape')
            else:
                raise ValueError('unsupported reviewed native effect: ' + str(effect))
            shapes.append((args, 1 - args, obligation))
        if len({(a, d) for a, d, _ in shapes}) != 1:
            raise ValueError('native aliases have conflicting stack shapes')
        return shapes[0][0], shapes[0][1], [s[2] for s in shapes]

    def analyze(entry):
        nonlocal remaining
        if entry in visiting:
            raise ValueError('recursive call has no bounded frame summary')
        if entry in summaries:
            return summaries[entry]
        if len(visiting) >= 64:
            raise ValueError('call nesting exceeds the original 64 saved-depth slots')
        header = by.get(entry)
        if header is None or header['opcode'] != 32:
            raise ValueError('callee/root does not begin with ENTER_FRAME')
        argc, locals_count, frame = map(integer_operand, header['operands'])
        if (None in (argc, locals_count, frame) or not 0 <= argc <= locals_count <= 2147483645 or
                not 0 <= frame < 32):
            raise ValueError('invalid frame-entry operands')
        result = dict(entry=entry, arguments=argc, local_count=locals_count, frame_index=frame,
                      protected_floor_from_argument_base=locals_count + 2, instructions=[],
                      diagnostics=[], conditional_floor_proven=False, returns=0, finishes=0,
                      maximum_depth_above_floor=0, maximum_frame_nesting=1)
        visiting.add(entry)
        depths, pending = {}, deque([(entry + header['size'], 0)])
        try:
            while pending:
                offset, depth = pending.popleft()
                if offset in depths:
                    if depths[offset] != depth:
                        raise ValueError(f'{offset:#x}: unequal evaluation-depth join: {depths[offset]} versus {depth}')
                    continue
                if remaining <= 0:
                    raise ValueError('forward caller-frame instruction budget exhausted')
                remaining -= 1
                depths[offset] = depth
                row = by.get(offset)
                if row is None:
                    raise ValueError(f'{offset:#x}: missing decoded instruction')
                op = row['opcode']
                fact = dict(offset=offset, evaluation_depth_before=depth)
                result['instructions'].append(fact)
                required, delta = 0, 0
                if op in (0, 1, 2):
                    pass
                elif op in (3, 4, 17):
                    required = 1
                elif op in tuple(range(5, 10)) + tuple(range(11, 17)) + (18, 19, 20, 21, 22, 23):
                    required, delta = 2, -1
                elif op in (24, 25):
                    delta = 1
                elif op in (10, 29, 31):
                    required, delta = 1, -1
                elif op == 33:
                    required = 1
                elif op == 34:
                    required, delta = 2, -2
                elif op == 28:
                    pass
                elif op == 27:
                    required, delta, obligations = native_shape(row)
                    fact['native_obligations'] = obligations
                elif op == 26:
                    child = analyze(row.get('target'))
                    if not child['conditional_floor_proven'] or child['returns'] == 0 or child['finishes']:
                        raise ValueError(f'{offset:#x}: callee has no complete returning summary')
                    required, delta = child['arguments'], 1 - child['arguments']
                    child_peak = depth - required + child['local_count'] + 2 + child['maximum_depth_above_floor']
                    result['maximum_depth_above_floor'] = max(result['maximum_depth_above_floor'], child_peak)
                    result['maximum_frame_nesting'] = max(result['maximum_frame_nesting'], 1 + child['maximum_frame_nesting'])
                    if result['maximum_frame_nesting'] > 64:
                        raise ValueError(f'{offset:#x}: call nesting exceeds the original 64 saved-depth slots')
                    fact['callee_entry'] = child['entry']
                elif op == 30:
                    if depth != 1:
                        raise ValueError(f'{offset:#x}: RETURN requires exactly one value above metadata')
                    result['returns'] += 1
                    fact['evaluation_depth_after'] = None
                    continue
                elif op == 35:
                    result['finishes'] += 1
                    fact['evaluation_depth_after'] = None
                    continue
                else:
                    raise ValueError(f'{offset:#x}: unsupported opcode or competing frame entry {op}')
                fact['required_evaluation_values'] = required
                if depth < required:
                    raise ValueError(f'{offset:#x}: operands would consume protected locals/frame metadata')
                after = depth + delta
                fact['evaluation_depth_after'] = after
                result['maximum_depth_above_floor'] = max(result['maximum_depth_above_floor'], depth, after)
                if locals_count + 2 + result['maximum_depth_above_floor'] > 2147483647:
                    raise ValueError(f'{offset:#x}: value depth exceeds signed32 representation')
                edges = row.get('successors', [])
                if op == 26:
                    edges = [e for e in edges if e['kind'] == 'return_continuation']
                if not edges:
                    raise ValueError(f'{offset:#x}: no understood continuation')
                pending.extend((e['offset'], after) for e in edges)
            result['conditional_floor_proven'] = True
        except (ValueError, KeyError, TypeError) as error:
            result['diagnostics'].append(str(error))
        finally:
            visiting.remove(entry)
        result['required_value_slots_from_argument_base'] = locals_count + 2 + result['maximum_depth_above_floor']
        summaries[entry] = result
        return result

    roots = []
    for entry in entries:
        try:
            summary = analyze(entry)
            roots.append({key: summary[key] for key in ('entry', 'conditional_floor_proven', 'diagnostics')})
        except (ValueError, KeyError, TypeError) as error:
            roots.append(dict(entry=entry, conditional_floor_proven=False, diagnostics=[str(error)]))
    return dict(roots=roots, summaries=list(summaries.values()), assumptions=assumptions,
                instruction_budget=budget, remaining_budget=remaining,
                scope='Conditional relative evaluation-stack floor only; no type/alias/lifetime/termination or runtime capacity proof.')
