"""Link attached-effect arguments and bounded integer-key effect programs.

The selected slots and their keys are authenticated through LocalImage. A held
key or repeated address ends a static walk; neither establishes allocation size,
runtime termination, visual meaning of other selectors, or gameplay reachability.
"""

import hashlib
import math
import re
import struct

from .context import ROOT
from .stack import incoming_edges, recover_arguments
from tools.source_quality import code_only


def enum_names(path, prefix):
    """Read distinct explicitly numbered source identifiers, without guessing holes."""
    rows = re.findall(r'\b(' + prefix + r'\w+)\s*=\s*(0[xX][\da-fA-F]+|\d+)\b',
                      code_only(path.read_text()))
    names = {int(value, 0): name for name, value in rows}
    if not names or len(names) != len(rows):
        raise ValueError('missing or duplicate recovered animation identifiers')
    return names


def integer_key_program(image, address, names, limit=128):
    """Follow static key successors within a read budget, retaining loops explicitly."""
    if type(limit) is not int or not 1 <= limit <= 4096:
        raise ValueError('integer-key budget must be from one through 4096')
    rows, seen, current = [], set(), address
    result = dict(address=address, keys=rows,
                  scope='ROM-backed static key walk; visited records are not a complete allocation or playback proof.')
    try:
        for _ in range(limit):
            if current in seen:
                result.update(status='loop', loop_target=current)
                return result
            seen.add(current)
            raw, location = image.read_address(current, 12)
            mode, duration, start, end = struct.unpack('>ifhh', raw)
            if not math.isfinite(duration):
                raise ValueError('nonfinite key duration is unsupported')
            row = dict(**location, mode=mode, duration=duration, start=start, end=end,
                       sha256=hashlib.sha256(raw).hexdigest())
            if mode in names:
                row['mode_name'] = names[mode]
            rows.append(row)
            if duration < 0:
                if mode == 3:
                    current += start * 12
                    row.update(operation='relative_jump', target_address=current)
                else:
                    row['operation'] = 'hold'
                    result['status'] = 'held'
                    return result
            elif mode in (0, 1, 2):
                row['operation'] = {0: 'constant', 1: 'inclusive_range', 2: 'one_shot'}[mode]
                if mode == 1 and duration == 0:
                    raise ValueError('zero-duration range is outside the supported arithmetic domain')
                current += 12
                row['next_address'] = current
            else:
                raise ValueError('unsupported nonnegative-duration key mode')
        if current in seen:
            result.update(status='loop', loop_target=current)
        else:
            result.update(status='unresolved', reason='integer-key budget exhausted')
    except (ValueError, struct.error) as error:
        result.update(status='unresolved', reason=str(error))
    return result


def effect_program_catalog(image):
    """Read only the named effect slots; do not infer the full table extent."""
    names = enum_names(ROOT / 'include/texture_animation_programs.h', 'TEXTURE_ANIMATION_')
    modes = enum_names(ROOT / 'include/integer_animation_types.h', 'INTEGER_ANIMATION_KEY_')
    if set(modes) != {0, 1, 2, 3}:
        raise ValueError('integer-key mode model differs from the recovered source')
    image.allowed = {'.boot', '.ovl'}
    base = image.symbol('gTextureFrameAnimationPrograms')[1]
    programs = []
    for selector, name in sorted(names.items()):
        raw, location = image.read_address(base + selector * 4, 4)
        address, = struct.unpack('>I', raw)
        if not address:
            raise ValueError('named effect slot has no program')
        programs.append(dict(selector=selector, enum_name=name, table_slot=location,
                             table_slot_sha256=hashlib.sha256(raw).hexdigest(),
                             program=integer_key_program(image, address, modes)))
    return dict(programs=programs,
                scope='Selected named effect slots only; full table extent, texture resources and runtime playback remain separate obligations.')


def constant_integer(expression):
    """Convert only represented finite numeric constants within signed32."""
    if expression.get('kind') in ('integer', 'float'):
        number = expression['value']
        if math.isfinite(number) and -2147483648 <= number < 2147483648:
            return int(number)
    return None


def attached_effect_requests(instructions, catalog):
    """Link effect kind and track expressions without treating creation as observed."""
    incoming = incoming_edges(instructions)
    frames = {r['offset']: r for r in instructions if r['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    programs = {p['selector']: p for p in catalog['programs']}
    result = []
    for index, call in enumerate(instructions):
        if call.get('native', {}).get('functions') != ['actorScriptCreateAttachedEffect']:
            continue
        row = dict(instruction_offset=call['offset'], function='actorScriptCreateAttachedEffect',
                   scope='Structural argument/resource link; creation, valid numeric types, operand floors, ownership, rendering and reachability are not proved.')
        options = dict(incoming=incoming, allow_float=True, allow_symbolic=True, frame_entries=frames)
        try:
            kind, track = recover_arguments(instructions, index, 2, **options)
            row.update(status='recovered_arguments', effect_kind_expression=kind, track_expression=track)
            number = constant_integer(kind)
            if number is not None:
                row['constant_effect_kind'] = number
        except ValueError as error:
            row.update(status='unresolved_arguments', reason=str(error))
            try:
                track, = recover_arguments(instructions, index, 1, **options)
                row.update(status='recovered_track_only', track_expression=track)
            except ValueError:
                result.append(row)
                continue
        number = constant_integer(track)
        if number is not None:
            row['constant_track_selector'] = number
            if number in programs:
                row['effect_program'] = programs[number]
        result.append(row)
    return result
