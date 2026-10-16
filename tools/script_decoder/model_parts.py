"""Authenticated actor template topology and configured functional part roles.

Graphics pointers remain opaque. Bind transforms and attachment roles do not
establish anatomy, handedness, rendered gestures or gameplay outcomes.
"""
import hashlib
import math
import struct

from .context import ROOT, ROM_SHA1
from .native_frames import fingerprint

SOURCE_PINS = {
    'include/actor_parts.h': 'e87f494367d64cbebb8896cc6388b35542e7ad2c6f2ff11453296de087ff95cc',
    'src/game/actor_configuration.c': 'f014e464cc9c086edc371f32b891da82d7a5b233050b7b3bf6f65dee8f84df9f',
    'src/game/actor_model_motion.c': '1cab149c269dce85748cbaf022cef9e014642f2eed8cd4dc6068c65c03a149af',
    'src/game/actor_model_parts.c': '90c895f0d49d62252524789852093a98fea25e46fe7f9d23cd528e86e84bb2c1',
    'src/game/motion_playback.c': '739841587dad79444dc772fde04d29609db6d15a760ef00954a7b35ceb9941bc',
    'src/game/object_hierarchy.c': 'd2e4e761161617f2989d1abbd165c9e4eb3089ffca22168f14c5efb73517f7eb',
    'src/ovl/item_physics.c': 'd91e16a5fa9d24f1d005cc161534d028837735edacbf6ded48dffb0cdeb9fa9a',
    'include/object_template.h': '4ac4abe8965ed6eb5ae1f8c32153a0b670b14ebe030d4f7bbb4fbee84c098a26',
    'include/object_kind_callbacks.h': '16b8c7d03046d15ff1cbeef889b8568337d7637b7039a7c6f9c0e2ab4d1eaa8f',
}


def source_bindings(root=ROOT):
    """Fail closed when any reviewed layout or role consumer changes."""
    result = []
    for path, expected in SOURCE_PINS.items():
        actual = fingerprint((root / path).read_text())
        if actual != expected:
            raise ValueError(f'stale model part source binding: {path}')
        result.append(dict(path=path, sha256=actual))
    return result


def decode_template_hierarchy(read_address, root, *, max_nodes=256, max_depth=64):
    """Traverse explicit template links using an authenticated bounded reader.

    The reader returns (bytes, location). Repeated addresses are rejected rather
    than inventing ownership for cycles or shared templates. Duplicate kinds are
    retained. Root siblings retain a null parent instead of becoming children.
    """
    if type(root) is not int or root <= 0 or root > 0xFFFFFFFF:
        raise ValueError('unsupported root template address')
    if type(max_nodes) is not int or not 0 < max_nodes <= 4096:
        raise ValueError('invalid template node budget')
    if type(max_depth) is not int or not 0 < max_depth <= 256:
        raise ValueError('invalid template depth budget')
    result, seen = [], set()
    pending = [(root, None, 0, [0])]
    while pending:
        address, parent, depth, path = pending.pop()
        if address in seen:
            raise ValueError('cycle or shared template address')
        if len(result) >= max_nodes or depth >= max_depth:
            raise ValueError('template traversal budget exhausted')
        if address <= 0 or address > 0xFFFFFFFF or address % 4:
            raise ValueError('unsupported template address alignment')
        seen.add(address)
        raw, location = read_address(address, 56)
        if len(raw) != 56:
            raise ValueError('incomplete object template')
        values = struct.unpack('>10fIIIHH', raw)
        if not all(math.isfinite(value) for value in values[:10]):
            raise ValueError('nonfinite template bind transform or radius')
        sibling, child, display_list, kind, flags = values[10:]
        result.append(dict(index=len(result), address=address, parent_address=parent,
                           path=path, depth=depth, sibling_address=sibling,
                           child_address=child, kind=kind, template_flags=flags,
                           scale=list(values[:3]), rotation_degrees=list(values[3:6]),
                           position=list(values[6:9]), radius=values[9],
                           display_list_word=display_list,
                           display_list_scope='Opaque graphics reference, possibly segmented; not CPU-dereferenced.',
                           sha256=hashlib.sha256(raw).hexdigest(), location=location))
        if sibling:
            pending.append((sibling, parent, depth, path[:-1] + [path[-1] + 1] if path else [1]))
        if child:
            pending.append((child, address, depth + 1, path + [0]))
    return result


