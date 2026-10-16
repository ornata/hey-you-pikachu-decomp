"""Validate explicit source reviews of native VM-storage effects.

Fingerprints bind an existing human review; they never infer a contract or
refresh evidence. Scalar append reviews require free stack capacity, protected
locals below the active depth, valid borrowed state and an intact VM context.
They cover handler writes, not concurrent mutation or later lifecycle changes.
Argument replacement also requires a numeric operand above every protected
local and frame record; the static caller analysis does not prove that floor.
Borrowed string-pointer results retain explicit nullability and lifetime contracts;
the five-byte value owns only its copied address payload, never the text.
Inline-count text reviews separately bind packed count consumption, instruction
advance and bounded string/formatter/sink storage. The caller analysis does not
prove their count, type, length or alias prerequisites.
Immediate action requests retain a separate deferred-effect frontier; their
request footprint does not establish the later handler's writes or operands.
"""

import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import tempfile

from .context import ROOT, LocalImage
from tools.source_quality import code_only


_LEGACY_INTEGER_CLOSURE = (
    ('actorScriptPushLegacyVoiceResult', 0x801211AC, 68),
    ('scriptVmPushInteger', 0x8011601C, 48),
    ('scriptValueSetInteger', 0x80115544, 52),
    ('scriptVmPushValue', 0x80115228, 180),
    ('bcopy', 0x80052610, 772),
)


def original_target_integer_supported(root, review):
    """Authenticate the complete reviewed legacy machine closure, never its C definedness.

    This is a deliberately specific exception. Matching arbitrary data spans or
    a different function does not establish the original handler's footprint.
    LocalImage checks the supplied ROM identity and corresponding linked bytes.
    """
    if (review.get('function') != _LEGACY_INTEGER_CLOSURE[0][0] or
            review.get('semantic_scope') != 'original_instructions_only' or
            review.get('c_result_definedness') != 'indeterminate_on_nonnegative_index' or
            not isinstance(review.get('caller_obligation'), str) or not review['caller_obligation'].strip()):
        return False
    evidence = review.get('target_evidence')
    if not isinstance(evidence, list) or len(evidence) != len(_LEGACY_INTEGER_CLOSURE):
        return False
    try:
        for row, (name, address, size) in zip(evidence, _LEGACY_INTEGER_CLOSURE):
            if (row['name'] != name or int(row['address'], 0) != address or
                    type(row['size']) is not int or row['size'] != size or
                    not re.fullmatch('[0-9a-f]{64}', row['sha256'])):
                return False
        image = LocalImage(root / 'rom/baserom.us.z64', root / 'build/pikachu.elf')
        for row, (name, address, size) in zip(evidence, _LEGACY_INTEGER_CLOSURE):
            if image.symbol(name)[1:3] != (address, size):
                return False
            data, _ = image.read_address(address, size)
            if hashlib.sha256(data).hexdigest() != row['sha256']:
                return False
    except (OSError, ValueError, KeyError, TypeError, IndexError):
        return False
    return True


def _review_directory(root):
    """Keep index reads and writes inside the selected repository."""
    directory = root / 'tools/script_decoder'
    if not directory.resolve().is_relative_to(root.resolve()):
        raise ValueError('native-review index escapes its repository')
    return directory


def _review_part_path(root, relative):
    """Restrict review parts to ordinary JSON files in the review directory."""
    if not isinstance(relative, str) or not re.fullmatch(r'native_frame_reviews/[A-Za-z0-9_-]+\.json', relative):
        raise ValueError('invalid native-review part path')
    directory = _review_directory(root)
    path = directory / relative
    parts_directory = (directory / 'native_frame_reviews').resolve()
    if parts_directory.parent != directory.resolve() or path.resolve().parent != parts_directory:
        raise ValueError('native-review part escapes its directory')
    return path


def _callbacks_by_name(callbacks):
    """Reject ambiguous identities before assembling or publishing reviews."""
    if not isinstance(callbacks, list):
        raise ValueError('native-review callbacks must be a list')
    result = {}
    for row in callbacks:
        if not isinstance(row, dict) or not isinstance(row.get('function'), str) or not row['function']:
            raise ValueError('native-review callback requires a function name')
        if row['function'] in result:
            raise ValueError('duplicate native-review callback: ' + row['function'])
        result[row['function']] = row
    return result


