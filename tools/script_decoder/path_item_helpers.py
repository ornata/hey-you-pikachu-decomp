"""Recognize bounded path-point item-query interfaces in authenticated VM code.

Recognition describes authored inputs/results under explicit native and physical
frame prerequisites. It infers no valid point, selected identity or live handle.
"""

import hashlib
import json

from .callee_contexts import context_snapshot
from .formats import integer_operand
from .stack import incoming_edges

# Independently reviewed full expanded native contracts, not all-active purity.
NATIVE_BINDINGS = {
    'actorScriptGetAreaPathPointCoordinate': dict(arguments=3, review_sha256='86b5998903dea69b9c71670ac35eef019d4ce209d0e670e6984b5e644644cad5'),
    'actorScriptFindItemNearPosition': dict(arguments=8, review_sha256='54b675af968fd60244693eb45d2726205d1010764cecf9eeb04152b800b07b09'),
}


def _digest(value):
    return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()


def _shape():
    rows=[('ENTER_FRAME',[1,4,1],0)]
    for axis in range(3):
        rows.extend([('PUSH_LITERAL',[0],0),('LOAD_VARIABLE',[1,0],0),
                     ('PUSH_LITERAL',[axis],0),('CALL_NATIVE',None,2),
                     ('STORE_VARIABLE',[1,axis+1],0)])
    rows.extend([('LOAD_VARIABLE',[1,axis+1],0)for axis in range(3)])
    rows.extend([('PUSH_LITERAL',[value],0)for value in(165,165,100,0,0)])
    rows.extend([('CALL_NATIVE',None,2),('RETURN',[],0)])
    return rows


def path_item_helper_roles(instructions, *, snapshot=None):
    """Return every exact linear path-zero nearest-world-item helper interface.

    Inputs must be canonical authenticated decoded instructions. The ephemeral
    snapshot is current source evidence, invalid after any source mutation.
    """
    if snapshot is None:snapshot=context_snapshot()
    reviews,preservation=snapshot
    vm=preservation.get('context_vm_contract',{})
    if preservation.get('stale_vm_functions')or not vm.get('current'):return []
    names=('actorScriptGetAreaPathPointCoordinate','actorScriptFindItemNearPosition')
    if any(name not in NATIVE_BINDINGS for name in names):return []
    for name in names:
        review=reviews.get('active',{}).get(name);binding=NATIVE_BINDINGS[name]
        if review is None or _digest(review)!=binding['review_sha256'] or type(review.get('arguments'))is not int or review['arguments']!=binding['arguments']:return []
    by={r['offset']:r for r in instructions}
    if len(by)!=len(instructions):return []
    incoming=incoming_edges(instructions);shape=_shape();results=[]
    for entry in instructions:
        if entry['name']!='SCRIPT_OP_ENTER_FRAME':continue
        try:
            current=entry;rows=[]
            for index,(name,values,tag)in enumerate(shape):
                if current['name']!='SCRIPT_OP_'+name:raise ValueError('different instruction')
                operands=current.get('operands',[])
                expected_count=1 if values is None else len(values)
                if len(operands)!=expected_count or current['size']!=1+5*expected_count:raise ValueError('different instruction extent')
                if any(type(v.get('tag'))is not int or v['tag']!=tag for v in operands):raise ValueError('different operand boxing')
                if any(type(integer_operand(v))is not int for v in operands):raise ValueError('operand integer payload is malformed')
                if values is not None and [integer_operand(v)for v in operands]!=values:raise ValueError('different constants or frame coordinates')
                if name=='CALL_NATIVE':
                    expected=names[0]if index<16 else names[1]
                    if current.get('native',{}).get('functions')!=[expected]:raise ValueError('ambiguous or different native')
                rows.append(current)
                if index==len(shape)-1:
                    if current.get('successors')!=[]:raise ValueError('RETURN has unexpected continuation')
                    continue
                next_offset=current['offset']+current['size']
                kind='resume_or_fallthrough'if name=='CALL_NATIVE'else'fallthrough'
                if current.get('successors')!=[dict(kind=kind,offset=next_offset)]or incoming.get(next_offset,set())!={current['offset']}:raise ValueError('prefix is not exclusive adjacent linear code')
                current=by[next_offset]
            if rows[-1]['offset']+rows[-1]['size']-entry['offset']!=206:raise ValueError('different definition extent')
        except(ValueError,KeyError,TypeError,IndexError):continue
        results.append(dict(entry_offset=entry['offset'],definition_bytes=206,instruction_count=26,
            role='nearest_world_item_handle_at_area_path_zero_point',
            parameter=dict(position=0,role='path_zero_point_index',boxed_value='caller incoming operand, aliased by ENTER_FRAME',conversion='integer at each coordinate native'),
            result=dict(tag=0,role='nearest_qualifying_world_item_handle_or_zero',lifetime='Borrowed handle at lookup only; no future survival or ownership inference.'),
            frame_contract=dict(arguments=1,locals=4,frame_index=1,local_count_includes_arguments=True,additional_local_cells=3),
            query=dict(path=0,axes=[0,1,2],minimum_kind_selector=165,maximum_kind_selector=165,effective_kind_range=[1,164],maximum_approximate_distance=100,distance_bound='inclusive',ownership_state=0,weighted=0),
            instruction_evidence=[dict(offset=r['offset'],size=r['size'],name=r['name'],operands=r['operands'],native=r.get('native'))for r in rows],
            native_review_sha256={name:NATIVE_BINDINGS[name]['review_sha256']for name in names},
            vm_implementation_binding=vm,
            preconditions=[
                'Canonical authenticated full definition and current exact native/VM source bindings; every original native and SDK prerequisite applies.',
                'Current-area path zero and converted point index must be valid, with nonnegative index below count. The helper proves neither: retail coordinate reads lack a lower-bound check and high indices return -65536 without rejection here.',
                'Valid balanced ENTER/RETURN above the protected caller/root floor; incoming argument cells, own locals and saved metadata are allocated/nonwrapping and physically disjoint from protected storage and all native scratch/query/registry ranges.',
                'The coordinate native preserves this active formal and already stored XYZ cells under its exact reviewed no-alias prerequisites; no asynchronous mutation or VM/owner/area/frame rebinding occurs.',
                'Item registry/search state is valid and serialized; query filters/approximate distance and borrowed handle lifetime follow the reviewed search contract.'],
            scope='Exact linear interface only; no callsite argument substitution, runtime coordinate, point validation, successful search, selected identity, future handle lifetime or tracking outcome.'))
    return results