def pikachu_model_catalog(image, motion_archives=None):
    """Bind the reviewed Pikachu configuration, hierarchy and optional archives.

    Unsupported layouts fail explicitly; no fallback anatomy or role guesses are
    produced. All table/template reads must authenticate against the supplied ELF.
    """
    bindings = source_bindings()
    if image.sha1 != ROM_SHA1 or hashlib.sha1(image.rom).hexdigest() != ROM_SHA1:
        raise ValueError('model metadata requires authenticated original USA ROM')

    def read(address, size):
        raw, location = image.read_address(address, size)
        return raw, location

    raw, location = read(0x80088BEC, 84)
    root = struct.unpack_from('>I', raw, 24)[0]
    default, first, second, mode, resource = struct.unpack_from('>5i', raw, 64)
    if (root, default, first, second, mode, resource) != (0x800797B0, 63, 60, 53, 0, 0):
        raise ValueError('unsupported Pikachu configuration role binding')
    nodes = decode_template_hierarchy(read, root)
    if len(nodes) != 57:
        raise ValueError('unsupported Pikachu template count')
    by_kind = {}
    for node in nodes:
        by_kind.setdefault(node['kind'], []).append(node)
    callbacks = []
    for kind in range(48, 62):
        if len(by_kind.get(kind, [])) != 1:
            raise ValueError('ambiguous or missing configured attachment-chain part')
        data, loc = read(0x80078140 + kind * 24, 24)
        hooks = struct.unpack('>6I', data)
        expected_draw = {54: 0x800148B0, 61: 0x80014880}.get(kind, 0)
        if hooks != (0x80015110, 0x80014E54, 0x800155E0, expected_draw, 0, 0):
            raise ValueError('unsupported attachment-chain callback binding')
        callbacks.append(dict(kind=kind, location=loc, sha256=hashlib.sha256(data).hexdigest(),
                              hooks=dict(zip(('init', 'destroy', 'update', 'draw', 'draw_child', 'response'), hooks))))
    slots, slotloc = read(0x800760E8, 8)
    if struct.unpack('>2i', slots) != (54, 61):
        raise ValueError('unsupported appearance-slot binding')
    chains = []
    for kinds, attachment, slot in ((list(range(48, 55)), 53, 0), (list(range(55, 62)), 60, 1)):
        parent = by_kind[41][0]['address'] if len(by_kind.get(41, [])) == 1 else None
        for kind in kinds:
            node = by_kind[kind][0]
            if parent is None or node['parent_address'] != parent:
                raise ValueError('unsupported attachment-chain topology')
            parent = node['address']
        chains.append(dict(kinds=kinds, parent_kind=41, attachment_part=attachment,
                           appearance_slot=slot, terminal_part=kinds[-1]))
    comparison = dict(status='not_requested')
    if motion_archives is not None:
        from .motion_archives import compare_archive_tracks, source_bindings as archive_bindings
        if (motion_archives.get('rom_sha1') != image.sha1 or
                motion_archives.get('source_bindings') != archive_bindings()):
            raise ValueError('unsupported archive authentication/source binding')
        archives = motion_archives.get('archives')
        if (not isinstance(archives, list) or len(archives) <= 132 or
                any(not isinstance(archives[i], dict) for i in (131, 132)) or
                [archives[i].get('archive_id') for i in (131, 132)] != [131, 132]):
            raise ValueError('unsupported archive identity ordering')
        comparison = compare_archive_tracks(archives[131], archives[132])
        omitted = [p['object_kind'] for p in comparison['omitted_left_parts']]
        comparison['omits_exactly_both_configured_chains'] = (
            comparison['right_is_ordered_exact_track_subsequence'] and omitted == list(range(48, 62)))
        comparison['scope'] = ('Actor-scoped functional attachment/appearance chains only. '
                               'Omitted local rotations are not driven by these tracks; '
                               'parent transforms, scripts and separate callbacks remain independent. '
                               'No anatomical or handedness names are established.')
    return dict(status='decoded', actor_kind=37, source_bindings=bindings,
                configuration=dict(address=0x80088BEC, location=location, sha256=hashlib.sha256(raw).hexdigest(),
                                   root_template=root, default_part=default, first_attachment_part=first,
                                   second_attachment_part=second, initial_attachment_mode=mode, resource_index=resource),
                nodes=nodes, attachment_chains=chains, callbacks=callbacks,
                appearance_slots=dict(location=slotloc, values=[54, 61], sha256=hashlib.sha256(slots).hexdigest()),
                attachment_modes={
                    '0': dict(enum_name='ACTOR_ATTACHMENT_CONFIGURED_MIDPOINT', role='configured_midpoint', part_ids=[first, second]),
                    '1': dict(enum_name='ACTOR_ATTACHMENT_CONFIGURED_FIRST', role='configured_first', part_id=first),
                    '2': dict(enum_name='ACTOR_ATTACHMENT_CONFIGURED_SECOND', role='configured_second', part_id=second),
                    '3': dict(enum_name='ACTOR_ATTACHMENT_RAISED', role='raised', part_id=118, vertical_offset='caller_height_offset'),
                    '4': dict(enum_name=None, role='unresolved', part_id=119),
                    '5': dict(enum_name=None, role='unresolved', part_id=66)},
                attachment_position_scope='Selected parts fall back to the configured default part when absent. Unsupported modes leave the output unchanged. These rules do not establish live part availability or anatomical identities.',
                root_sibling_scope='Root siblings remain separate roots in this template forest; they are not assigned as children or parts of the Pikachu root.',
                archive_131_132_comparison=comparison,
                scope='Authenticated template metadata and actor-scoped configured functional roles; no anatomy, pose rendering or gameplay outcome proof.')
