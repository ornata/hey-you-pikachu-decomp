"""Conditional producer return/mode relations for specific bytecode callers.

A shared formal argument remains unresolved. Relations preserve the pair of a
borrowed mode value and returned integer; timed-program returns are never folded
into cost selectors merely because they share a producer.
"""

from .formats import integer_operand
from .functions import function_summary
from .local_flow import KNOWN_OPERATIONS, vm_comparison_contract
from .stack import incoming_edges, recover_arguments


def annotate_costed_caller_relations(instructions, requests, policies, *, preserved_natives=(), limit=30000):
    """Attach bounded caller-specific evidence without substituting shared formals."""
    comparison_contract = vm_comparison_contract()
    by = {r['offset']: r for r in instructions}
    indices = {r['offset']: i for i, r in enumerate(instructions)}
    frames = {off: r for off, r in by.items() if r['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    incoming = incoming_edges(instructions)
    predecessors = {}
    for row in instructions:
        for edge in row.get('successors', []):
            if edge['kind'] != 'call':
                predecessors.setdefault(edge['offset'], set()).add(row['offset'])
    preserved_natives = set(preserved_natives)
    root_frames = [r for r in frames.values() if integer_operand(r['operands'][2]) == 0]
    root_extent = None
    if len(root_frames) == 1 and root_frames[0]['offset'] == 0 and integer_operand(root_frames[0]['operands'][0]) == 0:
        root_extent = integer_operand(root_frames[0]['operands'][1])
    work = 0

    def charge():
        nonlocal work
        work += 1
        if work > limit:
            raise ValueError('paired-flow work budget exhausted')

    def successors(row):
        return [e['offset'] for e in row.get('successors', []) if e['kind'] != 'call']

    def arguments(row, count):
        return recover_arguments(instructions, indices[row['offset']], count,
                                 incoming=incoming, allow_float=True, allow_symbolic=True, frame_entries=frames)

    def owner(target):
        pending, seen, owners = [target], set(), set()
        while pending:
            off = pending.pop()
            if off in seen:
                continue
            charge()
            seen.add(off)
            row = by[off]
            if row['name'] == 'SCRIPT_OP_ENTER_FRAME':
                owners.add(off)
            else:
                if not predecessors.get(off):
                    raise ValueError('unframed caller path')
                pending.extend(predecessors[off])
        if len(owners) != 1:
            raise ValueError('ambiguous caller frame entry')
        return owners.pop()

    def frame_contract(entry):
        row = frames.get(entry)
        if row is None:
            raise ValueError('missing called frame entry')
        count, local_count, frame = map(integer_operand, row['operands'])
        if count is None or local_count is None or frame is None or not 0 <= count <= local_count or not 1 <= frame < 32:
            raise ValueError('invalid called frame contract')
        return count, local_count, frame

    def body(entry):
        pending, seen = [entry], set()
        while pending:
            off = pending.pop()
            if off in seen:
                continue
            charge()
            seen.add(off)
            row = by.get(off)
            if row is None or row['name'] not in KNOWN_OPERATIONS:
                raise ValueError('missing or unknown instruction effect')
            if off != entry and row['name'] == 'SCRIPT_OP_ENTER_FRAME':
                raise ValueError('flow enters a competing frame')
            if row['name'] == 'SCRIPT_OP_RETURN':
                continue
            if row['name'] == 'SCRIPT_OP_FINISH':
                raise ValueError('called function can finish without a balanced return')
            if not successors(row):
                raise ValueError('missing called-function continuation')
            pending.extend(successors(row))
        return seen

    def native_safe(row):
        names = row.get('native', {}).get('functions', [])
        if len(names) != 1 or names[0] not in preserved_natives:
            raise ValueError(f"{row['offset']}: native lacks a frozen preservation review")
        return names[0]

    def root_slot(coordinates):
        return (coordinates[0] == 0 and root_extent is not None and
                0 <= coordinates[1] < root_extent)

    def coordinates(row):
        values = tuple(integer_operand(v) for v in row['operands'])
        if len(values) != 2 or None in values:
            raise ValueError('unresolved variable coordinates')
        return values

    helper_cache = {}
    def preserve_helper(entry, mode, active=()):
        key = entry, mode
        if key in helper_cache:
            return helper_cache[key]
        if entry in active:
            raise ValueError('recursive intervening helper')
        _, count, frame = frame_contract(entry)
        offsets = body(entry)
        writes, natives, callees = [], set(), set()
        for off in sorted(offsets):
            row = by[off]
            if row['name'] == 'SCRIPT_OP_STORE_ELEMENT':
                raise ValueError('intervening helper has an indirect writer')
            if row['name'] == 'SCRIPT_OP_STORE_VARIABLE':
                coords = coordinates(row)
                if coords == mode:
                    raise ValueError('intervening helper writes the paired mode')
                if not (coords[0] == frame and 0 <= coords[1] < count or root_slot(coords)):
                    raise ValueError('intervening helper has an unbounded borrowed writer')
                writes.append(dict(instruction_offset=off, frame=coords[0], slot=coords[1]))
            if row['name'] == 'SCRIPT_OP_CALL_NATIVE':
                natives.add(native_safe(row))
            if row['name'] == 'SCRIPT_OP_CALL':
                target = integer_operand(row['operands'][0])
                nested = preserve_helper(target, mode, (*active, entry))
                callees.add(target)
                callees.update(nested['script_callees'])
                writes.extend(nested['writes'])
                natives.update(nested['native_functions'])
        result = dict(entry_offset=entry, writes=writes, native_functions=sorted(natives), script_callees=sorted(callees))
        helper_cache[key] = result
        return result

    producer_cache = {}
    def producer_relation(entry, mode):
        key = entry, mode
        if key in producer_cache:
            return producer_cache[key]
        _, count, frame = frame_contract(entry)
        if not root_slot(mode):
            raise ValueError('paired mode lacks a bounded root slot')
        offsets = body(entry)
        summary = function_summary(instructions, entry)
        if not summary['static_return_domain_complete']:
            raise ValueError('producer return expressions are incomplete')
        returns = {r['instruction_offset']: r for r in summary['returns']}
        natives, writes = set(), []
        for off in sorted(offsets):
            row = by[off]
            if row['name'] == 'SCRIPT_OP_CALL':
                raise ValueError('producer has an unmodeled script call')
            if row['name'] == 'SCRIPT_OP_CALL_NATIVE':
                natives.add(native_safe(row))
            if row['name'] == 'SCRIPT_OP_STORE_VARIABLE':
                coords = coordinates(row)
                if not (coords == mode or coords[0] == frame and 0 <= coords[1] < count):
                    raise ValueError('producer writes an unmodeled borrowed variable')
            if row['name'] == 'SCRIPT_OP_STORE_ELEMENT':
                coords = coordinates(row)
                values = arguments(row, 2)
                if coords[0] != frame or not 0 <= coords[1] < count or values[0]['kind'] != 'integer' or not 0 <= coords[1] + values[0]['value'] < count:
                    raise ValueError('producer indexed writer is not bounded to its own locals')
        pending, seen, pairs = [(entry, None)], set(), []
        while pending:
            off, value = pending.pop()
            if (off, value) in seen:
                continue
            charge()
            seen.add((off, value))
            row = by[off]
            if row['name'] == 'SCRIPT_OP_STORE_VARIABLE' and coordinates(row) == mode:
                expression = arguments(row, 1)[0]
                if expression['kind'] != 'integer':
                    raise ValueError('paired mode writer is not an integer constant')
                value = expression['value']
                writes.append(dict(instruction_offset=off, value=value))
            if row['name'] == 'SCRIPT_OP_RETURN':
                if value is None:
                    raise ValueError('producer can return without defining paired mode')
                result = returns[off]
                values = [result['integer_conversion']] if 'integer_conversion' in result else result['resolved_lookup']['integer_return_domain']
                pairs.extend(dict(mode=value, return_value=v, return_offset=off) for v in values)
                continue
            pending.extend((next_off, value) for next_off in successors(row))
        relation = dict(entry_offset=entry, mode_domain='shared_helper_dispatch_mode',
                        first_argument_domain='cost_selector_only_when_dispatch_mode_is_six; other_modes_keep_their_own_domains', mode_coordinate=dict(frame=mode[0], slot=mode[1]),
                        mode_writes=sorted({r['instruction_offset']: r for r in writes}.values(), key=lambda r:r['instruction_offset']),
                        mode_return_pairs=sorted(pairs, key=lambda r:(r['mode'], r['return_value'], r['return_offset'])),
                        native_functions=sorted(natives), root_frame_extent=root_extent)
        producer_cache[key] = relation
        return relation

    def forwarding_guards(entry, target, mode, prefix):
        def reaches(start, blocked):
            pending, seen = [start], set()
            while pending:
                off = pending.pop()
                if off == target:
                    return True
                if off in seen or off == blocked:
                    continue
                charge()
                seen.add(off)
                pending.extend(successors(by[off]))
            return False
        guards = []
        for index in range(len(instructions)-3):
            load, literal, compare, jump = instructions[index:index+4]
            if any(row['offset'] not in prefix for row in (load,literal,compare,jump)):
                continue
            if load['name'] != 'SCRIPT_OP_LOAD_VARIABLE' or coordinates(load) != mode:
                continue
            if literal['name'] != 'SCRIPT_OP_PUSH_LITERAL' or literal['operands'][0]['tag'] not in (0,2):
                continue
            literal_value = integer_operand(literal['operands'][0])
            if type(literal_value) is not int or not -2147483648 <= literal_value < 2147483648:
                continue
            if compare['name'] not in ('SCRIPT_OP_EQUAL','SCRIPT_OP_NOT_EQUAL') or jump['name'] != 'SCRIPT_OP_JUMP_IF_ZERO':
                continue
            if any(incoming.get(right['offset'],set()) != {left['offset']} or successors(left) != [right['offset']]
                   for left,right in ((load,literal),(literal,compare),(compare,jump))):
                continue
            true = [e['offset'] for e in jump['successors'] if e['kind']=='fallthrough']
            false = [e['offset'] for e in jump['successors'] if e['kind']=='branch']
            if len(true)!=1 or len(false)!=1 or reaches(entry,jump['offset']):
                continue
            true_reaches, false_reaches = reaches(true[0],jump['offset']), reaches(false[0],jump['offset'])
            if true_reaches == false_reaches:
                continue
            guards.append(dict(load_offset=load['offset'], comparison_offset=compare['offset'],
                branch_offset=jump['offset'], comparison=compare['name'], literal_value=integer_operand(literal['operands'][0]),
                required_comparison_truth=true_reaches,
                scope='Dominating branch evidence about this mode coordinate; applies to the copied mode only under the separately validated unchanged-prefix conditions.'))
        return guards

    def sentinel_exclusion(entry, target, formal_slot, expression):
        if not comparison_contract['current'] or expression.get('kind') != 'integer' or type(expression.get('value')) is not int:
            return None
        argc, count, frame = frame_contract(entry)
        if not 0 <= formal_slot < argc:
            return None
        coordinate = frame, formal_slot
        def reaches(start, blocked):
            pending, seen = [start], set()
            while pending:
                off = pending.pop()
                if off == target:
                    return True
                if off == blocked or off in seen:
                    continue
                charge(); seen.add(off)
                pending.extend(successors(by[off]))
            return False
        for index, load in enumerate(instructions):
            if load['name'] != 'SCRIPT_OP_LOAD_VARIABLE' or coordinates(load) != coordinate:
                continue
            chain = instructions[index:index+5]
            if len(chain) < 4 or chain[1]['name'] != 'SCRIPT_OP_PUSH_LITERAL':
                continue
            literal = integer_operand(chain[1]['operands'][0])
            if chain[1]['operands'][0]['tag'] not in (0,2) or type(literal) is not int or not -2147483648 <= literal < 2147483648:
                continue
            if chain[2]['name'] == 'SCRIPT_OP_NEGATE':
                if literal == -2147483648:
                    continue
                literal = -literal
                compare, jump = chain[3:5]
            else:
                chain = chain[:4]
                compare, jump = chain[2:4]
            if literal != expression['value'] or compare['name'] not in ('SCRIPT_OP_EQUAL','SCRIPT_OP_NOT_EQUAL') or jump['name'] != 'SCRIPT_OP_JUMP_IF_ZERO':
                continue
            if any(incoming.get(right['offset'],set()) != {left['offset']} or successors(left) != [right['offset']] for left,right in zip(chain,chain[1:])):
                continue
            true = [e['offset'] for e in jump['successors'] if e['kind']=='fallthrough']
            false = [e['offset'] for e in jump['successors'] if e['kind']=='branch']
            if len(true)!=1 or len(false)!=1 or reaches(entry,jump['offset']):
                continue
            selected = true[0] if compare['name']=='SCRIPT_OP_EQUAL' else false[0]
            other = false[0] if compare['name']=='SCRIPT_OP_EQUAL' else true[0]
            if reaches(selected,jump['offset']) or not reaches(other,jump['offset']):
                continue
            # A sentinel arm that revisits the guard may redefine the formal
            # before a later successful test; this is not a one-pass exclusion.
            pending, visited = [selected], set()
            guard_offsets = {r['offset'] for r in chain}
            revisits_guard = False
            while pending:
                off = pending.pop()
                if off in guard_offsets:
                    revisits_guard = True
                    break
                if off in visited:
                    continue
                charge(); visited.add(off)
                pending.extend(successors(by[off]))
            if revisits_guard:
                continue
            pending, prefix, natives = [load['offset']], set(), set()
            while pending:
                off = pending.pop()
                if off in prefix:
                    continue
                charge(); prefix.add(off)
                row = by[off]
                if off == entry:
                    continue
                if row['name'] not in KNOWN_OPERATIONS or row['name'] in ('SCRIPT_OP_ENTER_FRAME','SCRIPT_OP_STORE_ELEMENT','SCRIPT_OP_CALL'):
                    raise ValueError('sentinel prefix has unproved effects or rebinding')
                if row['name']=='SCRIPT_OP_STORE_VARIABLE':
                    coords = coordinates(row)
                    if coords==coordinate or not (coords[0]==frame and 0<=coords[1]<count or root_slot(coords)):
                        raise ValueError('sentinel prefix overwrites formal or has unbounded writer')
                if row['name']=='SCRIPT_OP_CALL_NATIVE':
                    natives.add(native_safe(row))
                if not predecessors.get(off):
                    raise ValueError('sentinel prefix lacks a valid entry')
                pending.extend(predecessors[off])
            return dict(status='conditional_sentinel_exclusion', actual_expression=expression,
                wrapper_entry=entry, excluded_inner_call_offset=target, formal_index=formal_slot,
                sentinel_value=literal, load_offset=load['offset'], comparison_offset=compare['offset'],
                branch_offset=jump['offset'], sentinel_branch_offset=selected,
                prefix_instruction_offsets=sorted(prefix), preserved_native_functions=sorted(natives),
                comparison_contract=comparison_contract,
                scope='Only this syntactic wrapper-to-shared call edge is excluded for this exact integer actual, under stable program/VM/binding, valid nonaliasing physical frames and full reviewed native prerequisites. Off-branch descendant calls, reentry, scheduler effects and whole-wrapper behavior are outside the claim. No selector policy or producer pair is inferred.')
        return None

    def non_cost_guard_evidence(call, formal_entry):
        if not comparison_contract['current']:
            raise ValueError('stale VM comparison contract: ' + str(comparison_contract['stale']))
        argc, _, _ = frame_contract(formal_entry)
        actual = arguments(call, argc)
        if len(actual) != 2 or actual[1]['kind'] != 'variable':
            return None
        mode_expr = actual[1]
        mode = mode_expr['frame'], mode_expr['slot']
        if not root_slot(mode):
            return None
        entry = owner(call['offset'])
        _, own_count, own_frame = frame_contract(entry)
        pending, prefix = [call['offset']], set()
        while pending:
            off = pending.pop()
            if off in prefix:
                continue
            charge()
            prefix.add(off)
            if off != entry:
                pending.extend(predecessors.get(off, []))
        for guard in forwarding_guards(entry, call['offset'], mode, prefix):
            equal = guard['comparison'] == 'SCRIPT_OP_EQUAL'
            requires_equal = guard['required_comparison_truth'] == equal
            literal = guard['literal_value']
            if not (requires_equal and literal != 6 or not requires_equal and literal == 6):
                continue
            jump = by[guard['branch_offset']]
            selected_kind = 'fallthrough' if guard['required_comparison_truth'] else 'branch'
            selected = [e['offset'] for e in jump['successors'] if e['kind'] == selected_kind]
            if len(selected) != 1:
                continue
            # Only suffix paths to this call are relevant; earlier producer effects
            # and off-path/post-call effects are deliberately not audited here.
            pending, ancestors = [call['offset']], set()
            while pending:
                off = pending.pop()
                if off in ancestors or off == guard['branch_offset']:
                    continue
                charge()
                ancestors.add(off)
                pending.extend(predecessors.get(off, []))
            pending, suffix, natives, helpers = selected[:], set(), set(), []
            while pending:
                off = pending.pop()
                if off == call['offset'] or off in suffix or off not in ancestors or off == guard['branch_offset']:
                    continue
                charge()
                suffix.add(off)
                row = by[off]
                if row['name'] not in KNOWN_OPERATIONS or row['name'] in ('SCRIPT_OP_ENTER_FRAME','SCRIPT_OP_STORE_ELEMENT'):
                    raise ValueError('non-cost guard suffix has unknown, rebinding or indirect effects')
                if row['name'] == 'SCRIPT_OP_STORE_VARIABLE':
                    coords = coordinates(row)
                    if coords == mode:
                        raise ValueError('non-cost guard suffix overwrites copied mode')
                    if not (coords[0] == own_frame and 0 <= coords[1] < own_count or root_slot(coords)):
                        raise ValueError('non-cost guard suffix has an unbounded borrowed writer')
                if row['name'] == 'SCRIPT_OP_CALL_NATIVE':
                    natives.add(native_safe(row))
                if row['name'] == 'SCRIPT_OP_CALL':
                    helpers.append(preserve_helper(integer_operand(row['operands'][0]), mode))
                pending.extend(successors(row))
            return dict(status='conditional_non_cost_caller', comparison_contract=comparison_contract, mode_expression=mode_expr,
                excluded_dispatch_mode=6, mode_domain='shared_helper_dispatch_mode', guard=guard,
                guard_relation='equal' if requires_equal else 'not_equal', suffix_instruction_offsets=sorted(suffix),
                intervening_native_functions=sorted(natives), intervening_helpers=helpers,
                scope='Only this dominating guard-to-call suffix preserves the copied finite numeric mode under original comparison semantics and valid disjoint root/caller/callee storage, balanced frames and frozen native prerequisites. Program/VM/frame bindings remain stable through the suffix, with no asynchronous mutation. Earlier producer preservation, policy meaning and whole-call execution are not established.')
        return None

    def caller_relation(call, formal_entry, formal_index, mode_index, *, mode_override=None, forwarding_stack=()):
        nonlocal work
        argc, _, _ = frame_contract(formal_entry)
        actual = arguments(call, argc)
        if not 0 <= formal_index < len(actual) or mode_index is not None and not 0 <= mode_index < len(actual):
            raise ValueError('forwarded argument index exceeds the called frame contract')
        result = actual[formal_index]
        mode_expr = mode_override if mode_index is None else actual[mode_index]
        if mode_expr['kind'] != 'variable' or result['kind'] not in ('variable','script_return'):
            raise ValueError('caller does not pass a local result and borrowed mode variable')
        mode = mode_expr['frame'], mode_expr['slot']
        own_entry = owner(call['offset'])
        own_argc, own_count, own_frame = frame_contract(own_entry)
        if own_entry in forwarding_stack:
            raise ValueError('recursive formal-forwarding caller chain')
        if len(forwarding_stack) >= 32:
            raise ValueError('formal-forwarding depth budget exhausted')
        if result['kind'] == 'script_return':
            if mode_index is not None or argc != 1 or formal_index != 0 or not root_slot(mode):
                raise ValueError('direct producer argument is outside the bounded one-argument forwarding form')
            producer_index = indices[call['offset']] - 1
            if producer_index < 0:
                raise ValueError('direct producer argument lacks a preceding call')
            producer = instructions[producer_index]
            if (producer['name'] != 'SCRIPT_OP_CALL' or
                integer_operand(producer['operands'][0]) != result['target_offset'] or
                producer['offset'] + producer['size'] != call['offset'] or
                incoming.get(call['offset'],set()) != {producer['offset']} or
                [e['offset'] for e in producer.get('successors',[]) if e['kind']=='return_continuation'] != [call['offset']]):
                raise ValueError('direct producer and wrapper calls are not adjacent exclusive continuations')
            relation = producer_relation(result['target_offset'],mode)
            selectors = sorted({pair['return_value'] for pair in relation['mode_return_pairs'] if pair['mode']==6})
            if not selectors or any(not 0<=value<len(policies) for value in selectors):
                raise ValueError('direct producer mode6 has no complete bounded policy domain')
            return dict(status='conditional_paired_caller_relation',caller_offset=call['offset'],caller_frame_entry=own_entry,
                arguments=actual,reaching_producer_stores=[],producer_relations=[relation],
                actual_producer_call=dict(instruction_offset=producer['offset'],target_offset=result['target_offset'],argument_expression=result),
                consumed_mode=6,conditional_cost_selectors=selectors,conditional_policies=[policies[value] for value in selectors],
                intervening_helpers=[],intervening_native_functions=[],
                scope='This adjacent exclusive producer-return to one-argument-wrapper call only. Valid disjoint frames/operand floors, stable program/VM/root binding and all frozen native prerequisites apply; no asynchronous mutation may change mode/result between the returning producer and wrapper. No fictitious local store or global formal substitution is inferred.')
        if result['frame'] != own_frame or not 0 <= result['slot'] < own_count:
            raise ValueError('caller result is not a bounded own-frame local')
        pending, seen, stores, helper_proofs, native_proofs = list(predecessors.get(call['offset'], [])), set(), [], [], set()
        reached_formal, preservation_errors = False, []
        while pending:
            off = pending.pop()
            if off in seen:
                continue
            charge()
            seen.add(off)
            row = by[off]
            if off == own_entry:
                if result['slot'] < own_argc:
                    reached_formal = True
                    continue
                raise ValueError('caller path lacks a producer result store')
            if row['name'] not in KNOWN_OPERATIONS or row['name'] == 'SCRIPT_OP_ENTER_FRAME':
                raise ValueError('caller has unknown effects or competing frame entry')
            if row['name'] == 'SCRIPT_OP_STORE_ELEMENT':
                raise ValueError('caller has an intervening indexed writer')
            if row['name'] == 'SCRIPT_OP_STORE_VARIABLE':
                coords = coordinates(row)
                if coords == mode:
                    raise ValueError('caller overwrites the paired mode')
                if coords == (result['frame'], result['slot']):
                    expression = arguments(row, 1)[0]
                    if expression['kind'] != 'script_return':
                        raise ValueError('caller result is not exclusively a script-return producer')
                    stores.append(dict(instruction_offset=off, expression=expression))
                    continue
                if not (coords[0] == own_frame and 0 <= coords[1] < own_count or root_slot(coords)):
                    raise ValueError('caller has an unbounded borrowed writer')
            if row['name'] == 'SCRIPT_OP_CALL_NATIVE':
                try:
                    native_proofs.add(native_safe(row))
                except ValueError as error:
                    preservation_errors.append(str(error))
            if row['name'] == 'SCRIPT_OP_CALL':
                try:
                    helper_proofs.append(preserve_helper(integer_operand(row['operands'][0]), mode))
                except ValueError as error:
                    preservation_errors.append(str(error))
            pending.extend(predecessors.get(off, []))
        if reached_formal:
            if stores:
                raise ValueError('ambiguous formal forwarding mixes entry arguments and local redefinitions')
            if not root_slot(mode):
                raise ValueError('forwarded mode is not a bounded borrowed root slot')
            upstream = []
            for outer in instructions:
                if outer['name'] != 'SCRIPT_OP_CALL' or integer_operand(outer['operands'][0]) != own_entry:
                    continue
                try:
                    relation = caller_relation(outer, own_entry, result['slot'], None,
                        mode_override=mode_expr, forwarding_stack=(*forwarding_stack, own_entry))
                except (ValueError, KeyError, IndexError) as error:
                    relation = dict(status='unresolved_caller_relation', caller_offset=outer['offset'], reason=str(error))
                    try:
                        relation['arguments'] = arguments(outer, own_argc)
                    except ValueError as argument_error:
                        relation['argument_recovery_reason'] = str(argument_error)
                prior_work = work
                work = 0
                try:
                    actual_outer = relation['arguments'] if 'arguments' in relation else arguments(outer, own_argc)
                    exclusion = sentinel_exclusion(own_entry, call['offset'], result['slot'], actual_outer[result['slot']])
                    if exclusion is not None:
                        relation['conditional_sentinel_exclusion'] = exclusion
                except (ValueError, KeyError, IndexError) as error:
                    relation['sentinel_exclusion_diagnostics'] = [str(error)]
                finally:
                    work = prior_work
                upstream.append(relation)
            supported = any(relation['status'] in ('conditional_paired_caller_relation', 'conditional_formal_forwarding_relation')
                            for relation in upstream)
            prefix_valid = not preservation_errors
            return dict(status='conditional_formal_forwarding_relation' if prefix_valid and supported else 'unresolved_caller_relation',
                        caller_offset=call['offset'], caller_frame_entry=own_entry, arguments=actual,
                        reason=(preservation_errors[0] if preservation_errors else
                                'No supported concrete upstream caller.' if not supported else
                                'Forwarded alternatives apply only through the individually supported upstream callsites.'),
                        formal_forwarding=dict(wrapper_entry=own_entry, formal_index=result['slot'],
                            argument_count=own_argc, local_count=own_count, frame_index=own_frame,
                            mode_coordinate=dict(frame=mode[0], slot=mode[1]),
                            prefix_instruction_offsets=sorted(seen),
                            mode_guards=forwarding_guards(own_entry, call['offset'], mode, seen),
                            prefix_preservation_proven=prefix_valid,
                            prefix_preservation_diagnostics=preservation_errors,
                            intervening_helpers=helper_proofs, intervening_native_functions=sorted(native_proofs),
                            upstream_callers=upstream, all_recovered_upstream_callers_supported=bool(upstream) and all(
                                relation['status'] in ('conditional_paired_caller_relation', 'conditional_formal_forwarding_relation') for relation in upstream),
                            universal_formal_domain_established=False,
                            scope='Only the unchanged entry argument is mapped to each separate actual caller expression. Upstream relations do not apply through an unproved wrapper prefix; unknown callers remain unresolved.'),
                        scope='Conditional call-chain evidence under valid disjoint physical frames, stable program/VM/root mode and balanced nested rebinding. All per-native/deferred prerequisites and no asynchronous mutation through waits remain mandatory. No global substitution or scheduler/path-feasibility proof.')
        if preservation_errors:
            raise ValueError(preservation_errors[0])
        if not stores:
            raise ValueError('no reaching producer result stores')
        relations = [producer_relation(store['expression']['target_offset'], mode) for store in stores]
        selectors = sorted({pair['return_value'] for relation in relations for pair in relation['mode_return_pairs'] if pair['mode'] == 6})
        if not selectors or any(not 0 <= value < len(policies) for value in selectors):
            raise ValueError('mode6 has no complete bounded policy domain')
        return dict(status='conditional_paired_caller_relation', caller_offset=call['offset'], caller_frame_entry=own_entry,
                    arguments=actual, reaching_producer_stores=stores, producer_relations=relations,
                    consumed_mode=6, conditional_cost_selectors=selectors,
                    conditional_policies=[policies[value] for value in selectors],
                    intervening_helpers=helper_proofs, intervening_native_functions=sorted(native_proofs),
                    scope='This caller only, with preserved paired mode/result and valid physically nonaliasing active root/caller/callee frames. Stable decoded program, VM and frame bindings throughout, balanced calls and no asynchronous mutation are required. All frozen native/deferred/SDK review prerequisites apply. Static alternatives do not prove feasibility or selected runtime policy.')

    def consumer_mode_guard(entry, target, frame):
        def reaches(start, destination, excluded):
            pending, seen = [start], set()
            while pending:
                off = pending.pop()
                if off == destination:
                    return True
                if off in seen or off in excluded:
                    continue
                charge()
                seen.add(off)
                pending.extend(successors(by[off]))
            return False
        # The copied formal mode/result must survive the path to this native call.
        pending, ancestors = [target], set()
        while pending:
            off = pending.pop()
            if off in ancestors or off == entry:
                continue
            charge()
            ancestors.add(off)
            pending.extend(predecessors.get(off, []))
        pending, seen, preservation = [entry], set(), set()
        _, count, _ = frame_contract(entry)
        while pending:
            off = pending.pop()
            if off == target or off in seen or off != entry and off not in ancestors:
                continue
            charge()
            seen.add(off)
            row = by[off]
            if row['name'] not in KNOWN_OPERATIONS or row['name'] == 'SCRIPT_OP_STORE_ELEMENT' or off != entry and row['name'] == 'SCRIPT_OP_ENTER_FRAME':
                raise ValueError('consumer path has unknown, indirect or competing-frame effects')
            if row['name'] == 'SCRIPT_OP_CALL_NATIVE':
                preservation.add(native_safe(row))
            if row['name'] == 'SCRIPT_OP_CALL':
                raise ValueError('consumer path contains an unmodeled script call')
            if row['name'] == 'SCRIPT_OP_STORE_VARIABLE':
                coords = coordinates(row)
                if coords in ((frame, 0), (frame, 1)):
                    raise ValueError('consumer path overwrites the copied selector or mode formal')
                if not (coords[0] == frame and 0 <= coords[1] < count or root_slot(coords)):
                    raise ValueError('consumer path contains an unbounded borrowed writer')
            pending.extend(successors(row))
        for index in range(len(instructions)-3):
            load, literal, compare, jump = instructions[index:index+4]
            if load['name'] != 'SCRIPT_OP_LOAD_VARIABLE' or coordinates(load) != (frame, 1):
                continue
            if [literal['name'], compare['name'], jump['name']] != ['SCRIPT_OP_PUSH_LITERAL', 'SCRIPT_OP_EQUAL', 'SCRIPT_OP_JUMP_IF_ZERO']:
                continue
            if literal['operands'][0]['tag'] not in (0,2) or integer_operand(literal['operands'][0]) != 6:
                continue
            if any(incoming.get(right['offset'], set()) != {left['offset']} or successors(left) != [right['offset']]
                   for left,right in ((load,literal),(literal,compare),(compare,jump))):
                continue
            true = [e['offset'] for e in jump['successors'] if e['kind']=='fallthrough']
            false = [e['offset'] for e in jump['successors'] if e['kind']=='branch']
            if len(true)!=1 or len(false)!=1:
                continue
            if reaches(entry,target,{jump['offset']}) or reaches(false[0],target,{jump['offset']}):
                continue
            if reaches(true[0],target,{jump['offset']}):
                return dict(load_offset=load['offset'], comparison_offset=compare['offset'], branch_offset=jump['offset'], required_mode=6, preserved_native_functions=sorted(preservation))
        raise ValueError('consumer cost request lacks an unambiguous mode6 guard')

    for request in requests:
        expression = request.get('arguments', {}).get('cost_selector', {})
        if expression.get('kind') != 'variable' or request.get('conditional_local_policy_alternatives'):
            continue
        work = 0
        inventory = []
        try:
            entry = owner(request['instruction_offset'])
            argc, count, frame = frame_contract(entry)
            if expression['frame'] != frame or expression['slot'] >= argc or argc != 2 or expression['slot'] != 0:
                continue
            offsets = body(entry)
            if any(by[off]['name'] in ('SCRIPT_OP_STORE_ELEMENT',) or by[off]['name'] == 'SCRIPT_OP_STORE_VARIABLE' and coordinates(by[off]) in ((frame, 0),(frame,1)) for off in offsets):
                raise ValueError('shared function can overwrite the selector formal')
            guard = consumer_mode_guard(entry, request['instruction_offset'], frame)
            request['consumer_mode_guard'] = guard
            for call in instructions:
                if call['name'] != 'SCRIPT_OP_CALL' or integer_operand(call['operands'][0]) != entry:
                    continue
                try:
                    result = caller_relation(call, entry, 0, 1)
                except (ValueError, KeyError, IndexError) as error:
                    result = dict(status='unresolved_caller_relation', caller_offset=call['offset'], reason=str(error))
                relation_work = work
                work = 0
                try:
                    exclusion = non_cost_guard_evidence(call, entry)
                    if exclusion is not None:
                        result['conditional_non_cost_caller'] = exclusion
                except (ValueError, KeyError, IndexError) as error:
                    result['non_cost_guard_diagnostics'] = [str(error)]
                finally:
                    work = relation_work
                inventory.append(result)
            request['conditional_caller_policy_relations'] = inventory
            request['universal_policy_domain_established'] = False
            request['caller_relation_scope'] = 'Recovered direct callers are a static inventory, not an exhaustive runtime-entry proof. Unsupported callers and the raw shared formal remain unresolved.'
        except (ValueError, KeyError, IndexError) as error:
            request['caller_relation_diagnostics'] = [str(error)]
