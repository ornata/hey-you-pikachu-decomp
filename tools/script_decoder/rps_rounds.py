"""Source-bound recognition of complete authored rock-paper-scissors routines.

Recognized requests and phases are conditional script observations. External
script invocations remain opaque; no preservation, termination or outcome proof.
"""

import hashlib
import re

from .callee_contexts import context_snapshot
from .companion_snapshots import _edges, companion_snapshots
from .context import ROOT
from tools.source_quality import code_only
from .native_frames import fingerprint
from .request_arguments import _digest, validate_instructions
from .stack import incoming_edges

NATIVE_BINDINGS = {'actorScriptAcknowledgeMessage': {'address': 2148649036,
                                   'review_sha256': 'f1e0a7a9aa429c606db2b16cbed462886de689626eaa1d22937811c6ab194327'},
 'actorScriptAdjustPikachuSecondaryMeter': {'address': 2148659428,
                                            'review_sha256': 'ad707956ca02a34708e441f4916193e28b410875772d09c7a6455d3be5665f1c'},
 'actorScriptCreateAttachedEffect': {'address': 2148649356,
                                     'review_sha256': 'ef640c052269675ac2c4ebff5c8761b339dab25fe4202c6ca3e90f141dbaa1ee'},
 'actorScriptEnqueueMessage': {'address': 2148647744,
                               'review_sha256': '84ebda162953537988a95f51d173b98011ef9b0937a09e813eb16d9247defacc'},
 'actorScriptGetTargetCoordinate': {'address': 2148651920,
                                    'review_sha256': '54ae646b94cbc82d1f6a23642bf31c1cb718f212f569a092f1afa64bbcf174c1'},
 'actorScriptPushActionScriptStopped': {'address': 2148648748,
                                        'review_sha256': '70c8afa259805820e65a4d63d8bb695ed5805e4efec59642dd201cc8ddd60e04'},
 'actorScriptPushPlayerDistance': {'address': 2148649784,
                                   'review_sha256': 'ad540217e7ef337c7ee243558874f909aa875f9912c3f4fcc41d5e1aa8d7da06'},
 'actorScriptPushTimer': {'address': 2148647700,
                          'review_sha256': 'ffddfbb946413bd0f75eb279ec0636d33c05d24410a732d94ac39fb2bbcdae3c'},
 'actorScriptPushVoiceCommandStatus': {'address': 2148651172,
                                       'review_sha256': '373e8601116bb23c67a7d2c3869cfb7ca823e2805dbe3d6b1d27e38698424e86'},
 'actorScriptRandomBelow': {'address': 2148647408,
                            'review_sha256': '45206f67a57091cbca5b17b2e640a6618c518b8c7461fc7899614bf7630e4a8f'},
 'actorScriptRequestMicDictionary': {'address': 2148652496,
                                     'review_sha256': '2b655d4bb4a2ee2eae8215e6a08564fb46884a720a6e37ce6ef1dcfeb5866849'},
 'actorScriptSetDefaultActionMode': {'address': 2148654648,
                                     'review_sha256': 'd2a281d4d491c14208ce217537e4d13c10e9d0cbb610630bbefeab68cedc0fdf'},
 'actorScriptSetDestination': {'address': 2148653568,
                               'review_sha256': '2eb140a4be88760bb2660b82ea3265a66b9abb9a003402ce4147cffa1f5e71cd'},
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
 'actorScriptTrackPlayerPrimaryObject': {'address': 2148648960,
                                         'review_sha256': 'd00e68a3517f8f0631050f1d302ea605c983f4af330dcb7ad4ecdf73de6ea77d'},
 'actorScriptTrackPlayerSecondaryObject': {'address': 2148652360,
                                           'review_sha256': '6fd1e82b021dbf0d289abee78a4ac6599d7c22f9eeb2198df53636dcf95b4724'},
 'actorScriptYield': {'address': 2148646576,
                      'review_sha256': 'a32e41ad6f38946c8b93e4a70a50e6600499bea62a2a82c22900dc39e298d3dc'}}
