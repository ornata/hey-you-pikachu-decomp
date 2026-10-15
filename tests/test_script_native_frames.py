"""Source-review binding tests use invented callbacks, not heuristic approvals."""

from tools.script_decoder.native_frames import fingerprint, function_source, native_frame_reviews


def setup_review(tmp_path):
    (tmp_path / 'unit.c').write_text('void helper(void) { return; }\ns32 getter(void) { helper(); return 0; }')
    return dict(preconditions=['Invented bounded frame.'],
                dependencies=[dict(kind='function', file='unit.c', function='helper', sha256=fingerprint('void helper(void) { return; }'))],
                callbacks=[dict(function='getter', file='unit.c', sha256=fingerprint('s32 getter(void) { helper(); return 0; }'),
                                effect='append_scalar', arguments=0, stack_delta=1, result_tag='integer', scheduling='continue')])


def test_fingerprint_ignores_formatting_comments_but_distinguishes_operator_tokens():
    assert fingerprint('a += 2;') == fingerprint('a /* note */ +=\n 2;')
    assert fingerprint('a ++ + b;') != fingerprint('a + ++ b;')
    assert fingerprint('one two') != fingerprint('onetwo')
    assert fingerprint('"// literal"') != fingerprint('"// changed"')
    assert fingerprint("value['a']") != fingerprint("value['b']")
    definition = 's32 f(void) { const char *s = "}"; /* { */ return 0; }'
    assert function_source(definition, 'f') == definition
    pointer = 'Entry *lookup(s32 key) { return NULL; }'
    assert function_source(pointer, 'lookup') == pointer
    legacy = 's16 field(index, unused)\ns32 index;\ns32 unused;\n{ return index; }'
    assert function_source(legacy, 'field') == legacy


def test_checked_in_reviews_have_current_source_and_dependency_bindings():
    report = native_frame_reviews()
    assert not report['stale_dependencies'] and not report['stale_callbacks']
    assert not report['stale_deferred_reviews']
    assert 'actorScriptStartLoopExitAndWait' in report['active_deferred']
    assert 'actorScriptStartActionAndWait' in report['active_deferred']
    assert 'actorScriptQueryRequestedMicDictionary' in report['active']
    assert 'actorScriptPushTotalElapsedTime' in report['active']


def test_only_explicit_unchanged_review_activates_a_callback(tmp_path):
    manifest = setup_review(tmp_path)
    with (tmp_path / 'unit.c').open('a') as stream:
        stream.write('\ns32 otherGetter(void) { helper(); return 0; }')
    report = native_frame_reviews(tmp_path, manifest)
    assert set(report['active']) == {'getter'}
    assert not report['stale_callbacks'] and not report['stale_dependencies']
    assert 'no scheduler' in report['scope']


def test_changed_callback_revokes_review_without_inventing_a_replacement(tmp_path):
    manifest = setup_review(tmp_path)
    path = tmp_path / 'unit.c'
    path.write_text(path.read_text().replace('helper(); return', 'helper(); helper(); return'))
    report = native_frame_reviews(tmp_path, manifest)
    assert not report['active'] and report['stale_callbacks'] == ['getter']


def test_argument_replacement_requires_exact_reviewed_arity_and_delta(tmp_path):
    manifest = setup_review(tmp_path)
    row = manifest['callbacks'][0]
    row.update(effect='replace_argument_with_scalar', arguments=1, stack_delta=0)
    assert set(native_frame_reviews(tmp_path, manifest)['active']) == {'getter'}
    for arguments, delta in ((0, 0), (1, 1), (2, -1), (True, 0), (1, False)):
        row.update(arguments=arguments, stack_delta=delta)
        assert not native_frame_reviews(tmp_path, manifest)['active']


def test_action_request_requires_explicit_phase_without_a_stack_result(tmp_path):
    manifest = setup_review(tmp_path)
    row = manifest['callbacks'][0]
    row.update(effect='select_action_handler', arguments=0, stack_delta=0, result_tag='none',
               scheduling='wait_for_action', coverage='immediate_request_only', action_handler=2)
    assert set(native_frame_reviews(tmp_path, manifest)['active']) == {'getter'}
    for field, invalid in (('coverage', 'complete'), ('result_tag', 'integer'), ('arguments', 1),
                           ('stack_delta', 1), ('action_handler', 15), ('action_handler', True)):
        previous = row[field]
        row[field] = invalid
        assert not native_frame_reviews(tmp_path, manifest)['active']
        row[field] = previous


