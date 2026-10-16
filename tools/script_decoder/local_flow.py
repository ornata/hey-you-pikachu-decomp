"""Conservative finite local-value domains under explicit frame preservation.

This analysis inventories static alternatives, not feasible executions. Caller,
callee and global storage must be valid and physically nonaliasing. Native names
are accepted only through a separately reviewed preservation allowlist.
"""

import hashlib
import json

from .context import ROOT
from .native_frames import fingerprint, function_source
from .formats import integer_operand
from .stack import incoming_edges, recover_arguments

KNOWN_OPERATIONS = {'SCRIPT_OP_' + name for name in (
    'NEGATE BITWISE_NOT ADD SUBTRACT MULTIPLY DIVIDE REMAINDER STORE_VARIABLE '
    'EQUAL NOT_EQUAL GREATER_THAN LESS_THAN GREATER_OR_EQUAL LESS_OR_EQUAL '
    'LOGICAL_NOT LOGICAL_AND LOGICAL_OR BITWISE_AND BITWISE_OR SHIFT_RIGHT SHIFT_LEFT '
    'PUSH_LITERAL LOAD_VARIABLE CALL CALL_NATIVE JUMP JUMP_IF_ZERO RETURN DISCARD '
    'ENTER_FRAME LOAD_ELEMENT STORE_ELEMENT FINISH').split()}


# Exact reviewed VM semantics and expanded native footprints; stale evidence fails closed.
VM_PRESERVATION_PINS = {'actorRunScriptVm': '530f60a11aa40eaf985ddaedf886236565d4089c6e2d75b3981d5ac289e3ea6b',
 'scriptVmReadVariableReference': '484549f6aca0a31c46bc7598f7f98a8f48e7889494c368e966439e34238e4450',
 'scriptVmSaveStackDepth': 'bc0e2ced26df1b46f070deb78e903f550cbdf834e8406849c14a3dcaee8a91da',
 'scriptVmRestoreStackDepth': '331329708e1311d8c9a50d5219b71f580c6b6929c1f21c9dbd2727a29e4d7794',
 'scriptVmSaveReturnLocation': '30de7c1a3e2d94e03f05de97502aafc074ed973816f03a3799fcb80281bcfeb2',
 'scriptVmRestoreReturnLocation': 'acb1f757b44289cf068cf40f85c88f7515c54cac1f61e3b235528124b0333d41'}
NATIVE_PRESERVATION_PINS = {'actorScriptSetMessengerContactMode': 'd4eaca70214bbc1f4afe08ab190ad5ebdc83a90507cbc284adddd6928f6496cd',
 'actorScriptAdjustPikachuSecondaryMeter': '28152370080c8f5f6b4cc01831d3e1283448ae2c69a459fb4400e628a365e2a5',
 'actorScriptCreateAttachedEffect': 'c9ddac62038f577113d09ea8fa22ecd7867f1b2cf34e3bb7f4cddbda1d979f9c',
 'actorScriptPushDefaultActionMode': '904c7ebf3040ebbe3d82ceebdcf23d666d256c7c165ae8c056421834cd544974',
 'actorScriptYield': 'a32e41ad6f38946c8b93e4a70a50e6600499bea62a2a82c22900dc39e298d3dc',
 'actorScriptPushActionScriptStopped': '70c8afa259805820e65a4d63d8bb695ed5805e4efec59642dd201cc8ddd60e04',
 'actorScriptStartLoopExitAndWait': '0640615d314d7f1c6e5580040874db372aaf0318f2e39cccc3176f6c98ff7e17',
 'actorScriptRandomBelow': '45206f67a57091cbca5b17b2e640a6618c518b8c7461fc7899614bf7630e4a8f',
 'actorScriptTrackObject': '19a800aa635ba4055d743c23261c202bc2c51ea2f359ac39cfe62916fed5cc91',
 'actorScriptSetDefaultActionMode': '07eb0f22f922eca790f2a8fb24d7ca6ddd6a054b9e3e48e2f7498e6493db392e',
 'actorScriptStartActionScript': '934593cea28da10f36980e6c541238b6d0d98949e20645d69d80fdf2cf99ed8e',
 'actorScriptStartActionWithMode': 'f394029c3988187cf97e019fe4d595025ef0dfc0bec7bf6de1acf6a54bc8d614',
 'actorScriptSetSoundEffectPan': '75cc7ce489489830735fb52daf3f1280f0b2702e4d12fcef6abdd53339dc4323',
 'actorScriptSetSoundEffectVolume': 'fc2c3a2fb242129474473071f8642cbb6a134db72844cd0289516e08a36b3b1f',
 'actorScriptPlaySoundEffect': '2fe7df18adaa43193bb0eaaf1a30c1e337ebcda52c29fdd0550e466c955e41d4'}
