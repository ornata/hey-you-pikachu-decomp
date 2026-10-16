"""Link item creation operands to the authenticated retained item-kind catalog.

Names are borrowed display labels read from the local ROM, not invented enum
identities. Catalog membership and a nonnull model do not prove creation safety,
resource readiness, ownership, placement or gameplay reachability.
"""

import hashlib
import math
import re
import struct

from .context import ROOT
from .stack import incoming_edges, recover_arguments
from tools.source_quality import code_only

_COUNT = 166
_STRIDE = 0x74
_TABLE_SHA256 = '9d0236268ce71a9aa43dd18458974d00f57995483ac75075270fed209557da14'
_CREATE_NATIVES = {'actorScriptCreateItem', 'actorScriptCreateStationaryItem'}
_ARGUMENT_NAMES = ('x', 'y', 'z', 'pitch_degrees', 'yaw_degrees', 'kind')


def storage_class_names():
    """Read explicitly named toolbox policies without naming other raw classes."""
    code = code_only((ROOT / 'include/item_kind.h').read_text())
    entries = re.findall(r'\b(ITEM_STORAGE_CLASS_\w+)\s*=\s*(\d+)\s*(?=[,}])', code)
    names = {int(value): name for name, value in entries}
    if len(names) != len(entries):
        raise ValueError('duplicate item storage class value')
    return names


def state_flag_names():
    """Read only explicit single-bit members of the recovered ItemStateFlag enum."""
    code = code_only((ROOT / 'include/item_types.h').read_text())
    match = re.search(r'\btypedef\s+enum\s+ItemStateFlag\s*\{([^}]+)\}\s*ItemStateFlag\s*;', code)
    if match is None:
        raise ValueError('missing ItemStateFlag enum')
    names, identifiers = {}, set()
    for entry in match[1].split(','):
        if not entry.strip():
            continue
        member = re.fullmatch(r'\s*(ITEM_STATE_\w+)\s*=\s*(0[xX][0-9a-fA-F]+|\d+)\s*', entry)
        if member is None:
            raise ValueError('item state flags require explicit numeric single-bit values')
        name, raw = member.groups()
        value = int(raw, 0)
        if not 0 < value <= 0x80000000 or value & (value - 1):
            raise ValueError('item state flag is not a 32-bit single-bit value')
        if value in names or name in identifiers:
            raise ValueError('duplicate item state flag')
        names[value] = name
        identifiers.add(name)
    if not names:
        raise ValueError('empty ItemStateFlag enum')
    return names


def interaction_response_categories():
    """Keep query selectors separate from their returned unsigned response codes."""
    code = code_only((ROOT / 'include/item_response.h').read_text())
    match = re.search(r'\btypedef\s+enum\s+ItemInteractionResponseCategory\s*\{([^}]+)\}\s*ItemInteractionResponseCategory\s*;', code)
    if match is None:
        raise ValueError('missing ItemInteractionResponseCategory enum')
    names, identifiers = {}, set()
    for entry in match[1].split(','):
        if not entry.strip():
            continue
        member = re.fullmatch(r'\s*(ITEM_RESPONSE_\w+)\s*=\s*(0[xX][0-9a-fA-F]+|\d+)\s*', entry)
        if member is None:
            raise ValueError('item response categories require explicit numeric selectors')
        name, raw = member.groups()
        selector = int(raw, 0)
        if not 1 <= selector <= 5 or selector in names or name in identifiers:
            raise ValueError('invalid or duplicate item response category')
        names[selector] = name
        identifiers.add(name)
    if not names:
        raise ValueError('empty ItemInteractionResponseCategory enum')
    return [dict(category_id=selector, enum_name=names.get(selector),
                 naming_status='named' if selector in names else 'unresolved',
                 record_byte_offset=0x17 + selector, result_domain='unsigned_u8_response_code')
            for selector in range(1, 6)]


