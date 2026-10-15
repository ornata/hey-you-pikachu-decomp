"""Validate explicit source reviews of native VM-storage effects.

Fingerprints bind an existing human review; they never infer a contract or
refresh evidence. Scalar append reviews require free stack capacity, protected
locals below the active depth, valid borrowed state and an intact VM context.
They cover handler writes, not concurrent mutation or later lifecycle changes.
Argument replacement also requires a numeric operand above every protected
local and frame record; the static caller analysis does not prove that floor.
Immediate action requests retain a separate deferred-effect frontier; their
request footprint does not establish the later handler's writes or operands.
"""

import hashlib
import json
from pathlib import Path
import re

from .context import ROOT
from tools.source_quality import code_only


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


def native_frame_reviews(root=ROOT, manifest=None):
    """Activate only explicitly reviewed callbacks with unchanged dependencies."""
    if manifest is None:
        manifest = json.loads((root / 'tools/script_decoder/native_frame_reviews.json').read_text())
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
                            (('append_scalar', 0, 1), ('replace_argument_with_scalar', 1, 0)) and
                            type(review['arguments']) is int and type(review['stack_delta']) is int)
        action_request = (review['effect'] == 'select_action_handler' and
                          type(review['arguments']) is int and review['arguments'] == 0 and
                          type(review['stack_delta']) is int and review['stack_delta'] == 0 and
                          review['result_tag'] == 'none' and review['scheduling'] == 'wait_for_action' and
                          review.get('coverage') == 'immediate_request_only' and
                          type(review.get('action_handler')) is int and 0 <= review['action_handler'] <= 14)
        supported_effect = ((supported_effect and review['result_tag'] in ('integer', 'float') and
                             review['scheduling'] in ('continue', 'yield')) or action_request)
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
        reviewed_effect = (
            selector == 3 and deferred.get('effect') == 'loop_exit_then_conditional_zero_append' and deferred.get('arguments') == 0 or
            selector == 2 and deferred.get('effect') == 'start_program_then_conditional_zero_append' and deferred.get('arguments') == 1 and
            type(deferred.get('initialization_stack_delta')) is int and deferred['initialization_stack_delta'] == -1 and
            deferred.get('initialization_scheduling') == 'wait_for_action' or
            selector == 9 and deferred.get('effect') == 'costed_program_then_conditional_result_append' and deferred.get('arguments') == 2 and
            type(deferred.get('initialization_stack_delta')) is int and deferred['initialization_stack_delta'] == -2 and
            deferred.get('initialization_scheduling') == 'wait_for_action')
        valid = (review.get('coverage') == 'immediate_request_only' and type(selector) is int and
                 selector == review.get('action_handler') and reviewed_effect and
                 type(deferred.get('arguments')) is int and
                 deferred.get('waiting_stack_delta') == 0 and deferred.get('completion_stack_delta') == 1 and
                 type(deferred.get('waiting_stack_delta')) is int and type(deferred.get('completion_stack_delta')) is int and
                 deferred.get('waiting_scheduling') == 'wait_for_action' and deferred.get('completion_scheduling') == 'yield' and
                 bool(deferred.get('dependencies')))
        if valid and not changed:
            active_deferred[name] = deferred
        else:
            stale_deferred[name] = dict(changed_dependencies=changed, supported_contract=valid)
    return dict(active=active, active_deferred=active_deferred, stale_deferred_reviews=stale_deferred,
                stale_dependencies=stale_dependencies, stale_callbacks=stale_callbacks,
                stale_callback_dependencies=stale_callback_dependencies,
                preconditions=manifest['preconditions'],
                scope='Reviewed immediate handler write footprints under stated stack/ownership preconditions; no scheduler, context-lifetime or concurrent-mutation proof.')
