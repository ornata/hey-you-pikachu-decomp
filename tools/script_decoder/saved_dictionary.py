"""Separate bytecode-local preservation from unresolved native alias effects.

The model follows returning call continuations and frame rebinding. It assumes
valid stack/array references and intact, nonoverlapping frames across yields.
Native callbacks remain an explicit review frontier; no runtime ID is invented.
"""

from .formats import integer_operand
from .stack import incoming_edges


def saved_dictionary_analysis(instructions, request_offset, expression, budget=50000, native_reviews=None):
    """Trace a dominating requested-ID save and audit writes across script calls."""
    if expression.get('kind') != 'variable':
        return None
    native_reviews = native_reviews or {}
    by = {row['offset']: row for row in instructions}
    indices = {row['offset']: i for i, row in enumerate(instructions)}
    entries = [offset for offset, row in by.items() if offset < request_offset and row['name'] == 'SCRIPT_OP_ENTER_FRAME']
    if not entries:
        return None
    entry = max(entries)
    frame = integer_operand(by[entry]['operands'][2])
    slot = expression['slot']
    if expression['frame'] != frame:
        return None
    incoming = incoming_edges(instructions)
    graphs, diagnostics, hazards, natives, audited = {}, [], [], set(), set()
    remaining = budget

    def graph(start):
        nonlocal remaining
        if start in graphs:
            return graphs[start]
        header = by.get(start, {})
        if header.get('name') != 'SCRIPT_OP_ENTER_FRAME':
            diagnostics.append(f'{start:#x}: missing callee frame contract')
            return {}
        argc, locals_count, own_frame = map(integer_operand, header['operands'])
        if None in (argc, locals_count, own_frame) or not 0 <= argc <= locals_count or not 0 <= own_frame < 32:
            diagnostics.append(f'{start:#x}: invalid frame contract')
            return {}
        result, pending = {}, [start]
        graphs[start] = result
        while pending:
            offset = pending.pop()
            if offset in result:
                continue
            if remaining <= 0:
                diagnostics.append('saved-local analysis budget exhausted')
                break
            remaining -= 1
            row = by.get(offset)
            if row is None or offset != start and row['name'] == 'SCRIPT_OP_ENTER_FRAME':
                diagnostics.append(f'{offset:#x}: missing instruction or competing frame entry')
                continue
            edges = row.get('successors', [])
            if row['name'] in ('SCRIPT_OP_RETURN', 'SCRIPT_OP_FINISH'):
                edges = []
            elif row['name'] == 'SCRIPT_OP_CALL':
                edges = [edge for edge in edges if edge['kind'] == 'return_continuation']
            if not edges and row['name'] not in ('SCRIPT_OP_RETURN', 'SCRIPT_OP_FINISH'):
                diagnostics.append(f'{offset:#x}: no understood continuation')
            result[offset] = [edge['offset'] for edge in edges]
            pending.extend(result[offset])
        return result

    def reach(edges, start, *, omit=None, stop=None):
        seen, pending = set(), [start]
        while pending:
            node = pending.pop()
            if node in seen or node == omit:
                continue
            seen.add(node)
            if node != stop:
                pending.extend(edges.get(node, []))
        return seen

    caller = graph(entry)
    if request_offset not in caller or diagnostics:
        return None
    candidates = []
    for offset in caller:
        row = by[offset]
        if row['name'] != 'SCRIPT_OP_STORE_VARIABLE' or tuple(map(integer_operand, row['operands'])) != (frame, slot):
            continue
        index = indices[offset]
        if not index:
            continue
        previous = instructions[index - 1]
        if previous.get('native', {}).get('functions') == ['actorScriptQueryRequestedMicDictionary'] and (
                previous['offset'] + previous['size'] == offset and incoming.get(offset) == {previous['offset']}):
            candidates.append((offset, previous['offset']))
    dominating = [(save, query) for save, query in candidates if request_offset not in reach(caller, entry, omit=save)]
    if len(dominating) != 1:
        return None
    save, query = dominating[0]
    natives.add('actorScriptQueryRequestedMicDictionary')
    reverse = {}
    for node, successors in caller.items():
        for successor in successors:
            reverse.setdefault(successor, []).append(node)
    corridor = reach(caller, save, stop=request_offset) & reach(reverse, request_offset)

    def audit(start, visible, nodes=None):
        key = (start, visible)
        if key in audited:
            return
        if len(audited) >= 256:
            diagnostics.append('saved-local function-view budget exhausted')
            return
        audited.add(key)
        edges = graph(start)
        own_frame = integer_operand(by[start]['operands'][2]) if start in by else None
        local_count = integer_operand(by[start]['operands'][1]) if start in by else None
        for offset in edges if nodes is None else nodes:
            row = by[offset]
            if row['name'] in ('SCRIPT_OP_STORE_VARIABLE', 'SCRIPT_OP_STORE_ELEMENT'):
                reference, index = map(integer_operand, row['operands'])
                if reference == own_frame and (index is None or not 0 <= index < local_count):
                    hazards.append(dict(offset=offset, reason='local base index outside the declared frame'))
                if visible and reference == frame and offset != save and (
                        row['name'] == 'SCRIPT_OP_STORE_ELEMENT' or index == slot):
                    hazards.append(dict(offset=offset, reason='direct or possible indexed write to saved caller local'))
            if row['name'] == 'SCRIPT_OP_CALL_NATIVE' and offset != request_offset:
                names = row.get('native', {}).get('functions', [])
                natives.update(names or ['<unresolved native>'])
            if row['name'] == 'SCRIPT_OP_CALL':
                target = integer_operand(row['operands'][0])
                header = by.get(target, {})
                if header.get('name') != 'SCRIPT_OP_ENTER_FRAME':
                    diagnostics.append(f'{offset:#x}: unresolved callee frame')
                    continue
                child_frame = integer_operand(header['operands'][2])
                audit(target, visible and child_frame != frame)

    audit(entry, True, corridor)
    frontier = sorted(natives - set(native_reviews))
    return dict(kind='saved_requested_dictionary_candidate', function_entry=entry, query_offset=query,
                save_offset=save, request_offset=request_offset, local=dict(frame=frame, slot=slot),
                save_dominates_request=True, bytecode_preserves_saved_local=not hazards and not diagnostics,
                script_function_views=len(audited), write_hazards=hazards, diagnostics=diagnostics,
                native_effect_frontier=frontier,
                reviewed_native_effects=sorted(natives & set(native_reviews)),
                origin_effect_reviewed='actorScriptQueryRequestedMicDictionary' in native_reviews,
                scope='Static bytecode preservation under valid stack/array accesses and intact nonoverlapping frames; native effects, context replacement, runtime restoration and numeric ID remain unproven.')