LOOP_EXIT_DEFERRED_PIN = '687dd86d8dd2376c3f9d03d05ac4e12b2177dcebb5f682a77634a603ef1c3957'


def local_preservation_contracts(reviews, root=ROOT):
    """Bind a narrow conditional native allowlist to frozen current source reviews."""
    code = (root / 'src/ovl/script_vm.c').read_text()
    stale_vm = []
    for name, expected in VM_PRESERVATION_PINS.items():
        try:
            current = fingerprint(function_source(code, name))
        except ValueError:
            current = None
        if current != expected:
            stale_vm.append(name)
    def digest(value):
        return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    names, stale = [], []
    for name, expected in NATIVE_PRESERVATION_PINS.items():
        if name not in reviews['active'] or digest(reviews['active'][name]) != expected:
            stale.append(name)
            continue
        if name == 'actorScriptStartLoopExitAndWait' and digest(reviews['active_deferred'].get(name)) != LOOP_EXIT_DEFERRED_PIN:
            stale.append(name)
            continue
        names.append(name)
    return dict(preserved_native_functions=[] if stale_vm else names,
                stale_vm_functions=stale_vm, stale_native_reviews=stale,
                native_review_sha256=NATIVE_PRESERVATION_PINS,
                deferred_loop_exit_review_sha256=LOOP_EXIT_DEFERRED_PIN,
                vm_source_pins=VM_PRESERVATION_PINS,
                scope='Only these exact source-bound native footprints under their full per-row prerequisites; not a general active-native preservation claim or runtime frame-lifetime proof.')


