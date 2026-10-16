"""Recognize one complete source-bound selected-region snapshot extension.

The matched producer samples a selected kind-175 region's highlight state.
It conditionally adds a value to a script-owned word; it establishes neither
an object flag nor universal player occupancy or scene-independent bit meaning.
"""

import re
import struct

from .callee_contexts import context_snapshot
from .companion_snapshots import _digest, _normalize_prefix
from .context import ROOT
from .native_frames import fingerprint, function_source
from tools.source_quality import code_only

PREFIX_INSTRUCTIONS = 25
PREFIX_BYTES = 170
PREFIX_SHA256 = '8acac34ea2793de0a6a558383d2197787af9978e51baffc9200b4f6bd30fb862'
ADDED_VALUE = 0x01000000
RUNTIME_KIND = 175
NATIVE_BINDINGS = {'actorScriptFindObjectNearPosition': {'address': 2148657504,
                                       'arguments': 6,
                                       'file': 'src/ovl/actor_script_player_world.c',
                                       'review_sha256': '27681715dc5cd823b9db3e5a60595c53ea76a6820a10b9d67d4ecfd6c0ecd2bf',
                                       'sha256': 'a909a8c16d552cdeeb9533e166b79df1d83489d2e82b28d1a1dcd4c577d5269d'},
 'actorScriptGetPlayerCoordinate': {'address': 2148655112,
                                    'arguments': 1,
                                    'file': 'src/ovl/actor_script_messages_items.c',
                                    'review_sha256': 'e35e10efdcabe31324333f82f911f78df26e38aa4727d5444f8342c4344033bc',
                                    'sha256': '95595b9161697931d069a782c1101ab3244770e6f61a1c3d40a58644f40241e6'},
 'actorScriptIsSceneRegionHighlighted': {'address': 2148658028,
                                         'arguments': 1,
                                         'file': 'src/ovl/actor_script_player_world.c',
                                         'review_sha256': '7f5441fe258e2d96730263825156b17a5576f9b6d01219a82aec69d18e3e04b6',
                                         'sha256': '33ad826c146353894c97dd6ac2bad6e983dfeed1834068503b42c70690a7058a'}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptGetPlayerCoordinate', 'actorScriptFindObjectNearPosition', 'actorScriptIsSceneRegionHighlighted'))
SUPPORT_BINDING = {'file': 'src/game/scene_region.c', 'function': 'sceneRegionUpdateHighlight', 'sha256': '40e0905d2ce74a1a986d577a7530b0152d22b4cf64abe7728801f201459c26ec'}
CONSTRUCTOR_BINDING = {'file': 'src/game/scene_region.c', 'function': 'sceneRegionCreate', 'sha256': '858fa2b1521117a67fd9a46616837e6d8704418256559c7f5bc2b0dade40ca38'}
TEMPLATE_LAYOUT_FILE = 'include/object_template.h'
TEMPLATE_LAYOUT_SHA256 = '2408a28bd2a16db0e88ea3ceb8e61263506a616ad8867b80ed448cdb05953959'
RETAIL_RESOURCE_EVIDENCE = dict(template_address=0x80088E60, template_size=56,
    template_sha256='6efbde5018cf1810a392cb1a0deb200deb511690546dd7ee731102347ce20363',
    runtime_kind=175, callback_record_address=0x800791A8,
    callback_slots=[0, 0x800086E0, 0x8001F318, 0, 0, 0])
LAYOUT_FILE = 'include/scene_types.h'
LAYOUT_SHA256 = '6408dc8d013960f00a71e0ae46da25771edb8fc6f938a1932021f0fe4605e8df'
VM_UNIT_SHA256 = 'cded04462b1f2fbece0ee8d937f9d4fcfc8c659719094b92768715d9669fe8fb'


def _layout_fingerprint(source, name="SceneRegionState"):
    match = re.search(r'typedef\s+struct\s+' + re.escape(name) + r'\s*\{.*?\}\s*' + re.escape(name) + r'\s*;',
                      code_only(source), re.S)
    if match is None:
        raise ValueError('Missing ' + name + ' layout.')
    return fingerprint(match.group())


def _dependency_fingerprint(binding, read):
    source = read(binding['file'])
    if binding['kind'] == 'function':
        source = function_source(source, binding['function'])
    elif binding['kind'] == 'macro':
        match = re.search(r'^\s*#define\s+' + re.escape(binding['macro']) + r'\b[^\n]*',
                          code_only(source), re.M)
        if match is None:
            raise ValueError('Missing reviewed macro.')
        source = match.group()
    elif binding['kind'] != 'file':
        raise ValueError('Unknown reviewed dependency kind.')
    return fingerprint(source)


def _source_bindings(root, snapshot):
    """Recheck source bodies, dependencies and layout even with a supplied snapshot."""
    errors, files = [], {}
    def read(path):
        if path not in files:
            files[path] = (root / path).read_text()
        return files[path]
    if set(NATIVE_BINDINGS) != REQUIRED_NATIVE_NAMES:
        return dict(status='unresolved', reasons=['Incomplete native binding set.'])
    try:
        reviews, preservation = context_snapshot(root=root) if snapshot is None else snapshot
        vm = preservation.get('context_vm_contract', {})
        if preservation.get('stale_vm_functions') or not vm.get('current'):
            errors.append('Stale full VM contract.')
        if fingerprint(read('src/ovl/script_vm.c')) != VM_UNIT_SHA256:
            errors.append('Stale VM source unit.')
        for name, binding in NATIVE_BINDINGS.items():
            review = reviews.get('active', {}).get(name)
            if (review is None or _digest(review) != binding['review_sha256'] or
                    review.get('arguments') != binding['arguments']):
                errors.append('Stale expanded native review: ' + name)
                continue
            if fingerprint(function_source(read(binding['file']), name)) != binding['sha256']:
                errors.append('Stale native source: ' + name)
            for dependency in review.get('dependencies', []):
                if _dependency_fingerprint(dependency, read) != dependency['sha256']:
                    errors.append('Stale native dependency: ' + name)
        if fingerprint(function_source(read(SUPPORT_BINDING['file']), SUPPORT_BINDING['function'])) != SUPPORT_BINDING['sha256']:
            errors.append('Stale scene-region highlight producer.')
        if fingerprint(function_source(read(CONSTRUCTOR_BINDING['file']), CONSTRUCTOR_BINDING['function'])) != CONSTRUCTOR_BINDING['sha256']:
            errors.append('Stale scene-region constructor.')
        if _layout_fingerprint(read(TEMPLATE_LAYOUT_FILE), 'ObjectTemplate') != TEMPLATE_LAYOUT_SHA256:
            errors.append('Stale ObjectTemplate layout.')
        if _layout_fingerprint(read(LAYOUT_FILE)) != LAYOUT_SHA256:
            errors.append('Stale SceneRegionState layout.')
    except (OSError, ValueError, KeyError, TypeError, AttributeError) as error:
        errors.append('Missing or invalid source binding: ' + str(error))
        vm = {}
    return dict(status='unresolved' if errors else 'current', reasons=errors,
                native_review_sha256={n: b['review_sha256'] for n, b in NATIVE_BINDINGS.items()},
                function_sha256={n: b['sha256'] for n, b in NATIVE_BINDINGS.items()},
                highlight_producer_sha256=SUPPORT_BINDING['sha256'],
                constructor_sha256=CONSTRUCTOR_BINDING['sha256'],
                object_template_layout_sha256=TEMPLATE_LAYOUT_SHA256,
                authenticated_retail_resource_requirement=RETAIL_RESOURCE_EVIDENCE,
                scene_region_layout_sha256=LAYOUT_SHA256, vm_unit_sha256=VM_UNIT_SHA256,
                vm_implementation_binding=vm)


def scene_region_snapshots(instructions, *, snapshot=None, root=ROOT):
    """Report complete canonical producer prefixes, never a universal flag definition.

    A supplied review snapshot is ephemeral. Every invocation independently
    checks relevant source files; stale bodies, dependencies and layouts fail
    closed. Recognition applies only to the matched producer and root mapping.
    """
    scope = ('Selected scene-region highlight producer only. Player XYZ are separate samples; '
             'the nearest-other-kind-175 query changes shared search state and does not prove a live result. '
             'A nonzero handle must independently pass the live highlight query. The conditional addition '
             'is not bitwise OR, a global object flag, any-region occupancy, coherent containment, '
             'retail reachability or a guarantee about later snapshot consumers.')
    bindings = _source_bindings(root, snapshot)
    result = dict(status='unresolved', snapshots=[], source_bindings=bindings, scope=scope)
    if bindings['status'] != 'current':
        return result
    result['status'] = 'conditional_scene_region_snapshots'
    for index, row in enumerate(instructions):
        if row.get('name') != 'SCRIPT_OP_ENTER_FRAME':
            continue
        rows = instructions[index:index + PREFIX_INSTRUCTIONS]
        if len(rows) != PREFIX_INSTRUCTIONS:
            continue
        try:
            if [o.get('value') for o in rows[0].get('operands', [])] != [0, 0, 1]:
                continue
            if (rows[-1].get('name') != 'SCRIPT_OP_STORE_VARIABLE' or
                    rows[-1]['offset'] + rows[-1]['size'] - rows[0]['offset'] != PREFIX_BYTES):
                continue
            normalized = _normalize_prefix(rows, instructions, native_bindings=NATIVE_BINDINGS)
            if normalized['sha256'] != PREFIX_SHA256:
                continue
            roles = {role: slot for slot, role in normalized['root_role_mapping'].items()}
            if set(roles) != {0, 1} or normalized['absolute_helpers']:
                continue
        except (ValueError, KeyError, TypeError, IndexError, OverflowError, struct.error):
            continue
        result['snapshots'].append(dict(
            entry_offset=rows[0]['offset'], prefix_end_exclusive=normalized['end_exclusive'],
            instruction_count=PREFIX_INSTRUCTIONS, bytes=PREFIX_BYTES, canonical_sha256=PREFIX_SHA256,
            frame_contract=dict(arguments=0, total_local_cells=0, frame_index=1),
            root_role_mapping=normalized['root_role_mapping'], selected_handle_variable=[0, roles[0]],
            status_word_variable=[0, roles[1]], runtime_kind=RUNTIME_KIND,
            selection=dict(origin='separately sampled player X/Y/Z', minimum_kind=RUNTIME_KIND,
                           maximum_kind=RUNTIME_KIND, exclusive_maximum_distance=10000,
                           excludes_executing_actor=True, lifetime_not_established=True),
            conditional_addition=dict(value=ADDED_VALUE, expression='0x01000000 << 0',
                predicate='selected handle is nonzero and its independently queried live region highlight is nonzero',
                role='selected_scene_region_highlight', store_offset=rows[-1]['offset']),
            instruction_offsets=[r['offset'] for r in rows],
            preconditions=[
                'Normal entry; valid initialized root and own-frame bindings, complete allocated nonwrapping values and enough operand/saved-frame capacity with a separately established caller floor.',
                'Root handle/status storage, VM/values, native search workspaces, actor/player/region records and saved metadata are physically disjoint; every expanded native resource, handle and layout prerequisite applies.',
                'No asynchronous mutation or rebinding across samples. Kind-175 search candidates may become unavailable; the highlight getter checks liveness separately and assumes the correct scene-region state layout.',
                'Authenticated original ROM template 0x80088E60 (kind 175, 56 bytes, pinned SHA-256) and callback record 0x800791A8 must remain unchanged. The catalog original-ROM LocalImage authentication discharges this resource prerequisite; this recognizer does not rescan ROM.',
                'Signed integer addition must remain representable. This prefix does not establish a fresh status word, single invocation, an initially clear bit or permission to replace addition with OR.'],
            template_scope='Only this complete producer and its mapped script-owned roots; the same numerical value has other meanings in other templates.',
            scope=scope))
    return result
