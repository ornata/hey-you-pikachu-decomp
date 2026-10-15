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


def relink(rows):
    for offset, row in enumerate(rows):
        row.update(offset=offset, size=1, successors=[dict(offset=offset + 1)] if offset < len(rows) - 1 else [])
    return rows


def test_handle_getters_recover_recipient_and_sender_without_assuming_validity():
    rows = sender()
    rows[0].update(name='SCRIPT_OP_CALL_NATIVE',
                   native=dict(functions=['actorScriptPushPikachuHandle']))
    rows[3].update(name='SCRIPT_OP_CALL_NATIVE',
                   native=dict(functions=['actorScriptPushSelfHandle']))
    result = message_posts(rows)[0]
    assert result['arguments']['recipient_handle']['role'] == 'pikachu'
    assert result['arguments']['arg2']['role'] == 'executing_actor'


def test_constant_addition_retains_expression_and_rejects_overflow_or_unknown_types():
    rows = sender()
    extra = [dict(name='SCRIPT_OP_PUSH_LITERAL', operands=[dict(tag=0, value=1)]),
             dict(name='SCRIPT_OP_ADD', operands=[])]
    rows = relink(rows[:3] + extra + rows[3:])
    result = message_posts(rows)[0]
    assert result['arguments']['arg0']['value'] == 2
    assert result['arguments']['arg0']['expression']['operator'] == 'integer_add'
    rows[2]['operands'][0]['value'] = 2147483647
    assert message_posts(rows)[0]['status'] == 'unresolved'
    rows[2]['operands'][0]['tag'] = 1
    assert message_posts(rows)[0]['status'] == 'unresolved'
