"""Authenticated, bounded static links for the shared item-response dispatcher.

Conditional paths are inventories, not execution, emotion or portable-C proofs.
Response categories, reaction codes, cost selectors and timed IDs stay distinct.
"""

import hashlib
import json
import math
import re
import struct

from .actions import CONTRACTS, action_calls
from .formats import integer_operand
from .items import reaction_code_names
from .functions import function_summary
from .stack import incoming_edges, recover_arguments
from .local_flow import local_integer_domain, local_preservation_contracts
from .context import ROOT
from .native_frames import fingerprint, native_frame_reviews
from tools.source_quality import code_only

CONFIG = 0x8014FED4
PROGRAM = 0x8013C8B8
PROGRAM_SIZE = 79388
PROGRAM_SHA256 = '2383c7bb40fdfe40c441188581259e1f69b59940e36f3a3dd1043f1eafb9c12a'
SCOPE = ('Static potentially reachable paths with returning call continuations; '
         'conditional calls need not execute. No interprocedural substitution, '
         'termination, runtime reachability or reaction-emotion interpretation.')
# These deferred commands consume these operands when their handler initializes.
EXTRA_ARGUMENTS = {
    'actorScriptStartEatingHeldItem': ['script_id'],
    'actorScriptStartCostedAction': ['cost_selector', 'repetitions'],
    'actorScriptCreateAttachedEffect': ['effect_kind', 'texture_track'],
    'actorScriptReleaseHeldItem': [],
    'actorScriptAddProfileCounter': ['profile_key', 'adjustment'],
}


COST_POLICY_ADDRESS = 0x8012A660
COST_POLICY_BYTES = 5 * 16
COST_POLICY_SHA256 = '73a3f9992ada4ad18dbb1b5e78948239aeaa21f4b8d62c450fa774da77538172'
COST_POLICY_ENUM = 'ActorCostedActionPolicy'
COST_POLICY_HEADER = 'include/actor_script.h'
_COST_POLICY_NAMES = {
    0: 'ACTOR_COSTED_ACTION_POLICY_THUNDER',
    1: 'ACTOR_COSTED_ACTION_POLICY_THUNDERBOLT',
    2: 'ACTOR_COSTED_ACTION_POLICY_THUNDERSHOCK',
    3: 'ACTOR_COSTED_ACTION_POLICY_THUNDER_SKIP_RESOURCE_CHECK',
    4: 'ACTOR_COSTED_ACTION_POLICY_THUNDERBOLT_SKIP_RESOURCE_CHECK',
}


def _cost_policy_enum(source):
    """Require one complete, explicitly numbered reviewed policy declaration."""
    code = code_only(source)
    starts = list(re.finditer(r'\benum\s+' + COST_POLICY_ENUM + r'\s*\{', code))
    if len(starts) != 1:
        raise ValueError('cost policy enum must have one unambiguous declaration')
    start = starts[0]
    end = code.find('}', start.end())
    if end < 0 or '{' in code[start.end():end]:
        raise ValueError('cost policy enum body is malformed')
    entries = code[start.end():end].split(',')
    if not entries[-1].strip():
        entries.pop()
    names, identifiers = {}, set()
    for entry in entries:
        match = re.fullmatch(r'\s*(\w+)\s*=\s*(0[xX][\da-fA-F]+|\d+)\s*', entry)
        if match is None:
            raise ValueError('cost policy enum requires explicit integer entries')
        name, number = match.groups()
        value = int(number, 16 if number.lower().startswith('0x') else 10)
        if name in identifiers or value in names:
            raise ValueError('cost policy enum contains duplicate entries')
        names[value] = name
        identifiers.add(name)
    if names != _COST_POLICY_NAMES:
        raise ValueError('cost policy enum differs from the five reviewed selectors')
    return names, code[start.start():end + 1]


def cost_policy_names(source):
    """Read the exact reviewed enum, excluding comments and other declarations."""
    return _cost_policy_enum(source)[0]