def load_review_manifest(root=ROOT, manifest=None):
    """Load inline or indexed reviews without changing any frozen source evidence.

    Schema three indexes checksummed callback parts by source unit. Their exact
    order is retained separately; missing, altered or ambiguous parts fail closed.
    The returned inline schema two is compatible with dependency expansion.
    """
    root = Path(root)
    directory = _review_directory(root)
    if manifest is None:
        manifest = json.loads((directory / 'native_frame_reviews.json').read_text())
    if not isinstance(manifest, dict):
        raise ValueError('native-review manifest must be an object')
    if manifest.get('schema_version', 1) != 3:
        _callbacks_by_name(manifest['callbacks'])
        return manifest
    if 'callbacks' in manifest:
        raise ValueError('indexed native reviews cannot also contain inline callbacks')
    order, parts = manifest.get('callback_order'), manifest.get('callback_files')
    if (not isinstance(order, list) or any(not isinstance(name, str) for name in order) or
            len(set(order)) != len(order) or not isinstance(parts, list)):
        raise ValueError('invalid native-review part index')
    callbacks, paths = [], set()
    for part in parts:
        if not isinstance(part, dict) or set(part) != {'file', 'sha256'}:
            raise ValueError('invalid native-review part record')
        path = _review_part_path(root, part['file'])
        if part['file'] in paths:
            raise ValueError('duplicate native-review part path')
        paths.add(part['file'])
        data = path.read_bytes()
        if hashlib.sha256(data).hexdigest() != part['sha256']:
            raise ValueError('native-review part checksum differs: ' + part['file'])
        content = json.loads(data)
        if not isinstance(content, dict) or set(content) != {'callbacks'}:
            raise ValueError('invalid native-review part contents')
        _callbacks_by_name(content['callbacks'])
        callbacks.extend(content['callbacks'])
    by_name = _callbacks_by_name(callbacks)
    if set(by_name) != set(order):
        raise ValueError('native-review callback order differs from indexed parts')
    result = {key: value for key, value in manifest.items() if key not in ('callback_order', 'callback_files')}
    result.update(schema_version=2, callbacks=[by_name[name] for name in order])
    return result


def review_manifest_fingerprint(root=ROOT):
    """Bind the complete logical review set, including every indexed callback."""
    manifest = expand_review_dependencies(load_review_manifest(root))
    data = json.dumps(manifest, sort_keys=True, separators=(',', ':')).encode()
    return hashlib.sha256(data).hexdigest()


