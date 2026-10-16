"""Recognize two complete private script status producers, with no universal bit enum."""

import struct

from .callee_contexts import context_snapshot
from .companion_snapshots import _digest, _normalize_prefix
from .context import ROOT
from .native_frames import fingerprint, function_source
from .scene_region_snapshots import _dependency_fingerprint

NATIVE_BINDINGS = {'actorScriptFindItemInFacingCone': {'address': 2148653400,
                                     'arguments': 6,
                                     'file': 'src/ovl/actor_script_messages_items.c',
                                     'review_sha256': '4d72bd4ae15d2eeb5b33406a0df8f5afa8653f43d17df55589a55f145c520b2a',
                                     'sha256': 'e75f34264337661a0958a0e3828027ec53ed841b49ee52ceda710a0463ecb7eb'},
 'actorScriptGetObjectDistance': {'address': 2148651584,
                                  'arguments': 1,
                                  'file': 'src/ovl/actor_script_messages_items.c',
                                  'review_sha256': '16f67058c70921bd63c1950aa3415f2fdd74bda24cd4dec566cf3fd0ee55b161',
                                  'sha256': '1ac45e0e2a1291daf392bf08bc20a1a2dff0a4571a743a57c64a6c7fadedecbe'},
 'actorScriptPushTimer': {'address': 2148647700,
                          'arguments': 0,
                          'file': 'src/ovl/actor_script_action_commands.c',
                          'review_sha256': 'ffddfbb946413bd0f75eb279ec0636d33c05d24410a732d94ac39fb2bbcdae3c',
                          'sha256': '78efbe421c180f47c6d20ffcba3efc2e5eb62e5438832224001e233ec32135ec'}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptPushTimer', 'actorScriptFindItemInFacingCone', 'actorScriptGetObjectDistance'))
VM_UNIT_SHA256 = 'cded04462b1f2fbece0ee8d937f9d4fcfc8c659719094b92768715d9669fe8fb'
PATTERNS = {'coast_selected_items': {'bytes': 500,
                          'frame': [0, 1, 1],
                          'instructions': 75,
                          'root_roles': 2,
                          'sha256': '19dff7008871f9d50f51d2f397c76c05ee180183d28aeebc9db7550b1dbf663c'},
 'fishing_timer': {'bytes': 224,
                   'frame': [0, 0, 1],
                   'instructions': 39,
                   'root_roles': 2,
                   'sha256': 'bd63ff70f30b466d729b9b2ee35a986660e271c4eec2029199c7daeea82461ca'}}


def _source_bindings(root, snapshot):
    """Independently recheck every expanded native dependency and the whole VM."""
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
            errors.append('Stale VM contract.')
        if fingerprint(read('src/ovl/script_vm.c')) != VM_UNIT_SHA256:
            errors.append('Stale VM source unit.')
        for name, binding in NATIVE_BINDINGS.items():
            review = reviews.get('active', {}).get(name)
            if (review is None or _digest(review) != binding['review_sha256'] or
                    review.get('arguments') != binding['arguments']):
                errors.append('Stale expanded native review: ' + name)
                continue
            if fingerprint(function_source(read(binding['file']), name)) != binding['sha256']:
                errors.append('Stale native body: ' + name)
            for dependency in review.get('dependencies', []):
                if _dependency_fingerprint(dependency, read) != dependency['sha256']:
                    errors.append('Stale native dependency: ' + name)
    except (OSError, ValueError, KeyError, TypeError, AttributeError) as error:
        errors.append('Missing or invalid source binding: ' + str(error))
        vm = {}
    return dict(status='unresolved' if errors else 'current', reasons=errors,
                native_review_sha256={n:b['review_sha256'] for n,b in NATIVE_BINDINGS.items()},
                function_sha256={n:b['sha256'] for n,b in NATIVE_BINDINGS.items()},
                vm_unit_sha256=VM_UNIT_SHA256, vm_implementation_binding=vm)


def _predicates(kind, roles):
    if kind == 'fishing_timer':
        return dict(deadline_variable=[0, roles[0]], status_word_variable=[0, roles[1]],
            additions=[
                dict(value=0x01000000, predicate='first timer sample > stored deadline',
                     effects='A second timer sample plus integer 10 replaces the deadline before addition.'),
                dict(value=0x02000000, predicate='timer sample at the third static read site > (150 - 60)'),
                dict(value=0x04000000, predicate='timer sample at the fourth static read site > 150')],
            sampling='The second static read site executes only when the deadline comparison succeeds; three or four timer reads execute. Each reads retained VM elapsed seconds, normally unchanged during one uninterrupted invocation.')
    return dict(status_word_variable=[0, roles[0]], secondary_handle_variable=[0, roles[1]],
        first_search=dict(kind_bounds=[78,78], maximum_distance=100, distance_boundary="inclusive", cone=360,
                          state_filter=6, ownership_filter="ANY, including all ownership states", priority_mode=0, saved_global=33),
        list_scan=dict(count_global=27, base_global=28, local_counter=[1,0], result_index_global=34,
                       effects='Scan indices from zero while index < count; first equal handle stores index and exits. No match preserves the old result index. A zero selected handle can match a zero ledger entry.'),
        second_search=dict(kind_bounds=[77,77], maximum_distance=10, distance_boundary="inclusive", cone=360,
                           state_filter=6, ownership_filter="ANY, including all ownership states", priority_mode=0),
        additions=[dict(value=0x01000000,predicate='first selected handle object-distance sample <= 5'),
                   dict(value=0x02000000,predicate='a separate first selected handle object-distance sample <= 2'),
                   dict(value=0x04000000,predicate='second selected handle != 0 and its object-distance sample <= 2')],
        unavailable_distance='Object-distance returns -1 for unavailable objects, which satisfies both unguarded first-handle comparisons; no available-item proximity conclusion follows.',
        search_effects='Real item searches mutate shared query/tree/visitor state. State filter 6 is ANY ownership, not world-only or pickup eligibility. Priority mode zero still writes shared query state.')


def private_status_snapshots(instructions, *, snapshot=None, root=ROOT):
    """Recognize only complete canonical full functions under current source bindings."""
    bindings = _source_bindings(root, snapshot)
    scope = ('Private matched producer/root mapping only. Conditional signed additions are not OR, '
             'universal object flags, coherent atomic snapshots, retail reachability or later-consumer outcomes. '
             'Repeated invocations may carry into other bits; freshness and single invocation are not established.')
    result = dict(status='unresolved', snapshots=[], source_bindings=bindings, scope=scope)
    if bindings['status'] != 'current':
        return result
    result['status'] = 'conditional_private_status_snapshots'
    for index, row in enumerate(instructions):
        if row.get('name') != 'SCRIPT_OP_ENTER_FRAME':
            continue
        for kind, pattern in PATTERNS.items():
            rows = instructions[index:index + pattern['instructions']]
            try:
                if len(rows) != pattern['instructions'] or [o['value'] for o in rows[0]['operands']] != pattern['frame']:
                    continue
                if rows[-1]['name'] != 'SCRIPT_OP_RETURN' or rows[-1]['offset'] + rows[-1]['size'] - rows[0]['offset'] != pattern['bytes']:
                    continue
                normalized = _normalize_prefix(rows, instructions, native_bindings=NATIVE_BINDINGS)
                roles = {role:slot for slot,role in normalized['root_role_mapping'].items()}
                if (normalized['sha256'] != pattern['sha256'] or
                        set(roles) != set(range(pattern['root_roles'])) or normalized['absolute_helpers']):
                    continue
            except (ValueError, KeyError, TypeError, IndexError, OverflowError, struct.error):
                continue
            result['snapshots'].append(dict(kind=kind, entry_offset=rows[0]['offset'],
                end_exclusive=normalized['end_exclusive'], instruction_count=pattern['instructions'],
                bytes=pattern['bytes'], canonical_sha256=pattern['sha256'],
                frame_contract=dict(arguments=0,total_local_cells=pattern['frame'][1],frame_index=1),
                root_role_mapping=normalized['root_role_mapping'], return_value=dict(tag='integer',value=0),
                predicates=_predicates(kind,roles), instruction_offsets=[r['offset'] for r in rows],
                preconditions=[
                    'Normal entry with initialized numeric root/global/local storage, valid disjoint physical frame bindings, enough nonwrapping operand and saved metadata capacity, and a separately established caller floor.',
                    'Every expanded native resource, handle, layout and nonreentrant shared search prerequisite applies. No asynchronous mutation or rebinding between samples.',
                    'Coast list count is an initialized integer from zero through five, every indexed packed five-byte global cell is allocated and nonwrapping, list storage does not overlap selected handle/index globals, root status/handle cells, own counter, VM metadata or native workspaces.',
                    'All timer/deadline arithmetic, counters, shifts and status additions remain representable in their actual VM numeric domain. No fresh status word or initially clear bits are established.'],
                scope=scope))
    return result
