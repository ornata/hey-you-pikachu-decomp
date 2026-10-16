"""Connect reviewed VM action commands to timed programs and motion definitions.

All resource bytes and debug labels come from the user's authenticated local ROM.
Outputs are local evidence, not rendered-animation or gameplay-reachability proof.
"""

import hashlib
import struct
import re

from .formats import decode_records
from .stack import incoming_edges, recover_arguments
from .context import ROOT, opcode_names
from .effects import enum_names
from tools.source_quality import code_only

# Argument order is push order. Deferred commands leave the operands on the
# stack until their selected action handler initializes on a later actor update.
CONTRACTS = {
    'actorScriptStartActionAndWait': dict(arguments=['script_id'], consumption='deferred', scheduling='wait_for_action', result='zero_when_sequencer_stops'),
    'actorScriptStartInterruptibleAction': dict(arguments=['script_id'], consumption='deferred', scheduling='wait_for_action', result='pending_message_or_zero_when_sequencer_stops'),
    'actorScriptStartActionScript': dict(arguments=['script_id'], consumption='immediate', scheduling='yield', result='zero'),
    'actorScriptStartActionWithMode': dict(arguments=['script_id', 'default_kind'], consumption='immediate', scheduling='yield', result='zero'),
    'actorScriptStartTimedAction': dict(arguments=['script_id', 'duration_seconds'], consumption='immediate', scheduling='yield', result='zero'),
    'actorScriptSetDefaultActionMode': dict(arguments=['default_kind'], consumption='immediate', scheduling='continue', result='zero'),
}