def local_integer_domain(instructions, target, frame, slot, *, preserved_natives=(), limit=2000):
    """Find constant reaching stores, rejecting unresolved writers and calls."""
    by = {r['offset']: r for r in instructions}
    indices = {r['offset']: i for i, r in enumerate(instructions)}
    frames = {off: r for off, r in by.items() if r['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    incoming = incoming_edges(instructions)
    predecessors = {}
    for row in instructions:
        for edge in row.get('successors', []):
            if edge['kind'] != 'call':
                predecessors.setdefault(edge['offset'], set()).add(row['offset'])
    preserved_natives = set(preserved_natives)
    diagnostics, definitions, native_proofs, call_proofs = [], {}, set(), set()
    work = 0
    borrowed_writes = set()
    root_frames = [(off, r) for off, r in frames.items() if integer_operand(r['operands'][2]) == 0]
    root_extent = None
    if len(root_frames) == 1 and root_frames[0][0] == 0:
        root_args, root_count, _ = map(integer_operand, root_frames[0][1]['operands'])
        if root_args == 0 and root_count is not None and root_count >= 0:
            root_extent = root_count

    def charge():
        nonlocal work
        work += 1
        if work > limit:
            raise ValueError('local-flow instruction budget exhausted')

    def next_offsets(row):
        return [e['offset'] for e in row.get('successors', []) if e['kind'] != 'call']

    def native_safe(row):
        names = row.get('native', {}).get('functions', [])
        if len(names) != 1 or names[0] not in preserved_natives:
            raise ValueError(f"{row['offset']}: native has no reviewed local-preservation contract")
        native_proofs.add(names[0])

    def store_coordinates(row):
        if row['name'] not in KNOWN_OPERATIONS:
            raise ValueError(f"{row['offset']}: unknown opcode effect")
        if row['name'] == 'SCRIPT_OP_STORE_ELEMENT':
            raise ValueError(f"{row['offset']}: indirect local write")
        if row['name'] != 'SCRIPT_OP_STORE_VARIABLE':
            return None
        coordinates = tuple(integer_operand(v) for v in row['operands'])
        if len(coordinates) != 2 or None in coordinates:
            raise ValueError(f"{row['offset']}: unresolved write coordinates")
        return coordinates

    def borrowed_safe(coords):
        if coords[0] != 0 or root_extent is None or not 0 <= coords[1] < root_extent:
            raise ValueError('borrowed frame write lacks a bounded active root-frame extent')
        borrowed_writes.add(coords)

    validated, validating = set(), set()

    def callee_safe(entry):
        if entry in validated:
            return
        if entry in validating:
            raise ValueError('recursive callee preservation is unresolved')
        call_proofs.add(entry)
        row = frames.get(entry)
        if row is None:
            raise ValueError('callee lacks a decoded frame entry')
        arguments, locals_count, own_frame = map(integer_operand, row['operands'])
        if arguments is None or locals_count is None or own_frame is None or not 0 <= arguments <= locals_count or not 1 <= own_frame < 32:
            raise ValueError('callee has an invalid or root/borrowed frame binding')
        validating.add(entry)
        pending, seen, returned = [entry], set(), False
        while pending:
            off = pending.pop()
            if off in seen:
                continue
            charge()
            seen.add(off)
            child = by.get(off)
            if child is None:
                raise ValueError('callee has a missing instruction')
            if off != entry and child['name'] == 'SCRIPT_OP_ENTER_FRAME':
                raise ValueError('callee enters a competing frame')
            if child['name'] == 'SCRIPT_OP_RETURN':
                returned = True
                continue
            if child['name'] == 'SCRIPT_OP_FINISH':
                raise ValueError('callee can finish without restoring its caller frame')
            coords = store_coordinates(child)
            if coords is not None and not (coords[0] == own_frame and 0 <= coords[1] < locals_count):
                borrowed_safe(coords)
            if child['name'] == 'SCRIPT_OP_CALL_NATIVE':
                native_safe(child)
            if child['name'] == 'SCRIPT_OP_CALL':
                nested = integer_operand(child['operands'][0])
                if nested is None:
                    raise ValueError('callee has a dynamic script call')
                callee_safe(nested)
            edges = next_offsets(child)
            if not edges:
                raise ValueError('callee has no understood continuation')
            pending.extend(edges)
        validating.remove(entry)
        if not returned:
            raise ValueError('callee has no restoring return')
        validated.add(entry)

    def check_effect(row):
        if row['name'] == 'SCRIPT_OP_CALL_NATIVE':
            native_safe(row)
        if row['name'] == 'SCRIPT_OP_CALL':
            callee_safe(integer_operand(row['operands'][0]))
        coords = store_coordinates(row)
        if coords is not None and coords[0] == frame and not 0 <= coords[1] < count:
            raise ValueError('caller-frame write is outside the active local extent')
        return coords

    try:
        if len(by) != len(instructions) or frame is None or slot is None or not 1 <= frame < 32 or slot < 0:
            raise ValueError('invalid local-flow instruction/coordinate input')
        # Establish one physical owner frame, without mistaking call edges for CFG predecessors.
        pending, seen, owners = [target], set(), set()
        while pending:
            off = pending.pop()
            if off in seen:
                continue
            charge()
            seen.add(off)
            row = by.get(off)
            if row is None:
                raise ValueError('missing local-flow instruction')
            if row['name'] == 'SCRIPT_OP_ENTER_FRAME':
                owners.add(off)
            else:
                previous = predecessors.get(off, set())
                if not previous:
                    raise ValueError('flow has an unframed entry')
                pending.extend(previous)
        if len(owners) != 1:
            raise ValueError('competing frame entries')
        owner = owners.pop()
        args, count, owner_frame = map(integer_operand, by[owner]['operands'])
        if args is None or count is None or owner_frame != frame or not 0 <= args <= count or slot >= count:
            raise ValueError('local coordinate is outside its owning frame')
        # Every path must hit a known store before the owner entry.
        pending, seen = list(predecessors.get(target, set())), set()
        while pending:
            off = pending.pop()
            if off in seen:
                continue
            charge()
            seen.add(off)
            row = by[off]
            if off == owner:
                raise ValueError('path reaches frame entry without a local definition')
            if row['name'] == 'SCRIPT_OP_ENTER_FRAME':
                raise ValueError('competing frame entry in reaching definitions')
            coords = check_effect(row)
            if coords == (frame, slot):
                expression = recover_arguments(instructions, indices[off], 1, incoming=incoming,
                    allow_float=True, allow_symbolic=True, frame_entries=frames)[0]
                if expression['kind'] != 'integer':
                    raise ValueError(f'{off}: local definition is not an integer constant')
                definitions[off] = expression['value']
                continue
            if coords is not None and coords[0] != frame:
                borrowed_safe(coords)
            pending.extend(predecessors.get(off, set()))
        if not definitions:
            raise ValueError('no bounded constant local definitions')
        values, guards = set(definitions.values()), []

        def reaches(start, goal, stop):
            todo, visited = [start], set()
            while todo:
                off = todo.pop()
                if off == goal:
                    return True
                if off in visited or off in stop:
                    continue
                charge()
                visited.add(off)
                row = by.get(off)
                if row is None:
                    raise ValueError('missing guard-flow instruction')
                todo.extend(next_offsets(row))
            return False

        for index in range(len(instructions) - 3):
            load, literal, compare, jump = instructions[index:index + 4]
            if load['name'] != 'SCRIPT_OP_LOAD_VARIABLE' or tuple(integer_operand(v) for v in load['operands']) != (frame, slot):
                continue
            if [literal['name'], compare['name'], jump['name']] != ['SCRIPT_OP_PUSH_LITERAL', 'SCRIPT_OP_NOT_EQUAL', 'SCRIPT_OP_JUMP_IF_ZERO']:
                continue
            if literal['operands'][0]['tag'] not in (0, 2):
                continue
            if any(incoming.get(right['offset'], set()) != {left['offset']} or
                   next_offsets(left) != [right['offset']]
                   for left, right in ((load, literal), (literal, compare), (compare, jump))):
                continue
            excluded = integer_operand(literal['operands'][0])
            true = [e['offset'] for e in jump['successors'] if e['kind'] == 'fallthrough']
            false = [e['offset'] for e in jump['successors'] if e['kind'] == 'branch']
            if excluded not in values or len(true) != 1 or len(false) != 1:
                continue
            if reaches(owner, target, {jump['offset']}) or reaches(false[0], target, {jump['offset']}):
                continue
            if not reaches(true[0], target, {jump['offset']}):
                continue
            # A later write invalidates the predicate; inspect every local continuation.
            todo, visited, clean = [true[0]], set(), True
            while todo:
                off = todo.pop()
                if off == target or off == jump['offset'] or off in visited:
                    continue
                charge()
                visited.add(off)
                row = by[off]
                coords = check_effect(row)
                if coords == (frame, slot) or coords is not None and coords[0] not in (0, frame):
                    clean = False
                todo.extend(next_offsets(row))
            if clean:
                values.discard(excluded)
                guards.append(dict(load_offset=load['offset'], comparison_offset=compare['offset'],
                                   branch_offset=jump['offset'], excluded_value=excluded))
        if not values:
            raise ValueError('guard filtering leaves no static alternatives')
        return dict(status='bounded_local_integer_domain', values=sorted(values), owner_frame_entry=owner,
                    definitions=[dict(instruction_offset=off, value=value) for off, value in sorted(definitions.items())],
                    guards=guards, preserved_native_functions=sorted(native_proofs),
                    reviewed_script_callees=sorted(call_proofs), visited_work=work,
                    root_frame_extent=root_extent, borrowed_frame_writes=[dict(frame=f, slot=s) for f, s in sorted(borrowed_writes)],
                    conditional_on_frame_lifetimes=True,
                    preconditions=['The decoded program, current VM and owner frame binding remain continuously valid from function entry to request, apart from verified balanced nested rebinding; earlier context replacement is not excluded by this local-store analysis.',
                                   'If borrowed frame0 writes occur, the sole decoded root frame at0 is active and its entire local interval lies below protected caller locals and metadata.',
                                   'All active and nested frames have valid nonoverlapping physical intervals, balanced returns, sufficient operand floors and stack capacity.',
                                   'Every listed native review and transitive prerequisite holds, including complete deferred effects and SDK resource nonaliasing.',
                                   'No asynchronous VM, context, frame-binding or caller-local mutation occurs across yields/waits.'],
                    scope='Static alternatives, not path feasibility. Valid nonaliasing caller/callee/persistent-root frame storage and reviewed native local-preservation contracts are required.')
    except (ValueError, KeyError, IndexError) as error:
        diagnostics.append(str(error))
        return dict(status='unresolved_local_integer_domain', diagnostics=diagnostics, visited_work=work,
                    scope='No finite local domain or policy alternatives are inferred.')
