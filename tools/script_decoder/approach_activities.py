"""Complete approach requests and their bounded completion-helper closures.

Shared targets, effect requests and retained message boxes imply neither physical
outcomes, emotional states, message delivery nor eventual completion.
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
from .workflow_cleanup import _summary, workflow_cleanup_roles

NATIVE_BINDINGS = {'actorScriptAcknowledgeMessage': {'address': 2148649036,
                                   'review_sha256': 'f1e0a7a9aa429c606db2b16cbed462886de689626eaa1d22937811c6ab194327'},
 'actorScriptAdjustPikachuSecondaryMeter': {'address': 2148659428,
                                            'review_sha256': 'ad707956ca02a34708e441f4916193e28b410875772d09c7a6455d3be5665f1c'},
 'actorScriptCreateAttachedEffect': {'address': 2148649356,
                                     'review_sha256': 'ef640c052269675ac2c4ebff5c8761b339dab25fe4202c6ca3e90f141dbaa1ee'},
 'actorScriptEnqueueMessage': {'address': 2148647744,
                               'review_sha256': '84ebda162953537988a95f51d173b98011ef9b0937a09e813eb16d9247defacc'},
 'actorScriptPlaySoundEffect': {'address': 2148653104,
                                'review_sha256': 'c63cb7a7521377a5161362dae638a2972bddfb3d419b727fdd93799fe31e60c4'},
 'actorScriptPushActionScriptStopped': {'address': 2148648748,
                                        'review_sha256': '70c8afa259805820e65a4d63d8bb695ed5805e4efec59642dd201cc8ddd60e04'},
 'actorScriptPushTargetApproachDistance': {'address': 2148648564,
                                           'review_sha256': '117f9156e3e996b2e05ca2b2957f4bb7fb9cefdbbb67d9b10c13c66c665245b1'},
 'actorScriptPushTimer': {'address': 2148647700,
                          'review_sha256': 'ffddfbb946413bd0f75eb279ec0636d33c05d24410a732d94ac39fb2bbcdae3c'},
 'actorScriptRandomBelow': {'address': 2148647408,
                            'review_sha256': '45206f67a57091cbca5b17b2e640a6618c518b8c7461fc7899614bf7630e4a8f'},
 'actorScriptSetDefaultActionMode': {'address': 2148654648,
                                     'review_sha256': 'd2a281d4d491c14208ce217537e4d13c10e9d0cbb610630bbefeab68cedc0fdf'},
 'actorScriptSetHudGuideStyle': {'address': 2148656300,
                                 'review_sha256': 'c2550c6be82042cc8721a3d50aa8de50fea13bdf0118ed1880eac3be43c471e7'},
 'actorScriptSetMessengerContactMode': {'address': 2148655520,
                                        'review_sha256': '6f9e91bbe06fc863e49c90120a55e5319b79158a65c8b511fc20161d6521055f'},
 'actorScriptSetSoundEffectPan': {'address': 2148653168,
                                  'review_sha256': 'b4618c0214eb57d5872e96d4ce7492fd4ca94db3c20e9ce0591c8a3ddc4cfb42'},
 'actorScriptSetSoundEffectVolume': {'address': 2148653248,
                                     'review_sha256': 'b575c5a5f70184b3aaf43b03fd3b53ef7c3d9789fd0a4b0f6ba3ed1908b5fc96'},
 'actorScriptSetTargetToFallbackObject': {'address': 2148648664,
                                          'review_sha256': '13bec748fa5590f295a1482f9d7e3bfdb92464c8920a038b4832d630281efbd5'},
 'actorScriptStartActionAndWait': {'address': 2148646620,
                                   'deferred_review_sha256': 'cea86bf5d32c6fe64854ab4573e18a7c24ce64cfa891e3e7e3d7a9c7a6c70e19',
                                   'review_sha256': 'd8496a261d8f65ffae6cc6e846f48a32d8651af47e1e89eda34b4cfbf43f6b50'},
 'actorScriptStartTimedAction': {'address': 2148654872,
                                 'review_sha256': '94e5dd7abcf693f34a346d3efcba474903c9989379e9e08faf1b8ee1a0f316d8'},
 'actorScriptTrackPlayerTertiaryObject': {'address': 2148652428,
                                          'review_sha256': 'bd4d17ab02e1b9682238944bbeb8ae46658b7077a6f3079fe7470fb80fda3375'},
 'actorScriptYield': {'address': 2148646576,
                      'review_sha256': 'a32e41ad6f38946c8b93e4a70a50e6600499bea62a2a82c22900dc39e298d3dc'}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptAcknowledgeMessage', 'actorScriptAdjustPikachuSecondaryMeter', 'actorScriptCreateAttachedEffect', 'actorScriptEnqueueMessage', 'actorScriptPlaySoundEffect', 'actorScriptPushActionScriptStopped', 'actorScriptPushTargetApproachDistance', 'actorScriptPushTimer', 'actorScriptRandomBelow', 'actorScriptSetDefaultActionMode', 'actorScriptSetHudGuideStyle', 'actorScriptSetMessengerContactMode', 'actorScriptSetSoundEffectPan', 'actorScriptSetSoundEffectVolume', 'actorScriptSetTargetToFallbackObject', 'actorScriptStartActionAndWait', 'actorScriptStartTimedAction', 'actorScriptTrackPlayerTertiaryObject', 'actorScriptYield'))
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


def _helper_closure(instructions, seeds, bindings):
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
                if type(native.get('address')) is not int or native['address'] != bindings[name_native]['address'] or native.get('inline_operands', []) != (['string_count'] if name_native=='actorScriptEnqueueMessage' else []):
                    raise ValueError('Rebound native or unsupported inline operands.')
                identity = (name_native, native['address'])
                if values[0] in selectors and selectors[values[0]] != identity:
                    raise ValueError('Ambiguous native selector.')
                selectors[values[0]] = identity
                normalized = [*identity, *values[1:]]
            instructions_encoded.append([row['offset'] - node['entry'], name, normalized, tags])
        encoded.append(instructions_encoded)
    return dict(sha256=_digest(encoded), nodes=nodes, roots=roots, anchor=anchor)

FAMILY_SHA256 = 'f8309d5742633a6456d74f0835cc3bacbac2956259920261ae7cdef4b9a3be34'
HELPER_HASHES = ('75fdf04f9f82202f6e9b0a1188c938bcebb154229fe7e09c0d860d94045ae6c8',
    'e67fc587e3fae402c897db69c9c26d2e4c5d75bf729486be3b5ec088d4c11e0b',
    'ded6e1f2de571df1ac854b5dd854ee2dc82dafc79a6aafec70d65b7da15174aa',
    '8a4bb7aab2cfbd47ae1a300685ac988a388a6b3020c48c87e352b566e8b36547')
SCOPE = ('Complete conditional selector121 routines and actual completion-helper closures. '
         'Operand floors, initialized coherent retained cells, nonaliasing storage and every '
         'native/deferred/SDK/resource/handle/string-table lifetime contract remain prerequisites. '
         'No physical motion, emotion, voice origin, delivery, probability or eventual return is proved.')


def _shared_binding(graph,dispatch,snapshots,helpers,policies,cleanups,instructions):
    """Pair each shared cut to this caller's exact snapshot and saved workflow."""
    cuts={role:target for target,role in graph['external'].items()}
    producers=[p for p in snapshots if p['entry_offset']==cuts[1]]
    if len(producers)!=1:raise ValueError('Ambiguous approach snapshot.')
    producer=producers[0]
    roots={role:int(slot) for slot,role in producer['root_role_mapping'].items()}
    own={role:slot for slot,role in graph['roots'].items()}
    if own!={0:roots.get(1)}:raise ValueError('Approach message-type coordinate differs from snapshot.')
    mask=_mask_helper(instructions,cuts[2],producer['status_variable'])
    matched=[c for c in cleanups if c['entry']==cuts[6] and c['workflow_variable']==dispatch['variable']]
    if len(matched)!=1:raise ValueError('Missing exact approach cleanup.')
    cleanup=matched[0]
    def same_context(row):
        return (row['workflow_variable']==dispatch['variable'] and
            row['cleanup_binding']['entry']==cleanup['entry'] and
            row['cleanup_binding']['workflow_variable']==dispatch['variable'] and
            row['cleanup_binding']['normalized_sha256']==cleanup['normalized_sha256'])
    eligible=[h for h in helpers if h['cut_identity']==0 and h['entry']==cuts[0] and same_context(h) and
        h['snapshot_binding']['entry']==producer['entry_offset'] and
        h['snapshot_binding']['canonical_sha256']==producer['canonical_sha256']]
    complete=[]
    for init in eligible:
        linked=[]
        for role in (3,4):
            rows=[p for p in policies if p['cut_identity']==role and p['entry']==cuts[role] and
                p['caller_entry']==init['caller_entry'] and same_context(p)]
            if len(rows)!=1:break
            linked.append(rows[0])
        if len(linked)!=2:continue
        tracking=[h for h in helpers if h['cut_identity']==7 and h['entry']==cuts[5] and
            h['caller_entry']==init['caller_entry'] and same_context(h) and
            h['snapshot_binding']==init['snapshot_binding']]
        if len(tracking)!=1:continue
        complete.append(dict(initializer=init,category_policy=linked[0],elapsed_policy=linked[1],
            tracking18=tracking[0],cleanup=cleanup,snapshot=producer,mask=mask))
    if len(complete)!=1:raise ValueError('Shared approach helper pairing is incomplete or ambiguous.')
    helper=_helper_closure(instructions,[cuts[7]],NATIVE_BINDINGS)
    if helper['sha256'] not in HELPER_HASHES:raise ValueError('Unreviewed completion helper closure.')
    cleanup_nodes={f['entry'] for f in cleanup['functions']}
    if any(n['entry'] not in cleanup_nodes for n in helper['nodes'][1:]):
        raise ValueError('Completion wait/audio descendants differ from this cleanup.')
    if any(a['entry']<b['end'] and b['entry']<a['end'] for a in helper['nodes'] for b in graph['functions']):
        raise ValueError('Completion closure overlaps approach functions.')
    return dict(complete[0],completion_helper=helper)


