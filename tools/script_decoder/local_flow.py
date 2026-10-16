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
NATIVE_PRESERVATION_PINS = {'actorScriptSetMessengerContactMode': '154619c961ec788ce94a56e35ae5b03072e1b06af3798c031f7cd44f97cab678',
 'actorScriptAdjustPikachuSecondaryMeter': '508500968db6eb0ddded3bebd199ffb34cded9493c476ac8bcccdb2132c3d8ee',
 'actorScriptCreateAttachedEffect': '8eb6a64516eef7448bc1d6598256ebafe45174602c3bdbf819835f3c930f62e5',
 'actorScriptPushDefaultActionMode': '904c7ebf3040ebbe3d82ceebdcf23d666d256c7c165ae8c056421834cd544974',
 'actorScriptYield': 'a32e41ad6f38946c8b93e4a70a50e6600499bea62a2a82c22900dc39e298d3dc',
 'actorScriptPushActionScriptStopped': '70c8afa259805820e65a4d63d8bb695ed5805e4efec59642dd201cc8ddd60e04',
 'actorScriptStartLoopExitAndWait': 'f312bb29ba63ea3061d6022f9fd8823187affddca2770aa8d62eac97930a3ab6',
 'actorScriptRandomBelow': '45206f67a57091cbca5b17b2e640a6618c518b8c7461fc7899614bf7630e4a8f',
 'actorScriptTrackObject': '19a800aa635ba4055d743c23261c202bc2c51ea2f359ac39cfe62916fed5cc91',
 'actorScriptSetDefaultActionMode': 'd85106b4014f3d3479b2b85e8d31df59ae2041eec102cecffabe969344413e7e',
 'actorScriptStartActionScript': '94629869f8f80a233831ed8ea65aa6c72df344e89975f1ec2fa8c87be9ed0c1f',
 'actorScriptStartActionWithMode': '4484c31cacbb02ab8efe8c18a08ea4dc581c9a2da5d1835ca3e603c638265859',
 'actorScriptSetSoundEffectPan': '75cc7ce489489830735fb52daf3f1280f0b2702e4d12fcef6abdd53339dc4323',
 'actorScriptSetSoundEffectVolume': 'fc2c3a2fb242129474473071f8642cbb6a134db72844cd0289516e08a36b3b1f',
 'actorScriptPlaySoundEffect': '2fe7df18adaa43193bb0eaaf1a30c1e337ebcda52c29fdd0550e466c955e41d4',
 'actorScriptPushInteractionCandidate': 'ecc2c4a2e028d78d5884071295b6d2294a276931481c369c1183024dcdeebf49',
 'actorScriptSetHudGuideStyle': 'c2df9a1154e8b680907605d060330c9aba08ccb766f31839414027e594ca3e23',
 'actorScriptStartActionAndWait': '1f26b398e3918b50435da5ee928773dacf89cfb596407c9309e1943486b18a7b',
 'actorScriptSetHorizontalMovementSuppressed': '6b88050c5d9f1de249106804c039818c6044ae8ae223580666553aa4011f4754',
 'actorScriptSetDestination': '0efa9a6f31532e65c922655116ca39757cef83498e3e0601ae7fadc08ce002c9',
 'actorScriptGetActorKind': '3aac60e3bdcdcb61153ac107a12540bff1511efd9bcdc1db6070a4e06b8c5d57',
 'actorScriptStopActionScript': 'b0316df61feddcc345e7e5bfc7304eb3fe683eb6dbed8445f4e2493637f6d070',
 'actorScriptGetItemKind': '3d9d842249982c2b8848170a8030dc70c208c513cf0fd8cd28bd6b9edac8f88a',
 'actorScriptGetObjectDistance': '16f67058c70921bd63c1950aa3415f2fdd74bda24cd4dec566cf3fd0ee55b161',
 'actorScriptFindItemInFacingCone': '96aa52980803a136149779f6e2fc2dd08fe7408cce05be27738833c3f4bd46c4',
 'actorScriptTrackPlayerPrimaryObject': 'd00e68a3517f8f0631050f1d302ea605c983f4af330dcb7ad4ecdf73de6ea77d',
 'actorScriptPushTrackingMode': '0ad1b4b9fe05cfb50e2931e7301900cf03b1845696a24292b2d07b218587d902',
 'actorScriptTrackPlayerSecondaryObject': 'e145f1c77aed4a134191e5c384e04a81871d9f2267b3bf6b7ccfc5409f166fbe',
 'actorScriptAcknowledgeMessage': 'f1e0a7a9aa429c606db2b16cbed462886de689626eaa1d22937811c6ab194327',
 'actorScriptGetObjectType': '284eb9bfb1fd26c6ce238945ccba2f56be3b58b79c5a8d0fd4f69439380c937b',
 'actorScriptPushTotalElapsedTime': '554fe92f02256193649ee7f2e71141a68a5c3f4b5c39977d5afae6a7fbbd3e89',
 'actorScriptPushTargetApproachDistance': '117f9156e3e996b2e05ca2b2957f4bb7fb9cefdbbb67d9b10c13c66c665245b1',
 'actorScriptGetItemState': '4272fc02bf1ff19994c1e117422c9e4a41bd438d31b4196f7ec95d5277121c60',
 'actorScriptStartTimedAction': 'ab7996eadee3961a01b41ce9855e02e0895645350b53e788133fa491eacbbc1e',
 'actorScriptGetTargetCoordinate': '54ae646b94cbc82d1f6a23642bf31c1cb718f212f569a092f1afa64bbcf174c1',
 'actorScriptGetGroundHeightAtPosition': '5311b7e0f4e771685d0f0cfee76afe964bf4fc36a7c545a76fa9f171b732ffb9',
 'actorScriptPushTargetHandle': '92bd31f1f81922ec60513a87725a1a96f428d11a5dec1beb1b94ebcc62487806',
 'actorScriptPushTargetDistanceAllowance': '44b1b5ffd9da55399f98ab1a0dfb68b72e7eec0754913ab93c0c75da000f625e',
 'actorScriptGetBoundingRadius': '904e0e80b802309de14fdac4c9d4cc31d3ec958ea5229c6fc238bbceca520dc3',
 'actorScriptEnqueueMessage': 'd91e1ee8e36b31072b79a75d379408f2bf869048a42ee41ccb7b639a7b552026',
 'actorScriptGetObjectPlayerDistance': '3de44ca0fdd37ea0c338ac4d326e779d12f58e16d21cf708f673f1e611feec2b',
 'actorScriptStartInterruptibleAction': 'c35939a924530140639f545f12e9bb6129a52044d9a988ea5b3416a014758b72',
 'actorScriptGetObjectCategory': '71edf0390b875c4d76e1c4565f347ac8f439b267fcc9447957feb3c8d0475f34',
 'actorScriptGetWorldCoordinate': 'c19b99d4917df49aa4d116f2cfe63306fa993158397b8da01d3d0b83efe9380d',
 'actorScriptPushTimedProgramId': '387097d7c9c494ab9e2aca7c40892c26186d53e3c4bff38dde0aee2f53be6f24',
 'actorScriptSetTargetToFallbackObject': '2a187118853afd5bc5c144fded50742fc6e8845a1d1b92ac350c619154f12207',
 'actorScriptGetPlayerCoordinate': 'e35e10efdcabe31324333f82f911f78df26e38aa4727d5444f8342c4344033bc',
 'actorScriptPushActionDuration': 'b3ed6f2f895741fca73dd168bb8ba8c0ba6af5ed1ee78eed507b72e4ab7ccaf3',
 'actorScriptTrackEscapeFromPlayerSecondaryObject': '87f11b26546be4a8ca030c6fbc6e979435b9af1bb270ecf01aff911ec847ed10',
 'actorScriptGetTargetFacingError': 'f04db8365f4e73dea6b81ba0c4a113634e758a8521e5261d9cfd1f88ccec1707',
 'actorScriptPushActionElapsedTime': '3cd6c0122273ed66cdc177f5ea92a23b4c327bde671b0aaeda1590c863cf602a',
 'actorScriptTrackEscapeFromObject': 'e4e8515fd7dd9abe91b1f6d0562f70cbb72962aa2e23a1cd01aacc26d70662b0',
 'actorScriptPlayCue': '71de4494e7c73bd69fb1d859cdb12acb3b38f4ad0a4e35a982330c4065674241',
 'actorScriptFindActorNearPosition': '5585f1f5bcdd257a6d60db7d01284e5f31b95db4e47bd7f41c2242c13ee84623'}
