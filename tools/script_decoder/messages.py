"""Recover bounded actor-message arguments from straight-line script producers.

The retained native handler pops arg4, arg3, arg2, arg0, type and recipient,
sets arg1 to zero, and pushes delivery success. Variable references remain
references; this analysis does not assign persistent meanings to reused slots.
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


def message_posts(instructions):
    """Recover six arguments within 32 straight-line producer instructions.

Only direct pushes, reviewed zero-argument getters and representable constant
integer additions are supported. Unknown stack effects and competing entries
stop recovery. This does not follow variable definitions or script calls.
"""
    incoming = {}
    for row in instructions:
        for edge in row.get('successors', []):
            incoming.setdefault(edge['offset'], set()).add(row['offset'])
    result = []
    for index, call in enumerate(instructions):
        if call.get('native', {}).get('functions') != ['actorScriptPostMessage']:
            continue
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
            if row['name'] == 'SCRIPT_OP_ADD':
                right, left = argument(), argument()
                if left['kind'] == right['kind'] == 'integer':
                    value = left['value'] + right['value']
                    if -2147483648 <= value <= 2147483647:
                        return dict(kind='integer', value=value,
                                    expression=dict(operator='integer_add', left=left, right=right))
                raise ValueError('Addition needs nonconstant or overflowing integer analysis.')
            raise ValueError('Argument needs stack-flow or native-call analysis.')

        try:
            arguments = list(reversed([argument() for _ in range(6)]))
            reason = None
        except ValueError as error:
            reason, arguments = str(error), []
        result.append(dict(instruction_offset=call['offset'],
                           status='unresolved' if reason else 'recovered_arguments',
                           **(dict(reason=reason) if reason else dict(arguments=dict(zip(
                               ('recipient_handle', 'type', 'arg0', 'arg2', 'arg3', 'arg4'), arguments)),
                               arg1=0, forced=False, result='delivery_success'))))
    return result
