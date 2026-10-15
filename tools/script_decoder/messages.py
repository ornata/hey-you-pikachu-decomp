"""Recover bounded actor-message arguments from straight-line script producers.

The retained native handler pops arg4, arg3, arg2, arg0, type and recipient,
sets arg1 to zero, and pushes delivery success. Variable references remain
references; this analysis does not assign persistent meanings to reused slots.
"""

from .formats import integer_operand


def message_posts(instructions):
    """Recognize six direct integer/lvalue pushes without a competing entry edge."""
    incoming = {}
    for row in instructions:
        for edge in row.get('successors', []):
            incoming.setdefault(edge['offset'], set()).add(row['offset'])
    result = []
    for index, call in enumerate(instructions):
        if call.get('native', {}).get('functions') != ['actorScriptPostMessage']:
            continue
        producers = instructions[max(0, index - 6):index]
        reason, arguments = None, []
        if len(producers) != 6:
            reason = 'Fewer than six preceding instructions.'
        else:
            for i, row in enumerate(producers):
                successor = producers[i + 1] if i < 5 else call
                if row['offset'] + row['size'] != successor['offset']:
                    reason = 'Argument producers are not contiguous.'
                    break
                if incoming.get(successor['offset'], set()) != {row['offset']}:
                    reason = 'Another control-flow edge enters the argument sequence.'
                    break
                if row['name'] == 'SCRIPT_OP_PUSH_LITERAL':
                    value = integer_operand(row['operands'][0])
                    if row['operands'][0]['tag'] not in (0, 2) or value is None:
                        reason = 'Literal requires a non-integer conversion.'
                        break
                    arguments.append(dict(kind='integer', value=value))
                elif row['name'] == 'SCRIPT_OP_LOAD_VARIABLE':
                    frame, slot = map(integer_operand, row['operands'])
                    if frame is None or slot is None:
                        reason = 'Variable operand is unresolved.'
                        break
                    arguments.append(dict(kind='variable', frame=frame, slot=slot))
                else:
                    reason = 'Argument needs stack-flow or native-call analysis.'
                    break
        result.append(dict(instruction_offset=call['offset'],
                           status='unresolved' if reason else 'recovered_arguments',
                           **(dict(reason=reason) if reason else dict(arguments=dict(zip(
                               ('recipient_handle', 'type', 'arg0', 'arg2', 'arg3', 'arg4'), arguments)),
                               arg1=0, forced=False, result='delivery_success'))))
    return result