def item_kind_catalog(image, progress_fields=None):
    """Read the bounded 166-entry catalog and optional bounded display names.

    ItemKind supplies the record stride, model and name pointers, collection
    score, notebook display-frame selector, collection-response group and five
    interaction-category response values, authored state-flag defaults and search
    priority maximum/replenishment. Names and query
    values are independent of whether a model exists. Optional progress metadata
    labels availability selectors without asserting current item possession.
    """
    image.allowed = {'.boot', '.ovl'}
    base = image.symbol('gItemKinds')[1]
    data, location = image.read_address(base, _COUNT * _STRIDE)
    if hashlib.sha256(data).hexdigest() != _TABLE_SHA256:
        raise ValueError('retained item-kind table differs')
    names = {}

    def display_name(address):
        if address in names:
            return names[address]
        row = dict(address=address)
        if not address:
            row['status'] = 'null'
        else:
            try:
                text = bytearray()
                for offset in range(256):
                    byte, _ = image.read_address(address + offset, 1)
                    if byte == b'\0':
                        row.update(status='decoded', text=text.decode('euc_jp'))
                        break
                    text.extend(byte)
                else:
                    row.update(status='unresolved', reason='Display name exceeds the 256-byte read bound.')
            except (ValueError, UnicodeError) as error:
                row.update(status='unresolved', reason=str(error))
        names[address] = row
        return row

    class_names = storage_class_names()
    flag_names = state_flag_names()
    known_flag_mask = sum(flag_names)
    field_names = {row['field_id']: row.get('enum_name')
                   for row in (progress_fields or {}).get('fields', [])}
    rows = []
    for kind in range(_COUNT):
        offset = kind * _STRIDE
        identifier, = struct.unpack_from('>i', data, offset)
        if identifier != kind:
            raise ValueError('item-kind catalog index and stored identifier disagree')
        state_flags, = struct.unpack_from('>I', data, offset + 4)
        priority_maximum, priority_rate = struct.unpack_from('>ff', data, offset + 0x10)
        model, = struct.unpack_from('>I', data, offset + 0x54)
        alternate, = struct.unpack_from('>I', data, offset + 0x5C)
        default, = struct.unpack_from('>I', data, offset + 0x64)
        display_material_frame, = struct.unpack_from('>h', data, offset + 0x3E)
        collection_value, = struct.unpack_from('>i', data, offset + 0x6C)
        collection_response_category, = struct.unpack_from('>b', data, offset + 0x70)
        item_class, availability_field = struct.unpack_from('>bh', data, offset + 0x71)
        interaction_responses = {str(category): data[offset + 0x17 + category]
                                 for category in range(1, 6)}
        rows.append(dict(item_kind=kind, configuration=base + offset, model_template=model,
                         initial_state_flags=state_flags,
                         initial_state_flag_names=[name for bit, name in sorted(flag_names.items())
                                                   if state_flags & bit],
                         initial_state_unresolved_flags=state_flags & (~known_flag_mask & 0xFFFFFFFF),
                         search_priority_maximum=priority_maximum,
                         search_priority_replenishment_per_second=priority_rate,
                         collection_value=collection_value, display_material_frame=display_material_frame,
                         collection_response_category=collection_response_category,
                         item_class=item_class, item_class_name=class_names.get(item_class),
                         availability_field=availability_field,
                         availability_field_name=field_names.get(availability_field),
                         interaction_responses=interaction_responses,
                         has_model_template=bool(model),
                         display_names=dict(alternate=display_name(alternate), default=display_name(default))))
    return dict(count=_COUNT, stride=_STRIDE, entries=rows,
                interaction_response_categories=interaction_response_categories(),
                table=dict(name='gItemKinds', size=len(data), sha256=_TABLE_SHA256, **location),
                initial_state_flags_scope='Authored ItemKind stateFlags before creation adds runtime state bits; named single-bit ItemStateFlag members and an explicit unresolved mask. These are not RuntimeObjectFlags or an observed live item state.',
                search_priority_scope='Initial maximum and per-second replenishment of item search-selection priority; these are not plant growth or physical dimensions. Runtime selection can reset priority, and this metadata does not establish a live value or selection eligibility.',
                scope='Retained catalog identities, borrowed display labels, collection scores, signed notebook display-frame selectors, signed collection-presentation response groups, signed storage classes and availability selectors, and unsigned interaction-category response values. Response categories do not establish transformation causes. Kinds 0 and 165 have null model templates; membership alone is not a valid creation contract.')


