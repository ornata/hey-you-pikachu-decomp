"""Bounded return-domain summaries for decoded script functions.

The graph follows returning call continuations without entering callee bodies.
Domains describe static returning paths, not termination, path feasibility, pure
functions, or arbitrary native side effects that replace execution contexts.
"""

from .formats import integer_operand
from .stack import incoming_edges, recover_arguments
from .table_returns import guarded_table_return


def function_summary(instructions, entry, limit=10000):
    """Inventory return expressions while keeping other frame entries separate."""
    by_offset = {row['offset']: row for row in instructions}
    indices = {row['offset']: index for index, row in enumerate(instructions)}
    frames = {offset: row for offset, row in by_offset.items() if row['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    incoming = incoming_edges(instructions)
    frame = frames.get(entry)
    if frame is None:
        raise ValueError('function summary requires a decoded frame entry')
    arguments, locals_count, frame_index = map(integer_operand, frame['operands'])
    if arguments is None or locals_count is None or frame_index is None or not 0 <= arguments <= locals_count or not 0 <= frame_index < 32:
        raise ValueError('invalid function frame contract')
    visited, pending, returns, calls, exits, diagnostics = set(), [entry], [], [], [], []
    while pending:
        offset = pending.pop()
        if offset in visited:
            continue
        if len(visited) >= limit:
            diagnostics.append('function instruction budget exhausted')
            break
        visited.add(offset)
        row = by_offset.get(offset)
        if row is None:
            diagnostics.append(f'{offset:#x}: missing decoded instruction')
            continue
        if offset != entry and row['name'] == 'SCRIPT_OP_ENTER_FRAME':
            diagnostics.append(f'{offset:#x}: flow enters another frame')
            continue
        if row['name'] == 'SCRIPT_OP_RETURN':
            result = dict(instruction_offset=offset)
            try:
                expression = recover_arguments(instructions, indices[offset], 1, incoming=incoming,
                                               allow_float=True, allow_symbolic=True, frame_entries=frames)[0]
                result['expression'] = expression
                if expression['kind'] in ('integer', 'float') and -2147483648 <= expression['value'] < 2147483648:
                    result['integer_conversion'] = int(expression['value'])
                elif expression['kind'] == 'array_element':
                    lookup = guarded_table_return(instructions, indices[offset], incoming, frame_index, locals_count)
                    if lookup is not None:
                        result['resolved_lookup'] = lookup
            except ValueError as error:
                result['reason'] = str(error)
            returns.append(result)
            continue
        if row['name'] == 'SCRIPT_OP_FINISH':
            exits.append(offset)
            continue
        successors = row.get('successors', [])
        if row['name'] == 'SCRIPT_OP_CALL':
            calls.append(dict(instruction_offset=offset, target_offset=integer_operand(row['operands'][0])))
            successors = [edge for edge in successors if edge['kind'] == 'return_continuation']
        if not successors:
            diagnostics.append(f'{offset:#x}: no understood continuation')
        pending.extend(edge['offset'] for edge in successors)
    complete = bool(returns) and not diagnostics and all('integer_conversion' in row or 'resolved_lookup' in row for row in returns)
    domain = {row['integer_conversion'] for row in returns if 'integer_conversion' in row}
    domain.update(value for row in returns for value in row.get('resolved_lookup', {}).get('integer_return_domain', []))
    return dict(entry_offset=entry, argument_count=arguments, local_count=locals_count, frame_index=frame_index,
                visited_instructions=len(visited), returns=sorted(returns, key=lambda row: row['instruction_offset']),
                integer_return_domain=sorted(domain),
                static_return_domain_complete=complete, calls=calls, nonreturning_exits=sorted(exits),
                diagnostics=diagnostics,
                scope='Integer-converted values on static returning paths with valid nonoverlapping active frames; no termination, path-feasibility or callee-purity proof.')


def action_producer_functions(instructions, calls):
    """Summarize functions whose return is used as a timed-program ID."""
    targets = {call['arguments']['script_id']['target_offset'] for call in calls
               if call['status'] == 'recovered_arguments' and call['arguments'].get('script_id', {}).get('kind') == 'script_return'}
    return [dict(role='timed_program_id_producer', **function_summary(instructions, target)) for target in sorted(targets)]
