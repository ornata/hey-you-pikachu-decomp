"""Recognize a complete conditional tracked-creature predicate and its callees.

This reports sampled script evidence, not creature liveness, movement outcome,
array validity, or effects of the surrounding snapshot extension.
"""
from .callee_contexts import context_snapshot
from .companion_snapshots import _digest, _operands, _edges
from .context import ROOT, opcode_names
from .stack import incoming_edges
from .native_frames import fingerprint, function_source
from .scene_region_snapshots import _dependency_fingerprint

NATIVE_BINDINGS = {'actorScriptGetActorKind': {'address': 2148654456,
                             'arguments': 1,
                             'file': 'src/ovl/actor_script_messages_items.c',
                             'review_sha256': 'ee5d8dd13bb391df6e0cb37c4c2d85a0262aeaf9f6b231ad891be1ae60dfbddc',
                             'sha256': '35d723f6ed40ddc17bc9f8462b8ef9863ed4f5d544ef7a793f44997578b93d8b'},
 'actorScriptGetCreationScriptArgument': {'address': 2148664684,
                                          'arguments': 1,
                                          'file': 'src/ovl/actor_script_collection_profile.c',
                                          'review_sha256': '0ed807254b4e38d0769df8424825e762f4c57fbde39cbda3e23b18ee7335823d',
                                          'sha256': '4e9f4574a1aa5f6806d2deb3818d5093f625ae039ddffec7e98972b85a4a05a3'},
 'actorScriptGetObjectWorldCoordinate': {'address': 2148657948,
                                         'arguments': 2,
                                         'file': 'src/ovl/actor_script_player_world.c',
                                         'review_sha256': '52bec3721f0635bc521285fd57800809da7005aa88a0645884a55b1818eace91',
                                         'sha256': 'c5b2d01ef2cfdc3631a53e6997f62dcea63bf49921987d593f8669e8790915d2'}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptGetActorKind', 'actorScriptGetCreationScriptArgument', 'actorScriptGetObjectWorldCoordinate'))
GRAPH_SHA256 = '21ea1cccc2eee99e7373e818a0ec299c4225927fe99596c31792bbde89dad340'
VM_UNIT_SHA256 = 'cded04462b1f2fbece0ee8d937f9d4fcfc8c659719094b92768715d9669fe8fb'


def _graph_index(instructions):
    """Build invocation-local indexes after validating the decoded ordering."""
    offsets = [r.get("offset") for r in instructions]
    if any(type(o) is not int or o < 0 for o in offsets) or offsets != sorted(set(offsets)):
        raise ValueError("Duplicate, unordered or invalid instructions.")
    return ({r["offset"]: r for r in instructions}, incoming_edges(instructions), opcode_names("vm"))


def _normalize_graph(instructions, entry, bindings, *, _prepared=None):
    """Validate complete reachable functions, preserving roots across CALL edges."""
    by, incoming, names = _graph_index(instructions) if _prepared is None else _prepared
    roots, nodes, entries, selectors = {}, [], [entry], {}
    for start in entries:
        if len(entries) > 12 or len(nodes) > 12:
            raise ValueError('Call closure budget exceeded.')
        pending, reached = [start], set()
        while pending:
            offset = pending.pop()
            if offset in reached:
                continue
            if offset not in by or len(reached) >= 256:
                raise ValueError('Incomplete function or instruction budget exceeded.')
            row = by[offset]
            values, tags = _operands(row, names)
            reached.add(offset)
            if row['name'] == 'SCRIPT_OP_CALL':
                if values[0] not in entries:
                    entries.append(values[0])
                pending.append(offset + row['size'])
            elif row['name'] != 'SCRIPT_OP_RETURN':
                pending.extend(edge['offset'] for edge in row['successors'])
        rows = [by[o] for o in sorted(reached)]
        if rows[0]['offset'] != start or rows[0]['name'] != 'SCRIPT_OP_ENTER_FRAME':
            raise ValueError('Missing complete function entry.')
        if not any(r['name'] == 'SCRIPT_OP_RETURN' for r in rows):
            raise ValueError('No function return.')
        operands = [_operands(r, names) for r in rows]
        expected_incoming = {}
        for r, (v, _) in zip(rows, operands):
            if r.get('successors') != _edges(r, v):
                raise ValueError('Incorrect full instruction edges.')
            for edge in r['successors']:
                expected_incoming.setdefault(edge['offset'], set()).add(r['offset'])
        for r in rows[1:]:
            if incoming.get(r['offset'], set()) != expected_incoming.get(r['offset'], set()):
                raise ValueError('Competing function interior entry.')
        for left, right in zip(rows, rows[1:]):
            if left['offset'] + left['size'] > right['offset']:
                raise ValueError('Overlapping function instructions.')
        if any(start < o < rows[-1]['offset'] and o not in reached for o in by):
            raise ValueError('Unreviewed decoded instruction inside function extent.')
        if any(set(reached) & set(n['offsets']) for n in nodes):
            raise ValueError('Overlapping or recursive function closure.')
        positions = {r['offset']: i for i, r in enumerate(rows)}
        encoded = []
        for row, (values, tags) in zip(rows, operands):
            name, value = row['name'], values[:]
            if name in ('SCRIPT_OP_LOAD_VARIABLE', 'SCRIPT_OP_STORE_VARIABLE', 'SCRIPT_OP_LOAD_ELEMENT', 'SCRIPT_OP_STORE_ELEMENT') and values[0] == 0:
                roots.setdefault(values[1], len(roots))
                value = [0, 'root-role-' + str(roots[values[1]])]
            elif name in ('SCRIPT_OP_JUMP', 'SCRIPT_OP_JUMP_IF_ZERO'):
                if values[0] not in positions:
                    raise ValueError('Branch leaves complete function.')
                value = ['instruction', positions[values[0]]]
            elif name == 'SCRIPT_OP_CALL':
                value = ['callee', entries.index(values[0])]
            elif name == 'SCRIPT_OP_CALL_NATIVE':
                native = row['native']; function = native['functions'][0]
                binding = bindings.get(function)
                if binding is None or binding['address'] != native['address']:
                    raise ValueError('Unreviewed native target.')
                identity = (function, native['address'])
                if values[0] in selectors and selectors[values[0]] != identity:
                    raise ValueError('Ambiguous native selector.')
                selectors[values[0]] = identity
                value = list(identity)
            encoded.append([name, value, tags])
        nodes.append(dict(entry=start, offsets=sorted(reached), normalized=encoded,
                          bytes=rows[-1]['offset'] + rows[-1]['size'] - start, reachable_bytes=sum(r['size'] for r in rows)))
    return dict(sha256=_digest([n['normalized'] for n in nodes]), nodes=nodes,
                root_role_mapping=roots)


def _source_bindings(root, snapshot):
    errors, files = [], {}
    if set(NATIVE_BINDINGS) != REQUIRED_NATIVE_NAMES:
        return dict(status="unresolved", reasons=["Incomplete native binding set."])
    def read(path):
        if path not in files:
            files[path] = (root / path).read_text()
        return files[path]
    try:
        reviews, preservation = context_snapshot(root=root) if snapshot is None else snapshot
        if not preservation.get('context_vm_contract', {}).get('current') or preservation.get('stale_vm_functions'):
            errors.append('Stale complete VM contract.')
        if fingerprint(read('src/ovl/script_vm.c')) != VM_UNIT_SHA256:
            errors.append('Stale VM source unit.')
        for name, binding in NATIVE_BINDINGS.items():
            review = reviews.get('active', {}).get(name)
            if review is None or _digest(review) != binding['review_sha256'] or review.get('arguments') != binding['arguments']:
                errors.append('Stale native review: ' + name)
                continue
            if fingerprint(function_source(read(binding['file']), name)) != binding['sha256']:
                errors.append('Stale native body: ' + name)
            for dependency in review.get('dependencies', []):
                if _dependency_fingerprint(dependency, read) != dependency['sha256']:
                    errors.append('Stale native dependency: ' + name)
    except (OSError, ValueError, KeyError, TypeError, AttributeError) as error:
        errors.append(str(error))
    return dict(status='unresolved' if errors else 'current', reasons=errors,
                native_review_sha256={n: b['review_sha256'] for n, b in NATIVE_BINDINGS.items()},
                vm_unit_sha256=VM_UNIT_SHA256)


def tracked_creature_predicates(instructions, *, snapshot=None, root=ROOT):
    """Report exact predicate graphs under explicit allocation and sampling conditions."""
    bindings = _source_bindings(root, snapshot)
    result = dict(status='unresolved', predicates=[], source_bindings=bindings)
    if bindings['status'] != 'current':
        return result
    result['status'] = 'conditional_tracked_creature_predicates'
    try:
        prepared = _graph_index(instructions)
    except (ValueError, KeyError, TypeError, AttributeError):
        result['status'] = 'unresolved'
        return result
    for row in instructions:
        if (row.get('name') != 'SCRIPT_OP_ENTER_FRAME' or
                [o.get('value') for o in row.get('operands', [])] != [0, 5, 1]):
            continue
        try:
            graph = _normalize_graph(instructions, row['offset'], NATIVE_BINDINGS, _prepared=prepared)
            if graph['sha256'] != GRAPH_SHA256:
                continue
        except (ValueError, KeyError, TypeError, IndexError, OverflowError):
            continue
        roles = {v: k for k, v in graph['root_role_mapping'].items()}
        result['predicates'].append(dict(entry_offset=row['offset'], canonical_sha256=GRAPH_SHA256,
            root_role_mapping=graph['root_role_mapping'], functions=[dict(entry=n['entry'], span_bytes=n['bytes'], reachable_bytes=n['reachable_bytes'], instruction_offsets=n['offsets']) for n in graph['nodes']],
            tracked_handle=[0, roles[0]], cached_coordinate_base=[0, roles[1]],
            predicate='A nonzero tracked handle passes the actor-kind 5..7 check and either its shared frame -1 array at base 23 indexed by creation argument is nonzero after the original VM integer conversion, or the sum of absolute sampled XYZ differences from cached coordinates is strictly greater than 1.0.',
            graph_scope='Complete reachable instruction closure; unreachable undecoded byte gaps are not interpreted.',
            result_role='Boolean integer zero or one; no surrounding extension effects are inferred.',
            preconditions=[
                'Normal entry, valid initialized nonwrapping VM frames and sufficient operand/saved-frame capacity above the caller floor; all expanded native prerequisites apply.',
                'Tracked handle is integer-boxed. Zero, unavailable and nonqualifying kinds follow the false branches; any resolved kind-5..7 object has initialized actor state. Registry bindings remain valid and coherent across separately sampled queries, without replacement or asynchronous mutation. Root, shared, local and native workspaces are physically disjoint.',
                'Creation argument indexes allocated initialized shared frame -1 array storage at base 23; the script performs no bounds check.',
                'Cached coordinate base addresses three allocated initialized numeric cells; indexed accesses remain within their respective frames. Numeric conversions, arithmetic and comparisons satisfy the original VM contract; integer negation excludes INT_MIN.',
                'This is conditional sampled Manhattan-coordinate change, not Euclidean distance, speed, universal occupancy, guaranteed runtime reachability or a complete snapshot extension.']))
    return result
