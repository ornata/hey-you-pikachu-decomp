"""Reconstruct the bounded retained pose map without claiming live playback.

Explicit entries precede eligible action definitions. The source table stops on
its source-pose sentinel, regardless of other bytes in that record or ELF size.
"""

import hashlib
import re
import struct

from tools.source_quality import code_only

from .context import ROOT
from .effects import enum_names


def reconstruct_pose_map(explicit, definitions, capacity=293, bias=256):
    """Model ordered insertion, retaining duplicate and unreachable candidates."""
    if type(capacity) is not int or not 1 <= capacity <= 4096:
        raise ValueError('pose-map capacity is outside the bounded model')
    slots = [None] * capacity
    candidates, count = [], 0

    def indices(key):
        mixed = ((((~(key >> 12) + (key >> 8)) & 0xFFFFFFFF) ^
                  (~(key >> 4) & 0xFFFFFFFF)) + key * 0x2E7) & 0xFFFFFFFF
        first = mixed % capacity
        index = first
        while True:
            yield index
            index = (index + 8 - (key & 7)) % capacity
            if index == first:
                return

    def lookup(key):
        for index in indices(key):
            row = slots[index]
            if row is None:
                return None
            if row['key'] == key:
                return row
        return None

    def add(source, destination, action, origin):
        nonlocal count
        if not (0 <= source <= 255 and 0 <= destination <= 255 and -32768 <= action <= 32767):
            raise ValueError('pose record lies outside its stored integer domain')
        key = source * 256 + destination
        value = (action + bias) & 0xFFFFFFFF
        row = dict(source_pose=source, destination_pose=destination,
                   action_id=action, key=key, map_value=value, origin=origin)
        row['transition_kind'] = ('pose_blend' if value == 254 else
                                  'direct_apply' if value == 253 else
                                  'bridge_action' if 0 <= action < 330 else
                                  'invalid_action_fallback')
        candidates.append(row)
        if value == 0:
            row['status'] = 'unresolved_zero_value'
        elif lookup(key) is not None:
            row['status'] = 'duplicate'
        elif count == capacity:
            row['status'] = 'full'
        else:
            for index in indices(key):
                if slots[index] is None:
                    row.update(status='inserted', slot=index)
                    slots[index] = row
                    count += 1
                    break
            else:
                row['status'] = 'unreachable_free_slot'

    for row in explicit:
        add(row['source_pose'], row['destination_pose'], row['action_id'], 'explicit')
    for row in definitions:
        if row['attributes'] & 0x40000 and row['initial_pose'] != row['final_pose']:
            add(row['initial_pose'], row['final_pose'], row['id'], 'action_definition')
    return dict(candidates=candidates, entries=[row for row in slots if row is not None],
                capacity=capacity, count=count,
                unresolved=any(row['status'] not in ('inserted', 'duplicate') for row in candidates))


def pose_transition_catalog(image, limit=256):
    """Authenticate a sentinel-bounded table and the fixed consumer's action domain."""
    if type(limit) is not int or not 1 <= limit <= 4096:
        raise ValueError('explicit pose-record budget is outside the bounded model')
    header = ROOT / 'include/motion_transitions.h'
    names = enum_names(header, 'POSE_TRANSITION_')
    direct = re.search(r'\bPOSE_TRANSITION_DIRECT_ACTION\s*=\s*(-?\d+)\b', code_only(header.read_text()))
    if direct is None or int(direct[1]) != -3:
        raise ValueError('direct pose-map encoding differs from the reviewed consumer')
    # The negative decoded direct-action identifier is checked independently of
    # enum_names, which deliberately reads only nonnegative encodings.
    if names != {48: 'POSE_TRANSITION_TABLE_END', 293: 'POSE_TRANSITION_MAP_CAPACITY',
                 256: 'POSE_TRANSITION_ACTION_BIAS', 254: 'POSE_TRANSITION_BLEND_VALUE'}:
        raise ValueError('pose-map encodings differ from the reviewed consumer')
    attributes = ROOT / 'include/action_attributes.h'
    if enum_names(attributes, 'ACTION_ATTRIBUTE_').get(0x40000) != 'ACTION_ATTRIBUTE_REGISTER_POSE_TRANSITION':
        raise ValueError('pose-map eligibility flag differs from the reviewed consumer')
    image.allowed = {'.boot', '.ovl'}
    base = image.symbol('gActionExplicitPoseTransitions')[1]
    explicit, raw_table = [], bytearray()
    for index in range(limit):
        raw, location = image.read_address(base + 4 * index, 4)
        raw_table.extend(raw)
        source, destination, action = struct.unpack('>BBh', raw)
        row = dict(**location, source_pose=source, destination_pose=destination, action_id=action,
                   sha256=hashlib.sha256(raw).hexdigest())
        if source == 48:
            sentinel = row
            break
        explicit.append(row)
    else:
        raise ValueError('explicit pose-table terminator not established within budget')
    base = image.symbol('gActionDefinitions')[1]
    definitions = []
    raw_definitions = bytearray()
    for index in range(330):
        raw, _ = image.read_address(base + 44 * index, 44)
        raw_definitions.extend(raw)
        identifier, = struct.unpack_from('>i', raw)
        if identifier != index:
            raise ValueError('pose-map action definition index disagrees with its record')
        definitions.append(dict(id=index, attributes=struct.unpack_from('>I', raw, 12)[0],
                                initial_pose=raw[18], final_pose=raw[19]))
    result = reconstruct_pose_map(explicit, definitions)
    result.update(explicit_records=explicit, sentinel=sentinel,
                  explicit_table_sha256=hashlib.sha256(raw_table).hexdigest(),
                  action_definitions_sha256=hashlib.sha256(raw_definitions).hexdigest(),
                  source_sha256={str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                                 for path in (header, attributes)},
                  scope='Static initialized-map model assuming successful backing allocation. Only sentinel-bounded explicit records and330 consumer-domain definitions are read. No live map, pose identity, resolver completion or rendered playback claim.')
    return result