def cost_policy_records(raw):
    """Describe the five neighbor-bounded policies without choosing an outcome."""
    if len(raw) != COST_POLICY_BYTES:
        raise ValueError('cost policy view must contain five 16-byte records')
    records = []
    for selector in range(5):
        program, insufficient, cost, requires = struct.unpack_from('>iifi', raw, selector * 16)
        if not math.isfinite(cost):
            raise ValueError('cost policy repetition cost is not finite')
        records.append(dict(cost_selector=selector, program_id=program,
                            insufficient_program_id=insufficient, repetition_cost=cost,
                            requires_sufficient_resource=requires,
                            referenced_timed_program_ids=sorted({program, insufficient}),
                            possible_timed_program_ids=sorted({program, insufficient} if requires else {program}),
                            outcome_condition='insufficient iff resource check is enabled and actionResource < repetitionCost * repetitions; otherwise accepted',
                            scope='Policy record, not a chosen action or resource sufficiency assertion. Disabled checks still charge resource.'))
    return records


def link_costed_request(native, policies):
    """Resolve only bounded constant selectors; retain repetitions as expressions."""
    selector = native.get('arguments', {}).get('cost_selector', {})
    value = selector.get('value')
    if selector.get('kind') not in ('integer', 'float') or not isinstance(value, (int, float)) or not math.isfinite(value) or not -2147483648 <= value < 2147483648:
        return dict(status='unresolved_selector', scope='No policy or timed program is inferred from a dynamic selector.')
    value = int(value)
    if not 0 <= value < len(policies):
        return dict(status='outside_reviewed_selector_view', cost_selector=value,
                    scope='Outside the five retained records; original code has no established bounds guard.')
    return dict(status='bounded_constant_policy', policy=policies[value],
                repetitions_expression=native.get('arguments', {}).get('repetitions'),
                scope='Potential resource-dependent selection on handler initialization, not an execution or affordability claim.')


def cost_policy_catalog(image):
    """Authenticate the retained cost-policy view against the supplied ROM/ELF."""
    previous = set(image.allowed)
    try:
        image.allowed.add('.ovl')
        raw, location = image.read_address(COST_POLICY_ADDRESS, COST_POLICY_BYTES)
    finally:
        image.allowed = previous
    if hashlib.sha256(raw).hexdigest() != COST_POLICY_SHA256:
        raise ValueError('reviewed cost policy table authentication failed')
    names, declaration = _cost_policy_enum((ROOT / COST_POLICY_HEADER).read_text())
    records = cost_policy_records(raw)
    for record in records:
        record['policy_name'] = names[record['cost_selector']]
    return dict(address=COST_POLICY_ADDRESS, byte_count=COST_POLICY_BYTES,
                sha256=COST_POLICY_SHA256, location=location, records=records,
                name_source=dict(file=COST_POLICY_HEADER, enum=COST_POLICY_ENUM,
                                 sha256=fingerprint(declaration),
                                 scope='Exact source enum binding; original numeric selectors and timed IDs remain authoritative.'),
                scope='Five neighbor-delimited records, not a complete allocation or runtime bounds check.')