def item_creation_requests(instructions, catalog):
    """Recover X/Y/Z/pitch/yaw/kind expressions for the two creation natives."""
    incoming = incoming_edges(instructions)
    frames = {row['offset']: row for row in instructions if row['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    known = {row['item_kind']: row for row in catalog['entries']}
    rows = []
    for index, instruction in enumerate(instructions):
        functions = instruction.get('native', {}).get('functions', [])
        if len(functions) != 1 or functions[0] not in _CREATE_NATIVES:
            continue
        row = dict(instruction_offset=instruction['offset'], function=functions[0],
                   scope='Structural operand/catalog link; complete CFG, numeric types, operand floor, model/resources, callbacks, allocation and placement preconditions remain separate obligations.')
        try:
            arguments = recover_arguments(instructions, index, 6, incoming=incoming, allow_float=True,
                                          allow_symbolic=True, frame_entries=frames)
            row.update(status='recovered_arguments', argument_expressions=dict(zip(_ARGUMENT_NAMES, arguments)))
            value = arguments[-1]
        except ValueError as error:
            row.update(status='unresolved_arguments', unresolved_arguments_reason=str(error))
            try:
                # Kind is the top operand. Its producer can be established even
                # when an earlier coordinate producer cannot be summarized.
                value = recover_arguments(instructions, index, 1, incoming=incoming, allow_float=True,
                                          allow_symbolic=True, frame_entries=frames)[0]
            except ValueError as kind_error:
                row['unresolved_kind_reason'] = str(kind_error)
                rows.append(row)
                continue
            row['status'] = 'recovered_kind_only'
        row['kind_expression'] = value
        if value['kind'] in ('integer', 'float'):
            number = value['value']
            if math.isfinite(number) and -2147483648 <= number < 2147483648:
                kind = int(number)
                row.update(constant_item_kind=kind, within_retained_catalog=kind in known)
                if kind in known:
                    row['item_configuration'] = known[kind]
                    row['has_model_template'] = known[kind]['has_model_template']
            else:
                row['numeric_conversion'] = 'outside_supported_s32_conversion'
        rows.append(row)
    return rows


def item_response_requests(instructions, catalog):
    """Link constant kind queries without treating live handles as catalog IDs."""
    incoming = incoming_edges(instructions)
    frames = {row['offset']: row for row in instructions if row['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    known = {row['item_kind']: row for row in catalog['entries']}
    categories = {row['category_id']: row for row in catalog['interaction_response_categories']}
    natives = {'actorScriptGetLiveItemResponse': 'item_handle',
               'actorScriptGetItemKindResponse': 'item_kind'}
    rows = []

    def constant(expression):
        if expression.get('kind') in ('integer', 'float'):
            number = expression['value']
            if math.isfinite(number) and -2147483648 <= number < 2147483648:
                return int(number)
        return None

    for index, instruction in enumerate(instructions):
        functions = instruction.get('native', {}).get('functions', [])
        if len(functions) != 1 or functions[0] not in natives:
            continue
        function = functions[0]
        target = natives[function]
        row = dict(instruction_offset=instruction['offset'], function=function,
                   query_target=target,
                   scope='Structural query argument/catalog link, not query safety, execution or a live response observation. Numeric conversions, operand floors, valid VM/actor state and live-item ownership remain separate obligations. Returned codes are not category selectors, booleans, collection groups or transformation causes.')
        try:
            category, item = recover_arguments(instructions, index, 2, incoming=incoming,
                                               allow_float=True, allow_symbolic=True, frame_entries=frames)
        except ValueError as error:
            row.update(status='unresolved_arguments', reason=str(error))
            rows.append(row)
            continue
        row.update(status='recovered_arguments', category_expression=category,
                   **{target + '_expression': item})
        selector = constant(category)
        if selector is None and category.get('kind') in ('integer', 'float'):
            row['category_numeric_conversion'] = 'outside_supported_s32_conversion'
        if selector is not None:
            row.update(constant_category=selector, category_supported=selector in categories)
            if selector in categories:
                row['category_metadata'] = categories[selector]
        if target == 'item_kind':
            kind = constant(item)
            if kind is None and item.get('kind') in ('integer', 'float'):
                row['item_kind_numeric_conversion'] = 'outside_supported_s32_conversion'
            if kind is not None:
                row.update(constant_item_kind=kind, within_retained_catalog=kind in known)
                if selector in categories and kind in known:
                    row['item_configuration'] = known[kind]
                    row['response_code'] = known[kind]['interaction_responses'][str(selector)]
        rows.append(row)
    return rows
