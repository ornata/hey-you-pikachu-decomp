"""Invented script senders exercise operand order and alternate-entry rejection."""

from tools.script_decoder.messages import message_posts


def sender():
    rows = []
    for offset, value in enumerate((99, 13, 1, 2, 3, 4)):
        rows.append(dict(offset=offset, size=1, name='SCRIPT_OP_PUSH_LITERAL',
                         operands=[dict(tag=0, value=value)],
                         successors=[dict(offset=offset + 1)]))
    rows.append(dict(offset=6, size=1, name='SCRIPT_OP_CALL_NATIVE',
                     native=dict(functions=['actorScriptPostMessage']), successors=[]))
    return rows


def test_post_arguments_preserve_stack_order_and_variable_identity():
    rows = sender()
    rows[0].update(name='SCRIPT_OP_LOAD_VARIABLE',
                   operands=[dict(tag=0, value=1), dict(tag=0, value=9)])
    result = message_posts(rows)[0]
    assert result['status'] == 'recovered_arguments'
    assert result['arguments'] == dict(
        recipient_handle=dict(kind='variable', frame=1, slot=9),
        **{key: dict(kind='integer', value=value) for key, value in
           zip(('type', 'arg0', 'arg2', 'arg3', 'arg4'), (13, 1, 2, 3, 4))})
    assert result['arg1'] == 0 and result['forced'] is False


def test_post_analysis_rejects_branch_into_middle_and_unknown_stack_effect():
    rows = sender()
    rows.append(dict(offset=10, successors=[dict(offset=3)]))
    assert message_posts(rows)[0]['status'] == 'unresolved'
    rows = sender()
    rows[2]['name'] = 'SCRIPT_OP_CALL_NATIVE'
    assert message_posts(rows)[0]['status'] == 'unresolved'
    rows = sender()
    rows[2]['operands'][0]['tag'] = 1
    assert message_posts(rows)[0]['status'] == 'unresolved'
