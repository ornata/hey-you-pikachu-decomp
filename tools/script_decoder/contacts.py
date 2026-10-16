"""Recipient messenger-contact policy requests from bounded VM expressions.

Policies are full signed words, separate from messenger flight states. A named
constant establishes a request value, not recipient validity or delivery success.
"""

import hashlib
import math
import re

from .context import ROOT
from .stack import incoming_edges, recover_arguments
from tools.source_quality import code_only


def messenger_contact_policy_catalog(root=ROOT):
    """Read only explicit known policies from their scoped shared enum."""
    path = root / 'include/messenger_contact.h'
    code = code_only(path.read_text())
    matches = list(re.finditer(r'\btypedef\s+enum\s+MessengerContactPolicy\s*\{([^}]+)\}\s*MessengerContactPolicy\s*;', code))
    if len(matches) != 1:
        raise ValueError('missing or ambiguous MessengerContactPolicy enum')
    match = matches[0]
    names, identifiers = {}, set()
    for entry in match[1].split(','):
        if not entry.strip():
            continue
        member = re.fullmatch(r'\s*(MESSENGER_CONTACT_\w+)\s*=\s*(0[xX][0-9a-fA-F]+|\d+)\s*', entry)
        if member is None:
            raise ValueError('messenger policies require explicit numeric enum members')
        name, raw = member.groups()
        value = int(raw, 0)
        if not 0 <= value <= 3:
            raise ValueError('messenger policy is outside the known zero-through-three view')
        if value in names or name in identifiers:
            raise ValueError('duplicate messenger contact policy')
        names[value] = name
        identifiers.add(name)
    if set(names) != {0, 1, 2, 3}:
        raise ValueError('incomplete known messenger contact policy view')
    return dict(known_policies=[dict(value=value, enum_name=name) for value, name in sorted(names.items())],
                source_file='include/messenger_contact.h', source_code_sha256=hashlib.sha256(code.encode()).hexdigest(),
                storage_domain='signed_s32_without_low_halfword_mask',
                scope='Known recipient contact policies, not projectile flight states or launch modes. Setters retain arbitrary other signed words; enum membership is not validation.')


def messenger_contact_requests(instructions, catalog):
    """Retain symbolic requests and label only bounded known constant words."""
    names = {row['value']: row['enum_name'] for row in catalog['known_policies']}
    incoming = incoming_edges(instructions)
    frames = {row['offset']: row for row in instructions if row['name'] == 'SCRIPT_OP_ENTER_FRAME'}
    result = []
    for index, call in enumerate(instructions):
        functions = call.get('native', {}).get('functions', [])
        if 'actorScriptSetMessengerContactMode' not in functions:
            continue
        request = dict(instruction_offset=call['offset'], function='actorScriptSetMessengerContactMode',
                       argument_domain='recipient_messenger_contact_policy_full_signed_word',
                       scope='Request for a valid recipient actor with live disjoint VM/actor storage and valid operand floor. No flight-state, contact occurrence, delivery, rebound or orbit execution is inferred.')
        try:
            if functions != ['actorScriptSetMessengerContactMode']:
                raise ValueError('messenger contact setter binding is not unique')
            expression = recover_arguments(instructions, index, 1, incoming=incoming,
                allow_float=True, allow_symbolic=True, frame_entries=frames)[0]
            request.update(status='recovered_argument', policy_expression=expression)
            if expression['kind'] in ('integer', 'float'):
                value = expression.get('value')
                if not isinstance(value, (int, float)) or not math.isfinite(value) or not -2147483648 <= value < 2147483648:
                    request.update(policy_status='unsupported_integer_conversion',
                                   reason='Numeric policy conversion is not finite and signed-32 representable.')
                else:
                    value = int(value)
                    request.update(constant_policy=value, policy_status='known_policy' if value in names else 'unnamed_signed_word')
                    if value in names:
                        request['policy_name'] = names[value]
            else:
                request['policy_status'] = 'dynamic_policy'
        except ValueError as error:
            request.update(status='unresolved_argument', policy_status='unresolved_policy', reason=str(error))
        result.append(request)
    return result
