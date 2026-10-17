"""Complete caller-bound initialization, loop-exit and tracking helper roles.

These are conditional script requests and sampled VM-cell assignments, not a
proof of reachability, target availability, movement or eventual completion.
"""

from .activity_responses import activity_response_roles
from .callee_contexts import context_snapshot
from .companion_snapshots import companion_snapshots
from .context import ROOT
from .native_frames import fingerprint
from .request_arguments import _digest, validate_instructions
from .workflow_cleanup import _graph, _summary, workflow_cleanup_roles

NATIVE_BINDINGS = {'actorScriptPlaySoundEffect': {'address': 2148653104,
                                'review_sha256': 'c63cb7a7521377a5161362dae638a2972bddfb3d419b727fdd93799fe31e60c4'},
 'actorScriptPushActionScriptStopped': {'address': 2148648748,
                                        'review_sha256': '70c8afa259805820e65a4d63d8bb695ed5805e4efec59642dd201cc8ddd60e04'},
 'actorScriptPushOriginDistance': {'address': 2148651528,
                                   'review_sha256': '5de5aaff6eaf396d357e313d1bf9812cb37c067abf1d5e85365b03f1242a4547'},
 'actorScriptRandomBelow': {'address': 2148647408,
                            'review_sha256': '45206f67a57091cbca5b17b2e640a6618c518b8c7461fc7899614bf7630e4a8f'},
 'actorScriptResetTimer': {'address': 2148647648,
                           'review_sha256': '1d7d3bbc38b29f468f2275ac7285e24507f53a3a7311fef7bac53ccaf378b47b'},
 'actorScriptSetMessengerContactMode': {'address': 2148655520,
                                        'review_sha256': '6f9e91bbe06fc863e49c90120a55e5319b79158a65c8b511fc20161d6521055f'},
 'actorScriptSetSoundEffectPan': {'address': 2148653168,
                                  'review_sha256': 'b4618c0214eb57d5872e96d4ce7492fd4ca94db3c20e9ce0591c8a3ddc4cfb42'},
 'actorScriptSetSoundEffectVolume': {'address': 2148653248,
                                     'review_sha256': 'b575c5a5f70184b3aaf43b03fd3b53ef7c3d9789fd0a4b0f6ba3ed1908b5fc96'},
 'actorScriptStartActionAndWait': {'address': 2148646620,
                                   'deferred_review_sha256': 'cea86bf5d32c6fe64854ab4573e18a7c24ce64cfa891e3e7e3d7a9c7a6c70e19',
                                   'review_sha256': 'd8496a261d8f65ffae6cc6e846f48a32d8651af47e1e89eda34b4cfbf43f6b50'},
 'actorScriptStartLoopExitAndWait': {'address': 2148646656,
                                     'deferred_review_sha256': 'b457c4620a5722a45d22a9950ca12c606a3a122e0b4e70f22939f0f61d929940',
                                     'review_sha256': '62c52aa6f12793af692f956d81a883c66559296974cce72f9b9ce7a02d8de0ed'},
 'actorScriptStartTimedAction': {'address': 2148654872,
                                 'review_sha256': '94e5dd7abcf693f34a346d3efcba474903c9989379e9e08faf1b8ee1a0f316d8'},
 'actorScriptTrackPlayerPrimaryObject': {'address': 2148648960,
                                         'review_sha256': 'd00e68a3517f8f0631050f1d302ea605c983f4af330dcb7ad4ecdf73de6ea77d'},
 'actorScriptTrackPlayerSecondaryObject': {'address': 2148652360,
                                           'review_sha256': '6fd1e82b021dbf0d289abee78a4ac6599d7c22f9eeb2198df53636dcf95b4724'},
 'actorScriptYield': {'address': 2148646576,
                      'review_sha256': 'a32e41ad6f38946c8b93e4a70a50e6600499bea62a2a82c22900dc39e298d3dc'}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptPushOriginDistance', 'actorScriptResetTimer', 'actorScriptSetMessengerContactMode', 'actorScriptTrackPlayerSecondaryObject', 'actorScriptStartActionAndWait', 'actorScriptTrackPlayerPrimaryObject', 'actorScriptStartLoopExitAndWait', 'actorScriptStartTimedAction', 'actorScriptPushActionScriptStopped', 'actorScriptYield', 'actorScriptPlaySoundEffect', 'actorScriptRandomBelow', 'actorScriptSetSoundEffectPan', 'actorScriptSetSoundEffectVolume'))
VM_UNIT_SHA256 = 'cded04462b1f2fbece0ee8d937f9d4fcfc8c659719094b92768715d9669fe8fb'
HELPER_HASHES = {
    0: ('89254e1e817f2852ab043715c14d78ce2d08bf7b841109a15e4c78e7772fb62b',),
    6: ('e79771766305b21b5a6ec2b4b3906522cdb701929c348ad377974a166ee55267',
        '0850196e9be80b3b1236dffa2e75c37985526827e4f9ccc59fbadc5ad07ad5ec'),
    7: ('6c8a5beb97562e09a533a2db42b2428b6232143e3070ce05bce4a85c60e3661f',),
}
SCOPE = ('Complete caller-bound helpers only. Valid initialized VM cells, operand floors, '
         'disjoint storage, numeric conversions and all native/deferred/resource contracts remain '
         'required. No atomic sample, workflow clear, target liveness, physical movement, '
         'successful requests, reachability or eventual return is established.')


def _initializer_coordinates(graph, producer):
    """Couple the five known sampled fields; retain the sixth unresolved flag."""
    stores = [r for n in graph['nodes'] for r in n['rows'] if r['name']=='SCRIPT_OP_STORE_VARIABLE']
    coordinates = [[o['value'] for o in r['operands']] for r in stores]
    roles = {int(slot): role for slot, role in producer['root_role_mapping'].items()}
    if (len(coordinates)!=6 or any(c[0]!=0 for c in coordinates) or
            len({c[1] for c in coordinates})!=6 or
            [roles.get(c[1]) for c in coordinates[1:]] != [12,14,6,13,22] or
            coordinates[0][1] != coordinates[3][1]+3):
        raise ValueError('Initializer coordinates do not match this snapshot producer.')
    return coordinates


def activity_helper_roles(instructions, *, snapshot=None, root=ROOT):
    """Link complete helpers to one actual response, snapshot and cleanup closure."""
    unresolved=dict(status='unresolved',helpers=[],source_bindings={},scope=SCOPE)
    try:
        if set(NATIVE_BINDINGS)!=REQUIRED_NATIVE_NAMES:
            raise ValueError('Incomplete helper native binding set.')
        reviews,preservation=context_snapshot(root=root) if snapshot is None else snapshot
        if preservation.get('stale_vm_functions') or preservation.get('context_vm_contract',{}).get('current') is not True:
            raise ValueError('Stale VM context.')
        if fingerprint((root/'src/ovl/script_vm.c').read_text())!=VM_UNIT_SHA256:
            raise ValueError('Stale complete VM source.')
        for name,binding in NATIVE_BINDINGS.items():
            active=reviews.get('active',{}).get(name)
            if active is None or _digest(active)!=binding['review_sha256']:
                raise ValueError('Stale helper native: '+name)
            if 'deferred_review_sha256' in binding:
                deferred=reviews.get('active_deferred',{}).get(name)
                if deferred is None or _digest(deferred)!=binding['deferred_review_sha256']:
                    raise ValueError('Stale helper deferred native: '+name)
        validate_instructions(instructions)
        response=activity_response_roles(instructions,snapshot=(reviews,preservation),root=root)
        saved=workflow_cleanup_roles(instructions,snapshot=(reviews,preservation),root=root)
        snapshots=companion_snapshots(instructions,snapshot=(reviews,preservation),root=root)
        if (response['status']!='conditional_responses' or saved['status']!='conditional_saved_workflows' or
                snapshots['status']!='conditional_snapshots'):
            raise ValueError('Unresolved response, cleanup or snapshot source binding.')
        helpers=[]
        for caller in response['responses']:
            bound=caller['cleanup_binding']
            if bound['workflow_variable']!=caller['workflow_variable']:continue
            cleanups=[c for c in saved['cleanups'] if c['entry']==bound['entry'] and
                      c['workflow_variable']==caller['workflow_variable'] and c['normalized_sha256']==bound['normalized_sha256']]
            if len(cleanups)!=1:continue
            cleanup=cleanups[0]
            cuts={c['identity']:c['target_offset'] for c in caller['external_cuts']}
            if cuts.get(8)!=cleanup['entry']:continue
            producers=[p for p in snapshots['snapshots'] if p['entry_offset']==cuts.get(1) and
                       p['entry_offset']==caller['message_snapshot_binding']['entry'] and
                       p['canonical_sha256']==caller['message_snapshot_binding']['canonical_sha256']]
            if len(producers)!=1:continue
            producer=producers[0]
            # The actual complete cleanup already authenticates the loop-exit chain.
            if cuts.get(6) not in {f['entry'] for f in cleanup['functions']}:continue
            for role in (0,6,7):
                try:
                    graph=_graph(instructions,[cuts[role]],NATIVE_BINDINGS)
                    if graph['sha256'] not in HELPER_HASHES.get(role,()):continue
                    if role==6 and any(n['entry'] not in {f['entry'] for f in cleanup['functions']} for n in graph['nodes']):continue
                    coordinates=_initializer_coordinates(graph,producer) if role==0 else None
                    helpers.append(_describe(graph,role,caller,cleanup,producer,coordinates))
                except (ValueError,KeyError,TypeError,IndexError):continue
        return dict(status='conditional_helpers',helpers=helpers,scope=SCOPE,
            source_bindings=dict(native_reviews=NATIVE_BINDINGS,vm_unit_sha256=VM_UNIT_SHA256,
                response=response['source_bindings'],cleanup=saved['source_bindings'],snapshots=snapshots['source_bindings']))
    except (ValueError,OSError,KeyError,TypeError,IndexError) as error:
        return dict(unresolved,reason=str(error))


def _describe(graph,role,caller,cleanup,producer,coordinates):
    result=dict(entry=graph['nodes'][0]['entry'],cut_identity=role,caller_entry=caller['entry'],
        workflow_variable=caller['workflow_variable'],cleanup_binding=caller['cleanup_binding'],
        snapshot_binding=caller['message_snapshot_binding'],normalized_sha256=graph['sha256'],functions=_summary(graph),
        native_requests=[dict(instruction_offset=r['offset'],function=r['native']['functions'][0],address=r['native']['address'])
                         for n in graph['nodes'] for r in n['rows'] if r['name']=='SCRIPT_OP_CALL_NATIVE'],
        gaps_interpreted=False,completion_proven=False)
    if role==0:
        result.update(role='elapsed_and_sampled_cell_initialization',
            elapsed_timer_reset='Clears VM elapsed time only; total time and enabled limits are not reset.',
            root_writes=[dict(variable=c,value=v,snapshot_root_role=sr) for c,v,sr in zip(coordinates,
                [1,0,0,0,'sampled_approximate_origin_distance',100],[None,12,14,6,13,22])],
            unresolved_flag={'variable':coordinates[0],'value':1,'purpose_recovered':False},
            distance_scope='Approximate three-dimensional distance from the coordinate origin; not distance traveled along a path.',
            messenger_contact_mode_request=2,workflow_clear_proven=False)
    elif role==7:
        result.update(role='secondary_tracking_action_primary_tracking',action_program=18,
            requests='Track retained player secondary, request timed program18 and wait conditionally, then track retained player primary.',
            tracking_scope='Secondary available lookup writes FOLLOW, handle, XYZ and zero allowance; unavailable/toolbox writes FIXED only and retains stale handle, XYZ and allowance. Primary writes its retained handle, FOLLOW and zero allowance without refreshing XYZ or validating liveness.',
            distinction='The counter/distance policy requests program17 in its analogous helper; no opposite tracking direction is inferred.')
    else:
        result.update(role='bound_cleanup_loop_exit_chain',same_cleanup_node=True,
            requests='Request loop-exit and wait conditionally, request timed program87 for0.5, then poll sequencer command absence.',
            polling_scope='Command absence is not motion completion; waits may remain pending indefinitely.',
            variant='bedroom_shared_global_audio_hook' if len(graph['nodes'])==6 else 'ordinary_yield_poll',
            shared_global_scope='Bedroom additionally tests shared global[-1,0] through LOGICAL_NOT before integer branch conversion: original numeric zero invokes audio, nonzero including float0.5 skips. Float NOT remains float-tagged. Shared cell meaning is unresolved.' if len(graph['nodes'])==6 else None,
            audio_scope='Bedroom conditionally requests sound133, pan16+RandomBelow96 and volume64+RandomBelow64, and updates its scoped counter including180+RandomBelow120 or -(15 + RandomBelow(30)); no audible outcome or integer-domain invariant.' if len(graph['nodes'])==6 else None)
    return result
