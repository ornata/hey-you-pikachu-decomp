"""Bounded authored interfaces for fishing record classification scripts.

Species and threshold table contents remain external. Indexed source roles are
conditional observations, never allocation, initialization or caller-floor proof.
"""

import hashlib
import json

from .callee_contexts import context_snapshot
from .formats import integer_operand
from .stack import incoming_edges, recover_arguments

NATIVE_BINDINGS = {
    'actorScriptPassThroughInteger': dict(arguments=1, review_sha256='7ce5019c6dafb96114d27892f09383b0f4af354c265b840c4a24c41823eb16d8'),
    'actorScriptGetProgressValue': dict(arguments=1, review_sha256='3d51ae69bf9ab291b45dfe4f839926f44f00eaa87325b8c5c55a0e8bf01c48df'),
    'actorScriptSetProgressValue': dict(arguments=2, review_sha256='0139883166d83c2855045ef88d951316e7006dc43eea5168cb8dd228b7d2d6ff'),
    'actorScriptBuildSharedTextAlternate': dict(arguments='inline_count', review_sha256='d755b5eed75d2724c91e8f8fa183d6671d7b87480c45271192cef886a8baa803'),
}


def _digest(value):
    return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()


def _template():
    """Describe the independently reviewed control and boxed-operand interface."""
    rows=[]
    def emit(name,*args,label=None):rows.append(dict(name=name,args=list(args),label=label))
    def load(slot):emit('LOAD_VARIABLE',1,slot)
    def store(slot):emit('STORE_VARIABLE',1,slot)
    def push(value):emit('PUSH_LITERAL',value)
    def native(name,count=None):emit('CALL_NATIVE',name,*([]if count is None else[count]))
    emit('ENTER_FRAME',2,6,1);push(0);store(2);load(1)
    native('actorScriptPassThroughInteger');store(3)
    emit('LOAD_VARIABLE',1,2,label='loop');push(15);emit('LESS_THAN');emit('JUMP_IF_ZERO','not_found')
    load(2);emit('LOAD_ELEMENT',0,'species');load(0);emit('EQUAL');emit('JUMP_IF_ZERO','next_species')
    push(138);load(2);emit('ADD');store(4);load(4);native('actorScriptGetProgressValue');store(5)
    load(3);load(5);emit('GREATER_OR_EQUAL');emit('JUMP_IF_ZERO','next_species')
    load(4);load(3);native('actorScriptSetProgressValue');emit('DISCARD')
    push(26);rows[-1]['literal_tag']=3;load(1);push(27);rows[-1]['literal_tag']=3;load(2);emit('LOAD_ELEMENT',0,'threshold')
    native('actorScriptBuildSharedTextAlternate',4);emit('DISCARD')
    load(1);load(2);emit('LOAD_ELEMENT',0,'threshold');emit('GREATER_OR_EQUAL');emit('JUMP_IF_ZERO','below_threshold')
    push(100);load(2);emit('ADD');emit('RETURN');rows[-1]['unreachable_gap_after']=6
    emit('LOAD_VARIABLE',1,5,label='below_threshold');push(0);emit('EQUAL');emit('JUMP_IF_ZERO','existing_record')
    push(2);emit('NEGATE');emit('RETURN');rows[-1]['unreachable_gap_after']=6
    emit('LOAD_VARIABLE',1,2,label='existing_record');emit('RETURN')
    emit('LOAD_VARIABLE',1,2,label='next_species');push(1);emit('ADD');store(2);emit('JUMP','loop')
    emit('PUSH_LITERAL',1,label='not_found');emit('NEGATE');emit('RETURN')
    offset=0;labels={}
    for row in rows:
        row['relative_offset']=offset
        if row['label']:labels[row['label']]=offset
        row['size']=1+5*len(row['args']);offset+=row['size']+row.get('unreachable_gap_after',0)
    assert len(rows)==63 and offset==455
    return rows,labels


