"""Bounded producer-time provenance for captured VM caller source cells.

Metadata preserves capture expressions and describes only exclusive adjacent
STORE-to-LOAD paths. It establishes no native success or later runtime value.
"""

import hashlib
import json
import math

from .callee_contexts import context_snapshot
from .formats import integer_operand
from .stack import incoming_edges, recover_arguments
from .path_item_helpers import path_item_helper_roles

# Exact expanded reviews, approved independently for their boxed integer results.
PRODUCER_BINDINGS = {'actorScriptFindObjectInFacingCone': {'arguments': 4,
                                       'review_sha256': '302d24fae5d2e9f8ca5dcc8ef4893336810b12b9948ede79dfa394dcf449a236',
                                       'result_role': 'selected_runtime_object_handle',
                                       'scope': 'No claim of surviving handle, ownership, '
                                                'caller range validity or successful future '
                                                'access.'},
 'actorScriptCreateItem': {'arguments': 6,
                           'review_sha256': 'be5fec35452bd2c797d1d54c2e7db1f55ea56a8f46cacf1c0a9a166bd553067e',
                           'result_role': 'created_item_handle',
                           'scope': 'Requires successful allocation and complete reviewed '
                                    'retail initializer/resource/transport domains. No '
                                    'failure-safe result or arbitrary kind support.'},
 'actorScriptRandomBelow': {'arguments': 1,
                            'review_sha256': '45206f67a57091cbca5b17b2e640a6618c518b8c7461fc7899614bf7630e4a8f',
                            'result_role': 'bounded_random_integer',
                            'scope': 'Positive converted bound permits range0..bound-1; '
                                     'require a separately established literal bound before '
                                     'publishing a numerical interval. No uniformity, seed, '
                                     'repeatability or actual result inference.'}}

PRODUCER_BINDINGS['actorScriptPassThroughInteger'] = dict(
    arguments=1, review_sha256='7ce5019c6dafb96114d27892f09383b0f4af354c265b840c4a24c41823eb16d8',
    result_role='integer_converted_numeric_value',
    scope='Numeric operand tags, finite signed32 conversion and defined producer arithmetic remain prerequisites; no actual converted value, division result, range or point validity is inferred.')