REQUIRED_NATIVE_NAMES = frozenset(('actorScriptAcknowledgeMessage', 'actorScriptAdjustPikachuSecondaryMeter', 'actorScriptCreateAttachedEffect', 'actorScriptEnqueueMessage', 'actorScriptGetTargetCoordinate', 'actorScriptPushActionScriptStopped', 'actorScriptPushPlayerDistance', 'actorScriptPushTimer', 'actorScriptPushVoiceCommandStatus', 'actorScriptRandomBelow', 'actorScriptRequestMicDictionary', 'actorScriptSetDefaultActionMode', 'actorScriptSetDestination', 'actorScriptSetHudGuideStyle', 'actorScriptSetMessengerContactMode', 'actorScriptStartActionAndWait', 'actorScriptStartActionScript', 'actorScriptStartTimedAction', 'actorScriptTrackPlayerPrimaryObject', 'actorScriptTrackPlayerSecondaryObject', 'actorScriptYield'))
VM_UNIT_SHA256 = 'cded04462b1f2fbece0ee8d937f9d4fcfc8c659719094b92768715d9669fe8fb'
VOICE_HEADER_SHA256 = 'aa2f1e13463fa12431071933829006c58253832b6e4821745b61cb8fb2d5747c'

ROUND_SHA256 = '04b66b9b017ad0579b7149dddd1103109b272ba9540a33467842cf4ac843f5dd'
VOICE_NAMES = {35: 'VOICE_GENERAL_COMMAND_ROCK', 36: 'VOICE_GENERAL_COMMAND_SCISSORS', 37: 'VOICE_GENERAL_COMMAND_PAPER'}
SCOPE = ('Conditional complete reachable routine only. Native/deferred preconditions, valid operands, '
         'protected frames, current resources, registry/storage bounds and coherent samples remain obligations. '
         'External script CALLs are opaque invocations, not pure or preserving. No recognized utterance, '
         'successful gesture, round completion, actual win/loss, retail reachability or termination proof.')


def _normalize(instructions, entry, bindings):
    """Normalize complete reachable rows, preserving aliases and opaque CALL identities."""
    by = {row['offset']: row for row in instructions}
    incoming = incoming_edges(instructions)
    reached, pending = set(), [entry]
    while pending:
        offset = pending.pop()
        if offset in reached:
            continue
        if offset not in by or len(reached) >= 512:
            raise ValueError('Incomplete round or reachable instruction budget exceeded.')
        row = by[offset]
        reached.add(offset)
        if row['name'] == 'SCRIPT_OP_CALL':
            pending.append(offset + row['size'])
        else:
            pending.extend(e['offset'] for e in row['successors'])
    rows = [by[o] for o in sorted(reached)]
    if not rows or rows[0]['name'] != 'SCRIPT_OP_ENTER_FRAME' or [o['value'] for o in rows[0]['operands']] != [0, 13, 1]:
        raise ValueError('Unsupported complete round frame.')
    if not any(row['name'] == 'SCRIPT_OP_RETURN' for row in rows):
        raise ValueError('Round has no returning path.')
    expected = {}
    for row in rows:
        for edge in row['successors']:
            expected.setdefault(edge['offset'], set()).add(row['offset'])
    if any(incoming.get(row['offset'], set()) != expected.get(row['offset'], set()) for row in rows[1:]):
        raise ValueError('Competing round interior entry.')
    end = rows[-1]['offset'] + rows[-1]['size']
    if end - entry > 4096 or any(entry < o < end and o not in reached for o in by):
        raise ValueError('Unreviewed decoded rows or excessive physical span.')
    positions = {row['offset']: i for i, row in enumerate(rows)}
    roots, callees, selectors, strings, encoded = {}, {}, {}, {}, []
    for row in rows:
        name = row['name']
        values = [o['value'] for o in row['operands']]
        tags = [o['tag'] for o in row['operands']]
        if row['successors'] != _edges(row, values):
            raise ValueError('Incorrect complete instruction edges.')
        normalized = values[:]
        if name in ('SCRIPT_OP_LOAD_VARIABLE', 'SCRIPT_OP_STORE_VARIABLE', 'SCRIPT_OP_LOAD_ELEMENT', 'SCRIPT_OP_STORE_ELEMENT') and values[0] == 0:
            roots.setdefault(values[1], len(roots))
            normalized = [0, 'root-role-' + str(roots[values[1]])]
        elif name in ('SCRIPT_OP_JUMP', 'SCRIPT_OP_JUMP_IF_ZERO'):
            if values[0] not in positions:
                raise ValueError('Branch leaves complete round.')
            normalized = ['instruction', positions[values[0]]]
        elif name == 'SCRIPT_OP_PUSH_LITERAL' and tags == [3]:
            if values[0] < 0:
                raise ValueError('Negative boxed string index.')
            strings.setdefault(values[0], len(strings))
            normalized = ['opaque-string-index', strings[values[0]]]
        elif name == 'SCRIPT_OP_CALL':
            if entry <= values[0] < end:
                raise ValueError('Opaque external call targets the round interior.')
            callees.setdefault(values[0], len(callees))
            normalized = ['opaque-callee', callees[values[0]]]
        elif name == 'SCRIPT_OP_CALL_NATIVE':
            native = row.get('native', {})
            functions = native.get('functions', [])
            if len(functions) != 1 or functions[0] not in bindings:
                raise ValueError('Unknown round native.')
            function = functions[0]
            expected_inline = ['string_count'] if function == 'actorScriptEnqueueMessage' else []
            if native.get('inline_operands', []) != expected_inline:
                raise ValueError('Incorrect round inline operand metadata.')
            if type(native.get('address')) is not int or native['address'] != bindings[function]['address']:
                raise ValueError('Rebound round native.')
            identity = (function, native['address'])
            if values[0] in selectors and selectors[values[0]] != identity:
                raise ValueError('Ambiguous round native selector.')
            selectors[values[0]] = identity
            normalized = [function, native['address'], *values[1:]]
        encoded.append([row['offset'] - entry, name, normalized, tags])
    return dict(sha256=_digest(encoded), normalized=encoded, rows=rows, roots=roots, callees=callees, strings=strings,
                span_bytes=end-entry, reachable_bytes=sum(r['size'] for r in rows))


