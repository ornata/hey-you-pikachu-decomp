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


def test_checked_in_reviews_have_current_source_and_dependency_bindings():
    report = native_frame_reviews()
    assert not report['stale_dependencies'] and not report['stale_callbacks']
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