def _digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def annotate_caller_source_provenance(instructions, contexts, *, snapshot=None, limit=128):
    """Attach separate provenance without changing caller captures or arguments.

    Instructions and contexts must originate in the same canonical authenticated
    decode. A snapshot is freshly source-validated and valid only until mutation.
    """
    if type(limit) is not int or limit <= 0:
        raise ValueError('caller source work budget must be a positive integer')
    for context in contexts:
        context.pop('caller_source_provenance', None)
    if snapshot is None:
        snapshot = context_snapshot()
    reviews, preservation = snapshot
    vm = preservation.get('context_vm_contract', {})
    if preservation.get('stale_vm_functions') or not vm.get('current'):
        return
    if len({row.get('offset') for row in instructions}) != len(instructions):
        return
    by = {row['offset']:row for row in instructions}
    indices = {row['offset']:i for i,row in enumerate(instructions)}
    incoming = incoming_edges(instructions)
    frames = {off:row for off,row in by.items() if row['name']=='SCRIPT_OP_ENTER_FRAME'}
    helper_roles = {row['entry_offset']:row for row in path_item_helper_roles(instructions,snapshot=snapshot)}

    def coords(row):
        if len(row.get('operands', [])) != 2:
            raise ValueError('source reference arity is invalid')
        frame, slot = map(integer_operand, row['operands'])
        if type(frame) is not int or not -1 <= frame < 32 or type(slot) is not int or not 0 <= slot <= 2147483647:
            raise ValueError('source reference is outside the admitted coordinate domain')
        return frame, slot

    def previous(offset, *, producer=False):
        predecessors = incoming.get(offset, set())
        if len(predecessors) != 1:
            raise ValueError('source path has competing or absent predecessors')
        row = by[next(iter(predecessors))]
        if row['name']=='SCRIPT_OP_CALL':
            if not (producer and row['offset']+row['size']==offset and
                    len(row.get('operands',[]))==1 and
                    sorted((e['kind'],e['offset'])for e in row.get('successors',[]))==
                    sorted([('call',integer_operand(row['operands'][0])),('return_continuation',offset)])):
                raise ValueError('script producer lacks exact CALL/return-continuation edges')
        elif row['offset'] + row['size'] != offset or row.get('successors') != [dict(kind='fallthrough',offset=offset)]:
            # A native may resume immediately before the STORE.
            if not (row['name']=='SCRIPT_OP_CALL_NATIVE' and row['offset']+row['size']==offset and
                    row.get('successors')==[dict(kind='resume_or_fallthrough',offset=offset)]):
                raise ValueError('source path is not exclusive adjacent continuation')
        return row

    def prove(context, capture):
        origin = capture['caller_origin']
        if origin.get('epoch') != 'caller_before_CALL' or origin['call_offset'] != context['caller_offset']:
            raise ValueError('capture caller epoch disagrees')
        call = by[origin['call_offset']]
        if call['name']!='SCRIPT_OP_CALL' or len(call['operands'])!=1 or integer_operand(call['operands'][0])!=context['callee_entry']:
            raise ValueError('capture does not identify this script CALL')
        if sorted((e['kind'],e['offset']) for e in call['successors']) != sorted([('call',context['callee_entry']),('return_continuation',call['offset']+call['size'])]):
            raise ValueError('capture CALL edges disagree')
        load = by[origin['load_instruction_offset']]
        if load['name']!='SCRIPT_OP_LOAD_VARIABLE':
            raise ValueError('capture is not a direct LOAD')
        source = coords(load)
        boxes=[box for box in context.get('caller_actual_boxing',[])
               if box.get('caller_origin')==origin]
        if len(boxes)!=1 or boxes[0].get('raw_load_operands')!=load['operands']:
            raise ValueError('captured raw LOAD operand tags or payloads disagree')
        if capture['source_reference'] != dict(kind='variable',frame=source[0],slot=source[1]):
            raise ValueError('capture source coordinates disagree')
        current=load['offset'];path=[];seen=set()
        for _ in range(limit):
            row=previous(current)
            if row['offset'] in seen:
                raise ValueError('source path loops')
            seen.add(row['offset']);path.append(row['offset']);name=row['name']
            if name=='SCRIPT_OP_STORE_VARIABLE':
                if coords(row)!=source:
                    raise ValueError('another store may alias the source')
                store=row;break
            if name=='SCRIPT_OP_LOAD_VARIABLE':coords(row)
            elif name=='SCRIPT_OP_PUSH_LITERAL':
                if len(row['operands'])!=1:raise ValueError('literal arity is invalid')
            else:raise ValueError('source path crosses an unreviewed effect or binding')
            current=row['offset']
        else:raise ValueError('source path work budget exhausted')
        producer=previous(store['offset'],producer=True)
        binding=None
        if producer['name']=='SCRIPT_OP_PUSH_LITERAL':
            if len(producer['operands'])!=1 or type(producer['operands'][0].get('tag')) is not int or producer['operands'][0]['tag'] not in (0,1,2):
                raise ValueError('producer is not a supported boxed literal')
            operand=producer['operands'][0]
            number=operand.get('value')
            if operand['tag'] in (0,2):
                if type(number) is not int or not -2147483648<=number<=2147483647:
                    raise ValueError('literal integer payload is invalid')
            elif type(number) not in (int,float) or not math.isfinite(number) or abs(number)>3.4028234663852886e38:
                raise ValueError('literal float payload is invalid')
            result=dict(kind='boxed_literal',raw_operand=producer['operands'][0],output_tag=producer['operands'][0]['tag'])
        elif producer['name']=='SCRIPT_OP_CALL_NATIVE':
            names=producer.get('native',{}).get('functions',[])
            if len(names)!=1 or names[0] not in PRODUCER_BINDINGS:
                raise ValueError('producer native result lacks an exact reviewed binding')
            name=names[0];binding=PRODUCER_BINDINGS[name];review=reviews.get('active',{}).get(name)
            if review is None or _digest(review)!=binding['review_sha256'] or review.get('arguments')!=binding['arguments']:
                raise ValueError('producer native review is stale or has different arity')
            arguments=recover_arguments(instructions,indices[producer['offset']],binding['arguments'],incoming=incoming,allow_float=True,allow_symbolic=True,frame_entries=frames)
            result=dict(kind='reviewed_native_boxed_result',function=name,output_tag=0,
                        result_role=binding['result_role'],arguments=arguments,review_sha256=binding['review_sha256'],
                        input_expression_epoch=dict(instruction_offset=producer['offset'],phase='before_producer_native'),
                        input_scope='Syntactic producer inputs in push order; variable/script-return expressions are not re-evaluated or assigned runtime values.',
                        result_scope=binding.get('scope','Original reviewed native result under all prerequisites.'))
        elif producer['name']=='SCRIPT_OP_CALL':
            target=integer_operand(producer['operands'][0])
            helper=helper_roles.get(target)
            if helper is None:
                raise ValueError('script producer has no freshly recognized role contract')
            arguments=recover_arguments(instructions,indices[producer['offset']],1,incoming=incoming,allow_float=True,allow_symbolic=True,frame_entries=frames)
            result=dict(kind='reviewed_script_helper_boxed_result',target_offset=target,output_tag=0,
                        result_role=helper['result']['role'],helper_role_contract=helper,
                        arguments=arguments,input_expression_epoch=dict(instruction_offset=producer['offset'],phase='before_producer_CALL'),
                        input_scope='Caller expressions captured before helper frame rebinding; no point-index value, conversion validity or runtime coordinate is inferred.')
        else:raise ValueError('producer is neither a literal nor an admitted native/script result')
        return dict(call_offset=call['offset'],load_offset=load['offset'],source_reference=capture['source_reference'],
                    caller_origin=origin,store_offset=store['offset'],producer_offset=producer['offset'],
                    producer_result=result,producer_instruction=dict(name=producer['name'],operands=producer['operands'],native=producer.get('native')),store_to_load_instruction_offsets=list(reversed(path))+[load['offset']],
                    vm_implementation_binding=vm,
                    preconditions=[
                        'Canonical authenticated instructions and current source reviews identify this exact producer, STORE, caller-time LOAD and CALL; no asynchronous mutation occurs on this interval.',
                        'The source cell has a valid stable caller root/local/global binding; complete five-byte source, destination and operand cells remain allocated and nonwrapping.',
                        'The source cell is physically disjoint from all intervening operand scratch writes and other live storage; reused callee arguments/locals/saved frame metadata are disjoint from protected caller/root storage.',
                        'The original producer native full operand/object/resource/allocation/SDK prerequisites apply; result roles do not establish successful search, object identity or creation safety.'],
                    scope='Only boxed producer-to-STORE-to-caller-LOAD provenance. Captures and native inputs remain unchanged; no consumer-time re-read, global substitution or later value/outcome claim.')

    for context in contexts:
        if context.get('status')!='conditional_forwarded_actual_context' or context.get('vm_implementation_binding')!=vm:
            continue
        consumer=reviews.get('active',{}).get(context.get('consumer_function'))
        if consumer is None or _digest(consumer)!=context.get('consumer_review_sha256'):
            continue
        results=[]
        for capture in context.get('caller_actuals',[]):
            if capture.get('kind')!='caller_box_capture':continue
            try:results.append(prove(context,capture))
            except (ValueError,KeyError,IndexError,TypeError,OverflowError):continue
        if results:context['caller_source_provenance']=results
