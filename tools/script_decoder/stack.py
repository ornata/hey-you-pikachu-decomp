"""Bounded backward stack expressions shared by script contract inventories.

Unknown effects, competing control-flow entries and unsupported conversions stop
recovery. Variable coordinates do not imply persistent roles or actor identities.
"""

from .formats import integer_operand

# Original entries consume no VM arguments, push one integer handle and return
# continue. Source and original-instruction contracts are reviewed separately
# from selectors, which depend on the configured command table.
HANDLE_GETTERS = {
    'actorScriptPushPikachuHandle': 'pikachu',     # 0x8011E674
    'actorScriptPushSelfHandle': 'executing_actor',  # 0x8011E6A8
    'actorScriptPushTargetHandle': 'available_target',  # 0x8011DBF8
    'actorScriptPushHeldItemHandle': 'held_item',  # 0x8011F574
}



def incoming_edges(instructions):
    """Index static predecessors, retaining competing entries into an expression."""
    incoming = {}
    for row in instructions:
        for edge in row.get('successors', []):
            incoming.setdefault(edge['offset'], set()).add(row['offset'])
    return incoming


def recover_arguments(instructions, index, count, *, incoming=None, allow_float=False):
    """Recover arguments in push order within 32 contiguous producer instructions."""
    if incoming is None:
        incoming = incoming_edges(instructions)
    cursor = index

    def argument():
        nonlocal cursor
        if cursor == 0 or index - cursor >= 32:
            raise ValueError('Argument exceeds the bounded producer window.')
        successor = instructions[cursor]
        cursor -= 1
        row = instructions[cursor]
        if row['offset'] + row['size'] != successor['offset']:
            raise ValueError('Argument producers are not contiguous.')
        if incoming.get(successor['offset'], set()) != {row['offset']}:
            raise ValueError('Another control-flow edge enters the argument sequence.')
        if row['name'] == 'SCRIPT_OP_PUSH_LITERAL':
            value = integer_operand(row['operands'][0])
            if allow_float and row['operands'][0]['tag'] == 1 and row['operands'][0].get('value') is not None:
                return dict(kind='float', value=row['operands'][0]['value'])
            if row['operands'][0]['tag'] not in (0, 2) or value is None:
                raise ValueError('Literal requires a non-integer conversion.')
            return dict(kind='integer', value=value)
        if row['name'] == 'SCRIPT_OP_LOAD_VARIABLE':
            frame, slot = map(integer_operand, row['operands'])
            if frame is None or slot is None:
                raise ValueError('Variable operand is unresolved.')
            return dict(kind='variable', frame=frame, slot=slot)
        if row['name'] == 'SCRIPT_OP_CALL_NATIVE':
            functions = row.get('native', {}).get('functions', [])
            if len(functions) == 1 and functions[0] in HANDLE_GETTERS:
                return dict(kind='handle', role=HANDLE_GETTERS[functions[0]],
                            function=functions[0])
        if row['name'] == 'SCRIPT_OP_NEGATE':
            operand = argument()
            if operand['kind'] in ('integer', 'float') and (operand['kind'] != 'integer' or operand['value'] != -2147483648):
                return dict(kind=operand['kind'], value=-operand['value'],
                            expression=dict(operator='negate', operand=operand))
            raise ValueError('Negation needs unresolved or overflowing arithmetic.')
        if row['name'] == 'SCRIPT_OP_ADD':
            right, left = argument(), argument()
            if left['kind'] == right['kind'] == 'integer':
                value = left['value'] + right['value']
                if -2147483648 <= value <= 2147483647:
                    return dict(kind='integer', value=value,
                                expression=dict(operator='integer_add', left=left, right=right))
            raise ValueError('Addition needs nonconstant or overflowing integer analysis.')
        raise ValueError('Argument needs stack-flow or native-call analysis.')

    return list(reversed([argument() for _ in range(count)]))
