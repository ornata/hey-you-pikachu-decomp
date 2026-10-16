"""Conditional activity selection and complete player-relative episode roles.

Workflow selectors, timed programs, sound IDs and message categories remain
separate domains. Observations establish no execution, probability or outcome.
"""

import hashlib

from .companion_snapshots import _edges, companion_snapshots
from .callee_contexts import context_snapshot
from .context import ROOT
from .effects import enum_names
from .native_frames import fingerprint
from .request_arguments import _digest, validate_instructions
from .rps_rounds import rps_rounds
from .stack import incoming_edges

NATIVE_BINDINGS = {'actorScriptChooseEscapeFromPlayer': {'address': 2148650188,
                                       'review_sha256': 'e3f75eaa50baa2b2e60a1276fb94c3cdcb074fbd452102b49d6bb123e4aac665'},
 'actorScriptEnqueueMessage': {'address': 2148647744,
                               'review_sha256': '84ebda162953537988a95f51d173b98011ef9b0937a09e813eb16d9247defacc'},
 'actorScriptPlayTrackedSound': {'address': 2148653032,
                                 'review_sha256': '47dce270cf37adc477b44c0af5c9f2de24592475081d39e1536900a2790790aa'},
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
 'actorScriptStartActionAndWait': {'address': 2148646620,
                                   'deferred_review_sha256': 'cea86bf5d32c6fe64854ab4573e18a7c24ce64cfa891e3e7e3d7a9c7a6c70e19',
                                   'review_sha256': 'd8496a261d8f65ffae6cc6e846f48a32d8651af47e1e89eda34b4cfbf43f6b50'},
 'actorScriptStartActionScript': {'address': 2148648308,
                                  'review_sha256': 'd50003a3331b39d98ba6bedb287ca32610703946124fec0e44072841954b83f8'},
 'actorScriptStartTimedAction': {'address': 2148654872,
                                 'review_sha256': '94e5dd7abcf693f34a346d3efcba474903c9989379e9e08faf1b8ee1a0f316d8'},
 'actorScriptTrackEscapeFromPlayerSecondaryObject': {'address': 2148650132,
                                                     'review_sha256': '87f11b26546be4a8ca030c6fbc6e979435b9af1bb270ecf01aff911ec847ed10'},
 'actorScriptYield': {'address': 2148646576,
                      'review_sha256': 'a32e41ad6f38946c8b93e4a70a50e6600499bea62a2a82c22900dc39e298d3dc'}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptChooseEscapeFromPlayer', 'actorScriptEnqueueMessage', 'actorScriptPlayTrackedSound', 'actorScriptPushTimer', 'actorScriptRandomBelow', 'actorScriptSetDefaultActionMode', 'actorScriptSetHudGuideStyle', 'actorScriptSetMessengerContactMode', 'actorScriptStartActionAndWait', 'actorScriptStartActionScript', 'actorScriptStartTimedAction', 'actorScriptTrackEscapeFromPlayerSecondaryObject', 'actorScriptYield'))
VM_UNIT_SHA256 = 'cded04462b1f2fbece0ee8d937f9d4fcfc8c659719094b92768715d9669fe8fb'
ACTION_HEADER_SHA256 = 'bf765dadf5e75d64af2f52cb840813ee22098b26142eafb6c9e5c5329f057fca'
EPISODE_SHA256 = '43ea2e3a0da48332b59083a270bee0b937e11ffe6e3291b3cf73532adbb0c3f3'
SCOPE = ('Conditional coordinate-scoped observations only. A source-bound random request and same-root '
         'dispatch do not prove reaching initialization, branch feasibility, probabilities or stable state. '
         'External helpers are opaque and may mutate context, wait or fail to return. All native/deferred '
         'resource, frame, conversion and lifetime preconditions remain; no emotion, physical movement or completion proof.')


def _closed(rows, incoming):
    for index, row in enumerate(rows):
        if index and (rows[index-1]['offset']+rows[index-1]['size'] != row['offset']
                      or incoming.get(row['offset'],set()) != {rows[index-1]['offset']}):
            return False
    return True


def _episode(instructions, entry, bindings):
    """Normalize all reachable rows of the reviewed three-function family."""
    by={r['offset']:r for r in instructions}
    incoming=incoming_edges(instructions)
    entries=[entry]
    functions=[]
    for start in entries:
        pending,reached=[start],set()
        while pending:
            offset=pending.pop()
            if offset in reached:continue
            if offset not in by or len(reached)>=256:raise ValueError('Incomplete or excessive episode function.')
            row=by[offset];reached.add(offset)
            pending.extend(e['offset'] for e in row['successors'] if e['kind']!='call')
        rows=[by[o] for o in sorted(reached)]
        if rows[0]['name']!='SCRIPT_OP_ENTER_FRAME' or [o['value'] for o in rows[0]['operands']]!=[0,0,1]:
            raise ValueError('Unsupported episode frame.')
        if not any(r['name']=='SCRIPT_OP_RETURN' for r in rows):raise ValueError('Missing episode return.')
        end=rows[-1]['offset']+rows[-1]['size']
        if end-start>2048 or any(start<o<end and o not in reached for o in by):raise ValueError('Unreviewed episode rows.')
        expected={}
        for row in rows:
            for edge in row['successors']:expected.setdefault(edge['offset'],set()).add(row['offset'])
        if any(incoming.get(r['offset'],set())!=expected.get(r['offset'],set()) for r in rows[1:]):
            raise ValueError('Competing episode interior entry.')
        if not functions:
            calls=list(dict.fromkeys(r['operands'][0]['value'] for r in rows if r['name']=='SCRIPT_OP_CALL'))
            if len(calls)!=6:raise ValueError('Unsupported episode call-role shape.')
            entries.extend(calls[4:6])
            if len(set(entries))!=3:raise ValueError('Overlapping episode entries.')
        functions.append(dict(entry=start,end=end,rows=rows))
    if len(functions)!=3:raise ValueError('Incomplete episode closure.')
    for i,a in enumerate(functions):
        if any(a['entry']<b['end'] and b['entry']<a['end'] for b in functions[i+1:]):
            raise ValueError('Overlapping episode function extents.')
    roots,external,strings,selectors,encoded={},{},{},{},[]
    for function in functions:
        rows=function['rows'];positions={r['offset']:i for i,r in enumerate(rows)};node=[]
        for row in rows:
            name=row['name'];values=[o['value'] for o in row['operands']];tags=[o['tag'] for o in row['operands']]
            if row['successors']!=_edges(row,values):raise ValueError('Incorrect episode edges.')
            normalized=values[:]
            if name in ('SCRIPT_OP_LOAD_VARIABLE','SCRIPT_OP_STORE_VARIABLE','SCRIPT_OP_LOAD_ELEMENT','SCRIPT_OP_STORE_ELEMENT') and values[0]==0:
                roots.setdefault(values[1],len(roots));normalized=[0,'root-role-'+str(roots[values[1]])]
            elif name in ('SCRIPT_OP_JUMP','SCRIPT_OP_JUMP_IF_ZERO'):
                if values[0] not in positions:raise ValueError('Episode branch leaves function.')
                normalized=['instruction',positions[values[0]]]
            elif name=='SCRIPT_OP_PUSH_LITERAL' and tags==[3]:
                if values[0]<0:raise ValueError('Negative text index.')
                strings.setdefault(values[0],len(strings));normalized=['opaque-string-index',strings[values[0]]]
            elif name=='SCRIPT_OP_CALL':
                if values[0] in entries:normalized=['internal-function',entries.index(values[0])]
                else:
                    if any(f['entry']<=values[0]<f['end'] for f in functions):raise ValueError('External call enters episode interior.')
                    external.setdefault(values[0],len(external));normalized=['opaque-callee',external[values[0]]]
            elif name=='SCRIPT_OP_CALL_NATIVE':
                native=row.get('native',{});names=native.get('functions',[])
                if len(names)!=1 or names[0] not in bindings:raise ValueError('Unknown episode native.')
                n=names[0];binding=bindings[n]
                if type(native.get('address')) is not int or native['address']!=binding['address']:raise ValueError('Rebound episode native.')
                if native.get('inline_operands',[]) != (['string_count'] if n=='actorScriptEnqueueMessage' else []):raise ValueError('Incorrect inline metadata.')
                identity=(n,native['address'])
                if values[0] in selectors and selectors[values[0]]!=identity:raise ValueError('Ambiguous episode selector.')
                selectors[values[0]]=identity;normalized=[n,native['address'],*values[1:]]
            node.append([row['offset']-function['entry'],name,normalized,tags])
        encoded.append(node)
    return dict(sha256=_digest(encoded),functions=functions,roots=roots,external=external,strings=strings)


def _mask_helper(instructions, entry, status_variable):
    """Validate the complete one-argument masked-status getter interface."""
    by={r['offset']:i for i,r in enumerate(instructions)}
    if entry not in by:raise ValueError('Missing status mask helper.')
    rows=instructions[by[entry]:by[entry]+5]
    if len(rows)!=5 or [r['name'] for r in rows]!=['SCRIPT_OP_'+n for n in ('ENTER_FRAME','LOAD_VARIABLE','LOAD_VARIABLE','BITWISE_AND','RETURN')]:
        raise ValueError('Unsupported status mask helper shape.')
    expected=[[1,1,1],status_variable,[1,0],[],[]]
    if [[o['value'] for o in r['operands']] for r in rows]!=expected or any(o['tag']!=0 for r in rows for o in r['operands']):
        raise ValueError('Status mask helper operands or root coupling differ.')
    if not _closed(rows,incoming_edges(instructions)) or rows[-1]['offset']+rows[-1]['size']-entry!=40:
        raise ValueError('Incomplete or competing status mask helper.')
    return dict(entry=entry,bytes=40,instruction_offsets=[r['offset'] for r in rows],
                status_variable=status_variable,mask_argument=[1,0],
                result='Integer-converted status bitwise-AND integer-converted supplied mask.',
                scope='Valid operand floor, initialized coherent status and VM numeric conversions remain prerequisites.')


def _observations(instructions, rps_entries, episodes):
    """Match exclusive authored producer and dispatch expressions, retaining epochs."""
    incoming=incoming_edges(instructions);producers=[];dispatches=[]
    for i,row in enumerate(instructions):
        q=instructions[i:i+7]
        if len(q)==7 and [r['name'] for r in q]==['SCRIPT_OP_'+n for n in ('PUSH_LITERAL','PUSH_LITERAL','PUSH_LITERAL','SUBTRACT','CALL_NATIVE','ADD','STORE_VARIABLE')]:
            native=q[4].get('native',{})
            if ([r['operands'][0]['value'] for r in q[:3]]==[116,124,116]
                and all(r['operands'][0]['tag']==0 for r in q[:3])
                and [o['value'] for o in q[6]['operands']][0]==0
                and native.get('functions')==['actorScriptRandomBelow']
                and type(native.get('address')) is int and native['address']==NATIVE_BINDINGS['actorScriptRandomBelow']['address']
                and native.get('inline_operands',[])==[] and _closed(q,incoming)):
                producers.append(dict(start_offset=q[0]['offset'],store_offset=q[6]['offset'],variable=[0,q[6]['operands'][1]['value']],
                                      nominal_integer_range=[116,123],random_bound=8,expression='116 + RandomBelow(124 - 116)',
                                      reaching_initialization_proven=False))
        q=instructions[i:i+5]
        if len(q)==5 and [r['name'] for r in q]==['SCRIPT_OP_'+n for n in ('LOAD_VARIABLE','PUSH_LITERAL','EQUAL','JUMP_IF_ZERO','CALL')]:
            values=[o['value'] for o in q[0]['operands']];literal=q[1]['operands'][0]
            if (values[0]==0 and literal['tag']==0 and 116<=literal['value']<=123
                and q[3]['operands'][0]['value']>=q[4]['offset']+q[4]['size'] and _closed(q,incoming)):
                target=q[4]['operands'][0]['value'];meaning=None
                if literal['value']==123 and target in rps_entries:meaning='source_bound_rps_round'
                elif literal['value']==122 and target in episodes:meaning='source_bound_player_relative_episode'
                dispatches.append(dict(test_offset=q[0]['offset'],call_offset=q[4]['offset'],variable=values,
                                       selector=literal['value'],target_offset=target,role=meaning,
                                       scope='Conditional equality dispatch only; other selectors remain numeric and targets opaque.'))
    roots={tuple(d['variable']) for d in dispatches if d['role']=='source_bound_rps_round'}
    roots &= {tuple(p['variable']) for p in producers}
    producers=[p for p in producers if tuple(p['variable']) in roots]
    dispatches=[d for d in dispatches if tuple(d['variable']) in roots]
    workflows=[dict(variable=list(v),producers=[p['store_offset'] for p in producers if tuple(p['variable'])==v],
                    dispatches=[d['call_offset'] for d in dispatches if tuple(d['variable'])==v],
                    selectors=sorted({d['selector'] for d in dispatches if tuple(d['variable'])==v}),
                    persistent_variable_role_proven=False) for v in sorted(roots)]
    return workflows,producers,dispatches


def activity_workflow_roles(instructions, *, snapshot=None, root=ROOT):
    """Inventory source-bound workflow coordinates and complete episode families."""
    unresolved=dict(status='unresolved',workflows=[],producers=[],dispatches=[],episodes=[],source_bindings={},scope=SCOPE)
    try:
        if set(NATIVE_BINDINGS)!=REQUIRED_NATIVE_NAMES:raise ValueError('Incomplete episode native binding set.')
        reviews,preservation=context_snapshot(root=root) if snapshot is None else snapshot
        if preservation.get('stale_vm_functions') or preservation.get('context_vm_contract',{}).get('current') is not True:
            raise ValueError('Stale VM context.')
        if fingerprint((root/'src/ovl/script_vm.c').read_text())!=VM_UNIT_SHA256:raise ValueError('Stale complete VM unit.')
        header=root/'include/action_script_ids.h'
        if hashlib.sha256(header.read_bytes()).hexdigest()!=ACTION_HEADER_SHA256:raise ValueError('Stale timed action identities.')
        action_names=enum_names(header,'ACTION_SCRIPT_')
        for n,b in NATIVE_BINDINGS.items():
            a=reviews.get('active',{}).get(n)
            if a is None or _digest(a)!=b['review_sha256']:raise ValueError('Stale episode native: '+n)
            if 'deferred_review_sha256' in b:
                a=reviews.get('active_deferred',{}).get(n)
                if a is None or _digest(a)!=b['deferred_review_sha256']:raise ValueError('Stale episode deferred native: '+n)
        validate_instructions(instructions)
        rounds=rps_rounds(instructions,snapshot=(reviews,preservation),root=root)
        if rounds['status']!='conditional_rounds':raise ValueError('Unresolved RPS linkage source bindings.')
        snapshots={r['message_snapshot_binding']['entry_offset']:r['sampled_message_fields']['type'] for r in rounds['rounds']}
        snapshot_report=companion_snapshots(instructions,snapshot=(reviews,preservation),root=root)
        if snapshot_report['status']!='conditional_snapshots':raise ValueError('Unresolved companion status producer.')
        snapshot_status={item['entry_offset']:item['status_variable'] for item in snapshot_report['snapshots']}
        episodes=[]
        for row in instructions:
            if row['name']!='SCRIPT_OP_ENTER_FRAME' or [o['value'] for o in row['operands']]!=[0,0,1]:continue
            try:g=_episode(instructions,row['offset'],NATIVE_BINDINGS)
            except (ValueError,KeyError,TypeError,IndexError):continue
            if g['sha256']!=EPISODE_SHA256:continue
            target=next((t for t,role in g['external'].items() if role==2),None)
            category=next((slot for slot,role in g['roots'].items() if role==0),None)
            if snapshots.get(target)!=[0,category]:continue
            mask_target=next((t for t,role in g['external'].items() if role==3),None)
            try:mask_binding=_mask_helper(instructions,mask_target,snapshot_status[target])
            except (ValueError,KeyError,TypeError,IndexError):continue
            episodes.append(_describe_episode(g,action_names,target,category,mask_binding))
        workflows,producers,dispatches=_observations(instructions,{r['entry'] for r in rounds['rounds']},{e['entry'] for e in episodes})
        linked={d['target_offset'] for d in dispatches if d['role']=='source_bound_player_relative_episode'}
        episodes=[e for e in episodes if e['entry'] in linked]
        return dict(status='conditional_workflows',workflows=workflows,producers=producers,dispatches=dispatches,episodes=episodes,scope=SCOPE,
                    source_bindings=dict(native_reviews=NATIVE_BINDINGS,vm_unit_sha256=VM_UNIT_SHA256,
                                         action_header_sha256=ACTION_HEADER_SHA256,rps_source_bindings=rounds['source_bindings']))
    except (ValueError,OSError,KeyError,TypeError,IndexError) as error:return dict(unresolved,reason=str(error))


def _describe_episode(graph,action_names,snapshot_target,category,mask_binding):
    functions=graph['functions'];rows=[r for f in functions for r in f['rows']]
    return dict(entry=functions[0]['entry'],normalized_sha256=graph['sha256'],
                functions=[dict(entry=f['entry'],span_bytes=f['end']-f['entry'],reachable_bytes=sum(r['size'] for r in f['rows']),instruction_count=len(f['rows'])) for f in functions],
                initial_requests={'choose_escape_from_player':True,'default_action_mode':7,'action_programs':[17,0,264],
                                  'named_programs':{str(n):action_names.get(n) for n in (17,264)},'tracked_sound':34,'timed_action_duration':5.0},
                sampled_message_type_variable=[0,category],message_snapshot_entry=snapshot_target,status_mask_binding=mask_binding,
                message_paths={'guard':'Source-bound ordinary-message status mask2 must be nonzero; stored type is then tested.',
                               'nonzero_type':'Request program219, internal helper1 requests program165 and invokes opaque cleanup.',
                               'zero_type':'Enqueue one opaque boxed string index; continue timer checks.'},
                timer_paths={'strictly_above_10':'Request messenger contact mode2.',
                             'strictly_above_60_or_enabled_elapsed_limit_flags_32768_65536':'Internal helper2 requests defaultmode7/HUDguide0/contactmode1/program48 then opaque cleanup.'},
                opaque_text_indices=list(graph['strings']),
                external_calls=[dict(instruction_offset=r['offset'],target_offset=r['operands'][0]['value'],opaque_identity=graph['external'][r['operands'][0]['value']]) for r in rows if r['name']=='SCRIPT_OP_CALL' and r['operands'][0]['value'] in graph['external']],
                completion_proven=False,external_calls_preserve_context_proven=False,gaps_interpreted=False)