LOOP_EXIT_DEFERRED_PIN = 'c8a71ddd5a8d768c6963d0b726580ca7065c51380bb4aa0e4e78133bdb837e8d'
ACTION_WAIT_DEFERRED_PIN = '4a92dd241eddd566bd81ea21fa3f9e9adaf29671f478109aaa43d763a0cad2d7'
INTERRUPTIBLE_DEFERRED_PIN = 'b70db2a0e841c93a836ccbe68b30969436f184c216ee5523b0883a902770cbb9'



VM_COMPARISON_UNIT_PIN = 'cded04462b1f2fbece0ee8d937f9d4fcfc8c659719094b92768715d9669fe8fb'
VM_COMPARISON_PIN = '6406d5a820565f00b42c6a75d7cc68ca9834b1040ab1490e64f52d6299230920'
VM_COMPARISON_VALUE_PINS = {
    'scriptValueGetFloat': '94e6ff080eea82e2661ec2adbc45a33a6a2141b10ff8d1263f580c2c5433c6b1',
    'scriptValueGetInteger': 'b3c74539dabcef92e573e0b7081b41eb33659fe2a4c502fdc3805cb16a5b267f'}
VM_COMPARISON_HEADER_PIN = 'f9f9e4509166d6c9aac4d4687f973da7c3b719df7413be4d4f738182850e5b08'
VM_COMPARISON_MACRO_PINS = {
    'SCRIPT_VALUE_RELATIVE_EQUALITY_TOLERANCE': '26115bb364a1c80caad300dafce5ab6eec49bc04ef13fcf7397c6065170c2425',
    'SCRIPT_VALUE_ABSOLUTE_EQUALITY_TOLERANCE': 'c536c9032711fbd37fac06012537ce3db9fb754f37b5fdb0ab8306efe42aa02a'}