def approach_activity_roles(instructions, *, snapshot=None, root=ROOT):
    """Bind complete approach families to actual scoped selector121 dispatches."""
    unresolved=dict(status='unresolved',activities=[],source_bindings={},scope=SCOPE)
    try:
        if set(NATIVE_BINDINGS)!=REQUIRED_NATIVE_NAMES:raise ValueError('Incomplete approach native bindings.')
        reviews,preservation=context_snapshot(root=root) if snapshot is None else snapshot
        if preservation.get('stale_vm_functions') or preservation.get('context_vm_contract',{}).get('current') is not True:
            raise ValueError('Stale VM context.')
        if fingerprint((root/'src/ovl/script_vm.c').read_text())!=VM_UNIT_SHA256:
            raise ValueError('Stale complete VM source.')
        if fingerprint((root/'include/action_script_ids.h').read_text())!=ACTION_HEADER_SHA256:
            raise ValueError('Stale timed action identity header.')
        for name,binding in NATIVE_BINDINGS.items():
            active=reviews.get('active',{}).get(name)
            if active is None or _digest(active)!=binding['review_sha256']:raise ValueError('Stale approach native: '+name)
            if 'deferred_review_sha256' in binding:
                deferred=reviews.get('active_deferred',{}).get(name)
                if deferred is None or _digest(deferred)!=binding['deferred_review_sha256']:raise ValueError('Stale approach deferred native: '+name)
        validate_instructions(instructions)
        activity=activity_workflow_roles(instructions,snapshot=(reviews,preservation),root=root)
        snapshots=companion_snapshots(instructions,snapshot=(reviews,preservation),root=root)
        helpers=activity_helper_roles(instructions,snapshot=(reviews,preservation),root=root)
        policies=activity_policy_roles(instructions,snapshot=(reviews,preservation),root=root)
        saved=workflow_cleanup_roles(instructions,snapshot=(reviews,preservation),root=root)
        if (activity['status']!='conditional_workflows' or snapshots['status']!='conditional_snapshots' or
                helpers['status']!='conditional_helpers' or policies['status']!='conditional_policies' or saved['status']!='conditional_saved_workflows'):
            raise ValueError('Unresolved approach shared source contracts.')
        activities=[]
        for dispatch in activity['dispatches']:
            if dispatch['selector']!=121:continue
            try:
                graph=_family(instructions,dispatch['target_offset'],NATIVE_BINDINGS)
                if graph['sha256']!=FAMILY_SHA256:continue
                bound=_shared_binding(graph,dispatch,snapshots['snapshots'],helpers['helpers'],policies['policies'],saved['cleanups'],instructions)
                activities.append(_describe(graph,dispatch,bound))
            except (ValueError,KeyError,TypeError,IndexError):continue
        return dict(status='conditional_approach_activities',activities=activities,scope=SCOPE,
            source_bindings=dict(native_reviews=NATIVE_BINDINGS,vm_unit_sha256=VM_UNIT_SHA256,action_header_sha256=ACTION_HEADER_SHA256,
                activity=activity['source_bindings'],snapshots=snapshots['source_bindings'],helpers=helpers['source_bindings'],policies=policies['source_bindings'],saved=saved['source_bindings']))
    except (ValueError,OSError,KeyError,TypeError,IndexError) as error:return dict(unresolved,reason=str(error))