def fishing_record_helper_roles(instructions, *, snapshot=None):
    """Recognize complete linear/branch classifier definitions with fresh pins."""
    if snapshot is None:snapshot=context_snapshot()
    reviews,preservation=snapshot;vm=preservation.get('context_vm_contract',{})
    if preservation.get('stale_vm_functions')or not vm.get('current'):return []
    required={'actorScriptPassThroughInteger','actorScriptGetProgressValue','actorScriptSetProgressValue','actorScriptBuildSharedTextAlternate'}
    if set(NATIVE_BINDINGS)!=required:return []
    for name,binding in NATIVE_BINDINGS.items():
        review=reviews.get('active',{}).get(name)
        if review is None or _digest(review)!=binding['review_sha256'] or review.get('arguments')!=binding['arguments']:return []
    by={row['offset']:row for row in instructions}
    if len(by)!=len(instructions):return []
    incoming=incoming_edges(instructions);template,labels=_template();results=[]
    for entry in instructions:
        if entry['name']!='SCRIPT_OP_ENTER_FRAME':continue
        start=entry['offset'];tables={};selected=[];expected_incoming={}
        try:
            for spec in template:
                offset=start+spec['relative_offset'];row=by[offset];name=spec['name']
                if row['name']!='SCRIPT_OP_'+name or row['size']!=spec['size']or len(row['operands'])!=len(spec['args']):raise ValueError('different instruction shape')
                for index,(operand,want)in enumerate(zip(row['operands'],spec['args'])):
                    tag=2 if name in('JUMP','JUMP_IF_ZERO')or(name=='CALL_NATIVE'and index==0)else spec.get('literal_tag',0)
                    decoded_value=operand.get('value')if tag==3 else integer_operand(operand)
                    if type(operand.get('tag'))is not int or operand['tag']!=tag or type(decoded_value)is not int:raise ValueError('different boxed operand')
                    value=decoded_value
                    if name=='CALL_NATIVE'and index==0:
                        if row.get('native',{}).get('functions')!=[want]:raise ValueError('different or ambiguous native')
                    elif want in('species','threshold'):
                        if not 0<=value<=2147483633:raise ValueError('table coordinate arithmetic out of range')
                        if want in tables and tables[want]!=value:raise ValueError('table coordinates disagree')
                        tables[want]=value
                    elif isinstance(want,str):
                        if value!=start+labels[want]:raise ValueError('different branch destination')
                    elif value!=want:raise ValueError('different literal or frame coordinate')
                next_offset=offset+row['size']
                if name=='RETURN':successors=[]
                elif name=='JUMP':successors=[dict(kind='branch',offset=start+labels[spec['args'][0]])]
                elif name=='JUMP_IF_ZERO':successors=[dict(kind='fallthrough',offset=next_offset),dict(kind='branch',offset=start+labels[spec['args'][0]])]
                else:successors=[dict(kind='resume_or_fallthrough'if name=='CALL_NATIVE'else'fallthrough',offset=next_offset)]
                if sorted((e['kind'],e['offset'])for e in row['successors'])!=sorted((e['kind'],e['offset'])for e in successors):raise ValueError('different complete CFG')
                for edge in successors:expected_incoming.setdefault(edge['offset'],set()).add(offset)
                selected.append(row)
            if any(incoming.get(offset,set())!=pred for offset,pred in expected_incoming.items()if offset!=start):raise ValueError('competing helper entry')
            if abs(tables['species']-tables['threshold'])<15:raise ValueError('species and threshold table coordinate spans overlap')
        except(ValueError,KeyError,IndexError,TypeError):continue
        results.append(dict(entry_offset=start,definition_bytes=455,instruction_count=63,unreachable_gap_intervals=[dict(relative_start=356,bytes=6),dict(relative_start=394,bytes=6)],
            role='fishing_species_record_classifier',frame_contract=dict(arguments=2,locals=6,frame_index=1,local_count_includes_arguments=True,additional_local_cells=4),
            parameters=[dict(position=0,role='species_kind'),dict(position=1,role='measurement_box',conversion='integer copy for persistent record; original box for threshold comparison/text')],
            tables=dict(species=dict(frame=0,base_slot=tables['species'],accessed_indices=[0,14]),threshold=dict(frame=0,base_slot=tables['threshold'],accessed_indices=[0,14]),contents='Not recovered by this shape; no threshold payload or species order embedded.'),
            persistent_fields=dict(base=138,count=15,role='existing fishing record length fields; caller measurement unit not independently proved here'),
            possible_results=[dict(expression='100 + species_index',condition='species matches; converted measurement >= previous record; original boxed measurement passes VM GREATER_OR_EQUAL threshold test'),dict(expression='species_index',condition='species matches and converted measurement >= previous record; original VM threshold test false; previous record nonzero'),dict(expression='-2',condition='species matches and converted measurement >= previous record; original VM threshold test false; previous record zero'),dict(expression='-1',condition='scan ends without qualifying species/record')],
            effects=['Potential request to write converted measurement to field138+species_index; actual stored value clamps to field limits, and the returned clamped value is discarded. Equal measurements qualify; no strict new-record or input-equals-stored claim.','Potential alternate shared text build from literal keys26,27 plus original measurement and species threshold.'],
            instruction_evidence=[dict(offset=r['offset'],name=r['name'],size=r['size'],operands=r['operands'],native=r.get('native'))for r in selected],
            native_review_sha256={name:b['review_sha256']for name,b in NATIVE_BINDINGS.items()},vm_implementation_binding=vm,
            preconditions=['Canonical authenticated entire definition with current full VM and exact four native contracts; every original native/text/SDK precondition applies.',
                'Valid allocated/nonwrapping stable root tables and own frame cells; source and protected caller/root cells are physically disjoint from producer operand scratch, own locals/saved metadata, progress and text/native workspaces.',
                'Runtime boxed types, finite numeric conversions/comparisons/arithmetic and index values are valid; no actual measurement/unit/range or table contents are inferred.',
                'No asynchronous mutation or VM/owner/root/frame/table rebinding across the helper; root-restart floor/capacity and indexed caller initialization/bounds remain separate unresolved obligations.'],
            scope='Authored complete CFG interface and conditional effects only; no path feasibility, successful record update, actual output, indexed-source provenance or whole-root caller-floor claim.'))
    # Indexed caller observations remain separate from admitted source provenance.
    indices={row['offset']:index for index,row in enumerate(instructions)}
    for helper in results:
        sites=[]
        for call in instructions:
            if call['name']!='SCRIPT_OP_CALL' or len(call['operands'])!=1 or integer_operand(call['operands'][0])!=helper['entry_offset']:continue
            i=indices[call['offset']]
            if i<5:continue
            chain=instructions[i-5:i+1]
            if [r['name']for r in chain]!=['SCRIPT_OP_LOAD_VARIABLE','SCRIPT_OP_LOAD_ELEMENT','SCRIPT_OP_STORE_VARIABLE','SCRIPT_OP_LOAD_VARIABLE','SCRIPT_OP_LOAD_VARIABLE','SCRIPT_OP_CALL']:continue
            try:
                for before,after in zip(chain,chain[1:]):
                    if before['offset']+before['size']!=after['offset']or before['successors']!=[dict(kind='fallthrough',offset=after['offset'])]or incoming.get(after['offset'],set())!={before['offset']}:raise ValueError('source site has competing control')
                values=[[integer_operand(v)for v in r['operands']]for r in chain[:5]]
                if any(len(v)!=2 or any(type(x)is not int for x in v)for v in values):raise ValueError('unbounded source site coordinates')
                if any(not -1<=frame<32 or not 0<=slot<=2147483647 for frame,slot in values):raise ValueError('source coordinate is outside admitted domain')
                if values[0][0]!=0 or values[1][0]!=0 or any(values[i][0]!=1 for i in (2,3,4)):raise ValueError('source site does not use root tables and caller frame one')
                if values[2]!=values[4]:raise ValueError('STORE is not the captured measurement source')
                if sorted((e['kind'],e['offset'])for e in call['successors'])!=sorted([('call',helper['entry_offset']),('return_continuation',call['offset']+call['size'])]):raise ValueError('CALL edges differ')
                sites.append(dict(call_offset=call['offset'],helper_entry=helper['entry_offset'],index_load_offset=chain[0]['offset'],element_load_offset=chain[1]['offset'],store_offset=chain[2]['offset'],capture_load_offset=chain[4]['offset'],
                    index_source=dict(frame=values[0][0],slot=values[0][1]),element_source=dict(frame=values[1][0],base_slot=values[1][1]),captured_source=dict(frame=values[2][0],slot=values[2][1]),
                    role='indexed_measurement_box_for_fishing_record_classifier',status='conditional_role_observation_not_admitted_provenance',
                    instruction_evidence=[dict(offset=r['offset'],name=r['name'],operands=r['operands'])for r in chain],
                    scope='Producer-time indexed read role only. Runtime index/type, initialization, allocation/capacity, physical aliasing, asynchronous preservation and root-restart caller floor remain unproved; no array extent or value is inferred.'))
            except(ValueError,KeyError,TypeError,IndexError):continue
        helper['indexed_source_role_sites']=sites
    return results
