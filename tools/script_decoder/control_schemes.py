"""Connect retained control schemes and mode callbacks to deferred script requests.

Table identity and argument expressions are bounded static evidence. Neither
callback addresses nor a resolved index prove transition effects or reachability.
"""

import math
import struct

from .stack import incoming_edges, recover_arguments


def input_flags(mask):
    """Separate physical controller buttons from the two synthetic stick flags."""
    buttons = [(0x8000, 'A_BUTTON'), (0x4000, 'B_BUTTON'), (0x2000, 'Z_TRIG'),
               (0x1000, 'START_BUTTON'), (0x800, 'U_JPAD'), (0x400, 'D_JPAD'),
               (0x200, 'L_JPAD'), (0x100, 'R_JPAD'), (0x20, 'L_TRIG'), (0x10, 'R_TRIG'),
               (8, 'U_CBUTTONS'), (4, 'D_CBUTTONS'), (2, 'L_CBUTTONS'), (1, 'R_CBUTTONS')]
    return dict(buttons=[name for bit, name in buttons if mask & bit],
                stick_axes=[name for bit, name in ((0x40, 'X'), (0x80, 'Y')) if mask & bit])


def control_scheme_catalog(image, scheme_count=28, mode_count=32):
    """Read bounded scheme/mode views using their established twelve-byte strides."""
    if type(scheme_count) is not int or type(mode_count) is not int or not 1 <= scheme_count <= 28 or not 1 <= mode_count <= 32:
        raise ValueError('control scheme/mode counts exceed reviewed table bounds')
    image.allowed = {'.boot', '.ovl'}
    modes, schemes = [], []
    mode_base = image.symbol('gPlayerModes')[1]
    scheme_base = image.symbol('gPlayerControlSchemes')[1]
    for index in range(mode_count):
        data, location = image.read_address(mode_base + index * 12, 12)
        update, leave, permissions = struct.unpack('>III', data)
        updates = image.functions(update)
        leaves = image.functions(leave) if leave else []
        if not updates or leave and not leaves:
            raise ValueError('player mode callback is not a resolved function')
        modes.append(dict(mode=index, location=location, update_address=update, update_functions=updates,
                          leave_address=leave, leave_functions=leaves, permission_mask=permissions))
    for index in range(scheme_count):
        data, location = image.read_address(scheme_base + index * 12, 12)
        mode, port, mask = struct.unpack_from('>iiH', data)
        if not 0 <= mode < mode_count or not -1 <= port <= 3:
            raise ValueError('control scheme has an invalid mode or controller port')
        schemes.append(dict(scheme=index, location=location, mode=mode, controller_port=port,
                            input_mask=mask, retained_inputs=input_flags(mask)))
    return dict(schemes=schemes, modes=modes,
                leave_callback_frontier=sorted({name for mode in modes for name in mode['leave_functions']}),
                scope='Retained table identities and input-mask meanings; prior leave callbacks, mode-nine rumble, transition effects and gameplay reachability remain separate obligations.')


def control_scheme_requests(instructions, schemes):
    """Recover the operand retained until the deferred control-scheme handler runs."""
    incoming = incoming_edges(instructions)
    frames = {row['offset']: row for row in instructions if row['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    known = {row['scheme']: row for row in schemes}
    rows = []
    for index, instruction in enumerate(instructions):
        if instruction.get('native', {}).get('functions') != ['actorScriptStartControlSchemeChange']:
            continue
        row = dict(instruction_offset=instruction['offset'], function='actorScriptStartControlSchemeChange',
                   consumption='deferred_until_outside_text_window_mode', completion_result='integer_zero',
                   scope='Structural operand/table link; valid numeric conversion, handler effects, feasibility and eventual completion are not established.')
        try:
            value = recover_arguments(instructions, index, 1, incoming=incoming, allow_float=True,
                                      allow_symbolic=True, frame_entries=frames)[0]
            row.update(status='recovered_arguments', scheme_expression=value)
            if value['kind'] in ('integer', 'float'):
                number = value['value']
                if math.isfinite(number) and -2147483648 <= number < 2147483648:
                    identifier = int(number)
                    row.update(constant_scheme_id=identifier, valid_scheme_index=identifier in known)
                    if identifier in known:
                        row['selected_mode'] = known[identifier]['mode']
                else:
                    row['numeric_conversion'] = 'outside_supported_s32_conversion'
        except ValueError as error:
            row.update(status='unresolved_arguments', reason=str(error))
        rows.append(row)
    return rows