def test_deferred_dependency_revokes_only_later_phase_and_selector_is_bound(tmp_path):
    manifest = setup_review(tmp_path)
    path = tmp_path / 'unit.c'
    body = 'void deferredHelper(void) { return; }'
    path.write_text(path.read_text() + '\n' + body)
    row = manifest['callbacks'][0]
    row.update(effect='select_action_handler', arguments=0, stack_delta=0, result_tag='none',
               scheduling='wait_for_action', coverage='immediate_request_only', action_handler=3)
    deferred = dict(action_handler=3, effect='loop_exit_then_conditional_zero_append', arguments=0,
                    waiting_stack_delta=0, completion_stack_delta=1,
                    waiting_scheduling='wait_for_action', completion_scheduling='yield',
                    dependencies=[dict(kind='function', file='unit.c', function='deferredHelper', sha256=fingerprint(body))])
    row['deferred_review'] = deferred
    assert set(native_frame_reviews(tmp_path, manifest)['active_deferred']) == {'getter'}
    deferred['action_handler'] = 4
    assert not native_frame_reviews(tmp_path, manifest)['active_deferred']
    deferred['action_handler'] = 3
    path.write_text(path.read_text().replace(body, 'void deferredHelper(void) { changed(); }'))
    report = native_frame_reviews(tmp_path, manifest)
    assert set(report['active']) == {'getter'}
    assert not report['active_deferred']
    assert report['stale_deferred_reviews']['getter']['changed_dependencies']


def test_deferred_start_consumes_one_argument_only_during_initialization(tmp_path):
    manifest = setup_review(tmp_path)
    row = manifest['callbacks'][0]
    row.update(effect='select_action_handler', arguments=0, stack_delta=0, result_tag='none',
               scheduling='wait_for_action', coverage='immediate_request_only', action_handler=2)
    deferred = dict(action_handler=2, effect='start_program_then_conditional_zero_append', arguments=1,
                    initialization_stack_delta=-1, initialization_scheduling='wait_for_action',
                    waiting_stack_delta=0, completion_stack_delta=1,
                    waiting_scheduling='wait_for_action', completion_scheduling='yield',
                    dependencies=manifest['dependencies'])
    row['deferred_review'] = deferred
    for selector, effect, arguments in ((2, 'start_program_then_conditional_zero_append', 1),
                                        (9, 'costed_program_then_conditional_result_append', 2)):
        row['action_handler'] = selector
        deferred.update(action_handler=selector, effect=effect, arguments=arguments,
                        initialization_stack_delta=-arguments)
        assert set(native_frame_reviews(tmp_path, manifest)['active_deferred']) == {'getter'}
        for field, invalid in (('arguments', 0), ('initialization_stack_delta', 0),
                               ('initialization_scheduling', 'yield'), ('action_handler', 3)):
            previous = deferred[field]
            deferred[field] = invalid
            report = native_frame_reviews(tmp_path, manifest)
            assert not report['active_deferred'] and report['active']
            deferred[field] = previous


def test_changed_dependency_or_unsupported_contract_keeps_callback_unreviewed(tmp_path):
    manifest = setup_review(tmp_path)
    path = tmp_path / 'unit.c'
    path.write_text(path.read_text().replace('{ return; }', '{ if (1) return; }'))
    report = native_frame_reviews(tmp_path, manifest)
    assert not report['active'] and report['stale_dependencies']
    manifest = setup_review(tmp_path)
    manifest['callbacks'][0]['stack_delta'] = 2
    report = native_frame_reviews(tmp_path, manifest)
    assert not report['active'] and report['stale_callbacks'] == ['getter']


def test_callback_specific_helper_changes_revoke_only_dependent_reviews(tmp_path):
    code = ('void firstHelper(void) { writeFirst(); }\nvoid secondHelper(void) { writeSecond(); }\n'
            's32 first(void) { firstHelper(); return 0; }\ns32 second(void) { secondHelper(); return 0; }')
    path = tmp_path / 'unit.c'
    path.write_text(code)
    callbacks = []
    for name in ('first', 'second'):
        helper = name + 'Helper'
        callbacks.append(dict(function=name, file='unit.c', sha256=fingerprint(function_source(code, name)),
                              effect='append_scalar', arguments=0, stack_delta=1, result_tag='integer', scheduling='continue',
                              dependencies=[dict(kind='function', file='unit.c', function=helper,
                                                 sha256=fingerprint(function_source(code, helper)))]))
    manifest = dict(preconditions=['Invented ownership conditions.'], dependencies=[], callbacks=callbacks)
    path.write_text(code.replace('writeFirst()', 'writeChanged()'))
    report = native_frame_reviews(tmp_path, manifest)
    assert set(report['active']) == {'second'}
    assert report['stale_callbacks'] == ['first']
    assert set(report['stale_callback_dependencies']) == {'first'}
