"""Link saved-progress operands to authenticated storage and area-selector tables.

These are structural links, not execution, operand-floor, save-layout or safety
proofs. Unnamed fields retain numeric IDs; area-relative selectors retain every
current-area alternative rather than assuming the script's containing scene.
"""

import hashlib
import math
import re
import struct

from .context import ROOT
from .stack import incoming_edges, recover_arguments
from tools.source_quality import code_only

_TABLES = (
    ('gProgressFieldTypes', 976, 'a575c84c007f07a6dd341561431eeca5cb59784b9bf6171f577e257cfb36fec1'),
    ('gProgressFieldTypeLimits', 96, '26eb752067042257c6b5d5a3df79a3e9f77f948b6cc759b7f21286fad7abd34b'),
    ('gAreaProgressFieldSelectors', 668, '8ef1a1b229cd4b2ca9d9c6fc26d62300995ea570cfc9de3b714194b1f32d0eca'),
)
_NATIVES = {
    'actorScriptGetProgressValue': ('read', 1),
    'actorScriptGetProgressMinimum': ('encoding_minimum', 1),
    'actorScriptGetProgressMaximum': ('encoding_maximum', 1),
    'actorScriptSetProgressValue': ('set_with_encoding_limits', 2),
    'actorScriptAddProgressValue': ('add_with_encoding_limits', 2),
}


def _names(path, prefix, upper):
    code = code_only((ROOT / path).read_text())
    entries = re.findall(r'\b(' + prefix + r'\w+)\s*=\s*(0[xX][0-9a-fA-F]+|\d+)\b', code)
    names = {int(value, 0): name for name, value in entries if int(value, 0) < upper}
    if not names or len(names) != sum(int(value, 0) < upper for _, value in entries):
        raise ValueError('recovered progress identifiers must have distinct numeric values')
    return names


def progress_field_catalog(image):
    """Read 244 encodings, eight limits and seven bounded area-selector rows.

    The final selector/current-area word has a separate ELF symbol. Authenticate
    the complete 668-byte span, including that word, rather than symbol size664.
    """
    image.allowed = {'.boot', '.ovl'}
    tables, locations = {}, []
    for name, size, digest in _TABLES:
        data, location = image.read_address(image.symbol(name)[1], size)
        if hashlib.sha256(data).hexdigest() != digest:
            raise ValueError('retained progress table differs: ' + name)
        tables[name] = data
        locations.append(dict(name=name, size=size, sha256=digest, **location))
    names = _names('include/progress_field_ids.h', 'PROGRESS_FIELD_', 244)
    encodings = _names('include/progress_field_types.h', 'PROGRESS_ENCODING_', 8)
    if set(encodings) != set(range(8)):
        raise ValueError('recovered progress encodings must cover 0..7')
    limits = []
    for encoding, (width, minimum, maximum) in enumerate(struct.iter_unpack('>iii', tables['gProgressFieldTypeLimits'])):
        if width not in (1, 2, 4, 8, 16, 32) or minimum > maximum:
            raise ValueError('invalid retained progress encoding limits')
        limits.append(dict(encoding=encoding, enum_name=encodings[encoding], bit_width=width,
                           minimum=minimum, maximum=maximum))
    fields = []
    for field, (encoding,) in enumerate(struct.iter_unpack('>i', tables['gProgressFieldTypes'])):
        if encoding not in encodings:
            raise ValueError('invalid retained progress field encoding')
        record = dict(field_id=field, encoding=encoding, encoding_limits=limits[encoding])
        if field in names:
            record['enum_name'] = names[field]
        fields.append(record)
    selectors = []
    for selector in range(7):
        areas = []
        for area in range(23):
            field, = struct.unpack_from('>i', tables['gAreaProgressFieldSelectors'], selector * 0x60 + area * 4)
            if field != -1 and not 0 <= field < len(fields):
                raise ValueError('area selector does not identify a retained progress field')
            row = dict(current_area=area, field_id=field, absent=field == -1)
            if field != -1:
                row['field'] = fields[field]
            areas.append(row)
        selectors.append(dict(selector_index=selector, encoded_selector=0x40000000 | selector, areas=areas))
    return dict(fields=fields, encodings=limits, area_selectors=selectors, tables=locations,
                scope='Retained storage encodings and current-area alternatives; limits are encoding-wide, not semantic thresholds. Absence is not checked by value/minimum/maximum accessors. No live save values, layout validity or gameplay reachability are inferred.')


def progress_field_requests(instructions, catalog):
    """Recover selector expressions for the five retained progress VM natives.

    The source's presence helper is called by runtime code, not a registered VM
    native; no synthetic native identity is introduced for it.
    """
    incoming = incoming_edges(instructions)
    frames = {row['offset']: row for row in instructions if row['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    fields = {row['field_id']: row for row in catalog['fields']}
    selectors = {row['selector_index']: row for row in catalog['area_selectors']}
    rows = []
    for index, instruction in enumerate(instructions):
        functions = instruction.get('native', {}).get('functions', [])
        if len(functions) != 1 or functions[0] not in _NATIVES:
            continue
        name = functions[0]
        operation, count = _NATIVES[name]
        row = dict(instruction_offset=instruction['offset'], function=name, operation=operation,
                   scope='Structural operand/table link; complete CFG, operand floor, valid numeric types, runtime current area, save layout and execution are separate obligations.')
        try:
            arguments = recover_arguments(instructions, index, count, incoming=incoming, allow_float=True,
                                          allow_symbolic=True, frame_entries=frames)
            value = arguments[0]
            row.update(status='recovered_arguments', selector_expression=value)
            if count == 2:
                row['value_expression' if operation.startswith('set') else 'amount_expression'] = arguments[1]
            if value['kind'] in ('integer', 'float'):
                number = value['value']
                if not math.isfinite(number) or not -2147483648 <= number < 2147483648:
                    row['numeric_conversion'] = 'outside_supported_s32_conversion'
                else:
                    selector = int(number)
                    row['constant_selector'] = selector
                    if selector & 0x40000000:
                        relative = selector & 0x3FFFFFFF
                        row.update(selector_kind='area_relative', selector_index=relative,
                                   within_retained_selector_domain=relative in selectors)
                        if relative in selectors:
                            row['current_area_alternatives'] = selectors[relative]['areas']
                    else:
                        row.update(selector_kind='direct', within_retained_field_domain=selector in fields)
                        if selector in fields:
                            row['field'] = fields[selector]
        except ValueError as error:
            row.update(status='unresolved_arguments', reason=str(error))
        rows.append(row)
    return rows