def costed_action_requests(instructions, policies, *, preserved_natives=()):
    """Inventory resource-dependent action requests separately from timed IDs."""
    incoming = incoming_edges(instructions)
    frames = {r['offset']: r for r in instructions if r['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    requests = []
    for index, call in enumerate(instructions):
        functions = call.get('native', {}).get('functions', [])
        if 'actorScriptStartCostedAction' not in functions:
            continue
        request = dict(instruction_offset=call['offset'], function='actorScriptStartCostedAction',
                       argument_domain='cost_selector_and_repetitions_not_timed_program_id',
                       consumption='deferred', scope='Static request; resource sufficiency and execution are not inferred.')
        try:
            if functions != ['actorScriptStartCostedAction']:
                raise ValueError('native costed-action binding is not unique')
            values = recover_arguments(instructions, index, 2, incoming=incoming,
                                       allow_float=True, allow_symbolic=True, frame_entries=frames)
            request.update(status='recovered_arguments',
                           arguments=dict(zip(('cost_selector', 'repetitions'), values)))
        except ValueError as error:
            request.update(status='unresolved_arguments', reason=str(error))
        request['cost_policy_link'] = link_costed_request(request, policies)
        selector = request.get('arguments', {}).get('cost_selector', {})
        if selector.get('kind') == 'variable':
            analysis = local_integer_domain(instructions, call['offset'], selector['frame'], selector['slot'],
                                            preserved_natives=preserved_natives)
            request['local_selector_analysis'] = analysis
            if analysis['status'] == 'bounded_local_integer_domain':
                request['conditional_local_policy_alternatives'] = [link_costed_request(
                    dict(arguments=dict(cost_selector=dict(kind='integer', value=value),
                                        repetitions=request['arguments']['repetitions'])), policies)
                    for value in analysis['values']]
        requests.append(request)
    return requests


def bounded_graph(instructions, entry, *, stop=(), limit=10000, arities=None):
    """Retain call edges and arguments without following them as local CFG edges."""
    by = {row['offset']: row for row in instructions}
    if len(by) != len(instructions):
        raise ValueError('duplicate decoded instruction offsets')
    indices = {row['offset']: i for i, row in enumerate(instructions)}
    frames = {off: row for off, row in by.items() if row['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    incoming = incoming_edges(instructions)
    seen, pending, calls, natives, diagnostics = set(), [entry], [], [], []
    arities = arities or {}

    def arguments(row, count):
        return recover_arguments(instructions, indices[row['offset']], count,
                                 incoming=incoming, allow_float=True,
                                 allow_symbolic=True, frame_entries=frames)

    while pending:
        off = pending.pop()
        if off in seen or off in stop:
            continue
        if len(seen) >= limit:
            diagnostics.append('instruction budget exhausted')
            break
        seen.add(off)
        row = by.get(off)
        if row is None:
            diagnostics.append(f'{off}: missing decoded instruction')
            continue
        name = row['name']
        if off != entry and name == 'SCRIPT_OP_ENTER_FRAME':
            diagnostics.append(f'{off}: enters another frame')
            continue
        if name in ('SCRIPT_OP_RETURN', 'SCRIPT_OP_FINISH'):
            continue
        successors = row.get('successors', [])
        if name == 'SCRIPT_OP_CALL':
            target = integer_operand(row['operands'][0])
            edge = dict(instruction_offset=off, target_offset=target)
            try:
                if target not in frames:
                    raise ValueError('callee is not a decoded frame entry')
                # Reuse the function-frame validation and bounded return inventory.
                summary = function_summary(instructions, target, limit=limit)
                edge['argument_count'] = summary['argument_count']
                edge['arguments'] = arguments(row, summary['argument_count'])
            except ValueError as error:
                edge['reason'] = str(error)
            calls.append(edge)
            successors = [e for e in successors if e['kind'] == 'return_continuation']
        if row.get('native'):
            functions = row['native'].get('functions', [])
            native = dict(instruction_offset=off, functions=functions)
            if len(functions) == 1:
                function = functions[0]
                names = CONTRACTS.get(function, {}).get('arguments', EXTRA_ARGUMENTS.get(function))
                count = len(names) if names is not None else arities.get(function)
                if count is not None:
                    try:
                        values = arguments(row, count)
                        native['arguments'] = dict(zip(names, values)) if names is not None else values
                    except ValueError as error:
                        native['reason'] = str(error)
                else:
                    native['reason'] = 'native argument contract not supplied'
                native['role'] = ({
                    'actorScriptStartEatingHeldItem': 'eating_request_and_later_consumption_attempt',
                    'actorScriptStartCostedAction': 'costed_action_request',
                    'actorScriptReleaseHeldItem': 'held_item_release_request',
                    'actorScriptAddProfileCounter': 'profile_counter_adjustment',
                    'actorScriptCreateAttachedEffect': 'attached_effect_request',
                    'actorScriptEnqueueMessage': 'message_request',
                    'actorScriptPlaySoundEffect': 'sound_effect_request',
                }).get(function, 'native_call')
                if function == 'actorScriptStartCostedAction':
                    native['argument_domain'] = 'costed_action_selector_and_repetition_count_not_timed_program_id'
                elif function == 'actorScriptStartEatingHeldItem':
                    native['argument_domain'] = 'timed_eating_program_id'
            else:
                native['reason'] = 'native binding is not unique'
            natives.append(native)
        if not successors:
            diagnostics.append(f'{off}: no understood continuation')
        pending.extend(e['offset'] for e in successors)
    timed = [row for row in action_calls(instructions) if row['instruction_offset'] in seen]
    return dict(entry_offset=entry, instruction_offsets=sorted(seen), script_calls=calls,
                native_calls=natives, timed_action_requests=timed,
                diagnostics=diagnostics, scope=SCOPE)


def helper_closure(instructions, roots, *, limit=100, instruction_limit=10000, arities=None):
    """Inventory a finite call graph, retaining recursive and unresolved edges."""
    frames = {r['offset'] for r in instructions if r['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    graphs, pending, diagnostics = {}, list(roots), []
    while pending:
        entry = pending.pop()
        if entry in graphs:
            continue
        if entry not in frames:
            diagnostics.append(f'{entry}: unresolved callee frame')
            continue
        if len(graphs) >= limit:
            diagnostics.append('helper budget exhausted')
            break
        graph = bounded_graph(instructions, entry, limit=instruction_limit, arities=arities)
        graphs[entry] = graph
        diagnostics.extend(graph['diagnostics'])
        pending.extend(e['target_offset'] for e in graph['script_calls'] if e['target_offset'] is not None)
        diagnostics.extend(f"{e['instruction_offset']}: {e['reason']}" for e in graph['script_calls'] if 'reason' in e)
    return dict(helpers=[graphs[key] for key in sorted(graphs)], diagnostics=diagnostics,
                static_graph_complete=not diagnostics, scope=SCOPE)


def dispatch_branches(instructions, entry, *, frame=1, variable=0):
    """Recognize equality-tested codes and bound true arms at their false targets."""
    graph = bounded_graph(instructions, entry)
    reachable = set(graph['instruction_offsets'])
    branches = []
    for index in range(len(instructions) - 3):
        load, literal, compare, jump = instructions[index:index + 4]
        if load['offset'] not in reachable or load['name'] != 'SCRIPT_OP_LOAD_VARIABLE':
            continue
        if [integer_operand(v) for v in load['operands']] != [frame, variable]:
            continue
        if (literal['name'], compare['name'], jump['name']) != (
                'SCRIPT_OP_PUSH_LITERAL', 'SCRIPT_OP_EQUAL', 'SCRIPT_OP_JUMP_IF_ZERO'):
            continue
        code = integer_operand(literal['operands'][0])
        true = [e['offset'] for e in jump['successors'] if e['kind'] == 'fallthrough']
        false = [e['offset'] for e in jump['successors'] if e['kind'] == 'branch']
        if code is None or len(true) != 1 or len(false) != 1:
            raise ValueError('ambiguous response comparison shape')
        # True arms converge with the false chain at a forward unconditional jump.
        by = {r['offset']: r for r in instructions}
        off, visited = true[0], set()
        while off not in visited:
            visited.add(off)
            row = by.get(off)
            if row is None or off == false[0]:
                raise ValueError('response arm does not have a bounded exit')
            if row['name'] == 'SCRIPT_OP_JUMP':
                end = integer_operand(row['operands'][0])
                if end is None or end not in by or end <= off:
                    raise ValueError('response arm exit is not a decoded forward target')
                break
            edges = [e for e in row.get('successors', []) if e['kind'] != 'call']
            if len(edges) != 1:
                raise ValueError('response arm has unclear conditional shape')
            off = edges[0]['offset']
        else:
            raise ValueError('cyclic response arm')
        body = bounded_graph(instructions, true[0], stop=(false[0], end))
        branches.append(dict(response_code=code, comparison_offset=compare['offset'],
                             false_target=false[0], exit_offset=end, **body))
    if len({b['response_code'] for b in branches}) != len(branches):
        raise ValueError('duplicate response comparison code')
    return sorted(branches, key=lambda b: b['response_code'])


def annotate_literal_native_contexts(instructions, graphs, *, root=ROOT, limit=256):
    """Link concrete constant caller actuals to a callee's first formal consumer.

    Only an exclusive linear prefix is admitted. Source-bound native preservation
    checks apply before the consumer; subsequent execution is outside this proof.
    Raw callee arguments remain unchanged, and no global formal substitution occurs.
    """
    if type(limit) is not int or limit <= 0:
        raise ValueError('context prefix work budget must be a positive integer')
    for graph in graphs:
        for edge in graph['script_calls']:
            edge.pop('conditional_native_argument_context', None)
    reviews = native_frame_reviews(root)
    preservation = local_preservation_contracts(reviews, root)
    if preservation['stale_vm_functions']:
        return
    admitted = set(preservation['preserved_native_functions'])
    by = {r['offset']: r for r in instructions}
    indices = {r['offset']: i for i, r in enumerate(instructions)}
    incoming = incoming_edges(instructions)
    frames = {o:r for o,r in by.items() if r['name']=='SCRIPT_OP_ENTER_FRAME'}

    def constant(value):
        number = value.get('value')
        if value.get('kind')=='integer':
            return type(number) is int and -2147483648 <= number <= 2147483647
        return value.get('kind')=='float' and type(number) in (int,float) and math.isfinite(number) and abs(number)<=3.4028234663852886e38

    def arguments(row, count):
        return recover_arguments(instructions, indices[row['offset']], count,
            incoming=incoming, allow_float=True, allow_symbolic=True, frame_entries=frames)

    def formal(expression, frame, argc):
        return expression.get('kind')=='variable' and expression.get('frame')==frame and type(expression.get('slot')) is int and 0<=expression['slot']<argc

    def contains_formal(expression, frame, argc):
        if isinstance(expression,dict):
            return formal(expression,frame,argc) or any(contains_formal(v,frame,argc)for v in expression.values())
        if isinstance(expression,list):return any(contains_formal(v,frame,argc)for v in expression)
        return False

    def prove(edge):
        call=by[edge['instruction_offset']]
        entry=integer_operand(call['operands'][0])
        if call['name']!='SCRIPT_OP_CALL' or entry!=edge['target_offset'] or entry not in frames:
            raise ValueError('context does not identify one decoded script call')
        header=frames[entry]
        argc,count,frame=map(integer_operand,header['operands'])
        if any(type(v)is not int for v in(argc,count,frame)) or not 0<argc<=count<=2147483645 or not 1<=frame<32:
            raise ValueError('context has an unsupported callee frame contract')
        actual=arguments(call,argc)
        if len(actual)!=argc or not all(constant(v)for v in actual):
            raise ValueError('context actuals are not bounded numeric constants')
        prefix=[entry];native_pins={};previous=header
        for _ in range(limit):
            successors=previous.get('successors',[])
            if len(successors)!=1 or successors[0]['kind']not in('fallthrough','resume_or_fallthrough'):
                raise ValueError('context prefix branches, calls or lacks continuation')
            off=successors[0]['offset'];row=by[off]
            if previous['offset']+previous['size']!=off or incoming.get(off,set())!={previous['offset']}:
                raise ValueError('context prefix is not adjacent and exclusively entered')
            name=row['name'];prefix.append(off)
            if name=='SCRIPT_OP_CALL_NATIVE':
                functions=row.get('native',{}).get('functions',[])
                if len(functions)!=1 or functions[0]not in reviews['active']:
                    raise ValueError('context native has no unique current source review')
                function=functions[0];review=reviews['active'][function];arity=review.get('arguments')
                if type(arity)is not int or arity<0:
                    raise ValueError('context native argument contract is not fixed')
                raw=arguments(row,arity)
                if any(contains_formal(v,frame,argc)for v in raw):
                    if not all(formal(v,frame,argc)or constant(v)for v in raw):
                        raise ValueError('first formal consumer has unsupported derived arguments')
                    resolved=[actual[v['slot']]if formal(v,frame,argc)else v for v in raw]
                    names=CONTRACTS.get(function,{}).get('arguments')
                    if names is not None and len(names) != arity:
                        raise ValueError('consumer argument names disagree with current source arity')
                    return dict(status='conditional_literal_actual_context',caller_offset=call['offset'],callee_entry=entry,
                        frame_index=frame,argument_count=argc,local_count=count,caller_actuals=actual,
                        consumer_native_offset=off,consumer_function=function,
                        raw_callee_arguments=dict(zip(names,raw))if names is not None else raw,
                        contextual_arguments=dict(zip(names,resolved))if names is not None else resolved,
                        prefix_instruction_offsets=prefix,intervening_native_review_sha256=native_pins,
                        consumer_review_sha256=hashlib.sha256(json.dumps(review,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
                        vm_source_pins=preservation['vm_source_pins'],
                        preconditions=['Caller arguments lie above protected operand/frame floors; matching ENTER_FRAME copies this concrete callsite actuals into valid callee formals.',
                            'Active caller/root/callee values and frame metadata are physically disjoint; absolute frame bases, scaled offsets and complete values remain allocated and nonwrapping.',
                            'Program/VM/owner/frame bindings remain stable, with no asynchronous mutation; every intervening native and original SDK prerequisite applies.',
                            'The consumer review input-conversion, object/resource and operand prerequisites remain obligations; contextual values are boxed input expressions, not converted results or proof of validity.',
                            'This potentially reachable syntactic call context only; no enclosing predicate feasibility, native outcome, later helper execution, restoration, callbacks or scheduler safety is established.'],
                        scope='Only this constant-actual callsite to the first unchanged-formal native consumer. Raw shared formal expressions are retained; no global substitution or runtime value inference.')
                if function not in admitted:
                    raise ValueError('context prefix native lacks exact preservation pin')
                native_pins[function]=preservation['native_review_sha256'][function]
            elif name=='SCRIPT_OP_STORE_VARIABLE':
                coords=tuple(integer_operand(v)for v in row['operands'])
                if len(coords)!=2 or coords[0]!=frame or type(coords[1])is not int or not argc<=coords[1]<count:
                    raise ValueError('context prefix overwrites formal or unbounded storage')
            elif name=='SCRIPT_OP_LOAD_VARIABLE':
                coords=tuple(integer_operand(v)for v in row['operands'])
                if len(coords)!=2 or coords[0]!=frame or type(coords[1])is not int or not 0<=coords[1]<count:
                    raise ValueError('context prefix reads outside callee storage')
            elif name not in('SCRIPT_OP_PUSH_LITERAL','SCRIPT_OP_DISCARD'):
                raise ValueError('context prefix has unsupported effects or rebinding')
            previous=row
        raise ValueError('context prefix work budget exhausted')

    for graph in graphs:
        for edge in graph['script_calls']:
            try:
                context=prove(edge)
            except (ValueError,KeyError,IndexError,TypeError,OverflowError):
                continue
            edge['conditional_native_argument_context']=context


def contextual_program_ids(graphs):
    """Collect timed IDs only from proven contexts with a script_id argument."""
    contextual_ids = set()
    for graph in graphs:
        for edge in graph['script_calls']:
            context = edge.get('conditional_native_argument_context')
            if context is None or 'script_id' not in CONTRACTS.get(context['consumer_function'], {}).get('arguments', []):
                continue
            expression = context['contextual_arguments']['script_id']
            if expression['kind'] in ('integer', 'float') and -2147483648 <= expression['value'] < 2147483648:
                contextual_ids.add(int(expression['value']))
    return contextual_ids


def item_response_dispatch(image, configurations, decodes, arities=None):
    """Bind this reviewed dispatcher to the exact local ROM-backed program."""
    candidates = [r for r in configurations if r['address'] == CONFIG and r['section'] == '.scene_bedroom']
    if len(candidates) != 1:
        raise ValueError('reviewed bedroom response configuration missing or ambiguous')
    row = candidates[0]
    if row['program'] != PROGRAM or row['window_bytes'] != PROGRAM_SIZE:
        raise ValueError('reviewed bedroom response program identity changed')
    old_allowed = set(image.allowed)
    try:
        image.allowed.add('.scene_bedroom')
        raw, _ = image.read_address(PROGRAM, PROGRAM_SIZE)
        header, _ = image.read_address(CONFIG, 12)
        cost_raw, cost_location = image.read_address(COST_POLICY_ADDRESS, COST_POLICY_BYTES)
    finally:
        image.allowed = old_allowed
    if int.from_bytes(header[:4], 'big') != PROGRAM or hashlib.sha256(raw).hexdigest() != PROGRAM_SHA256:
        raise ValueError('reviewed bedroom response program authentication failed')
    if hashlib.sha256(cost_raw).hexdigest() != COST_POLICY_SHA256:
        raise ValueError('reviewed cost policy table authentication failed')
    policies = cost_policy_records(cost_raw)
    instructions = decodes[row['listing'][:-5]]['instructions']
    branches = dispatch_branches(instructions, 15183)
    if [b['response_code'] for b in branches] != list(range(1, 21)):
        raise ValueError('reviewed response dispatcher shape changed')
    reaction_names = reaction_code_names()
    for branch in branches:
        branch['response_code_name'] = reaction_names.get(branch['response_code'])
    roots = {14168, 42203}
    roots.update(e['target_offset'] for b in branches for e in b['script_calls'] if e['target_offset'] is not None)
    closure = helper_closure(instructions, roots, arities=arities)
    annotate_literal_native_contexts(instructions, [*branches, *closure['helpers']])
    for graph in closure['helpers']:
        for native in graph['native_calls']:
            if native['functions'] == ['actorScriptStartCostedAction']:
                native['cost_policy_link'] = link_costed_request(native, policies)
    helpers = {g['entry_offset']: g for g in closure['helpers']}
    for branch in branches:
        reached, pending = set(), [e['target_offset'] for e in branch['script_calls']]
        while pending:
            target = pending.pop()
            if target in reached or target not in helpers:
                continue
            reached.add(target)
            pending.extend(e['target_offset'] for e in helpers[target]['script_calls'])
        branch['potential_helper_entries'] = sorted(reached)
        branch['potential_native_functions'] = sorted({f for entry in reached
            for call in helpers[entry]['native_calls'] for f in call['functions']})
        for key, roles in (
                ('potential_item_lifecycle_calls', {'eating_request_and_later_consumption_attempt', 'held_item_release_request'}),
                ('potential_profile_calls', {'profile_counter_adjustment'})):
            branch[key] = [dict(helper_entry=entry, instruction_offset=call['instruction_offset'],
                                functions=call['functions'], role=call['role'])
                           for entry in sorted(reached) for call in helpers[entry]['native_calls']
                           if call.get('role') in roles]
    special = [r for r in instructions if 42219 <= r['offset'] <= 42256]
    if ([r['name'] for r in special] != [
            'SCRIPT_OP_LOAD_VARIABLE', 'SCRIPT_OP_PUSH_LITERAL', 'SCRIPT_OP_EQUAL',
            'SCRIPT_OP_LOAD_VARIABLE', 'SCRIPT_OP_PUSH_LITERAL', 'SCRIPT_OP_EQUAL',
            'SCRIPT_OP_LOGICAL_AND', 'SCRIPT_OP_JUMP_IF_ZERO'] or
        [integer_operand(v) for v in special[0]['operands']] != [1, 0] or
        [integer_operand(v) for v in special[3]['operands']] != [1, 1] or
        integer_operand(special[1]['operands'][0]) != 18 or
        integer_operand(special[4]['operands'][0]) != 76):
        raise ValueError('reviewed special response/profile guard changed')
    timed_ids = sorted({call['constant_script_id'] for graph in closure['helpers']
                        for call in graph['timed_action_requests'] if 'constant_script_id' in call})
    contextual_ids = contextual_program_ids([*branches, *closure['helpers']])
    cost_ids = {value for graph in closure['helpers'] for native in graph['native_calls']
                for value in native.get('cost_policy_link', {}).get('policy', {}).get('referenced_timed_program_ids', [])}
    return dict(configuration_address=CONFIG, program_address=PROGRAM, program_bytes=PROGRAM_SIZE,
                program_sha256=PROGRAM_SHA256, configuration_prefix_sha256=hashlib.sha256(header).hexdigest(),
                dispatcher_entry=15183, wrapper_entry=14168, branches=branches, **closure,
                timed_program_resource_ids=sorted(set(timed_ids) | cost_ids | contextual_ids),
                cost_policies=dict(address=COST_POLICY_ADDRESS, byte_count=COST_POLICY_BYTES,
                                   sha256=COST_POLICY_SHA256, location=cost_location, records=policies,
                                   scope='Five neighbor-delimited records, not a complete allocation or runtime bounds check.'),
                resource_link='script_id in action_script_resources; cost selectors are not IDs in that domain',
                special_route=dict(entry_offset=42203, response_code=18, profile_key=76,
                                   scope='Wrapper tests this conjunction before ordinary dispatch; helper effects remain conditional.'),
                unresolved_response_codes=[dict(response_code=code, reason='Authored item-table value; handling beyond the reviewed 1..20 dispatcher is unresolved.') for code in (24, 25)],
                code_domain_note='Absence of 21..23 in this dispatcher is not a global invalidity claim.')
