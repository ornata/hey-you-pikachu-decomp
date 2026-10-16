"""Bounded backward stack expressions shared by script contract inventories.

Unknown effects, competing control-flow entries and unsupported conversions stop
recovery. Variable coordinates do not imply persistent roles or actor identities.
"""

import math

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


def recover_arguments(instructions, index, count, *, incoming=None, allow_float=False, allow_symbolic=False, frame_entries=None):
    """Recover arguments in push order within 32 contiguous producer instructions."""
    if incoming is None:
        incoming = incoming_edges(instructions)
    cursor = index
    by_offset = frame_entries if frame_entries is not None else ({row['offset']: row for row in instructions} if allow_symbolic else {})

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
        if allow_symbolic and row['name'] == 'SCRIPT_OP_LOAD_ELEMENT':
            frame, slot = map(integer_operand, row['operands'])
            if frame is None or slot is None:
                raise ValueError('Array reference is unresolved.')
            return dict(kind='array_element', frame=frame, slot=slot, index=argument())
        if allow_symbolic and row['name'] == 'SCRIPT_OP_CALL':
            target = integer_operand(row['operands'][0])
            entry = by_offset.get(target, {})
            if entry.get('name') != 'SCRIPT_OP_ENTER_FRAME':
                raise ValueError('Script result needs a known frame-entry contract.')
            arity, locals_count, frame = map(integer_operand, entry['operands'])
            if arity is None or locals_count is None or frame is None or not 0 <= arity <= locals_count or not 0 <= frame < 32:
                raise ValueError('Script frame contract is invalid.')
            return dict(kind='script_return', target_offset=target,
                        arguments=list(reversed([argument() for _ in range(arity)])),
                        scope='Value on a returning path; callee behavior and path feasibility are not inferred.')
        if row['name'] == 'SCRIPT_OP_CALL_NATIVE':
            functions = row.get('native', {}).get('functions', [])
            if len(functions) == 1 and functions[0] in HANDLE_GETTERS:
                return dict(kind='handle', role=HANDLE_GETTERS[functions[0]],
                            function=functions[0])
            if functions == ['actorScriptPassThroughInteger']:
                operand = argument()
                expression = dict(operator='to_integer', function=functions[0], operand=operand)
                if operand['kind'] in ('integer', 'float'):
                    value = operand['value']
                    if math.isfinite(value) and -2147483648 <= value < 2147483648:
                        return dict(kind='integer', value=int(value), expression=expression)
                    raise ValueError('Integer conversion is outside the supported signed-32 domain.')
                if allow_symbolic:
                    return dict(kind='integer_conversion', function=functions[0], operand=operand,
                                scope='Original numeric conversion to signed32 with truncation; input type, finite representability and execution remain preconditions.')
                raise ValueError('Integer conversion needs a known numeric operand.')
            if allow_symbolic and len(functions) == 1 and functions[0] in (
                    'actorScriptGetProgressValue', 'actorScriptGetProgressMinimum',
                    'actorScriptGetProgressMaximum'):
                return dict(kind='native_result', function=functions[0], arguments=[argument()], result_tag='integer',
                            scope='Runtime progress read; selector conversion, initialized storage and current-area validity remain preconditions. No saved value is inferred.')
            if allow_symbolic and functions == ['actorScriptGetItemKind']:
                return dict(kind='native_result', function=functions[0], arguments=[argument()], result_tag='integer',
                            scope='Runtime item-kind lookup from an integer-converted handle; zero for unavailable or non-item objects. No live handle or constant kind is inferred.')
            if allow_symbolic and functions == ['actorScriptPushTargetDistanceAllowance']:
                return dict(kind='native_result', function=functions[0], arguments=[], result_tag='float',
                            scope='Runtime actor target-distance allowance; no numeric range or target identity is inferred.')
            if allow_symbolic and functions == ['actorScriptRandomBelow']:
                bound = argument()
                result = dict(kind='native_result', function=functions[0], arguments=[bound])
                if bound['kind'] == 'integer':
                    result['integer_range'] = [0, max(0, bound['value'] - 1)]
                return result
        if row['name'] == 'SCRIPT_OP_NEGATE':
            operand = argument()
            if operand['kind'] in ('integer', 'float') and (operand['kind'] != 'integer' or operand['value'] != -2147483648):
                return dict(kind=operand['kind'], value=-operand['value'],
                            expression=dict(operator='negate', operand=operand))
            raise ValueError('Negation needs unresolved or overflowing arithmetic.')
        if row['name'] in ('SCRIPT_OP_ADD', 'SCRIPT_OP_SUBTRACT', 'SCRIPT_OP_MULTIPLY',
                           'SCRIPT_OP_DIVIDE', 'SCRIPT_OP_REMAINDER'):
            operator = {'SCRIPT_OP_ADD': 'add', 'SCRIPT_OP_SUBTRACT': 'subtract',
                        'SCRIPT_OP_MULTIPLY': 'multiply', 'SCRIPT_OP_DIVIDE': 'divide',
                        'SCRIPT_OP_REMAINDER': 'remainder'}[row['name']]
            right, left = argument(), argument()
            if left['kind'] == right['kind'] == 'integer':
                if operator in ('divide', 'remainder'):
                    if right['value'] == 0 or (left['value'] == -2147483648 and right['value'] == -1):
                        raise ValueError('Integer division or remainder is outside the supported signed-32 domain.')
                    quotient = abs(left['value']) // abs(right['value'])
                    if (left['value'] < 0) != (right['value'] < 0):
                        quotient = -quotient
                    value = quotient if operator == 'divide' else left['value'] - quotient * right['value']
                elif operator == 'multiply':
                    value = left['value'] * right['value']
                else:
                    value = left['value'] + right['value'] if operator == 'add' else left['value'] - right['value']
                if -2147483648 <= value <= 2147483647:
                    return dict(kind='integer', value=value,
                                expression=dict(operator='integer_' + operator, left=left, right=right))
            elif allow_symbolic:
                return dict(kind='vm_operator', operator=operator, operands=[left, right],
                            scope='Original VM conversion/arithmetic semantics; numeric type and definedness are not established.')
            raise ValueError('Arithmetic needs nonconstant or overflowing integer analysis.')
        raise ValueError('Argument needs stack-flow or native-call analysis.')

    return list(reversed([argument() for _ in range(count)]))
