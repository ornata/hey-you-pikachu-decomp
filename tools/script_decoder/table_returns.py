"""Recognize bounded, guarded local-table returns without guessing stored values.

Only a contiguous initialization and return beneath an inclusive range guard is
supported. Every internal instruction must have its sole predecessor in that
sequence. This is a deliberately limited reaching-definition proof, not general
dataflow, alias analysis or a proof that a returning path executes.
"""

from .formats import integer_operand


def guarded_table_return(instructions, index, incoming, frame_index, local_count):
    """Return evidence for an initialized local lookup, or None if unproven."""
    def names(rows):
        return [row['name'].removeprefix('SCRIPT_OP_') for row in rows]

    def operands(row):
        return tuple(map(integer_operand, row['operands']))

    def literal(row):
        operand = row['operands'][0]
        return integer_operand(operand) if operand['tag'] == 0 else None

    if index < 5:
        return None
    tail = instructions[index - 4:index + 1]
    if names(tail) != ['LOAD_VARIABLE', 'PUSH_LITERAL', 'SUBTRACT', 'LOAD_ELEMENT', 'RETURN']:
        return None
    variable, base = operands(tail[0]), literal(tail[1])
    array_frame, slot = operands(tail[3])
    if array_frame != frame_index or slot is None or slot < 0 or base is None:
        return None
    if variable[0] is None or not -1 <= variable[0] < 32 or variable[1] is None or variable[1] < 0:
        return None
    # The bound source must remain outside the initialized frame. Overlapping
    # local references need alias/reaching-definition analysis beyond this pattern.
    if variable[0] == array_frame:
        return None
    cursor = index - 4
    stores = []
    while cursor >= 3 and len(stores) < 256:
        triple = instructions[cursor - 3:cursor]
        if names(triple) != ['PUSH_LITERAL', 'PUSH_LITERAL', 'STORE_ELEMENT'] or operands(triple[2]) != (array_frame, slot):
            break
        element, value = literal(triple[0]), literal(triple[1])
        if element is None or value is None:
            return None
        stores.append(dict(index=element, value=value, instruction_offset=triple[2]['offset']))
        cursor -= 3
    if not stores or cursor < 8:
        return None
    guard = instructions[cursor - 8:cursor]
    if names(guard) != ['LOAD_VARIABLE', 'PUSH_LITERAL', 'GREATER_OR_EQUAL',
                       'LOAD_VARIABLE', 'PUSH_LITERAL', 'LESS_OR_EQUAL', 'LOGICAL_AND', 'JUMP_IF_ZERO']:
        return None
    low, high = literal(guard[1]), literal(guard[4])
    # Small exact integer bounds keep both the integer and promoted float paths
    # in a representable, nonnegative index domain after truncation.
    if low != base or high is None or not 0 <= low <= high <= 65535 or high - low >= 256:
        return None
    if operands(guard[0]) != variable or operands(guard[3]) != variable:
        return None
    if slot + high - low >= local_count:
        return None
    ordered = list(reversed(stores))
    if [row['index'] for row in ordered] != list(range(high - low + 1)):
        return None
    region = instructions[cursor - 8:index + 1]
    start, end = region[0]['offset'], region[-1]['offset']
    if any(left['offset'] + left['size'] != right['offset'] or
           incoming.get(right['offset'], set()) != {left['offset']}
           for left, right in zip(region, region[1:])):
        return None
    branch = guard[-1]
    expected_edges = {('fallthrough', instructions[cursor]['offset'])}
    target = integer_operand(branch['operands'][0])
    if target is None or start <= target <= end:
        return None
    expected_edges.add(('branch', target))
    if {(edge['kind'], edge['offset']) for edge in branch.get('successors', [])} != expected_edges:
        return None
    return dict(kind='guarded_local_table', guard_offset=branch['offset'],
                variable=dict(frame=variable[0], slot=variable[1]), variable_range=[low, high],
                array=dict(frame=array_frame, first_element=slot), initialized_elements=ordered,
                integer_return_domain=sorted({row['value'] for row in stores}),
                scope='Values on this guarded returning path with valid nonoverlapping active frames; no reachability or whole-program state proof.')
