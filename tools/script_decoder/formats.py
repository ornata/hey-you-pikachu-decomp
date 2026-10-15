"""Bounded static decoding of VM bytes and the two distinct timed record formats.

No game execution or original script content is embedded here. Numeric layouts
come from the retained consumers; unresolved encodings remain visible.
"""

import math
import struct

VM_OPERANDS = {10: 2, 24: 1, 25: 2, 26: 1, 27: 1, 28: 1, 29: 1, 32: 3, 33: 2, 34: 2}
VM_HANDLERS = {
    **dict.fromkeys((3, 4, 17), 'scriptVmApplyUnaryOperator'),
    **dict.fromkeys(range(5, 10), 'scriptVmApplyArithmeticOperator'),
    **dict.fromkeys(range(11, 17), 'scriptVmApplyComparisonOperator'),
    **dict.fromkeys((18, 19), 'scriptVmApplyComparisonOperator'),
    **dict.fromkeys(range(20, 24), 'scriptVmApplyBitwiseOperator'),
    27: 'actorScriptDispatchRegisteredCommand',
}
LAUNCH_NAMES = (
    'player_or_forward', 'forward', 'offset_heading', 'steep_upward',
    'downward', 'target_position', 'randomized_heading', 'scatter_with_effects',
)


def floating(raw):
    """Preserve nonfinite encodings without emitting nonstandard JSON numbers."""
    value = struct.unpack('>f', raw)[0]
    return value if math.isfinite(value) else None


def packed(raw):
    """Describe a five-byte value without resolving strings or guessing tag2's origin."""
    tag, bits = raw[0], raw[1:]
    result = dict(tag=tag, payload_hex=bits.hex())
    if tag in (0, 2):
        result.update(kind='integer' if tag == 0 else 'unresolved_integer_like',
                      value=int.from_bytes(bits, 'big', signed=True))
    elif tag == 1:
        result.update(kind='float', value=floating(bits))
    elif tag in (3, 4):
        result.update(kind='string_index' if tag == 3 else 'borrowed_string_pointer',
                      value=int.from_bytes(bits, 'big', signed=tag == 3))
    else:
        result['kind'] = 'unknown'
    return result


def integer_operand(value):
    """Apply only established numeric conversions, excluding invalid float casts."""
    if value['tag'] in (0, 2):
        return value['value']
    number = value.get('value')
    if value['tag'] == 1 and number is not None and -2147483648 <= number < 2147483648:
        return int(number)
    return None


def decode_vm(data, names, entries=(0,), natives=None, limit=10000):
    """Walk bounded static successors; native stack effects and returns stay opaque."""
    rows, occupied, pending, errors = {}, {}, list(reversed(entries)), []
    while pending:
        if len(rows) >= limit:
            errors.append('instruction budget exhausted; remaining paths are unparsed')
            break
        offset = pending.pop()
        if offset in rows:
            continue
        if offset < 0 or offset >= len(data):
            errors.append(f'control-flow target {offset:#x} is outside the supplied window')
            continue
        if offset in occupied:
            errors.append(f'control-flow target {offset:#x} enters an operand at {occupied[offset]:#x}')
            continue
        opcode = data[offset]
        if opcode > 35:
            rows[offset] = dict(offset=offset, opcode=opcode, name='INVALID_OPCODE',
                                size=1, operands=[], successors=[], handler='actorRunScriptVm')
            occupied[offset] = offset
            errors.append(f'{offset:#x}: unsupported opcode {opcode:#x}; this path stops')
            continue
        size = 1 + VM_OPERANDS.get(opcode, 0) * 5
        native, opaque_native = None, False
        if opcode == 27 and offset + size <= len(data):
            selector = integer_operand(packed(data[offset + 1:offset + 6]))
            native = natives.get(selector) if natives is not None else None
            if native and native['address'] and 'inline_operands' in native:
                size += 5 * len(native['inline_operands'])
            else:
                opaque_native = True
                errors.append(f'{offset:#x}: native selector {selector} has no reviewed inline layout; this path stops')
        if offset + size > len(data):
            errors.append(f'{offset:#x}: truncated operands for opcode {opcode}')
            continue
        if any(i in occupied for i in range(offset, offset + size)):
            errors.append(f'{offset:#x}: instruction overlaps an already decoded instruction')
            continue
        operands = [packed(data[i:i + 5]) for i in range(offset + 1, offset + size, 5)]
        row = dict(offset=offset, opcode=opcode, size=size, operands=operands,
                   name=names.get(opcode, f'IGNORED_BYTE_{opcode}'),
                   handler=VM_HANDLERS.get(opcode, 'actorRunScriptVm'), successors=[])
        row['operand_roles'] = {10: ['frame', 'variable_index'], 24: ['value'],
                                25: ['frame', 'variable_index'], 26: ['target_offset'],
                                27: ['native_selector'], 28: ['target_offset'],
                                29: ['target_offset'], 32: ['argument_count', 'local_count', 'frame_index'],
                                33: ['frame', 'first_element_index'], 34: ['frame', 'first_element_index']}.get(opcode, [])
        if native and 'inline_operands' in native:
            row['operand_roles'] += native['inline_operands']
        after = offset + size
        if opcode not in (28, 30, 35) and not opaque_native:
            row['successors'].append(dict(kind='return_continuation' if opcode == 26 else
                                                     'resume_or_fallthrough' if opcode == 27 else 'fallthrough', offset=after))
        if opcode in (26, 28, 29):
            target = integer_operand(operands[0])
            row['target'] = target
            if target is None:
                errors.append(f'{offset:#x}: unresolved or invalid branch operand')
            else:
                row['successors'].append(dict(kind='call' if opcode == 26 else 'branch', offset=target))
        if opcode == 30:
            row['dynamic_return'] = True
        if opcode == 27:
            selector = integer_operand(operands[0])
            row['native_selector'] = selector
            if natives is not None:
                if selector is None or selector not in natives or not natives[selector]['address']:
                    errors.append(f'{offset:#x}: native selector is outside the supplied table or selects null')
                else:
                    row['native'] = natives[selector]
        rows[offset] = row
        occupied.update((i, offset) for i in range(offset, offset + size))
        # Decode fallthrough before forward targets, catching jumps into its operands.
        pending.extend(edge['offset'] for edge in reversed(row['successors']))
    unparsed = []
    for i in range(len(data)):
        if i not in occupied:
            if not unparsed or unparsed[-1][1] != i:
                unparsed.append([i, i + 1])
            else:
                unparsed[-1][1] += 1
    return dict(format='vm', instructions=[rows[i] for i in sorted(rows)],
                diagnostics=list(dict.fromkeys(errors)), unparsed_ranges=unparsed,
                scope='Bounded static successors, not execution, stack verification or complete resource discovery.')


