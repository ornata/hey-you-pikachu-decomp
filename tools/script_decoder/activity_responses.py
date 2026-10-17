"""Complete source-bound repeated-action and integer-parity response routines.

Workflow selectors, program IDs, effect definitions and animation tracks remain
separate domains. External cuts are opaque unless their full interfaces are bound.
"""

import hashlib

from .activity_workflows import activity_workflow_roles, _mask_helper
from .callee_contexts import context_snapshot
from .companion_snapshots import _edges, companion_snapshots
from .context import ROOT
from .effects import enum_names
from .native_frames import fingerprint
from .request_arguments import _digest, validate_instructions
from .stack import incoming_edges
from .workflow_cleanup import workflow_cleanup_roles

NATIVE_BINDINGS = {'actorScriptAdjustPikachuSecondaryMeter': {'address': 2148659428,
                                            'review_sha256': 'ad707956ca02a34708e441f4916193e28b410875772d09c7a6455d3be5665f1c'},
 'actorScriptCreateAttachedEffect': {'address': 2148649356,
                                     'review_sha256': 'ef640c052269675ac2c4ebff5c8761b339dab25fe4202c6ca3e90f141dbaa1ee'},
 'actorScriptPassThroughInteger': {'address': 2148647536,
                                   'review_sha256': '7ce5019c6dafb96114d27892f09383b0f4af354c265b840c4a24c41823eb16d8'},
 'actorScriptPushActionScriptStopped': {'address': 2148648748,
                                        'review_sha256': '70c8afa259805820e65a4d63d8bb695ed5805e4efec59642dd201cc8ddd60e04'},
 'actorScriptRandomBelow': {'address': 2148647408,
                            'review_sha256': '45206f67a57091cbca5b17b2e640a6618c518b8c7461fc7899614bf7630e4a8f'},
 'actorScriptSetDefaultActionMode': {'address': 2148654648,
                                     'review_sha256': 'd2a281d4d491c14208ce217537e4d13c10e9d0cbb610630bbefeab68cedc0fdf'},
 'actorScriptSetHudGuideStyle': {'address': 2148656300,
                                 'review_sha256': 'c2550c6be82042cc8721a3d50aa8de50fea13bdf0118ed1880eac3be43c471e7'},
 'actorScriptStartActionAndWait': {'address': 2148646620,
                                   'deferred_review_sha256': 'cea86bf5d32c6fe64854ab4573e18a7c24ce64cfa891e3e7e3d7a9c7a6c70e19',
                                   'review_sha256': 'd8496a261d8f65ffae6cc6e846f48a32d8651af47e1e89eda34b4cfbf43f6b50'},
 'actorScriptStartActionScript': {'address': 2148648308,
                                  'review_sha256': 'd50003a3331b39d98ba6bedb287ca32610703946124fec0e44072841954b83f8'},
 'actorScriptYield': {'address': 2148646576,
                      'review_sha256': 'a32e41ad6f38946c8b93e4a70a50e6600499bea62a2a82c22900dc39e298d3dc'}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptAdjustPikachuSecondaryMeter', 'actorScriptCreateAttachedEffect', 'actorScriptPassThroughInteger', 'actorScriptPushActionScriptStopped', 'actorScriptRandomBelow', 'actorScriptSetDefaultActionMode', 'actorScriptSetHudGuideStyle', 'actorScriptStartActionAndWait', 'actorScriptStartActionScript', 'actorScriptYield'))
VM_UNIT_SHA256 = 'cded04462b1f2fbece0ee8d937f9d4fcfc8c659719094b92768715d9669fe8fb'
ACTION_HEADER_SHA256 = 'bf765dadf5e75d64af2f52cb840813ee22098b26142eafb6c9e5c5329f057fca'
FAMILY_SHA256 = '94b72c13c9d9b06c96655e03ca9945c962bdc796562215b2814aa9b8cdbd7b45'
SCOPE = ('Conditional complete local routines only. Actual selector118 dispatch and same-coordinate '
         'workflow linkage are required; no global selector identity or reaching initialization is inferred. '
         'Opaque helpers may mutate state, wait or fail to return. Coherent message cells across those '
         'calls, defined VM conversions, protected frames and all native/deferred resource/lifetime '
         'preconditions remain. No emotion, probability, physical gesture or completed response proof.')


def _family(instructions, entry, bindings):
    """Normalize both complete local functions and nine explicit external cuts."""
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
            if len(calls)!=8 or calls[4]==entry:
                raise ValueError('Unsupported main call-role shape.')
            entries.append(calls[4])
        functions.append(dict(entry=start,end=end,rows=rows))
    if len(functions)!=2 or functions[0]['entry']<functions[1]['end'] and functions[1]['entry']<functions[0]['end']:
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
    if len(external)!=9:
        raise ValueError('Unsupported external-cut alias pattern.')
    return dict(sha256=_digest(encoded),functions=functions,roots=roots,external=external)


def activity_response_roles(instructions, *, snapshot=None, root=ROOT):
    """Bind complete response families to actual scoped dispatch and helper targets."""
    unresolved=dict(status='unresolved',responses=[],source_bindings={},scope=SCOPE)
    try:
        if set(NATIVE_BINDINGS)!=REQUIRED_NATIVE_NAMES:
            raise ValueError('Incomplete response native binding set.')
        reviews,preservation=context_snapshot(root=root) if snapshot is None else snapshot
        if preservation.get('stale_vm_functions') or preservation.get('context_vm_contract',{}).get('current') is not True:
            raise ValueError('Stale VM context.')
        if fingerprint((root/'src/ovl/script_vm.c').read_text())!=VM_UNIT_SHA256:
            raise ValueError('Stale complete VM source.')
        header=root/'include/action_script_ids.h'
        if hashlib.sha256(header.read_bytes()).hexdigest()!=ACTION_HEADER_SHA256:
            raise ValueError('Stale timed action identities.')
        names=enum_names(header,'ACTION_SCRIPT_')
        for name,binding in NATIVE_BINDINGS.items():
            active=reviews.get('active',{}).get(name)
            if active is None or _digest(active)!=binding['review_sha256']:
                raise ValueError('Stale response native: '+name)
            if 'deferred_review_sha256' in binding:
                deferred=reviews.get('active_deferred',{}).get(name)
                if deferred is None or _digest(deferred)!=binding['deferred_review_sha256']:
                    raise ValueError('Stale response deferred native: '+name)
        validate_instructions(instructions)
        activity=activity_workflow_roles(instructions,snapshot=(reviews,preservation),root=root)
        if activity['status']!='conditional_workflows':
            raise ValueError('Unresolved activity linkage.')
        eligible=[d for d in activity['dispatches'] if d['selector']==118]
        source_bindings=dict(native_reviews=NATIVE_BINDINGS,vm_unit_sha256=VM_UNIT_SHA256,action_header_sha256=ACTION_HEADER_SHA256,
                             activity_source_bindings=activity['source_bindings'])
        if not eligible:
            return dict(status='conditional_responses',responses=[],source_bindings=source_bindings,scope=SCOPE)
        saved=workflow_cleanup_roles(instructions,snapshot=(reviews,preservation),root=root)
        snapshots=companion_snapshots(instructions,snapshot=(reviews,preservation),root=root)
        if saved['status']!='conditional_saved_workflows' or snapshots['status']!='conditional_snapshots':
            raise ValueError('Unresolved cleanup or snapshot source linkage.')
        responses=[]
        for dispatch in eligible:
            try:
                graph=_family(instructions,dispatch['target_offset'],NATIVE_BINDINGS)
                if graph['sha256']!=FAMILY_SHA256:continue
                cuts={role:target for target,role in graph['external'].items()}
                producer=next((p for p in snapshots['snapshots'] if p['entry_offset']==cuts[1]),None)
                if producer is None:continue
                roles={role:int(slot) for slot,role in producer['root_role_mapping'].items()}
                own={role:slot for slot,role in graph['roots'].items()}
                if own.get(0)!=roles.get(1) or own.get(1)!=roles.get(2):continue
                mask=_mask_helper(instructions,cuts[2],producer['status_variable'])
                cleanup=[c for c in saved['cleanups'] if c['entry']==cuts[8] and c['workflow_variable']==dispatch['variable']]
                if len(cleanup)!=1:continue
                responses.append(_describe(graph,dispatch,names,producer,mask,cleanup[0]))
            except (ValueError,KeyError,TypeError,IndexError):
                continue
        source_bindings.update(cleanup_source_bindings=saved['source_bindings'],snapshot_source_bindings=snapshots['source_bindings'])
        return dict(status='conditional_responses',responses=responses,source_bindings=source_bindings,scope=SCOPE)
    except (ValueError,OSError,KeyError,TypeError,IndexError) as error:
        return dict(unresolved,reason=str(error))


def _describe(graph,dispatch,names,producer,mask,cleanup):
    roots={role:slot for slot,role in graph['roots'].items()}
    return dict(entry=graph['functions'][0]['entry'],response_entry=graph['functions'][1]['entry'],selector=118,
        dispatch_call_offset=dispatch['call_offset'],workflow_variable=dispatch['variable'],normalized_sha256=graph['sha256'],
        functions=[dict(entry=f['entry'],span_bytes=f['end']-f['entry'],reachable_bytes=sum(r['size'] for r in f['rows']),instruction_count=len(f['rows'])) for f in graph['functions']],
        local_roles={'counter':[1,0],'random_limit':[1,1]},random_limit={'bound':3,'addend':3,'nominal_range':[3,5]},
        cycle='Test counter>limit before requesting another program. When the sequencer reports command absence, invoke opaque cut6, request program237, then increment the counter. Native request and explicit loop can yield; no completed-action count follows.',
        stored_message_fields={'type':[0,roots[0]],'argument0':[0,roots[1]]},
        message_snapshot_binding={'entry':producer['entry_offset'],'canonical_sha256':producer['canonical_sha256']},status_mask_binding=mask,
        branch_order=[{'condition':'ordinary-message mask2 nonzero and stored type4','invocation':'opaque cut3'},
                      {'condition':'ordinary-message mask2 nonzero and stored type not4','invocation':'complete local response'},
                      {'condition':'either enabled elapsed-limit mask32768/65536','invocation':'opaque cut4'},
                      {'condition':'player-distance-at-least60 mask256','invocation':'opaque cut5'},
                      {'condition':'counter strictly above random limit','invocation':'opaque cut5'}],
        parity_response={'input':'Integer conversion of the stored argument0 followed by original signed remainder2; cell coherence across opaque cut7 is required.',
                         'even':{'effect_definition':0,'animation_track':168,'action_program':256,'action_name':names.get(256)},
                         'odd':{'action_program':234,'action_name':names.get(234)},
                         'tail':'Request secondary-meter adjustment+10, then invoke the bound complete saved-workflow cleanup. No category2/voice-origin or successful effect/action/meter outcome proof.'},
        cleanup_binding={'entry':cleanup['entry'],'normalized_sha256':cleanup['normalized_sha256'],'workflow_variable':cleanup['workflow_variable']},
        external_cuts=[dict(identity=role,target_offset=target,binding_kind={1:'source_bound_companion_snapshot',2:'source_bound_status_mask',8:'source_bound_saved_workflow_cleanup'}.get(role,'opaque_script_invocation')) for target,role in graph['external'].items()],
        gaps_interpreted=False,external_cuts_preserve_context_proven=False,completion_proven=False)