def _describe(graph,dispatch,bound):
    helper=bound['completion_helper'];bedroom=len(helper['nodes'])==6
    borrowed=[r['operands'][0] for r in helper['nodes'][0]['rows'] if r['name']=='SCRIPT_OP_PUSH_LITERAL' and r['operands'][0]['tag']==3]
    caption=next(slot for slot,role in helper['roots'].items() if role==0)
    return dict(entry=graph['functions'][0]['entry'],selector=121,dispatch_call_offset=dispatch['call_offset'],
        workflow_variable=dispatch['variable'],normalized_sha256=graph['sha256'],
        functions=[dict(entry=n['entry'],span_bytes=n['end']-n['entry'],reachable_bytes=sum(r['size'] for r in n['rows']),instruction_count=len(n['rows'])) for n in graph['functions']],
        local_roles={'counter':[1,0],'random_limit':[1,1],'initialized_but_not_read_cell':[1,2]},
        random_limit={'bound':10,'addend':20,'nominal_range':[20,29],'probability_proven':False},
        initialization={'hud_guide_style':1,'helper_entry':bound['initializer']['entry']},loop_default_action_mode=1,
        snapshot_binding={'entry':bound['snapshot']['entry_offset'],'canonical_sha256':bound['snapshot']['canonical_sha256']},
        sampled_message_type=[0,next(slot for slot,role in graph['roots'].items() if role==0)],status_mask_binding=bound['mask'],
        branch_order=[{'condition':'ordinary-message mask2 and sampled type4','target':bound['category_policy']['entry']},
            {'condition':'ordinary-message mask2 and sampled type not4','target':graph['functions'][1]['entry']},
            {'condition':'either enabled elapsed-limit mask32768/65536','target':bound['elapsed_policy']['entry']},
            {'condition':'counter strictly greater than random limit','target':graph['functions'][2]['entry']}],
        repeated_request={'action_program':16,'duration':0.5,
            'target':'Request tertiary player tracking; query remaining approach distance (approximate target distance minus allowance, clamped at0); if result<30 request fallback target, otherwise retain the result of tertiary tracking. Unavailable handles can retain stale fields under native contracts.',
            'counter_effect':'Timed native yields before continuation increments counter; requests are not completed actions. Under stable integer counter/nominal limit, strict counter>limit permits limit+1 requests.'},
        other_message_response={'sequence':'Request program86 and wait; invoke exact Secondary/Wait18/Primary helper; request program258 and wait; adjust secondary meter+10; invoke bound cleanup.',
            'action_programs':[86,258],'meter_scope':'Active Pikachu owning root receives clamped resulting secondary meter0..100; net+10 not guaranteed.'},
        completion={'sequence':'Adjust secondary meter+10, invoke complete bound helper, then exact cleanup. The first adjustment precedes any helper wait or queue request.',
            'helper_entry':helper['nodes'][0]['entry'],'normalized_sha256':helper['sha256'],'functions':_summary(helper),
            'frame_contract':{'arguments':0,'total_local_cells':1,'frame_index':1},
            'initial_phase':'Fallback-target request, contactmode1, timed program166/3.0 (native yields), sample elapsed. While f32(elapsed-sample)<5, request fallback and restart166/3.0 only when sequencer command absent, then yield.',
            'middle_phase':'Invoke full timed-program helper with125/3.0, then actual command-absence poll; request effectdefinition1/animationtrack183; queue text from one borrowed script-string reference; acknowledge pending; request contactmode2.',
            'borrowed_script_string_reference':borrowed[0] if len(borrowed)==1 else None,
            'string_scope':'Tag3 is a borrowed string-table index under current VM/string-table lifetime prerequisites, not an integer formatted as text or a universally meaningful caption.',
            'pending_phase':'Sample elapsed again. While f32(elapsed-sample)<20, acknowledge pending; any nonzero returned prior pending flag requests effectdefinition1/track164 and breaks. No message content, voice origin or delivery is inferred.',
            'tail':{'queued_raw_box_variable':[0,caption],'inline_string_count':1,'conversion':'PopString may borrow string data or format numeric text; queued text is not the box itself. The box meaning and producer are unresolved.','action_program':128,'wait':'Conditional StartActionAndWait; no completion promise.'},
            'variant':'bedroom_shared_global_audio_hook' if bedroom else 'ordinary_yield_poll',
            'audio_scope':'Bedroom actual Yield/audio descendants belong to this cleanup; shared global[-1,0] truth guard and conditional sound/RNG/scoped-counter writes do not establish a global audio-mode meaning or audible outcome.' if bedroom else None},
        tracking18_binding={'entry':bound['tracking18']['entry'],'normalized_sha256':bound['tracking18']['normalized_sha256'],'caller_entry':bound['tracking18']['caller_entry']},
        cleanup_binding={'entry':bound['cleanup']['entry'],'workflow_variable':dispatch['variable'],'normalized_sha256':bound['cleanup']['normalized_sha256']},
        shared_policy_bindings=[{'entry':bound[k]['entry'],'normalized_sha256':bound[k]['normalized_sha256']} for k in ('category_policy','elapsed_policy')],
        external_cuts=[dict(identity=r,target_offset=t) for t,r in graph['external'].items()],
        gaps_interpreted=False,external_calls_preserve_context_proven=False,completion_proven=False)
