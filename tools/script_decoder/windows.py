"""Recover deferred text-window settings without treating a request as an open window."""

from .stack import incoming_edges, recover_arguments


def text_window_requests(instructions):
    """Describe pushed settings and the later handler's conditional consumption."""
    incoming = incoming_edges(instructions)
    frames = {row['offset']: row for row in instructions if row['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    result = []
    for index, instruction in enumerate(instructions):
        if instruction.get('native', {}).get('functions') != ['actorScriptStartTextWindow']:
            continue
        row = dict(instruction_offset=instruction['offset'], function='actorScriptStartTextWindow',
                   consumption='deferred_until_outside_text_window_mode',
                   completion_result='integer_zero_even_if_window_open_is_rejected',
                   scope='Structural settings and request semantics; conditional ownership/transport reviews are recorded separately in native_frame_effect_reviews. No proof that a window opens, renders, completes or is reachable.')
        try:
            values = recover_arguments(instructions, index, 3, incoming=incoming, allow_float=True,
                                       allow_symbolic=True, frame_entries=frames)
            row.update(status='recovered_arguments',
                       arguments=dict(zip(('indicator', 'style', 'scene_view_mode'), values)))
            converted = {name: int(value['value']) for name, value in row['arguments'].items()
                         if value['kind'] in ('integer', 'float') and -2147483648 <= value['value'] < 2147483648}
            if converted:
                row['constant_settings'] = converted
            if 'scene_view_mode' in converted:
                row['requests_scene_updates'] = converted['scene_view_mode'] != 0
        except ValueError as error:
            row.update(status='unresolved', reason=str(error))
        result.append(row)
    return result
