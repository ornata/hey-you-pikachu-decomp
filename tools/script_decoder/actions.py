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
    result = []
    for index, call in enumerate(instructions):
        functions = call.get('native', {}).get('functions', [])
        if len(functions) != 1 or functions[0] not in CONTRACTS:
            continue
        contract = CONTRACTS[functions[0]]
        row = dict(instruction_offset=call['offset'], function=functions[0], contract=contract)
        try:
            values = recover_arguments(instructions, index, len(contract['arguments']), incoming=incoming, allow_float=True)
            row.update(status='recovered_arguments', arguments=dict(zip(contract['arguments'], values)))
            value = row['arguments'].get('script_id', {})
            if value.get('kind') in ('integer', 'float') and -2147483648 <= value['value'] < 2147483648:
                row['constant_script_id'] = int(value['value'])
        except ValueError as error:
            row.update(status='unresolved', reason=str(error))
        result.append(row)
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
    names = {int(value): name for name, value in re.findall(r'\b(ACTION_SCRIPT_\w+)\s*=\s*(\d+)\b', code_only((ROOT / 'include/action_script_ids.h').read_text()))}
    for identifier in sorted(identifiers):
        row = dict(script_id=identifier, table_view_bytes=table[2],
                   scope='Requested slot within the linked table view; no complete allocation or execution claim.')
        if identifier in names:
            row['recovered_name'] = names[identifier]
        try:
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
                motions.append(dict(command_offset=command['offset'], motion_action_id=motion,
                                    debug_label=name.decode('euc_jp'), debug_label_bytes=name.hex(),
                                    definition_address=definitions[1] + motion * 44,
                                    requested_kind=command['arguments'][1],
                                    inherits_default_kind=command['arguments'][1] == 18))
            row.update(status='decoded' if not decoded['diagnostics'] else 'unresolved',
                       consumed_bytes=len(data), stream_sha256=hashlib.sha256(data).hexdigest(),
                       decoded=decoded, motion_requests=motions)
        except (ValueError, KeyError, IndexError, struct.error, UnicodeError) as error:
            row.update(status='unresolved', reason=str(error))
        results.append(row)
    return results