def vm_comparison_contract(root=ROOT):
    """Bind the admitted integer/finite-float comparison model to current source."""
    import re
    try:
        code = (root / 'src/ovl/script_vm.c').read_text()
        stale = []
        if fingerprint(code) != VM_COMPARISON_UNIT_PIN:
            stale.append('src/ovl/script_vm.c')
        if fingerprint(function_source(code, 'scriptVmApplyComparisonOperator')) != VM_COMPARISON_PIN:
            stale.append('scriptVmApplyComparisonOperator')
        for name, expected in VM_COMPARISON_VALUE_PINS.items():
            if fingerprint(function_source(code, name)) != expected:
                stale.append(name)
        if fingerprint((root / 'include/script_vm.h').read_text()) != VM_COMPARISON_HEADER_PIN:
            stale.append('include/script_vm.h')
        for name, expected in VM_COMPARISON_MACRO_PINS.items():
            definitions = re.findall(r'^\s*#define\s+' + name + r'\b[^\n]*', code, re.M)
            if len(definitions) != 1 or fingerprint(definitions[0]) != expected:
                stale.append(name)
    except (OSError, ValueError):
        stale = ['comparison_source_unavailable']
    return dict(current=not stale, stale=stale, vm_unit_sha256=VM_COMPARISON_UNIT_PIN, function_sha256=VM_COMPARISON_PIN,
                macro_sha256=dict(VM_COMPARISON_MACRO_PINS), value_helper_sha256=dict(VM_COMPARISON_VALUE_PINS),
                value_header_sha256=VM_COMPARISON_HEADER_PIN, relative_tolerance=0.001,
                absolute_tolerance=0.001,
                model='Integer comparisons are exact. Finite floating operands use abs(right-left) <= 0.001*abs(left)+0.001; NOT_EQUAL is its complement. With unchanged copied numeric mode, the equality region around 6 is disjoint from every other admitted signed-integer literal equality region. No exact floating equality or nonnumeric/NaN semantics is inferred.')


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
    deferred_pins = {'actorScriptStartLoopExitAndWait': LOOP_EXIT_DEFERRED_PIN,
                     'actorScriptStartActionAndWait': ACTION_WAIT_DEFERRED_PIN,
                     'actorScriptStartInterruptibleAction': INTERRUPTIBLE_DEFERRED_PIN}
    names, stale = [], []
    for name, expected in NATIVE_PRESERVATION_PINS.items():
        if name not in reviews['active'] or digest(reviews['active'][name]) != expected:
            stale.append(name)
            continue
        expected_deferred = deferred_pins.get(name)
        if expected_deferred is not None and digest(reviews['active_deferred'].get(name)) != expected_deferred:
            stale.append(name)
            continue
        names.append(name)
    return dict(preserved_native_functions=[] if stale_vm else names,
                stale_vm_functions=stale_vm, stale_native_reviews=stale,
                native_review_sha256=NATIVE_PRESERVATION_PINS,
                deferred_loop_exit_review_sha256=LOOP_EXIT_DEFERRED_PIN,
                deferred_action_wait_review_sha256=ACTION_WAIT_DEFERRED_PIN,
                deferred_interruptible_review_sha256=INTERRUPTIBLE_DEFERRED_PIN,
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