def rps_rounds(instructions, *, snapshot=None, root=ROOT):
    """Describe only complete normalized routines under one fresh source snapshot."""
    unresolved = dict(status='unresolved', rounds=[], source_bindings={}, scope=SCOPE)
    try:
        if set(NATIVE_BINDINGS) != REQUIRED_NATIVE_NAMES:
            raise ValueError('Incomplete or unexpected round native binding set.')
        reviews, preservation = context_snapshot(root=root) if snapshot is None else snapshot
        vm = preservation.get('context_vm_contract', {})
        if preservation.get('stale_vm_functions') or vm.get('current') is not True:
            raise ValueError('Stale VM context contract.')
        if fingerprint((root / 'src/ovl/script_vm.c').read_text()) != VM_UNIT_SHA256:
            raise ValueError('Stale complete VM implementation.')
        header = root / 'include/voice_phrases.h'
        block = re.search(r'typedef\s+enum\s+VoiceGeneralCommandValue\s*\{([^}]+)\}', code_only(header.read_text()))
        if block is None:
            raise ValueError('Missing voice selector enum.')
        pairs = re.findall(r'\b(VOICE_GENERAL_COMMAND_\w+)\s*=\s*(\d+)\b', block[1])
        names = {int(value): name for name,value in pairs}
        if len(names) != len(pairs):
            raise ValueError('Duplicate voice selector enum values.')
        if hashlib.sha256(header.read_bytes()).hexdigest() != VOICE_HEADER_SHA256 or any(names.get(v) != n for v,n in VOICE_NAMES.items()):
            raise ValueError('Stale voice selector domain.')
        for name, binding in NATIVE_BINDINGS.items():
            active = reviews.get('active', {}).get(name)
            if active is None or _digest(active) != binding['review_sha256']:
                raise ValueError('Stale round native contract: ' + name)
            if 'deferred_review_sha256' in binding:
                deferred = reviews.get('active_deferred', {}).get(name)
                if deferred is None or _digest(deferred) != binding['deferred_review_sha256']:
                    raise ValueError('Stale round deferred contract: ' + name)
        validate_instructions(instructions)
        snapshots = companion_snapshots(instructions, snapshot=(reviews, preservation), root=root)
        if snapshots.get('status') != 'conditional_snapshots':
            raise ValueError('Unresolved message snapshot producer bindings.')
        rounds = []
        for row in instructions:
            if row['name'] != 'SCRIPT_OP_ENTER_FRAME' or [o['value'] for o in row['operands']] != [0,13,1]:
                continue
            try:
                graph = _normalize(instructions, row['offset'], NATIVE_BINDINGS)
            except (ValueError, KeyError, TypeError, IndexError):
                continue
            if graph['sha256'] != ROUND_SHA256:
                continue
            snapshot_target = next((target for target, role in graph['callees'].items() if role == 5), None)
            message_snapshot = next((item for item in snapshots['snapshots'] if item['entry_offset'] == snapshot_target), None)
            if message_snapshot is None:
                continue
            root_roles = {role: slot for slot, role in graph['roots'].items()}
            snapshot_roles = {role: int(slot) for slot, role in message_snapshot['root_role_mapping'].items()}
            if root_roles.get(0) != snapshot_roles.get(1) or root_roles.get(1) != snapshot_roles.get(2):
                continue
            rounds.append(dict(_describe(graph), sampled_message_fields={'type':[0,root_roles[0]], 'argument0':[0,root_roles[1]]}, message_snapshot_binding=dict(
                entry_offset=snapshot_target, canonical_sha256=message_snapshot['canonical_sha256'],
                category_variable=[0,root_roles[0]], raw_sampled_message_argument0_variable=[0,root_roles[1]])))
        return dict(status='conditional_rounds', rounds=rounds, scope=SCOPE,
                    source_bindings=dict(native_reviews=NATIVE_BINDINGS, vm_unit_sha256=VM_UNIT_SHA256,
                                         voice_header_sha256=VOICE_HEADER_SHA256, vm_implementation_binding=vm,
                                         message_snapshot_source_bindings=snapshots['source_bindings']))
    except (ValueError, OSError, KeyError, TypeError, IndexError) as error:
        return dict(unresolved, reason=str(error))


