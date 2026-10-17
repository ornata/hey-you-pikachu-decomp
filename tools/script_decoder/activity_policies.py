"""Complete shared activity policy closures with an explicitly bound cleanup cut.

Trigger conditions belong to each verified caller. Native requests, shared VM
cells and retained values imply neither emotional outcomes nor eventual return.
"""

from .activity_responses import activity_response_roles
from .callee_contexts import context_snapshot
from .companion_snapshots import _edges
from .context import ROOT
from .native_frames import fingerprint
from .request_arguments import _digest, validate_instructions
from .stack import incoming_edges

NATIVE_BINDINGS = {'actorScriptAdjustPikachuSecondaryMeter': {'address': 2148659428,
                                            'review_sha256': 'ad707956ca02a34708e441f4916193e28b410875772d09c7a6455d3be5665f1c'},
 'actorScriptCreateAttachedEffect': {'address': 2148649356,
                                     'review_sha256': 'ef640c052269675ac2c4ebff5c8761b339dab25fe4202c6ca3e90f141dbaa1ee'},
 'actorScriptEnqueueMessage': {'address': 2148647744,
                               'review_sha256': '84ebda162953537988a95f51d173b98011ef9b0937a09e813eb16d9247defacc'},
 'actorScriptPlaySoundEffect': {'address': 2148653104,
                                'review_sha256': 'c63cb7a7521377a5161362dae638a2972bddfb3d419b727fdd93799fe31e60c4'},
 'actorScriptPushActionScriptStopped': {'address': 2148648748,
                                        'review_sha256': '70c8afa259805820e65a4d63d8bb695ed5805e4efec59642dd201cc8ddd60e04'},
 'actorScriptPushHeldItemHandle': {'address': 2148660596,
                                   'review_sha256': '8597a859e3feb8f70f77b6a5304a86a849ded5af866731b7cfb892a74425bc4a'},
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
 'actorScriptStartActionAndWait': {'address': 2148646620,
                                   'deferred_review_sha256': 'cea86bf5d32c6fe64854ab4573e18a7c24ce64cfa891e3e7e3d7a9c7a6c70e19',
                                   'review_sha256': 'd8496a261d8f65ffae6cc6e846f48a32d8651af47e1e89eda34b4cfbf43f6b50'},
 'actorScriptStartTimedAction': {'address': 2148654872,
                                 'review_sha256': '94e5dd7abcf693f34a346d3efcba474903c9989379e9e08faf1b8ee1a0f316d8'},
 'actorScriptTrackEscapeFromPlayerSecondaryObject': {'address': 2148650132,
                                                     'review_sha256': '87f11b26546be4a8ca030c6fbc6e979435b9af1bb270ecf01aff911ec847ed10'},
 'actorScriptTrackPlayerPrimaryObject': {'address': 2148648960,
                                         'review_sha256': 'd00e68a3517f8f0631050f1d302ea605c983f4af330dcb7ad4ecdf73de6ea77d'},
 'actorScriptTrackPlayerSecondaryObject': {'address': 2148652360,
                                           'review_sha256': '6fd1e82b021dbf0d289abee78a4ac6599d7c22f9eeb2198df53636dcf95b4724'},
 'actorScriptYield': {'address': 2148646576,
                      'review_sha256': 'a32e41ad6f38946c8b93e4a70a50e6600499bea62a2a82c22900dc39e298d3dc'}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptAdjustPikachuSecondaryMeter', 'actorScriptCreateAttachedEffect', 'actorScriptEnqueueMessage', 'actorScriptPlaySoundEffect', 'actorScriptPushActionScriptStopped', 'actorScriptPushHeldItemHandle', 'actorScriptRandomBelow', 'actorScriptSetDefaultActionMode', 'actorScriptSetHudGuideStyle', 'actorScriptSetMessengerContactMode', 'actorScriptSetSoundEffectPan', 'actorScriptSetSoundEffectVolume', 'actorScriptStartActionAndWait', 'actorScriptStartTimedAction', 'actorScriptTrackEscapeFromPlayerSecondaryObject', 'actorScriptTrackPlayerPrimaryObject', 'actorScriptTrackPlayerSecondaryObject', 'actorScriptYield'))
VM_UNIT_SHA256 = 'cded04462b1f2fbece0ee8d937f9d4fcfc8c659719094b92768715d9669fe8fb'
POLICY_HASHES = {3: ('1f22497bc1571253d808cfc222a9a75c524cff03bd777770cde56ed0b149faf7', '9af9f5af69d073d8f79d9ec841fa884e36f810bfef4763b8b0a4c66551b91aa1'), 4: ('b8dfcff7430cdd4f9df9a08eb59e1ee2304d773f61ed00e5235f94f699428d9b',), 5: ('27cd7aada31d2f1752e066cc7fcc45f04ef2c2d69c55e77f3ae45ec9b1e9c005',)}
SCOPE = ('Conditional complete local policies only. Every cut is paired to one actual source-bound '
         'response caller, workflow coordinate and cleanup entry. Frame/operand validity, coherent retained '
         'values, allocation, SDK and deferred/native preconditions remain required. Polls can wait forever; '
         'requests do not prove physical movement, audible sound, delivery, emotion or completion.')


def _closure(instructions, entry, cleanup, bindings):
    """Traverse every local CALL and RETURN, stopping only at the bound cleanup."""
    if entry==cleanup:
        raise ValueError('Policy entry is its own cleanup cut.')
    by={r['offset']:r for r in instructions};incoming=incoming_edges(instructions)
    entries,nodes=[entry],[]
    for start in entries:
        if len(entries)>12:
            raise ValueError('Policy function budget exceeded.')
        pending,reached=[start],set()
        while pending:
            offset=pending.pop()
            if offset in reached:continue
            if offset not in by or len(reached)>=256:
                raise ValueError('Incomplete or excessive policy function.')
            row=by[offset];reached.add(offset)
            if row['name']=='SCRIPT_OP_CALL':
                target=row['operands'][0]['value']
                if target!=cleanup and target not in entries:entries.append(target)
            pending.extend(e['offset'] for e in row['successors'] if e['kind']!='call')
        rows=[by[o] for o in sorted(reached)]
        if rows[0]['name']!='SCRIPT_OP_ENTER_FRAME' or not any(r['name']=='SCRIPT_OP_RETURN' for r in rows):
            raise ValueError('Missing complete policy frame or return.')
        end=rows[-1]['offset']+rows[-1]['size']
        if end-start>4096 or any(start<o<end and o not in reached for o in by):
            raise ValueError('Unreviewed policy interior or excessive span.')
        expected={}
        for row in rows:
            if row['successors']!=_edges(row,[o['value'] for o in row['operands']]):
                raise ValueError('Incorrect full policy control flow.')
            for edge in row['successors']:expected.setdefault(edge['offset'],set()).add(row['offset'])
        if any(incoming.get(r['offset'],set())!=expected.get(r['offset'],set()) for r in rows[1:]):
            raise ValueError('Competing policy interior entry.')
        nodes.append(dict(entry=start,end=end,rows=rows))
    for i,node in enumerate(nodes):
        if node['entry']<=cleanup<node['end'] or any(node['entry']<other['end'] and other['entry']<node['end'] for other in nodes[i+1:]):
            raise ValueError('Overlapping local functions or cleanup cut.')
    callgraph={n['entry']:{r['operands'][0]['value'] for r in n['rows'] if r['name']=='SCRIPT_OP_CALL' and r['operands'][0]['value']!=cleanup} for n in nodes}
    def visit(start,path):
        if start in path:raise ValueError('Recursive policy call is unsupported.')
        for target in callgraph[start]:visit(target,path|{start})
    for start in entries:visit(start,set())
    roots,selectors,encoded={},{},[]
    for node in nodes:
        positions={r['offset']:i for i,r in enumerate(node['rows'])};normalized_rows=[]
        for row in node['rows']:
            name=row['name'];values=[o['value'] for o in row['operands']];tags=[o['tag'] for o in row['operands']];normalized=values[:]
            if name in ('SCRIPT_OP_LOAD_VARIABLE','SCRIPT_OP_STORE_VARIABLE','SCRIPT_OP_LOAD_ELEMENT','SCRIPT_OP_STORE_ELEMENT') and values[0]==0:
                roots.setdefault(values[1],len(roots));normalized=[0,'root-role-'+str(roots[values[1]])]
            elif name in ('SCRIPT_OP_JUMP','SCRIPT_OP_JUMP_IF_ZERO'):
                if values[0] not in positions:raise ValueError('Policy branch leaves function.')
                normalized=['instruction',positions[values[0]]]
            elif name=='SCRIPT_OP_CALL':normalized=['cleanup-cut'] if values[0]==cleanup else ['local-function',entries.index(values[0])]
            elif name=='SCRIPT_OP_CALL_NATIVE':
                native=row.get('native',{});names=native.get('functions',[])
                if len(names)!=1 or names[0] not in bindings:raise ValueError('Unknown policy native.')
                n=names[0]
                if type(native.get('address')) is not int or native['address']!=bindings[n]['address'] or native.get('inline_operands',[])!=(['string_count'] if n=='actorScriptEnqueueMessage' else []):
                    raise ValueError('Rebound policy native or inline shape.')
                identity=(n,native['address'])
                if values[0] in selectors and selectors[values[0]]!=identity:raise ValueError('Ambiguous policy native selector.')
                selectors[values[0]]=identity;normalized=[n,native['address'],*values[1:]]
            normalized_rows.append([row['offset']-node['entry'],name,normalized,tags])
        encoded.append(normalized_rows)
    if not any(r['name']=='SCRIPT_OP_CALL' and r['operands'][0]['value']==cleanup for node in nodes for r in node['rows']):
        raise ValueError('Policy has no cleanup cut.')
    return dict(sha256=_digest(encoded),nodes=nodes,roots=roots,cleanup=cleanup)


def activity_policy_roles(instructions, *, snapshot=None, root=ROOT):
    """Bind complete policy graphs per caller, never by a union of workflow roots."""
    unresolved=dict(status='unresolved',policies=[],source_bindings={},scope=SCOPE)
    try:
        if set(NATIVE_BINDINGS)!=REQUIRED_NATIVE_NAMES:
            raise ValueError('Incomplete policy native binding set.')
        reviews,preservation=context_snapshot(root=root) if snapshot is None else snapshot
        if preservation.get('stale_vm_functions') or preservation.get('context_vm_contract',{}).get('current') is not True:
            raise ValueError('Stale VM context.')
        if fingerprint((root/'src/ovl/script_vm.c').read_text())!=VM_UNIT_SHA256:
            raise ValueError('Stale complete VM source.')
        for name,binding in NATIVE_BINDINGS.items():
            active=reviews.get('active',{}).get(name)
            if active is None or _digest(active)!=binding['review_sha256']:
                raise ValueError('Stale policy native: '+name)
            if 'deferred_review_sha256' in binding:
                deferred=reviews.get('active_deferred',{}).get(name)
                if deferred is None or _digest(deferred)!=binding['deferred_review_sha256']:
                    raise ValueError('Stale policy deferred native: '+name)
        validate_instructions(instructions)
        callers=activity_response_roles(instructions,snapshot=(reviews,preservation),root=root)
        if callers['status']!='conditional_responses':raise ValueError('Unresolved response caller source bindings.')
        policies=[]
        for caller in callers['responses']:
            bound=caller['cleanup_binding']
            if bound['workflow_variable']!=caller['workflow_variable']:continue
            cuts={c['identity']:c['target_offset'] for c in caller['external_cuts']}
            if cuts.get(8)!=bound['entry']:continue
            for role in (3,4,5):
                try:
                    graph=_closure(instructions,cuts[role],bound['entry'],NATIVE_BINDINGS)
                    if graph['sha256'] not in POLICY_HASHES.get(role,()):continue
                    policies.append(_describe(graph,role,caller,bound))
                except (ValueError,KeyError,TypeError,IndexError):continue
        return dict(status='conditional_policies',policies=policies,scope=SCOPE,
                    source_bindings=dict(native_reviews=NATIVE_BINDINGS,vm_unit_sha256=VM_UNIT_SHA256,response_source_bindings=callers['source_bindings']))
    except (ValueError,OSError,KeyError,TypeError,IndexError) as error:return dict(unresolved,reason=str(error))


def _describe(graph,role,caller,bound):
    description=dict(entry=graph['nodes'][0]['entry'],caller_entry=caller['entry'],cut_identity=role,
        workflow_variable=caller['workflow_variable'],cleanup_binding=bound,normalized_sha256=graph['sha256'],
        functions=[dict(entry=n['entry'],span_bytes=n['end']-n['entry'],reachable_bytes=sum(r['size'] for r in n['rows']),instruction_count=len(n['rows'])) for n in graph['nodes']],
        native_requests=[dict(instruction_offset=r['offset'],function=r['native']['functions'][0],address=r['native']['address']) for n in graph['nodes'] for r in n['rows'] if r['name']=='SCRIPT_OP_CALL_NATIVE'],
        cleanup_cut_sites=[r['offset'] for n in graph['nodes'] for r in n['rows'] if r['name']=='SCRIPT_OP_CALL' and r['operands'][0]['value']==bound['entry']],
        conditional_cleanup=True,gaps_interpreted=False,completion_proven=False)
    if role==5:
        description.update(trigger='Caller counter/distance-limit branch, not an intrinsic policy-only predicate.',
            requests={'default_action_mode':7,'hud_guide_style':0,'messenger_contact_mode':1,
                      'sequence':'Track player secondary, request program17 and wait, then track player primary; request program48 and wait; adjust secondary meter by -5; invoke bound cleanup.'},
            meter_scope='Request applies to the active Pikachu owning root and clamps the resulting secondary meter to0..100; net-5 is not guaranteed.',
            tracking_scope='Secondary tracking refreshes target position only for an available handle; invalid/toolbox falls back to FIXED mode with stale values. Primary tracking writes its retained handle and FOLLOW mode, clears allowance to zero, and does not refresh XYZ or validate that handle.')
    elif role==4:
        source=next(slot for slot,order in graph['roots'].items() if order==0)
        description.update(trigger='Caller enabled elapsed-limit branch; policy body alone establishes no time condition.',
            requests={'hud_guide_style':0,'effect_definition':0,'animation_track':168,'action_program':119},
            queued_value={'variable':[0,source],'inline_string_count':1,'semantics':'Convert the retained boxed root value through PopString and request queuing of the resulting text; no caption-ID, text content or delivered-message assertion.'})
    else:
        bedroom=len(graph['nodes'])==5
        description.update(trigger='Caller stored category4 branch; no universal voice-origin or insult-delivery proof.',
            requests={'hud_guide_style':0,'messenger_contact_mode':1,'effect_definition':0,'animation_track':167,'default_action_mode':5,
                      'sequence':'Request program109 and wait; if held handle nonzero, request program79 and wait; track escape from player secondary, request timed program15 for0.5, poll command absence, then timed program13 for2.0 and poll; invoke bound cleanup.'},
            attachment_scope='Definition0/track167 has allocation and callback prerequisites. When the executing actor is active Pikachu, its additional RNG and conditional negative-reaction progress-credit branch applies. No visual or emotional outcome is guaranteed.',
            polling_variant='bedroom_shared_global_audio_hook' if bedroom else 'ordinary_yield_poll',
            polling_scope='Command-absence polling is not motion completion and can remain pending indefinitely.')
        if bedroom:
            counter=next(slot for slot,order in graph['roots'].items() if order==0)
            description.update(shared_global_guard={'variable':[-1,0],
                'condition':'Original numeric value zero invokes the audio hook; original nonzero skips it. LOGICAL_NOT tests truth before JUMP_IF_ZERO converts the boolean result; for float input the result remains FLOAT-tagged 1.0/0.0, and float0.5 skips. Valid decimal strings are parsed before unary truth testing.',
                'meaning_unresolved':True},
                audio_counter_variable=[0,counter],
                audio_requests='Conditional sound133 playback, pan16+RandomBelow96, volume64+RandomBelow64; conditional counter reload180+RandomBelow120 or -(15 + RandomBelow(30)), increments or decrements. RNG consumption and writes depend on each branch; no sound-handle/audibility or integer-counter invariant is inferred.')
    return description
