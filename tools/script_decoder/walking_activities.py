"""Complete source-bound walking requests and resettable elapsed-delay helpers.

These authored programs describe conditional requests, not actual movement,
completed actions, probability, emotional outcomes or guaranteed termination.
"""
from .activity_workflows import activity_workflow_roles
from .activity_helpers import activity_helper_roles
from .activity_policies import activity_policy_roles
from .callee_contexts import context_snapshot
from .companion_snapshots import companion_snapshots, _edges
from .context import ROOT
from .native_frames import fingerprint
from .request_arguments import _digest, validate_instructions
from .sitting_activities import _shared_binding as _shared_sitting_binding
from .stack import incoming_edges
from .workflow_cleanup import _graph, _summary, workflow_cleanup_roles

NATIVE_BINDINGS = {'actorScriptAcknowledgeMessage': {'address': 2148649036,
                                   'review_sha256': 'f1e0a7a9aa429c606db2b16cbed462886de689626eaa1d22937811c6ab194327'},
 'actorScriptAdjustPikachuSecondaryMeter': {'address': 2148659428,
                                            'review_sha256': 'ad707956ca02a34708e441f4916193e28b410875772d09c7a6455d3be5665f1c'},
 'actorScriptGetTargetCoordinate': {'address': 2148651920,
                                    'review_sha256': '54ae646b94cbc82d1f6a23642bf31c1cb718f212f569a092f1afa64bbcf174c1'},
 'actorScriptPlaySoundEffect': {'address': 2148653104,
                                'review_sha256': 'c63cb7a7521377a5161362dae638a2972bddfb3d419b727fdd93799fe31e60c4'},
 'actorScriptPushActionScriptStopped': {'address': 2148648748,
                                        'review_sha256': '70c8afa259805820e65a4d63d8bb695ed5805e4efec59642dd201cc8ddd60e04'},
 'actorScriptPushTimer': {'address': 2148647700,
                          'review_sha256': 'ffddfbb946413bd0f75eb279ec0636d33c05d24410a732d94ac39fb2bbcdae3c'},
 'actorScriptRandomBelow': {'address': 2148647408,
                            'review_sha256': '45206f67a57091cbca5b17b2e640a6618c518b8c7461fc7899614bf7630e4a8f'},
 'actorScriptSetDefaultActionMode': {'address': 2148654648,
                                     'review_sha256': 'd2a281d4d491c14208ce217537e4d13c10e9d0cbb610630bbefeab68cedc0fdf'},
 'actorScriptSetHudGuideStyle': {'address': 2148656300,
                                 'review_sha256': 'c2550c6be82042cc8721a3d50aa8de50fea13bdf0118ed1880eac3be43c471e7'},
 'actorScriptSetSoundEffectPan': {'address': 2148653168,
                                  'review_sha256': 'b4618c0214eb57d5872e96d4ce7492fd4ca94db3c20e9ce0591c8a3ddc4cfb42'},
 'actorScriptSetSoundEffectVolume': {'address': 2148653248,
                                     'review_sha256': 'b575c5a5f70184b3aaf43b03fd3b53ef7c3d9789fd0a4b0f6ba3ed1908b5fc96'},
 'actorScriptSetTargetPosition': {'address': 2148648436,
                                  'review_sha256': '6d6f43a7e2f7e53fb5dae46ea7c4eea4be3544fd3d5773eb48299391387d0e83'},
 'actorScriptSetTargetToFallbackObject': {'address': 2148648664,
                                          'review_sha256': '13bec748fa5590f295a1482f9d7e3bfdb92464c8920a038b4832d630281efbd5'},
 'actorScriptStartActionAndWait': {'address': 2148646620,
                                   'deferred_review_sha256': 'cea86bf5d32c6fe64854ab4573e18a7c24ce64cfa891e3e7e3d7a9c7a6c70e19',
                                   'review_sha256': 'd8496a261d8f65ffae6cc6e846f48a32d8651af47e1e89eda34b4cfbf43f6b50'},
 'actorScriptStartActionScript': {'address': 2148648308,
                                  'review_sha256': 'd50003a3331b39d98ba6bedb287ca32610703946124fec0e44072841954b83f8'},
 'actorScriptStartTimedAction': {'address': 2148654872,
                                 'review_sha256': '94e5dd7abcf693f34a346d3efcba474903c9989379e9e08faf1b8ee1a0f316d8'},
 'actorScriptYield': {'address': 2148646576,
                      'review_sha256': 'a32e41ad6f38946c8b93e4a70a50e6600499bea62a2a82c22900dc39e298d3dc'}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptAcknowledgeMessage', 'actorScriptAdjustPikachuSecondaryMeter', 'actorScriptGetTargetCoordinate', 'actorScriptPlaySoundEffect', 'actorScriptPushActionScriptStopped', 'actorScriptPushTimer', 'actorScriptRandomBelow', 'actorScriptSetDefaultActionMode', 'actorScriptSetHudGuideStyle', 'actorScriptSetSoundEffectPan', 'actorScriptSetSoundEffectVolume', 'actorScriptSetTargetPosition', 'actorScriptSetTargetToFallbackObject', 'actorScriptStartActionAndWait', 'actorScriptStartActionScript', 'actorScriptStartTimedAction', 'actorScriptYield'))
VM_UNIT_SHA256 = 'cded04462b1f2fbece0ee8d937f9d4fcfc8c659719094b92768715d9669fe8fb'
ACTION_HEADER_SHA256 = '78ef64c36262712a99f20c4921306573630afdacd7f459fc7b3d2266d89f702b'
def _family(instructions, entry, bindings):
    """Normalize three complete local functions and eight explicit external cuts."""
    by = {r['offset']:r for r in instructions}
    incoming = incoming_edges(instructions)
    entries, functions = [entry], []
    for start in entries:
        pending, reached = [start], set()
        while pending:
            offset = pending.pop()
            if offset in reached:
                continue
            if offset not in by or len(reached)>=256:
                raise ValueError('Incomplete or excessive local response function.')
            row = by[offset]; reached.add(offset)
            pending.extend(e['offset'] for e in row['successors'] if e['kind']!='call')
        rows = [by[o] for o in sorted(reached)]
        shape = [0,3,1] if not functions else [0,0,1]
        if rows[0]['name']!='SCRIPT_OP_ENTER_FRAME' or [o['value'] for o in rows[0]['operands']]!=shape:
            raise ValueError('Unsupported response frame.')
        if not any(r['name']=='SCRIPT_OP_RETURN' for r in rows):
            raise ValueError('No returning path.')
        end = rows[-1]['offset']+rows[-1]['size']
        if end-start>2048 or any(start<o<end and o not in reached for o in by):
            raise ValueError('Unreviewed local rows or excessive span.')
        expected = {}
        for row in rows:
            for edge in row['successors']:
                expected.setdefault(edge['offset'],set()).add(row['offset'])
        if any(incoming.get(r['offset'],set())!=expected.get(r['offset'],set()) for r in rows[1:]):
            raise ValueError('Competing local interior entry.')
        if not functions:
            calls = list(dict.fromkeys(r['operands'][0]['value'] for r in rows if r['name']=='SCRIPT_OP_CALL'))
            if len(calls)!=7 or entry in (calls[6],calls[4]):
                raise ValueError('Unsupported main call-role shape.')
            entries.extend((calls[4],calls[6]))
        functions.append(dict(entry=start,end=end,rows=rows))
    if len(functions)!=3 or any(a['entry']<b['end'] and b['entry']<a['end'] for i,a in enumerate(functions) for b in functions[i+1:]):
        raise ValueError('Overlapping or incomplete local functions.')
    roots, external, selectors, encoded = {}, {}, {}, []
    for function in functions:
        positions = {r['offset']:i for i,r in enumerate(function['rows'])}; node=[]
        for row in function['rows']:
            name = row['name']; values=[o['value'] for o in row['operands']]; tags=[o['tag'] for o in row['operands']]
            if row['successors']!=_edges(row,values):
                raise ValueError('Incorrect local control flow.')
            normalized = values[:]
            if name in ('SCRIPT_OP_LOAD_VARIABLE','SCRIPT_OP_STORE_VARIABLE','SCRIPT_OP_LOAD_ELEMENT','SCRIPT_OP_STORE_ELEMENT') and values[0]==0:
                roots.setdefault(values[1],len(roots)); normalized=[0,'root-role-'+str(roots[values[1]])]
            elif name in ('SCRIPT_OP_JUMP','SCRIPT_OP_JUMP_IF_ZERO'):
                if values[0] not in positions:raise ValueError('Branch leaves local function.')
                normalized=['instruction',positions[values[0]]]
            elif name=='SCRIPT_OP_CALL':
                if values[0] in entries:
                    normalized=['local-function',entries.index(values[0])]
                else:
                    if any(f['entry']<=values[0]<f['end'] for f in functions):
                        raise ValueError('External cut enters local interior.')
                    external.setdefault(values[0],len(external)); normalized=['opaque-callee',external[values[0]]]
            elif name=='SCRIPT_OP_CALL_NATIVE':
                native=row.get('native',{}); names=native.get('functions',[])
                if len(names)!=1 or names[0] not in bindings:
                    raise ValueError('Unknown local native.')
                function_native=names[0]
                if type(native.get('address')) is not int or native['address']!=bindings[function_native]['address'] or native.get('inline_operands',[])!=[]:
                    raise ValueError('Rebound local native.')
                identity=(function_native,native['address'])
                if values[0] in selectors and selectors[values[0]]!=identity:
                    raise ValueError('Ambiguous native selector.')
                selectors[values[0]]=identity; normalized=list(identity)
            node.append([row['offset']-function['entry'],name,normalized,tags])
        encoded.append(node)
    if len(external)!=8:
        raise ValueError('Unsupported external-cut alias pattern.')
    return dict(sha256=_digest(encoded),functions=functions,roots=roots,external=external)

FAMILY_SHA256 = '52ff764bd1d5e302d44cbc675f434d780b19b6f13e6654f52ac3b9217e1b0d01'
DELAY_HASHES = ('ebddc21592393ffa906e7a1bf631270ec12629302f3699a840eb922298a57ed4',
                '90b27f20569b01b4d22fdc7688c79a7804ff9f84f5ebcf044b9c7962d73bfe39')
SCOPE = ('Complete conditional selector119 routines and actual delay call closures only. '
         'Valid operand floors, disjoint storage, coherent saved/sample values and all '
         'native/deferred/SDK/resource/handle preconditions remain required. Resettable VM '
         'elapsed time is not wall time, and no movement, emotion, action completion, '
         'probability, retail reachability or eventual return is proved.')


def _shared_binding(graph,dispatch,snapshots,helpers,policies,cleanups,instructions):
    """Reuse the exact shared interfaces with this family's distinct cut ordering."""
    adapted=dict(graph,external={target:{5:6,6:5}.get(role,role) for target,role in graph['external'].items() if role!=7})
    bound=_shared_sitting_binding(adapted,dispatch,snapshots,helpers,policies,cleanups,instructions)
    cuts={role:target for target,role in graph['external'].items()}
    delay=_graph(instructions,[cuts[7]],NATIVE_BINDINGS)
    if delay['sha256'] not in DELAY_HASHES:raise ValueError('Unreviewed walking delay closure.')
    cleanup_nodes={f['entry'] for f in bound['cleanup']['functions']}
    if any(n['entry'] not in cleanup_nodes for n in delay['nodes'][1:]):
        raise ValueError('Delay audio/Yield descendants do not belong to this exact cleanup.')
    if any(a['entry']<b['end'] and b['entry']<a['end'] for a in delay['nodes'] for b in graph['functions']):
        raise ValueError('Delay closure overlaps walking local functions.')
    return dict(bound,delay=delay)


def walking_activity_roles(instructions, *, snapshot=None, root=ROOT):
    """Bind the whole walking and delay family to an actual selector119 route."""
    unresolved=dict(status='unresolved',activities=[],source_bindings={},scope=SCOPE)
    try:
        if set(NATIVE_BINDINGS)!=REQUIRED_NATIVE_NAMES:raise ValueError('Incomplete walking native bindings.')
        reviews,preservation=context_snapshot(root=root) if snapshot is None else snapshot
        if preservation.get('stale_vm_functions') or preservation.get('context_vm_contract',{}).get('current') is not True:
            raise ValueError('Stale VM context.')
        if fingerprint((root/'src/ovl/script_vm.c').read_text())!=VM_UNIT_SHA256:
            raise ValueError('Stale complete VM source.')
        if fingerprint((root/'include/action_script_ids.h').read_text())!=ACTION_HEADER_SHA256:
            raise ValueError('Stale timed action identity header.')
        for name,binding in NATIVE_BINDINGS.items():
            active=reviews.get('active',{}).get(name)
            if active is None or _digest(active)!=binding['review_sha256']:raise ValueError('Stale walking native: '+name)
            if 'deferred_review_sha256' in binding:
                deferred=reviews.get('active_deferred',{}).get(name)
                if deferred is None or _digest(deferred)!=binding['deferred_review_sha256']:raise ValueError('Stale walking deferred native: '+name)
        validate_instructions(instructions)
        activity=activity_workflow_roles(instructions,snapshot=(reviews,preservation),root=root)
        snapshots=companion_snapshots(instructions,snapshot=(reviews,preservation),root=root)
        helpers=activity_helper_roles(instructions,snapshot=(reviews,preservation),root=root)
        policies=activity_policy_roles(instructions,snapshot=(reviews,preservation),root=root)
        saved=workflow_cleanup_roles(instructions,snapshot=(reviews,preservation),root=root)
        if (activity['status']!='conditional_workflows' or snapshots['status']!='conditional_snapshots' or
                helpers['status']!='conditional_helpers' or policies['status']!='conditional_policies' or saved['status']!='conditional_saved_workflows'):
            raise ValueError('Unresolved walking shared source contracts.')
        activities=[]
        for dispatch in activity['dispatches']:
            if dispatch['selector']!=119:continue
            try:
                graph=_family(instructions,dispatch['target_offset'],NATIVE_BINDINGS)
                if graph['sha256']!=FAMILY_SHA256:continue
                bound=_shared_binding(graph,dispatch,snapshots['snapshots'],helpers['helpers'],policies['policies'],saved['cleanups'],instructions)
                activities.append(_describe(graph,dispatch,bound))
            except (ValueError,KeyError,TypeError,IndexError):continue
        return dict(status='conditional_walking_activities',activities=activities,scope=SCOPE,
            source_bindings=dict(native_reviews=NATIVE_BINDINGS,vm_unit_sha256=VM_UNIT_SHA256,action_header_sha256=ACTION_HEADER_SHA256,
                activity=activity['source_bindings'],snapshots=snapshots['source_bindings'],helpers=helpers['source_bindings'],policies=policies['source_bindings'],saved=saved['source_bindings']))
    except (ValueError,OSError,KeyError,TypeError,IndexError) as error:return dict(unresolved,reason=str(error))


def _describe(graph,dispatch,bound):
    delay=bound['delay'];bedroom=len(delay['nodes'])==4
    return dict(entry=graph['functions'][0]['entry'],selector=119,dispatch_call_offset=dispatch['call_offset'],
        workflow_variable=dispatch['variable'],normalized_sha256=graph['sha256'],
        functions=[dict(entry=n['entry'],span_bytes=n['end']-n['entry'],reachable_bytes=sum(r['size'] for r in n['rows']),instruction_count=len(n['rows'])) for n in graph['functions']],
        local_roles={'counter':[1,0],'random_limit':[1,1],'initialized_but_not_read_cell':[1,2]},
        random_limit={'bound':5,'addend':5,'nominal_range':[5,9],'probability_proven':False},
        initialization={'hud_guide_style':1,'helper_entry':bound['initializer']['entry']},loop_default_action_mode=1,
        snapshot_binding={'entry':bound['snapshot']['entry_offset'],'canonical_sha256':bound['snapshot']['canonical_sha256']},
        sampled_message_type=[0,next(slot for slot,role in graph['roots'].items() if role==0)],status_mask_binding=bound['mask'],
        branch_order=[{'condition':'ordinary-message mask2 and sampled type4','target':bound['category_policy']['entry']},
            {'condition':'ordinary-message mask2 and sampled type not4','target':graph['functions'][1]['entry']},
            {'condition':'either enabled elapsed-limit mask32768/65536','target':bound['elapsed_policy']['entry']},
            {'condition':'counter strictly greater than random limit','target':graph['functions'][2]['entry']}],
        repeated_request={'action_program':14,'action_name':'ACTION_SCRIPT_WALK_TO_TARGET_START','duration':2.0,
            'target':'After command-absence predicate and prior branches fail, invoke fallback-target native, sample axes0,1,2 separately, add100 to sampledY, request SetTargetPosition XYZ; its contract sets FIXED and zero allowance.',
            'counter_effect':'Timed native yields before continuation increments counter. These are requests, not completed actions; under stable integer counter/nominal limit the strict comparison permits limit+1 requests.'},
        other_message_response={'sequence':'Request numeric program219 and wait; invoke bound Secondary/Wait17/Primary helper; request program163 and wait; adjust secondary meter+10; invoke exact cleanup.',
            'action_programs':[219,163],'named_program163':'ACTION_SCRIPT_TILT_VARIANT_2_CUE_2',
            'meter_scope':'Active Pikachu owning root receives clamped resulting secondary meter0..100; net+10 not guaranteed.'},
        completion={'sequence':'Request program180 through StartActionScript, which immediately yields; invoke delay with duration1.5; acknowledge pending mailbox; invoke initializer (reset elapsed); repeatedly sample snapshot and test mask2 first, otherwise float elapsed>10, yielding while neither test exits. Then HUD0, tracking17, program141 and wait, cleanup.',
            'action_program180_name':'ACTION_SCRIPT_WALK_TO_TRIP_CUE_0','action_program141_name':'ACTION_SCRIPT_SCRATCH_HEAD_CUE_0',
            'elapsed_test':{'operator':'greater_than','threshold':10,'comparison':'FLOAT elapsed compared with integer literal converted to f32; no integer truncation, so10.0 continues and adjacent representable float above10 exits unless earlier mask branch exits.'},
            'acknowledgement':'Clears pending only; retained payload fields remain. Snapshot can subsequently acknowledge a later pending message; no recentness or voice-origin claim.'},
        delay_binding={'entry':delay['nodes'][0]['entry'],'normalized_sha256':delay['sha256'],'functions':_summary(delay),
            'frame_contract':{'arguments':1,'total_local_cells':2,'frame_index':1,'includes_arguments':True},
            'parameter0':'Float-converted requested duration; this caller supplies1.5.', 'sample_variable':[1,1],
            'condition':'Continue yielding while f32(current VM elapsed minus initial sample) is less than requested duration; otherwise return boxed integer0. Timer reset, mutation or lack of scheduling progress can prevent return.',
            'variant':'bedroom_shared_global_audio_hook' if bedroom else 'ordinary_native_yield',
            'shared_global_audio_scope':'Actual descendants are in this exact cleanup. Shared global[-1,0] LOGICAL_NOT tests original numeric truth before branch integer conversion; FLOAT result retains tag, .5 skips audio. Conditional sound/RNG/counter writes are not audible outcomes or a meaning for that shared global.' if bedroom else None},
        tracking17_binding=bound['tracking17_binding'],cleanup_binding={'entry':bound['cleanup']['entry'],'workflow_variable':dispatch['variable'],'normalized_sha256':bound['cleanup']['normalized_sha256']},
        shared_policy_bindings=[{'entry':bound[k]['entry'],'normalized_sha256':bound[k]['normalized_sha256']} for k in ('category_policy','elapsed_policy')],
        external_cuts=[dict(identity=r,target_offset=t) for t,r in graph['external'].items()],
        gaps_interpreted=False,external_calls_preserve_context_proven=False,completion_proven=False)
