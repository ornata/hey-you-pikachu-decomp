"""Complete source-bound sitting activity requests and their shared helper links.

Authored programs and sampled targets do not prove physical movement, emotion,
retail reachability, coherent message history or eventual completion.
"""

from .activity_workflows import activity_workflow_roles, _mask_helper
from .activity_helpers import activity_helper_roles
from .activity_policies import activity_policy_roles
from .callee_contexts import context_snapshot
from .companion_snapshots import companion_snapshots, _edges
from .context import ROOT
from .native_frames import fingerprint
from .request_arguments import _digest, validate_instructions
from .stack import incoming_edges
from .workflow_cleanup import workflow_cleanup_roles

NATIVE_BINDINGS = {'actorScriptAdjustPikachuSecondaryMeter': {'address': 2148659428,
                                            'review_sha256': 'ad707956ca02a34708e441f4916193e28b410875772d09c7a6455d3be5665f1c'},
 'actorScriptGetTargetCoordinate': {'address': 2148651920,
                                    'review_sha256': '54ae646b94cbc82d1f6a23642bf31c1cb718f212f569a092f1afa64bbcf174c1'},
 'actorScriptPushActionScriptStopped': {'address': 2148648748,
                                        'review_sha256': '70c8afa259805820e65a4d63d8bb695ed5805e4efec59642dd201cc8ddd60e04'},
 'actorScriptRandomBelow': {'address': 2148647408,
                            'review_sha256': '45206f67a57091cbca5b17b2e640a6618c518b8c7461fc7899614bf7630e4a8f'},
 'actorScriptSetDefaultActionMode': {'address': 2148654648,
                                     'review_sha256': 'd2a281d4d491c14208ce217537e4d13c10e9d0cbb610630bbefeab68cedc0fdf'},
 'actorScriptSetHudGuideStyle': {'address': 2148656300,
                                 'review_sha256': 'c2550c6be82042cc8721a3d50aa8de50fea13bdf0118ed1880eac3be43c471e7'},
 'actorScriptSetTargetPosition': {'address': 2148648436,
                                  'review_sha256': '6d6f43a7e2f7e53fb5dae46ea7c4eea4be3544fd3d5773eb48299391387d0e83'},
 'actorScriptSetTargetToFallbackObject': {'address': 2148648664,
                                          'review_sha256': '13bec748fa5590f295a1482f9d7e3bfdb92464c8920a038b4832d630281efbd5'},
 'actorScriptStartActionAndWait': {'address': 2148646620,
                                   'deferred_review_sha256': 'cea86bf5d32c6fe64854ab4573e18a7c24ce64cfa891e3e7e3d7a9c7a6c70e19',
                                   'review_sha256': 'd8496a261d8f65ffae6cc6e846f48a32d8651af47e1e89eda34b4cfbf43f6b50'},
 'actorScriptStartTimedAction': {'address': 2148654872,
                                 'review_sha256': '94e5dd7abcf693f34a346d3efcba474903c9989379e9e08faf1b8ee1a0f316d8'},
 'actorScriptYield': {'address': 2148646576,
                      'review_sha256': 'a32e41ad6f38946c8b93e4a70a50e6600499bea62a2a82c22900dc39e298d3dc'}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptAdjustPikachuSecondaryMeter', 'actorScriptGetTargetCoordinate', 'actorScriptPushActionScriptStopped', 'actorScriptRandomBelow', 'actorScriptSetDefaultActionMode', 'actorScriptSetHudGuideStyle', 'actorScriptSetTargetPosition', 'actorScriptSetTargetToFallbackObject', 'actorScriptStartActionAndWait', 'actorScriptStartTimedAction', 'actorScriptYield'))
VM_UNIT_SHA256 = 'cded04462b1f2fbece0ee8d937f9d4fcfc8c659719094b92768715d9669fe8fb'
ACTION_HEADER_SHA256 = '78ef64c36262712a99f20c4921306573630afdacd7f459fc7b3d2266d89f702b'
def _family(instructions, entry, bindings):
    """Normalize three complete local functions and seven explicit external cuts."""
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
            entries.extend((calls[6],calls[4]))
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
    if len(external)!=7:
        raise ValueError('Unsupported external-cut alias pattern.')
    return dict(sha256=_digest(encoded),functions=functions,roots=roots,external=external)

FAMILY_SHA256 = 'e50c198cb015957ed53915395b22ba3aa5777822a98157b245a8c843b2611a7b'
SCOPE = ('Complete conditional selector120 routines only. Valid operand floors, initialized coherent '
         'root/local cells, nonaliasing storage and every native/deferred/resource/handle contract '
         'remain prerequisites. Authored requests do not prove retail reachability, probability, '
         'physical motion, emotion, delivered messages or eventual return.')


def _shared_binding(graph,dispatch,snapshots,helpers,policies,cleanups,instructions):
    """Require one exact snapshot and per-caller helper/cleanup association."""
    cuts={role:target for target,role in graph['external'].items()}
    producers=[p for p in snapshots if p['entry_offset']==cuts[1]]
    if len(producers)!=1:raise ValueError('Ambiguous sitting snapshot.')
    producer=producers[0]
    roots={role:int(slot) for slot,role in producer['root_role_mapping'].items()}
    own={role:slot for slot,role in graph['roots'].items()}
    if own!={0:roots.get(1)}:raise ValueError('Sitting category coordinate differs from snapshot.')
    mask=_mask_helper(instructions,cuts[2],producer['status_variable'])
    matched=[c for c in cleanups if c['entry']==cuts[5] and c['workflow_variable']==dispatch['variable']]
    if len(matched)!=1:raise ValueError('Missing exact sitting cleanup.')
    cleanup=matched[0]
    eligible=[h for h in helpers if h['cut_identity']==0 and h['entry']==cuts[0] and
        h['workflow_variable']==dispatch['variable'] and h['cleanup_binding']['workflow_variable']==dispatch['variable'] and
        h['cleanup_binding']['entry']==cleanup['entry'] and
        h['cleanup_binding']['normalized_sha256']==cleanup['normalized_sha256'] and
        h['snapshot_binding']['entry']==producer['entry_offset'] and
        h['snapshot_binding']['canonical_sha256']==producer['canonical_sha256']]
    complete=[]
    for init in eligible:
        linked=[]
        for role in (3,4,5):
            rows=[p for p in policies if p['cut_identity']==role and p['caller_entry']==init['caller_entry'] and
                p['workflow_variable']==dispatch['variable'] and p['cleanup_binding']['entry']==cleanup['entry'] and
                p['cleanup_binding']['workflow_variable']==dispatch['variable'] and
                p['cleanup_binding']['normalized_sha256']==cleanup['normalized_sha256']]
            if len(rows)!=1:break
            linked.append(rows[0])
        if len(linked)!=3:continue
        if linked[0]['entry']!=cuts[3] or linked[1]['entry']!=cuts[4]:continue
        # Complete policy5 pins the exact second function as Secondary/Wait17/Primary.
        tracking=linked[2]['functions']
        if (len(tracking)!=2 or tracking[1]['entry']!=cuts[6] or
                tracking[1]['span_bytes']!=50 or tracking[1]['reachable_bytes']!=50 or tracking[1]['instruction_count']!=10):continue
        complete.append(dict(initializer=init,category_policy=linked[0],elapsed_policy=linked[1],
            tracking17_binding={'entry':cuts[6],'policy_entry':linked[2]['entry'],'policy_normalized_sha256':linked[2]['normalized_sha256']},
            cleanup=cleanup,snapshot=producer,mask=mask))
    if len(complete)!=1:raise ValueError('Shared sitting helper pairing is incomplete or ambiguous.')
    return complete[0]


def sitting_activity_roles(instructions, *, snapshot=None, root=ROOT):
    """Bind whole sitting routines to actual scoped selector120 dispatches."""
    unresolved=dict(status='unresolved',activities=[],source_bindings={},scope=SCOPE)
    try:
        if set(NATIVE_BINDINGS)!=REQUIRED_NATIVE_NAMES:raise ValueError('Incomplete sitting native bindings.')
        reviews,preservation=context_snapshot(root=root) if snapshot is None else snapshot
        if preservation.get('stale_vm_functions') or preservation.get('context_vm_contract',{}).get('current') is not True:
            raise ValueError('Stale VM context.')
        if fingerprint((root/'src/ovl/script_vm.c').read_text())!=VM_UNIT_SHA256:
            raise ValueError('Stale complete VM source.')
        header=root/'include/action_script_ids.h'
        if fingerprint(header.read_text())!=ACTION_HEADER_SHA256:raise ValueError('Stale timed action identity header.')
        for name,binding in NATIVE_BINDINGS.items():
            active=reviews.get('active',{}).get(name)
            if active is None or _digest(active)!=binding['review_sha256']:raise ValueError('Stale sitting native: '+name)
            if 'deferred_review_sha256' in binding:
                deferred=reviews.get('active_deferred',{}).get(name)
                if deferred is None or _digest(deferred)!=binding['deferred_review_sha256']:raise ValueError('Stale sitting deferred native: '+name)
        validate_instructions(instructions)
        activity=activity_workflow_roles(instructions,snapshot=(reviews,preservation),root=root)
        snapshots=companion_snapshots(instructions,snapshot=(reviews,preservation),root=root)
        helpers=activity_helper_roles(instructions,snapshot=(reviews,preservation),root=root)
        policies=activity_policy_roles(instructions,snapshot=(reviews,preservation),root=root)
        saved=workflow_cleanup_roles(instructions,snapshot=(reviews,preservation),root=root)
        if (activity['status']!='conditional_workflows' or snapshots['status']!='conditional_snapshots' or
                helpers['status']!='conditional_helpers' or policies['status']!='conditional_policies' or saved['status']!='conditional_saved_workflows'):
            raise ValueError('Unresolved sitting shared source contracts.')
        activities=[]
        for dispatch in activity['dispatches']:
            if dispatch['selector']!=120:continue
            try:
                graph=_family(instructions,dispatch['target_offset'],NATIVE_BINDINGS)
                if graph['sha256']!=FAMILY_SHA256:continue
                bound=_shared_binding(graph,dispatch,snapshots['snapshots'],helpers['helpers'],policies['policies'],saved['cleanups'],instructions)
                activities.append(_describe(graph,dispatch,bound))
            except (ValueError,KeyError,TypeError,IndexError):continue
        return dict(status='conditional_sitting_activities',activities=activities,scope=SCOPE,
            source_bindings=dict(native_reviews=NATIVE_BINDINGS,vm_unit_sha256=VM_UNIT_SHA256,action_header_sha256=ACTION_HEADER_SHA256,
                activity=activity['source_bindings'],snapshots=snapshots['source_bindings'],helpers=helpers['source_bindings'],policies=policies['source_bindings'],saved=saved['source_bindings']))
    except (ValueError,OSError,KeyError,TypeError,IndexError) as error:return dict(unresolved,reason=str(error))


def _describe(graph,dispatch,bound):
    return dict(entry=graph['functions'][0]['entry'],selector=120,dispatch_call_offset=dispatch['call_offset'],
        workflow_variable=dispatch['variable'],normalized_sha256=graph['sha256'],
        functions=[dict(entry=n['entry'],span_bytes=n['end']-n['entry'],reachable_bytes=sum(r['size'] for r in n['rows']),instruction_count=len(n['rows'])) for n in graph['functions']],
        local_roles={'counter':[1,0],'random_limit':[1,1],'initialized_but_not_read_cell':[1,2]},
        random_limit={'bound':5,'addend':5,'nominal_range':[5,9],'probability_proven':False},
        initialization={'action_program':274,'action_name':'ACTION_SCRIPT_SIT_DOWN_CUE_1','duration':2.0,'hud_guide_style':1,
            'helper_entry':bound['initializer']['entry'],'sequence':'Request timed program274 for2.0; its native return yields the VM before continuation requests HUD1 and invokes the bound initialization helper. This is not an action-completion wait.'},
        snapshot_binding={'entry':bound['snapshot']['entry_offset'],'canonical_sha256':bound['snapshot']['canonical_sha256']},
        sampled_message_type=[0,next(slot for slot,role in graph['roots'].items() if role==0)],status_mask_binding=bound['mask'],
        loop_default_action_mode=1,
        branch_order=[{'condition':'ordinary-message mask2 and sampled type4','target':bound['category_policy']['entry']},
            {'condition':'ordinary-message mask2 and sampled type not4','target':graph['functions'][2]['entry']},
            {'condition':'either enabled elapsed-limit mask32768/65536','target':bound['elapsed_policy']['entry']},
            {'condition':'counter strictly greater than random limit','target':graph['functions'][1]['entry']}],
        repeated_request={'condition':'sequencer command absent after earlier branches failed','action_program':276,'action_name':'ACTION_SCRIPT_SITTING','duration':2.0,
            'target':'Invoke fallback-target native, sample target axes0,1,2 separately, add100 to sampledY, then request SetTargetPosition with XYZ in order; FIXED tracking and zero allowance follow under its contract.',
            'counter_effect':'After the timed native yields and execution resumes, increment the counter; this counts requests, not completed actions. With stable integer counter and nominal limit, counter>limit permits limit+1 requests.'},
        completion_request={'action_program':277,'action_name':'ACTION_SCRIPT_SITTING_SCRATCH_EAR','wait':'Conditional StartActionAndWait, then invoke exact bound cleanup.'},
        other_message_response={'sequence':'Request program277 and wait; invoke bound Secondary/Wait17/Primary helper; set defaultmode3, request program29 and wait, set defaultmode1, adjust secondary meter+10, then invoke bound cleanup.',
            'meter_scope':'Applies to active Pikachu owning root and clamps resulting secondary meter0..100; net+10 is not guaranteed.'},
        tracking17_binding=bound['tracking17_binding'],cleanup_binding={'entry':bound['cleanup']['entry'],'workflow_variable':dispatch['variable'],'normalized_sha256':bound['cleanup']['normalized_sha256']},
        shared_policy_bindings=[{'entry':bound[k]['entry'],'normalized_sha256':bound[k]['normalized_sha256']} for k in ('category_policy','elapsed_policy')],
        external_cuts=[dict(identity=r,target_offset=t) for t,r in graph['external'].items()],
        gaps_interpreted=False,external_calls_preserve_context_proven=False,completion_proven=False)
