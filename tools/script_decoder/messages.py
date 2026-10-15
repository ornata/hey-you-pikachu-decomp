"""Recover bounded actor-message arguments from straight-line script producers.

The retained native handler pops arg4, arg3, arg2, arg0, type and recipient,
sets arg1 to zero, and pushes delivery success. Variable references remain
references; this analysis does not assign persistent meanings to reused slots.
"""

from .stack import incoming_edges, recover_arguments


def message_posts(instructions):
    """Recover six arguments within 32 straight-line producer instructions.

Only direct pushes, reviewed zero-argument getters and representable constant
integer additions/negations are supported. Unknown stack effects and competing entries
stop recovery. This does not follow variable definitions or script calls.
"""
    incoming = incoming_edges(instructions)
    result = []
    for index, call in enumerate(instructions):
        if call.get('native', {}).get('functions') != ['actorScriptPostMessage']:
            continue
        try:
            arguments = recover_arguments(instructions, index, 6, incoming=incoming)
            reason = None
        except ValueError as error:
            reason, arguments = str(error), []
        result.append(dict(instruction_offset=call['offset'],
                           status='unresolved' if reason else 'recovered_arguments',
                           **(dict(reason=reason) if reason else dict(arguments=dict(zip(
                               ('recipient_handle', 'type', 'arg0', 'arg2', 'arg3', 'arg4'), arguments)),
                               arg1=0, forced=False, result='delivery_success'))))
    return result
