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


def item_kind_catalog(image, progress_fields=None):
    """Read the bounded 166-entry catalog and optional bounded display names.

    ItemKind supplies the record stride, model and name pointers, collection
    score, notebook display-frame selector, collection-response group and five
    interaction-category response values. Names and query
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
    field_names = {row['field_id']: row.get('enum_name')
                   for row in (progress_fields or {}).get('fields', [])}
    rows = []
    for kind in range(_COUNT):
        offset = kind * _STRIDE
        identifier, = struct.unpack_from('>i', data, offset)
        if identifier != kind:
            raise ValueError('item-kind catalog index and stored identifier disagree')
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
                         collection_value=collection_value, display_material_frame=display_material_frame,
                         collection_response_category=collection_response_category,
                         item_class=item_class, item_class_name=class_names.get(item_class),
                         availability_field=availability_field,
                         availability_field_name=field_names.get(availability_field),
                         interaction_responses=interaction_responses,
                         has_model_template=bool(model),
                         display_names=dict(alternate=display_name(alternate), default=display_name(default))))
    return dict(count=_COUNT, stride=_STRIDE, entries=rows,
                table=dict(name='gItemKinds', size=len(data), sha256=_TABLE_SHA256, **location),
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