def _replace_review_file(path, data):
    """Replace a file without following preexisting temporary-file symlinks."""
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix='.' + path.name + '.',
                                         suffix='.tmp', delete=False) as output:
            temporary = Path(output.name)
            output.write(data)
        temporary.replace(path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def write_review_manifest(manifest, root=ROOT):
    """Publish compact, indexed reviews while preserving all source fingerprints.

    Part checksums authenticate storage, not source semantics. Replacing the index
    last prevents partially written parts from silently becoming active reviews.
    Only previously indexed parts that are no longer referenced are removed.
    """
    root = Path(root)
    path = _review_directory(root) / 'native_frame_reviews.json'
    previous = json.loads(path.read_text()) if path.exists() else {}
    compacted = compact_review_dependencies(load_review_manifest(root, manifest))
    callbacks = compacted.pop('callbacks')
    _callbacks_by_name(callbacks)
    groups, sources = {}, {}
    for row in callbacks:
        source = PurePosixPath(row['file'])
        if source.is_absolute() or '..' in source.parts or source.suffix != '.c':
            raise ValueError('invalid callback source unit')
        relative = 'native_frame_reviews/' + '__'.join(source.with_suffix('').parts) + '.json'
        _review_part_path(root, relative)
        if relative in sources and sources[relative] != row['file']:
            raise ValueError('callback source units have conflicting part names')
        sources[relative] = row['file']
        groups.setdefault(relative, []).append(row)
    previous_paths = [_review_part_path(root, part['file'])
                      for part in previous.get('callback_files', [])] if previous.get('schema_version') == 3 else []
    parts = []
    for relative, rows in sorted(groups.items()):
        target = _review_part_path(root, relative)
        target.parent.mkdir(parents=True, exist_ok=True)
        data = (json.dumps(dict(callbacks=rows), indent=2) + '\n').encode()
        _replace_review_file(target, data)
        parts.append(dict(file=relative, sha256=hashlib.sha256(data).hexdigest()))
    compacted.update(schema_version=3, callback_order=[row['function'] for row in callbacks], callback_files=parts)
    path.parent.mkdir(parents=True, exist_ok=True)
    _replace_review_file(path, (json.dumps(compacted, indent=2) + '\n').encode())
    retained = {_review_part_path(root, part['file']) for part in parts}
    for obsolete in previous_paths:
        if obsolete not in retained:
            obsolete.unlink(missing_ok=True)


def fingerprint(code):
    """Bind C tokens and literal contents while ignoring formatting and comments."""
    tokens = re.findall(r'(?:u8|[LuU])?"(?:\\[\s\S]|[^"\\])*"|(?:[LuU])?\x27(?:\\[\s\S]|[^\x27\\])*\x27|'
                        r'//[^\n]*|/\*[\s\S]*?\*/|[A-Za-z_]\w*|0[xX][\da-fA-F]+[uUlL]*|(?:\d+\.\d*|\.\d+|\d+)(?:[eEpP][+-]?\d+)?[uUlLfF]*|'
                        r'>>=|<<=|\+\+|--|->|&&|\|\||<=|>=|==|!=|\+=|-=|\*=|/=|%=|&=|\|=|\^=|<<|>>|\.\.\.|##|\S',
                        code)
    tokens = [token for token in tokens if not token.startswith(('//', '/*'))]
    return hashlib.sha256('\0'.join(tokens).encode()).hexdigest()


def function_source(code, name):
    """Use masked braces to extract a complete definition with literals retained."""
    masked = code_only(code)
    match = re.search(r'^[ \t]*(?:(?:static|extern|inline)\s+)*(?:const\s+)?(?:struct\s+)?[A-Za-z_]\w*\s*\**\s*' +
                      re.escape(name) + r'\([^{};]*\)\s*(?:[A-Za-z_]\w*\s+\**\s*[A-Za-z_]\w*\s*;\s*)*\{', masked, re.M)
    if match is None:
        raise ValueError('missing reviewed function definition')
    depth = 1
    for end in range(match.end(), len(masked)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if not depth:
            return code[match.start():end + 1]
    raise ValueError('unclosed reviewed function definition')


_DEFERRED_PHASE_MODELS = {
    5: ('start_program_then_message_or_sequencer_result', 1,
        'action_initializing_then_message_or_sequence_stop', None, (
            ('initialize', 1, -1, 'wait_for_action', None, None),
            ('wait', 0, 0, 'wait_for_action', None, None),
            ('interrupt', 0, 1, 'yield', 'pending_message_word', None),
            ('complete', 0, 1, 'yield', 0, None))),
    6: ('held_handle_gate_start_program_then_sequencer_result', 1,
        'action_initializing_then_sequence_stop', None, (
            ('initialize_reject', 1, 0, 'yield', -1, None),
            ('initialize_accept', 1, -1, 'wait_for_action', None, None),
            ('wait', 0, 0, 'wait_for_action', None, None),
            ('complete', 0, 1, 'yield', 0, None))),
    7: ('acquisition_settings_then_handle_gate_or_full_action_poll', 3,
        'action_initializing_then_sequence_and_last_action_end', None, (
            ('initialize_existing_handle', 3, -2, 'yield', 'held_item_kind', None),
            ('initialize_start', 3, -3, 'wait_for_action', None, None),
            ('wait', 0, 0, 'wait_for_action', None, None),
            ('complete', 0, 1, 'yield', 'held_item_kind', None))),
    8: ('eat_with_early_rejection_or_result_before_followup', 1,
        'action_initializing_then_sequence_stop', 1, (
            ('initialize_reject', 1, 0, 'yield', -1, None),
            ('initialize_accept', 1, -1, 'wait_for_action', None, None),
            ('wait', 0, 0, 'wait_for_action', None, None),
            ('consume_and_finish', 0, 1, 'yield', 'pre_consumption_item_history', None),
            ('consume_and_followup', 0, 1, 'wait_for_action', 'pre_consumption_item_history', 1),
            ('followup_wait_or_finish', 0, 0, 'wait_for_action_or_yield', None, None))),
    11: ('show_area_label_then_handoff_and_wait_secondary_overlay', 0,
         'action_initializing_then_accumulated_time_then_secondary_overlay', 12, (
             ('initialize_show_label', 0, 0, 'wait_for_action', None, None),
             ('wait_accumulated_time', 0, 0, 'wait_for_action', None, None),
             ('fade_and_handoff', 0, 0, 'wait_for_action', None, 12),
             ('successor_initializing_wait', 0, 0, 'wait_for_action', None, None),
             ('wait_secondary_fade', 0, 0, 'wait_for_action', None, None),
             ('complete', 0, 1, 'yield', 0, None))),
    14: ('transform_kind_after_timed_effect', 1,
         'action_initializing_then_action_timer', None, (
             ('initialize', 0, 0, 'wait_for_action', None, None),
             ('wait', 0, 0, 'wait_for_action', None, None),
             ('complete', 1, 0, 'yield', 'replacement_actor_handle', None))),
}


def phase_contract_supported(deferred):
    """Recognize reviewed branch phases without flattening rejection or handoff effects."""
    selector = deferred.get('action_handler')
    if type(selector) is not int or selector not in _DEFERRED_PHASE_MODELS:
        return False
    effect, arguments, gate, followup, expected = _DEFERRED_PHASE_MODELS[selector]
    if selector == 14 and deferred.get('preserved_context') != 'same_retained_vm_transferred_to_replacement_actor':
        return False
    if (deferred.get('effect') != effect or type(deferred.get('arguments')) is not int or
            deferred['arguments'] != arguments or deferred.get('phase_gate') != gate or
            deferred.get('result_tag') != 'integer' or
            deferred.get('followup_action_handler') != followup or
            followup is not None and type(deferred.get('followup_action_handler')) is not int):
        return False
    phases = deferred.get('phases')
    if not isinstance(phases, list) or len(phases) != len(expected):
        return False
    if any(key in deferred for key in ('initialization_stack_delta', 'initialization_scheduling',
                                      'waiting_stack_delta', 'completion_stack_delta',
                                      'waiting_scheduling', 'completion_scheduling')):
        return False
    return all(
        isinstance(phase, dict) and
        type(phase.get('consumed_arguments')) is int and type(phase.get('stack_delta')) is int and
        (phase.get('phase'), phase['consumed_arguments'], phase['stack_delta'],
         phase.get('scheduling'), phase.get('result'), phase.get('next_action_handler')) == shape and
        ('result' not in phase or type(phase['result']) is type(shape[4])) and
        ('next_action_handler' not in phase or type(phase['next_action_handler']) is int)
        for phase, shape in zip(phases, expected))


def expand_review_dependencies(manifest):
    """Resolve shared frozen records without changing review or fingerprint data.

    Schema one remains readable. Schema two allows source-identity references or
    inline dependency records; missing references fail before any review activates.
    The input is never mutated.
    """
    version = manifest.get('schema_version', 1)
    if version == 1:
        return manifest
    if version != 2 or not isinstance(manifest.get('dependency_catalog'), dict):
        raise ValueError('unsupported native-review dependency schema')
    catalog = manifest['dependency_catalog']

    def dependency(value):
        if isinstance(value, str):
            if value not in catalog or not isinstance(catalog[value], dict):
                raise ValueError('missing native-review dependency: ' + value)
            return dict(catalog[value])
        if isinstance(value, dict):
            return dict(value)
        raise ValueError('invalid native-review dependency record')

    def walk(value):
        if isinstance(value, dict):
            result = {}
            for key, child in value.items():
                if key == 'dependency_catalog':
                    continue
                if key == 'dependencies':
                    if not isinstance(child, list):
                        raise ValueError('native-review dependencies must be a list')
                    result[key] = [dependency(item) for item in child]
                else:
                    result[key] = walk(child)
            return result
        if isinstance(value, list):
            return [walk(child) for child in value]
        return value
    result = walk(manifest)
    result['schema_version'] = 1
    return result


def compact_review_dependencies(manifest):
    """Intern identical frozen records; never refresh hashes or accept a review."""
    manifest = expand_review_dependencies(manifest)
    catalog = {}

    def reference(record):
        if not isinstance(record, dict) or record.get('kind') not in ('file', 'function', 'macro'):
            raise ValueError('invalid frozen dependency for compaction')
        key = ':'.join(str(record[k]) for k in ('kind', 'file', 'function', 'macro') if k in record)
        if key in catalog and catalog[key] != record:
            # Retain distinct historical fingerprints or metadata rather than
            # replacing an older review with a newer dependency definition.
            digest = hashlib.sha256(json.dumps(record, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
            key += '@' + digest
        if key in catalog and catalog[key] != record:
            raise ValueError('conflicting frozen dependency identity')
        catalog[key] = dict(record)
        return key

    def walk(value):
        if isinstance(value, dict):
            return {key: [reference(item) for item in child] if key == 'dependencies' else walk(child)
                    for key, child in value.items()}
        if isinstance(value, list):
            return [walk(child) for child in value]
        return value
    result = walk(manifest)
    result['schema_version'] = 2
    result['dependency_catalog'] = dict(sorted(catalog.items()))
    return result


def native_frame_reviews(root=ROOT, manifest=None):
    """Activate only explicitly reviewed callbacks with unchanged dependencies."""
    manifest = load_review_manifest(root, manifest)
    manifest = expand_review_dependencies(manifest)
    cache, stale_dependencies, stale_callbacks, stale_callback_dependencies = {}, [], [], {}

    def digest(row):
        path = row['file']
        if path not in cache:
            cache[path] = (root / path).read_text()
        code = cache[path]
        if row['kind'] == 'function':
            code = function_source(code, row['function'])
        elif row['kind'] == 'macro':
            match = re.search(r'^\s*#define\s+' + re.escape(row['macro']) + r'\b[^\n]*', code_only(code), re.M)
            if match is None:
                raise ValueError('missing reviewed VM stack macro')
            code = code[match.start():match.end()]
        elif row['kind'] != 'file':
            raise ValueError('unknown source-review dependency kind')
        return fingerprint(code)

    for row in manifest['dependencies']:
        try:
            agrees = digest(row) == row['sha256']
        except (OSError, ValueError):
            agrees = False
        if not agrees:
            stale_dependencies.append(row)
    active = {}
    for review in manifest['callbacks']:
        row = dict(kind='function', file=review['file'], function=review['function'])
        try:
            agrees = digest(row) == review['sha256']
        except (OSError, ValueError):
            agrees = False
        changed_helpers = []
        for dependency in review.get('dependencies', []):
            try:
                unchanged = digest(dependency) == dependency['sha256']
            except (OSError, ValueError):
                unchanged = False
            if not unchanged:
                changed_helpers.append(dependency)
        if changed_helpers:
            stale_callback_dependencies[review['function']] = changed_helpers
        supported_effect = ((review['effect'], review['arguments'], review['stack_delta']) in
                            (('append_scalar', 0, 1), ('replace_argument_with_scalar', 1, 0),
                             ('replace_arguments_with_scalar', 2, -1),
                             ('replace_arguments_with_scalar', 3, -2),
                             ('replace_arguments_with_scalar', 4, -3),
                             ('replace_arguments_with_scalar', 5, -4),
                             ('replace_arguments_with_scalar', 6, -5),
                             ('replace_arguments_with_scalar', 7, -6),
                             ('replace_arguments_with_scalar', 8, -7),
                             ('replace_arguments_with_scalar', 18, -17)) and
                            type(review['arguments']) is int and type(review['stack_delta']) is int)
        action_request = (review['effect'] == 'select_action_handler' and
                          type(review['arguments']) is int and review['arguments'] == 0 and
                          type(review['stack_delta']) is int and review['stack_delta'] == 0 and
                          review['result_tag'] == 'none' and review['scheduling'] == 'wait_for_action' and
                          review.get('coverage') == 'immediate_request_only' and
                          type(review.get('action_handler')) is int and 0 <= review['action_handler'] <= 14)
        borrowed_string_result = (
            (review['effect'], review['arguments'], review['stack_delta']) in
            (('append_borrowed_string_pointer', 0, 1),
             ('replace_argument_with_borrowed_string_pointer', 1, 0),
             ('replace_arguments_with_borrowed_string_pointer', 2, -1)) and
            type(review['arguments']) is int and type(review['stack_delta']) is int and
            review['result_tag'] == 'string_pointer' and review['scheduling'] == 'continue' and
            review.get('result_ownership') == 'borrowed' and
            type(review.get('result_nullable')) is bool and
            isinstance(review.get('result_lifetime'), str) and bool(review['result_lifetime'].strip()) and
            bool(review.get('dependencies')))
        original_target_integer = (
            review['effect'] == 'append_original_target_integer' and
            type(review['arguments']) is int and review['arguments'] == 0 and
            type(review['stack_delta']) is int and review['stack_delta'] == 1 and
            review['result_tag'] == 'integer' and review['scheduling'] == 'continue' and
            bool(review.get('dependencies')) and original_target_integer_supported(root, review))
        count = review.get('inline_count')
        inline_strings = (
            review['effect'] == 'inline_count_strings_then_append_scalar' and
            review['arguments'] == 'inline_count' and review['stack_delta'] == '1-count' and
            review['result_tag'] == 'integer' and review['scheduling'] == 'continue' and
            review.get('sink') in ('message_bar', 'pending_window', 'shared_text') and isinstance(count, dict) and
            count.get('encoding') == 'packed_script_value' and
            type(count.get('bytes')) is int and count['bytes'] == 5 and
            count.get('conversion') == 'scriptValueGetInteger' and count.get('domain') == 'nonnegative_s32' and
            type(count.get('instruction_offset_delta')) is int and count['instruction_offset_delta'] == 5 and
            bool(review.get('dependencies')))
        supported_effect = ((supported_effect and review['result_tag'] in ('integer', 'float') and
                             review['scheduling'] in ('continue', 'yield')) or action_request or
                            borrowed_string_result or inline_strings or original_target_integer)
        if review['function'] == _LEGACY_INTEGER_CLOSURE[0][0] and not original_target_integer:
            supported_effect = False
        if (not agrees or not supported_effect or
                changed_helpers):
            stale_callbacks.append(review['function'])
        elif not stale_dependencies:
            active[review['function']] = review
    active_deferred, stale_deferred = {}, {}
    for name, review in active.items():
        deferred = review.get('deferred_review')
        if deferred is None:
            continue
        changed = []
        for dependency in deferred.get('dependencies', []):
            try:
                unchanged = digest(dependency) == dependency['sha256']
            except (OSError, ValueError):
                unchanged = False
            if not unchanged:
                changed.append(dependency)
        selector = deferred.get('action_handler')
        mode_gate = (
            (selector, deferred.get('effect'), deferred.get('arguments')) in
            ((10, 'poll_game_mode_then_replace_scheme_argument', 1),
             (13, 'poll_game_mode_then_replace_text_window_arguments', 3)) and
            deferred.get('phase_gate') == 'runtime_game_mode_only' and
            deferred.get('uses_initialization_flag') is False and
            type(deferred.get('waiting_game_mode')) is int and deferred['waiting_game_mode'] == 3 and
            type(deferred.get('completion_result')) is int and deferred['completion_result'] == 0 and
            'initialization_stack_delta' not in deferred and 'initialization_scheduling' not in deferred)
        explicit_phases = phase_contract_supported(deferred)
        reviewed_effect = (
            selector == 3 and deferred.get('effect') == 'loop_exit_then_conditional_zero_append' and deferred.get('arguments') == 0 or
            selector == 2 and deferred.get('effect') == 'start_program_then_conditional_zero_append' and deferred.get('arguments') == 1 and
            type(deferred.get('initialization_stack_delta')) is int and deferred['initialization_stack_delta'] == -1 and
            deferred.get('initialization_scheduling') == 'wait_for_action' or
            selector == 9 and deferred.get('effect') == 'costed_program_then_conditional_result_append' and deferred.get('arguments') == 2 and
            type(deferred.get('initialization_stack_delta')) is int and deferred['initialization_stack_delta'] == -2 and
            deferred.get('initialization_scheduling') == 'wait_for_action' or mode_gate or explicit_phases)
        scalar_phases = (
            deferred.get('waiting_stack_delta') == 0 and
            deferred.get('completion_stack_delta') == (1 - deferred['arguments'] if mode_gate else 1) and
            type(deferred.get('waiting_stack_delta')) is int and type(deferred.get('completion_stack_delta')) is int and
            deferred.get('waiting_scheduling') == 'wait_for_action' and deferred.get('completion_scheduling') == 'yield')
        valid = (review.get('coverage') == 'immediate_request_only' and type(selector) is int and
                 selector == review.get('action_handler') and reviewed_effect and
                 type(deferred.get('arguments')) is int and
                 (explicit_phases or scalar_phases) and
                 bool(deferred.get('dependencies')))
        if valid and not changed:
            active_deferred[name] = deferred
        else:
            stale_deferred[name] = dict(changed_dependencies=changed, supported_contract=valid)
    return dict(active=active, active_deferred=active_deferred,
                original_target_only={name: row for name, row in active.items()
                                      if row.get('semantic_scope') == 'original_instructions_only'},
                stale_deferred_reviews=stale_deferred,
                stale_dependencies=stale_dependencies, stale_callbacks=stale_callbacks,
                stale_callback_dependencies=stale_callback_dependencies,
                preconditions=manifest['preconditions'],
                scope='Reviewed immediate handler write footprints under stated stack/ownership preconditions; no scheduler, context-lifetime or concurrent-mutation proof. Original-target-only entries do not establish defined C behavior or result meaning.')
