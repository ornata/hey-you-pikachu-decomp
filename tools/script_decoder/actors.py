"""Resolve named actor configurations against the local ROM's inspection identities.

Actor kinds include props and distinct same-name configurations. The records
establish configuration identity, not rendering, behavior or live allocation.
"""

import re
import struct

from .context import ROOT
from tools.source_quality import code_only


def actor_kind_names():
    """Read the explicit creation domain without treating the search alias as a kind."""
    code = code_only((ROOT / 'include/actor_kind.h').read_text())
    entries = re.findall(r'\b(ACTOR_KIND_\w+)\s*=\s*(\d+)\b', code)
    names = {int(value): name for name, value in entries}
    if len(entries) != 37 or set(names) != set(range(1, 38)) or len(set(names.values())) != 37:
        raise ValueError('actor kinds must agree with the retained 1..37 query domain')
    return names


def actor_kind_records(image, kinds=None):
    """Read bounded 0x54-byte configuration views and bounded borrowed names."""
    reviewed_names = kinds is None
    kinds = actor_kind_names() if reviewed_names else kinds
    base = image.symbol('gActorKindConfigurationsFromOne')[1]
    image.allowed = {'.boot', '.ovl'}

    def text(address):
        if not address:
            return None
        data = bytearray()
        for offset in range(256):
            byte, _ = image.read_address(address + offset, 1)
            if byte == b'\0':
                return data.decode('euc_jp')
            data.extend(byte)
        raise ValueError('actor inspection name exceeds its bounded read')

    rows = []
    for kind, name in sorted(kinds.items()):
        if not 1 <= kind <= 37:
            raise ValueError('search aliases and unrelated object types are not creation kinds')
        address = base + (kind - 1) * 0x54
        data, location = image.read_address(address, 0x54)
        alternate, _, default = struct.unpack_from('>3I', data)
        template = struct.unpack_from('>I', data, 0x18)[0]
        root, _ = image.read_address(template + 0x34, 2)
        runtime_kind = struct.unpack('>H', root)[0]
        if runtime_kind != kind:
            raise ValueError('actor configuration and root-template kind disagree')
        default_name = text(default)
        if reviewed_names:
            expected = name.removeprefix('ACTOR_KIND_').removesuffix('_ALTERNATE').replace('_', ' ').casefold()
            if default_name is None or ' '.join(default_name.split()).casefold() != expected:
                raise ValueError('named actor kind disagrees with its retained inspection identity')
        rows.append(dict(actor_kind=kind, enum_name=name, configuration=address, location=location,
                         default_name=default_name, alternate_name=text(alternate),
                         hierarchy_template=template, runtime_kind=runtime_kind,
                         resource_index=struct.unpack_from('>i', data, 0x50)[0],
                         scope='Retained configuration identity; distinct names/indices are not merged.'))
    return rows