def decode_records(data, names, kind, address=None):
    """Decode timed records without following arbitrary target pointers or guessing payloads."""
    if kind not in ('cue', 'sequence'):
        raise ValueError('unknown timed record format')
    stride = 20 if kind == 'cue' else 16
    rows, errors, consumed = [], [], 0
    for offset in range(0, len(data), stride):
        if len(rows) >= 10000:
            errors.append('record budget exhausted; remainder is unparsed')
            break
        if offset + stride > len(data):
            errors.append(f'{offset:#x}: truncated {stride}-byte {kind} record')
            break
        code, = struct.unpack_from('>i', data, offset)
        raw = data[offset + 8:offset + stride]
        row = dict(offset=offset, size=stride, opcode=code, time=floating(data[offset + 4:offset + 8]),
                   time_bits=data[offset + 4:offset + 8].hex(), payload_hex=raw.hex(),
                   name=names.get(code, 'ACTION_KIND_TAG' if kind == 'cue' and code < 19 else 'INVALID_OPCODE'))
        consumed = offset + stride
        if kind == 'cue':
            row['time_applies'] = code >= 19
            row['handler'] = 'actorUpdateActionScript'
            if code == 25:
                selector = int.from_bytes(raw[:4], 'big', signed=True)
                row['launch'] = dict(kind=selector, name=LAUNCH_NAMES[selector] if 0 <= selector < 8 else 'unsupported',
                                     speed=floating(raw[4:8]))
                if not 0 <= selector < 8:
                    errors.append(f'{offset:#x}: unsupported launch kind {selector}')
            elif code == 46:
                target = int.from_bytes(raw[:4], 'big')
                row['jump'] = dict(address=target, reset_clock=floating(raw[4:8]), reset_bits=raw[4:8].hex())
                if address is not None and address <= target < address + len(data):
                    relative = target - address
                    row['jump']['offset'] = relative
                    if relative % stride:
                        errors.append(f'{offset:#x}: cue jump enters a record at {relative:#x}')
            elif code in (44, 45):
                row['clock'] = dict(amount=floating(raw[4:8]))
                if code == 45:
                    row['clock']['count'] = int.from_bytes(raw[:4], 'big', signed=True)
            if code > 49:
                errors.append(f'{offset:#x}: unsupported cue operation {code}')
                rows.append(row)
                break
            # STOP ends one program, not a tagged collection; only49 ends selection.
            rows.append(row)
            if code == 49:
                break
        else:
            args = list(struct.unpack('>2i', raw))
            row['handler'] = {0: 'actionScriptCommandStartAction', 1: 'actionScriptCommandPlaySound',
                              2: 'actionScriptCommandStopAfterIterations',
                              3: 'actionScriptCommandWaitForIterations'}.get(code, 'actionScriptUpdate')
            row['arguments'] = args
            if not 0 <= code <= 4:
                errors.append(f'{offset:#x}: unsupported sequencer opcode {code}')
            rows.append(row)
            if not 0 <= code < 4:
                break
    return dict(format=kind, instructions=rows, diagnostics=errors,
                unparsed_ranges=[[consumed, len(data)]] if consumed < len(data) else [],
                scope='Bounded record listing, not playback, timing validation or resource lifetime verification.')