def action_calls(instructions):
    """Retain unresolved producers and separate command scheduling from stack results."""
    incoming = incoming_edges(instructions)
    frames = {row['offset']: row for row in instructions if row['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    result = []
    for index, call in enumerate(instructions):
        functions = call.get('native', {}).get('functions', [])
        if len(functions) != 1 or functions[0] not in CONTRACTS:
            continue
        contract = CONTRACTS[functions[0]]
        row = dict(instruction_offset=call['offset'], function=functions[0], contract=contract)
        try:
            values = recover_arguments(instructions, index, len(contract['arguments']), incoming=incoming, allow_float=True, allow_symbolic=True, frame_entries=frames)
            row.update(status='recovered_arguments', arguments=dict(zip(contract['arguments'], values)))
            value = row['arguments'].get('script_id', {})
            if value.get('kind') in ('integer', 'float') and -2147483648 <= value['value'] < 2147483648:
                row['constant_script_id'] = int(value['value'])
        except ValueError as error:
            row.update(status='unresolved', reason=str(error))
        result.append(row)
    return result


def script_action_flag_projection(word, names):
    """Project active action attributes, without including actor inhibition state."""
    flags = (16 if word & 0x200 else 0) + (8 if word & 0x100 else 0)
    if word & 0xF0:
        flags += 2 if word & 0x800 else 4
    flags += (32 if word & 0x20000000 else 0) + (1 if word & 0x400 else 0) + (64 if word & 0x800 else 0)
    if names != {16: 'SCRIPT_ACTION_HIGH_UPWARD_MOTION_LIMIT',
                 32: 'SCRIPT_ACTION_BYPASS_TERRAIN_AND_GRAVITY'}:
        raise ValueError('script action flag meanings differ from the reviewed projection')
    return dict(flags=flags, known_names=[name for bit, name in sorted(names.items()) if flags & bit],
                unresolved_flag_bits=flags & ~0x30,
                scope='Attribute projection for a live creature with active playback; excludes behavior inhibition and disabled-playback defaults.')


def motion_definition_metadata(raw, attributes, modes, meter_policies=None, script_flags=None, identities=None):
    """Describe a fixed resource record, preserving bits without recovered meanings."""
    if len(raw) != 44:
        raise ValueError('action definition must contain exactly 44 bytes')
    mask = next((value for value, name in attributes.items()
                 if name == 'ACTION_ATTRIBUTE_PLAYBACK_MODE_MASK'), None)
    if mask != 7:
        raise ValueError('action playback selector mask differs from reviewed source')
    meter_mask = next((value for value, name in attributes.items()
                       if name == 'ACTION_ATTRIBUTE_METER_POLICY_MASK'), 0)
    if meter_mask and (meter_mask != 0xC0000000 or meter_policies is None or
                       set(meter_policies) != {0, 0x40000000, 0x80000000, 0xC0000000}):
        raise ValueError('action meter policy differs from the reviewed source')
    flags = {value: name for value, name in attributes.items()
             if value != mask and name != 'ACTION_ATTRIBUTE_METER_POLICY_MASK'}
    if any(value <= 0 or value > 0xFFFFFFFF or value & (value - 1) for value in flags):
        raise ValueError('recovered action flags must be distinct single bits')
    word, = struct.unpack_from('>I', raw, 12)
    known = mask | meter_mask
    for bit in flags:
        known |= bit
    mode = word & mask
    result = dict(definition_sha256=hashlib.sha256(raw).hexdigest(),
                attribute_word=word, playback_mode=mode,
                playback_mode_name=modes.get(mode),
                attribute_names=[name for bit, name in sorted(flags.items()) if word & bit],
                unresolved_attribute_bits=word & (~known & 0xFFFFFFFF),
                motion_archive_id=struct.unpack_from('>h', raw, 16)[0],
                initial_pose_id=raw[18], final_pose_id=raw[19],
                continuation_action_id=struct.unpack_from('>h', raw, 20)[0],
                initializer_address=struct.unpack_from('>I', raw, 28)[0],
                update_address=struct.unpack_from('>I', raw, 32)[0],
                scope='Original resource fields and recovered attribute meanings; unknown bits are retained. No playback, callback or transition completion is implied.')
    if meter_mask:
        policy = word & meter_mask
        result.update(action_meter_policy=policy, action_meter_policy_name=meter_policies[policy])
    if script_flags is not None:
        result['script_action_flag_projection'] = script_action_flag_projection(word, script_flags)
    if identities is not None:
        result['motion_action_name'] = identities.get(struct.unpack_from('>i', raw)[0])
        result['continuation_action_name'] = identities.get(result['continuation_action_id'])
    return result



def action_resources(image, identifiers, record_limit=4096):
    """Read requested table slots and END-terminated linear streams with explicit bounds."""
    if not identifiers:
        return []
    table = image.symbol('gActionScriptPrograms')
    if not table[2] or table[2] % 4 or not 0 < record_limit <= 4096:
        raise ValueError('action program table needs a bounded linked view and record budget')
    definitions = image.symbol('gActionDefinitions')
    image.allowed = {'.boot', '.ovl'}
    results = []
    attributes = enum_names(ROOT / 'include/action_attributes.h', 'ACTION_ATTRIBUTE_')
    modes = enum_names(ROOT / 'include/motion_playback_types.h', 'MOTION_PLAYBACK_')
    meter_policies = enum_names(ROOT / 'include/action_attributes.h', 'ACTION_METER_POLICY_')
    script_flags = enum_names(ROOT / 'include/script_action_flags.h', 'SCRIPT_ACTION_')
    identities = enum_names(ROOT / 'include/motion_action_ids.h', 'MOTION_ACTION_')
    names = {int(value): name for name, value in re.findall(r'\b(ACTION_SCRIPT_\w+)\s*=\s*(-?\d+)\b', code_only((ROOT / 'include/action_script_ids.h').read_text()))}
    for identifier in sorted(identifiers):
        row = dict(script_id=identifier, table_view_bytes=table[2],
                   scope='Literal request or static return candidate; ordinary IDs use the linked table view, -1 uses authenticated sentinel backing. No complete allocation or execution claim.')
        if identifier in names:
            row['recovered_name'] = names[identifier]
        try:
            if identifier == -1:
                # The fixed retail consumer indexes one word before its declared
                # table view. Authenticate both the null word and its provenance
                # as the final END operand of the last positive-ID program.
                raw, slot = image.read_address(table[1] - 4, 4)
                end, _ = image.read_address(table[1] - 16, 16)
                last, _ = image.read_address(table[1] + table[2] - 4, 4)
                if raw != b'\0' * 4 or struct.unpack_from('>i', end)[0] != 4 or struct.unpack('>I', last)[0] != table[1] - 32:
                    raise ValueError('no-program sentinel is not backed by its retained null END operand')
                row.update(status='no_program', address=0, table_slot=slot, consumed_bytes=0,
                           backing='Final unused END operand of the last positive-ID timed program.',
                           portability='Negative indexing lies outside the declared C array view; host ports must handle the sentinel explicitly.')
                results.append(row)
                continue
            if not 0 <= identifier < table[2] // 4:
                raise ValueError('script ID lies outside the bounded linked table view')
            raw, slot = image.read_address(table[1] + identifier * 4, 4)
            address = struct.unpack('>I', raw)[0]
            row.update(address=address, table_slot=slot)
            data = bytearray()
            for index in range(record_limit):
                command, location = image.read_address(address + index * 16, 16)
                if index == 0:
                    row['location'] = location
                data.extend(command)
                opcode = struct.unpack_from('>i', command)[0]
                if opcode == 4 or not 0 <= opcode < 4:
                    break
            decoded = decode_records(bytes(data), opcode_names('sequence'), 'sequence')
            if not decoded['instructions'] or decoded['instructions'][-1]['opcode'] != 4:
                decoded['diagnostics'].append('END not established within the record budget')
            motions = []
            for command in decoded['instructions']:
                if command['opcode'] != 0:
                    continue
                motion = command['arguments'][0]
                if not 0 <= motion < 330:
                    decoded['diagnostics'].append(f'motion action {motion} is outside the retained consumer domain')
                    continue
                definition, _ = image.read_address(definitions[1] + motion * 44, 44)
                stored_id, name_address = struct.unpack_from('>iI', definition)
                if stored_id != motion:
                    raise ValueError('motion definition ID disagrees with its requested index')
                # String reads are bounded individually; no neighboring allocation
                # size or encoding fallback is inferred from a missing terminator.
                name = bytearray()
                for offset in range(256):
                    byte, _ = image.read_address(name_address + offset, 1)
                    if byte == b'\0':
                        break
                    name.extend(byte)
                else:
                    raise ValueError('motion debug label exceeds its bounded read')
                metadata = motion_definition_metadata(definition, attributes, modes, meter_policies, script_flags, identities)
                for role in ('initializer', 'update'):
                    target = metadata[role + '_address']
                    metadata[role + '_functions'] = image.functions(target) if target else []
                motions.append(dict(command_offset=command['offset'], motion_action_id=motion,
                                    debug_label=name.decode('euc_jp'), debug_label_bytes=name.hex(),
                                    definition_address=definitions[1] + motion * 44,
                                    definition=metadata,
                                    requested_kind=command['arguments'][1],
                                    inherits_default_kind=command['arguments'][1] == 18))
            row.update(status='decoded' if not decoded['diagnostics'] else 'unresolved',
                       consumed_bytes=len(data), stream_sha256=hashlib.sha256(data).hexdigest(),
                       decoded=decoded, motion_requests=motions)
        except (ValueError, KeyError, IndexError, struct.error, UnicodeError) as error:
            row.update(status='unresolved', reason=str(error))
        results.append(row)
    return results