def _describe(graph):
    rows = graph['rows']
    return dict(entry=rows[0]['offset'], instruction_count=len(rows), span_bytes=graph['span_bytes'],
                reachable_bytes=graph['reachable_bytes'], normalized_sha256=graph['sha256'],
                root_roles={str(role):slot for slot,role in graph['roots'].items()},
                local_roles={'phase':[1,0], 'choice':[1,1], 'random_sample':[1,2], 'lookup_result':[1,3]},
                phases={'0':'Wait for cached voice status five.', '1':'Status five observed; wait for a non-five sample before issuing the gesture request.',
                        '2':'Choice selected before gesture requests; waiting for stored message evaluation.'},
                random_partition={'bound':6, 'sample_0':0, 'sample_1_through_2':2, 'sample_3_through_5':1},
                choice_names={'0':'rock','1':'scissors','2':'paper'},
                choice_action_scripts={'0':136,'1':137,'2':138},
                outcome_tables={'local4':[2,1,0],'local7':[0,2,1],'local10':[1,0,2]},
                input_selection={'35':'local4','36':'local7','else':'local10'},
                conditional_outcome_names={'0':'chosen_side_wins','1':'chosen_side_loses','2':'tie'},
                conventional_outcome_preconditions={'valid_input_required':[35,36,37],'choice_index_range':[0,2]},
                message_scope='After an opaque external guard succeeds, stored type4 takes a separate exit. Otherwise phase2 compares stored argument0 with35, then36, and uses ELSE fallback; no category2 or exact37 guard.',
                requested_result_branches={'2':{'requested_action_script':174,'resets_phase':0,'role':'tie under valid RPS inputs'},
                    '0':{'effect_definition':0,'animation_track':174,'requested_action_script':142,'random_action_scripts':[145,163],'secondary_meter_adjustment':20},
                    '1':{'effect_definition':0,'animation_track':165,'requested_action_script':220,'random_action_scripts':[48,248],'secondary_meter_adjustment':10}},
                outcome_scope='Choice identities and conventional tie/win/loss interpretation require evaluated integer selector35..37 and coherent unchanged chosen index0..2. Fallback accepts other values; no actual gesture, winner or result execution is established.',
                phase_writes=[dict(instruction_offset=r['offset'],value=rows[i-1]['operands'][0]['value']) for i,r in enumerate(rows) if r['name']=='SCRIPT_OP_STORE_VARIABLE' and [v['value'] for v in r['operands']]==[1,0]],
                opaque_text_indices=list(graph['strings']),
                native_calls=[dict(instruction_offset=r['offset'],function=r['native']['functions'][0],address=r['native']['address']) for r in rows if r['name']=='SCRIPT_OP_CALL_NATIVE'],
                external_calls=[dict(instruction_offset=r['offset'],target_offset=r['operands'][0]['value'],opaque_identity=graph['callees'][r['operands'][0]['value']]) for r in rows if r['name']=='SCRIPT_OP_CALL'],
                gaps_interpreted=False, external_calls_preserve_context_proven=False, completion_proven=False)
