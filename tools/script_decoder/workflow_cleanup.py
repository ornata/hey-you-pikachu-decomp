"""Source-bound saved-workflow producers, restoration and complete cleanup calls.

Saved continuations are bounded packed VM cells under explicit index/storage
conditions, not an ordinary emptyable stack or a guaranteed zero/idle state.
"""

from .activity_workflows import activity_workflow_roles
from .callee_contexts import context_snapshot
from .companion_snapshots import _edges
from .context import ROOT
from .native_frames import fingerprint
from .request_arguments import _digest, validate_instructions
from .stack import incoming_edges


NATIVE_BINDINGS = {'actorScriptAcknowledgeMessage': {'address': 2148649036,
                                   'review_sha256': 'f1e0a7a9aa429c606db2b16cbed462886de689626eaa1d22937811c6ab194327'},
 'actorScriptFindItemInFacingCone': {'address': 2148653400,
                                     'review_sha256': '2e4c77f06f0afdd75c88fd3aeab4df01a9c27b4116060fd36bd7fcb94d221fcf'},
 'actorScriptFindNearestInFacingCone': {'address': 2148649600,
                                        'review_sha256': 'a47c97dcaa3c97ed9f7dd4437eb9a3ed7e10294ed88cde45fdaaf5822639fa27'},
 'actorScriptFindObjectInFacingCone': {'address': 2148657376,
                                       'review_sha256': '269036d997d9dec712b330abd9dabd1e519c6a683b230420fd8c546216b652e4'},
 'actorScriptGetTargetCoordinate': {'address': 2148651920,
                                    'review_sha256': '54ae646b94cbc82d1f6a23642bf31c1cb718f212f569a092f1afa64bbcf174c1'},
 'actorScriptPlaySoundEffect': {'address': 2148653104,
                                'review_sha256': 'c63cb7a7521377a5161362dae638a2972bddfb3d419b727fdd93799fe31e60c4'},
 'actorScriptPushActionScriptStopped': {'address': 2148648748,
                                        'review_sha256': '70c8afa259805820e65a4d63d8bb695ed5805e4efec59642dd201cc8ddd60e04'},
 'actorScriptPushHeldItemHandle': {'address': 2148660596,
                                   'review_sha256': '8597a859e3feb8f70f77b6a5304a86a849ded5af866731b7cfb892a74425bc4a'},
 'actorScriptPushTargetDistanceAllowance': {'address': 2148654028,
                                            'review_sha256': '44b1b5ffd9da55399f98ab1a0dfb68b72e7eec0754913ab93c0c75da000f625e'},
 'actorScriptPushTargetHandle': {'address': 2148654072,
                                 'review_sha256': 'ba53ecbc2dfd1f51620895335c1ba1c5b9ad6a8094820d721c0df2fd6223691e'},
 'actorScriptPushTrackingMode': {'address': 2148654128,
                                 'review_sha256': '0ad1b4b9fe05cfb50e2931e7301900cf03b1845696a24292b2d07b218587d902'},
 'actorScriptRandomBelow': {'address': 2148647408,
                            'review_sha256': '45206f67a57091cbca5b17b2e640a6618c518b8c7461fc7899614bf7630e4a8f'},
 'actorScriptSetDestination': {'address': 2148653568,
                               'review_sha256': '2eb140a4be88760bb2660b82ea3265a66b9abb9a003402ce4147cffa1f5e71cd'},
 'actorScriptSetHudGuideStyle': {'address': 2148656300,
                                 'review_sha256': 'c2550c6be82042cc8721a3d50aa8de50fea13bdf0118ed1880eac3be43c471e7'},
 'actorScriptSetSoundEffectPan': {'address': 2148653168,
                                  'review_sha256': 'b4618c0214eb57d5872e96d4ce7492fd4ca94db3c20e9ce0591c8a3ddc4cfb42'},
 'actorScriptSetSoundEffectVolume': {'address': 2148653248,
                                     'review_sha256': 'b575c5a5f70184b3aaf43b03fd3b53ef7c3d9789fd0a4b0f6ba3ed1908b5fc96'},
 'actorScriptSetTrackingMode': {'address': 2148654184,
                                'review_sha256': '1510e24b5b0bb7bd57a1b41ee54f86e821f226a0b54c2dea5c405f3ca9fb42c6'},
 'actorScriptStartActionAndWait': {'address': 2148646620,
                                   'deferred_review_sha256': 'cea86bf5d32c6fe64854ab4573e18a7c24ce64cfa891e3e7e3d7a9c7a6c70e19',
                                   'review_sha256': 'd8496a261d8f65ffae6cc6e846f48a32d8651af47e1e89eda34b4cfbf43f6b50'},
 'actorScriptStartLoopExitAndWait': {'address': 2148646656,
                                     'deferred_review_sha256': 'b457c4620a5722a45d22a9950ca12c606a3a122e0b4e70f22939f0f61d929940',
                                     'review_sha256': '62c52aa6f12793af692f956d81a883c66559296974cce72f9b9ce7a02d8de0ed'},
 'actorScriptStartTimedAction': {'address': 2148654872,
                                 'review_sha256': '94e5dd7abcf693f34a346d3efcba474903c9989379e9e08faf1b8ee1a0f316d8'},
 'actorScriptStopActionScript': {'address': 2148648380,
                                 'review_sha256': 'bae0616fb6a6f5dd6a06898f11a77d96262283e91616dfccf508d74ff2cb87e5'},
 'actorScriptTrackObject': {'address': 2148653708,
                            'review_sha256': '1bfbbac0f79827263cb0844250e840a2e02613e530d00a8ff88a87aa926b62db'},
 'actorScriptYield': {'address': 2148646576,
                      'review_sha256': 'a32e41ad6f38946c8b93e4a70a50e6600499bea62a2a82c22900dc39e298d3dc'}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptAcknowledgeMessage', 'actorScriptFindItemInFacingCone', 'actorScriptFindNearestInFacingCone', 'actorScriptFindObjectInFacingCone', 'actorScriptGetTargetCoordinate', 'actorScriptPlaySoundEffect', 'actorScriptPushActionScriptStopped', 'actorScriptPushHeldItemHandle', 'actorScriptPushTargetDistanceAllowance', 'actorScriptPushTargetHandle', 'actorScriptPushTrackingMode', 'actorScriptRandomBelow', 'actorScriptSetDestination', 'actorScriptSetHudGuideStyle', 'actorScriptSetSoundEffectPan', 'actorScriptSetSoundEffectVolume', 'actorScriptSetTrackingMode', 'actorScriptStartActionAndWait', 'actorScriptStartLoopExitAndWait', 'actorScriptStartTimedAction', 'actorScriptStopActionScript', 'actorScriptTrackObject', 'actorScriptYield'))
VM_UNIT_SHA256 = 'cded04462b1f2fbece0ee8d937f9d4fcfc8c659719094b92768715d9669fe8fb'


def _graph(instructions, seeds, bindings):
    """Normalize complete reachable CALL closures and coordinate spacing."""
    by = {r['offset']: r for r in instructions}
    incoming = incoming_edges(instructions)
    entries = list(seeds)
    if len(set(entries)) != len(entries):
        raise ValueError('Duplicate closure seeds.')
    nodes = []
    for start in entries:
        if len(entries) > 12:
            raise ValueError('Function closure budget exceeded.')
        pending, reached = [start], set()
        while pending:
            offset = pending.pop()
            if offset in reached:
                continue
            if offset not in by or len(reached) >= 256:
                raise ValueError('Incomplete or excessive reachable function.')
            row = by[offset]
            reached.add(offset)
            if row['name'] == 'SCRIPT_OP_CALL':
                target = row['operands'][0]['value']
                if target not in entries:
                    entries.append(target)
            pending.extend(e['offset'] for e in row['successors'] if e['kind'] != 'call')
        rows = [by[o] for o in sorted(reached)]
        if rows[0]['name'] != 'SCRIPT_OP_ENTER_FRAME' or not any(r['name'] == 'SCRIPT_OP_RETURN' for r in rows):
            raise ValueError('Missing complete frame or return.')
        end = rows[-1]['offset'] + rows[-1]['size']
        if end - start > 4096 or any(start < o < end and o not in reached for o in by):
            raise ValueError('Unreviewed interior rows or excessive span.')
        expected = {}
        for row in rows:
            values = [o['value'] for o in row['operands']]
            if row['successors'] != _edges(row, values):
                raise ValueError('Incorrect closure control flow.')
            for edge in row['successors']:
                expected.setdefault(edge['offset'], set()).add(row['offset'])
        if any(incoming.get(r['offset'], set()) != expected.get(r['offset'], set()) for r in rows[1:]):
            raise ValueError('Competing function interior entry.')
        nodes.append(dict(entry=start, end=end, rows=rows))
    for i, node in enumerate(nodes):
        if any(node['entry'] < other['end'] and other['entry'] < node['end'] for other in nodes[i+1:]):
            raise ValueError('Overlapping closure functions.')
    calls = {n['entry']: {r['operands'][0]['value'] for r in n['rows'] if r['name'] == 'SCRIPT_OP_CALL'} for n in nodes}
    def visit(entry, path):
        if entry in path:
            raise ValueError('Recursive closure is unsupported.')
        for target in calls[entry]:
            visit(target, path | {entry})
    for entry in entries:
        visit(entry, set())
    roots, anchor, selectors, encoded = {}, None, {}, []
    for node in nodes:
        positions = {r['offset']: i for i, r in enumerate(node['rows'])}
        instructions_encoded = []
        for row in node['rows']:
            name = row['name']
            values = [o['value'] for o in row['operands']]
            tags = [o['tag'] for o in row['operands']]
            normalized = values[:]
            if name in ('SCRIPT_OP_LOAD_VARIABLE', 'SCRIPT_OP_STORE_VARIABLE', 'SCRIPT_OP_LOAD_ELEMENT', 'SCRIPT_OP_STORE_ELEMENT') and values[0] == 0:
                if anchor is None:
                    anchor = values[1]
                roots.setdefault(values[1], len(roots))
                normalized = [0, 'root-role-' + str(roots[values[1]]), values[1] - anchor]
            elif name in ('SCRIPT_OP_JUMP', 'SCRIPT_OP_JUMP_IF_ZERO'):
                if values[0] not in positions:
                    raise ValueError('Branch leaves complete function.')
                normalized = ['instruction', positions[values[0]]]
            elif name == 'SCRIPT_OP_CALL':
                normalized = ['function', entries.index(values[0])]
            elif name == 'SCRIPT_OP_CALL_NATIVE':
                native = row.get('native', {})
                names = native.get('functions', [])
                if len(names) != 1 or names[0] not in bindings:
                    raise ValueError('Unknown closure native.')
                name_native = names[0]
                if type(native.get('address')) is not int or native['address'] != bindings[name_native]['address'] or native.get('inline_operands', []) != []:
                    raise ValueError('Rebound native or unsupported inline operands.')
                identity = (name_native, native['address'])
                if values[0] in selectors and selectors[values[0]] != identity:
                    raise ValueError('Ambiguous native selector.')
                selectors[values[0]] = identity
                normalized = list(identity)
            instructions_encoded.append([row['offset'] - node['entry'], name, normalized, tags])
        encoded.append(instructions_encoded)
    return dict(sha256=_digest(encoded), nodes=nodes, roots=roots, anchor=anchor)


RESTORE_SHA256 = '6d29494fa248ce38de026e55c16407fbb5575a0483179b18fd8b3e444de77dcc'
STORE_SHA256 = 'ea743f3db1d91545ab7d30a0c50b7d7f777e12fa7f8948d1617ed728f9995010'
CLEANUP_SHA256 = '85eac062ef39779084dd983ec4fb6b53a5bd891573badf398e6bbc5f33bf7633'
BEDROOM_CLEANUP_SHA256 = 'ceea11f940f0fb4510f90b8252df088107c65e489460b579b0ef2ae9e08eab1a'
SCOPE = ('Conditional saved-workflow interfaces only. Valid packed root storage, initialized saved entries, '
         'integer indices, defined VM conversions, protected operands and all native/deferred lifetime and '
         'nonaliasing prerequisites remain required. Save equality-five is not an arbitrary overflow check. '
         'Restoration reads before testing index; it does not zero workflow state or prove return, movement or motion completion.')


def _summary(graph):
    return [dict(entry=n['entry'],span_bytes=n['end']-n['entry'],reachable_bytes=sum(r['size'] for r in n['rows']),
                 instruction_count=len(n['rows'])) for n in graph['nodes']]


def _restore_description(graph):
    roles={role:slot for slot,role in graph['roots'].items()}
    columns=[('ordering_attribute',1),('target_category_discriminator',3),('kind_filter',5),
             ('tracking_mode',7),('distance_allowance',9),('saved_target_handle',8),
             ('destination_x',10),('destination_y',11),('destination_z',12),('workflow_continuation',13)]
    return dict(entry=graph['nodes'][0]['entry'],normalized_sha256=graph['sha256'],functions=_summary(graph),
                index_variable=[0,roles[0]],workflow_variable=[0,roles[14]],
                restored_root_fields={'ordering_attribute':[0,roles[2]],'target_category_discriminator':[0,roles[4]],
                                      'kind_filter':[0,roles[6]],'distance_allowance':[0,roles[15]]},
                saved_columns=[dict(role=name,base_variable=[0,roles[role]],logical_cells=5) for name,role in columns],
                required_integer_index_range=[0,4],physical_allocation_proven=False,
                tracking_paths={'initial':'Request integer-converted saved tracking mode; boxed numeric comparison against zero separately selects the branch.',
                    'nonzero_boxed_mode':'Conditional searches update local candidates, but TrackObject receives the original saved-handle column and saved allowance, not a search result. A valid available target forces FOLLOW mode1; unavailable/toolbox forces FIXED mode0 while retaining stale handle, position and allowance.',
                    'zero_boxed_mode':'Request SetDestination with saved XYZ and allowance; that native selects FIXED mode0.'},
                search_result_used_for_tracking=False,
                continuation='Copy saved workflow value into the workflow root; this is dynamic and is not a zero/idle postcondition.',
                index_effect='All indexed reads and restoration requests occur first. Integer index0 returns -1 and remains0; index1..4 decrements afterward and returns0. Initial index-1 is not a supported restore input.',
                preconditions=['Every saved column entry and root field is initialized, readable, live and disjoint from protected VM metadata; logical five-cell columns do not establish allocation.',
                    'Index is a coherent integer0..4, not merely a value that passes the final comparison. All native handle, numeric conversion, terrain, search and ownership prerequisites apply.',
                    'Searches can mutate query/tree state even when the candidate is not consumed by tracking. No asynchronous mutation invalidates saved entry or context.'],
                completion_proven=False)


def workflow_cleanup_roles(instructions, *, snapshot=None, root=ROOT):
    """Recover complete save/restore and linked cleanup closures under fresh source."""
    unresolved=dict(status='unresolved',stores=[],restores=[],cleanups=[],source_bindings={},scope=SCOPE)
    try:
        if set(NATIVE_BINDINGS)!=REQUIRED_NATIVE_NAMES:
            raise ValueError('Incomplete saved-workflow native binding set.')
        reviews,preservation=context_snapshot(root=root) if snapshot is None else snapshot
        if preservation.get('stale_vm_functions') or preservation.get('context_vm_contract',{}).get('current') is not True:
            raise ValueError('Stale VM context.')
        if fingerprint((root/'src/ovl/script_vm.c').read_text())!=VM_UNIT_SHA256:
            raise ValueError('Stale complete VM implementation.')
        for name,binding in NATIVE_BINDINGS.items():
            active=reviews.get('active',{}).get(name)
            if active is None or _digest(active)!=binding['review_sha256']:
                raise ValueError('Stale saved-workflow native: '+name)
            if 'deferred_review_sha256' in binding:
                deferred=reviews.get('active_deferred',{}).get(name)
                if deferred is None or _digest(deferred)!=binding['deferred_review_sha256']:
                    raise ValueError('Stale saved-workflow deferred native: '+name)
        validate_instructions(instructions)
        activity=activity_workflow_roles(instructions,snapshot=(reviews,preservation),root=root)
        if activity['status']!='conditional_workflows':
            raise ValueError('Unresolved activity-workflow source linkage.')
        workflow_roots={tuple(w['variable']) for w in activity['workflows']}
        target_roots={}
        for episode in activity['episodes']:
            episode_roots={tuple(d['variable']) for d in activity['dispatches']
                           if d['role']=='source_bound_player_relative_episode' and d['target_offset']==episode['entry']}
            episode_roots &= workflow_roots
            for call in episode['external_calls']:
                if call['opaque_identity']==5:
                    target_roots.setdefault(call['target_offset'],set()).update(episode_roots)
        stores,restores,cleanups=[],[],[]
        for target in sorted(target_roots):
            try:
                cleanup=_graph(instructions,[target],NATIVE_BINDINGS)
                if cleanup['sha256'] not in (CLEANUP_SHA256,BEDROOM_CLEANUP_SHA256):
                    continue
                restore_entry=cleanup['nodes'][2]['entry']
                restore=_graph(instructions,[restore_entry],NATIVE_BINDINGS)
                if restore['sha256']!=RESTORE_SHA256:
                    continue
                description=_restore_description(restore)
                if target_roots[target] != {tuple(description['workflow_variable'])}:
                    continue
            except (ValueError,KeyError,TypeError,IndexError):
                continue
            restores.append(description)
            cleanup_root=next(slot for slot,role in cleanup['roots'].items() if role==0)
            cleanups.append(dict(entry=target,restore_entry=restore_entry,workflow_variable=description['workflow_variable'],normalized_sha256=cleanup['sha256'],functions=_summary(cleanup),
                variant='bedroom_eight_function_closure' if len(cleanup['nodes'])==8 else 'five_function_closure',
                reset_variable=[0,cleanup_root],reset_value=0,
                requests='When the sampled held handle is nonzero, request action79 and wait conditionally. Reset the scoped value, request HUD guide0 and action stop, call the complete loop-exit/poll chain, restore the saved continuation, then acknowledge the mailbox.',
                waits='Loop-exit and stopped polling test sequencer command absence, not motion completion; no eventual return is established.',
                acknowledgement='Clears pending flag under its native contract; retained message fields are not cleared.',
                ignored_restore_return=True,zero_workflow_postcondition_proven=False,completion_proven=False,
                additional_bedroom_effects='Complete additional yield/audio hook closure includes RNG, sound playback, pan/volume and its scoped saved sound state.' if len(cleanup['nodes'])==8 else None))
            for row in instructions:
                if row['name']!='SCRIPT_OP_ENTER_FRAME' or [o['value'] for o in row['operands']]!=[0,4,1]:
                    continue
                try:
                    store=_graph(instructions,[row['offset'],restore_entry],NATIVE_BINDINGS)
                    if store['sha256']!=STORE_SHA256:
                        continue
                except (ValueError,KeyError,TypeError,IndexError):
                    continue
                stores.append(dict(entry=row['offset'],restore_entry=restore_entry,compaction_entry=store['nodes'][2]['entry'],
                    normalized_sha256=store['sha256'],functions=_summary(store),index_variable=description['index_variable'],
                    saved_columns=description['saved_columns'],logical_capacity=5,physical_allocation_proven=False,
                    required_integer_index_range=[-1,4],
                    ordinary_save='For initialized integer index-1..3, increment and write the new current entry; return0.',
                    full_save='Tests index+1==5, not a general bounds check. Under index4, an ordering-column scan chooses compaction start; shift all ten columns from j+1 to j while j<4, then overwrite current entry4 and return-1.',
                    full_save_compaction_rule={'required_current_index':4,'scan_indices':[1,4],
                        'initial_previous':'ordering column at index0','initial_remembered_candidate':4,
                        'break_rule':'Stop at first current ordering value <= previous; compact from scan_index-1. This comparison occurs before candidate update.',
                        'ascending_rule':'If all adjacent values strictly ascend, compact from the first ordering==2 candidate visited at indices1..4; use4 when none is remembered.'},
                    always_written_columns=['ordering_attribute','target_category_discriminator','kind_filter','tracking_mode','distance_allowance','workflow_continuation'],
                    conditional_written_columns={'nonzero_native_tracking_mode':['saved_target_handle'],'zero_native_tracking_mode':['destination_x','destination_y','destination_z']},
                    unselected_columns='Columns not selected by tracking mode retain previous contents; compaction does not clear the final row or decrement the index.',
                    initialization_proven=False,completion_proven=False))
        return dict(status='conditional_saved_workflows',stores=stores,restores=restores,cleanups=cleanups,scope=SCOPE,
                    source_bindings=dict(native_reviews=NATIVE_BINDINGS,vm_unit_sha256=VM_UNIT_SHA256,
                                         activity_source_bindings=activity['source_bindings']))
    except (ValueError,OSError,KeyError,TypeError,IndexError) as error:
        return dict(unresolved,reason=str(error))
