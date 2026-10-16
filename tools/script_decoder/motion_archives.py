"""Bounded local-ROM motion metadata, without animation payload redistribution.

These records describe archived channels, not rendered gestures or game outcomes.
The consumer bindings guard the format and the conditional omission interpretation.
"""

import hashlib
import math
import struct

from .context import ROOT, ROM_SHA1
from .native_frames import fingerprint

ARCHIVE_COUNT = 230
ARCHIVE_ROM_BASE = 0xB77EB0
SOURCE_PINS = {
    'src/game/motion_playback.c': '739841587dad79444dc772fde04d29609db6d15a760ef00954a7b35ceb9941bc',
    'src/game/animation_tracks.c': 'eb7320f95a5cb2548c558a2904f0d77541e589cc6d5cec81ad80a68318fe92fc',
    'src/game/motion_cache.c': '00136a7975ade8a81f173ac989242cc9dc2defbb7d94efa7a35a101aef8f00ea',
    'include/animation_tracks.h': 'cb8ae02394bd3d838aa968bd4fee5bb23ad6517028bddc2957cfd2d7c4c6c40a',
}
OMISSION_SCOPE = (
    'For child kinds38..99, starting track playback disables child playback without resetting rotations, then '
    'initializes listed parts only. Omitted child rotations are not driven by these '
    'archive tracks; separate callbacks/controllers can still change them. No body-part '
    'name, fixed world transform, gesture, mood or gameplay outcome is inferred.'
)


def source_bindings(root=ROOT):
    """Reject changed format consumers before assigning source-derived meanings."""
    result = []
    for path, expected in SOURCE_PINS.items():
        actual = fingerprint((root / path).read_text())
        if actual != expected:
            raise ValueError(f'stale motion archive source binding: {path}')
        result.append(dict(path=path, sha256=actual))
    return result


def decode_archive(rom, *, archive_base, start_offset, end_offset,
                   track_table_offset, duration, max_parts=256, max_keys=4096):
    """Decode one bounded archive; invalid or unclosed spans raise ValueError.

    Offsets are archive-relative, including zero. Key coefficients stay private;
    only fingerprints, counts and time metadata are returned. Unknown descriptor
    kinds remain explicit and are never interpreted as supported key sequences.
    """
    if type(max_parts) is not int or not 0 < max_parts <= 4096:
        raise ValueError('invalid part budget')
    if type(max_keys) is not int or not 0 < max_keys <= 65536:
        raise ValueError('invalid key budget')
    try:
        valid_duration = type(duration) in (int, float) and math.isfinite(duration) and duration >= 0
    except OverflowError:
        valid_duration = False
    if not valid_duration:
        raise ValueError('motion duration must be finite and nonnegative')
    if any(type(v) is not int for v in (archive_base, start_offset, end_offset, track_table_offset)):
        raise ValueError('archive offsets must be integers')
    if not 0 <= archive_base <= len(rom) or not 0 <= start_offset < end_offset:
        raise ValueError('invalid archive extent')
    if archive_base + end_offset > len(rom):
        raise ValueError('archive extent outside ROM')

    def read(offset, size):
        if not start_offset <= offset or size <= 0 or offset + size > end_offset:
            raise ValueError('motion reference outside selected archive extent')
        return rom[archive_base + offset:archive_base + offset + size]

    def curve(offset, role):
        raw = read(offset, 8)
        kind, value = struct.unpack('>ii', raw)
        row = dict(kind=kind, axis_role=role, descriptor_rom_offset=archive_base + offset)
        if kind == 0:
            row.update(status='constant', signed_fixed16_16=value, constant_value=value / 65536.0,
                       curve_sha256=hashlib.sha256(raw).hexdigest())
            return row
        row['archive_key_offset'] = value
        if kind not in (1, 2):
            row.update(status='unknown_kind', reason='Original evaluator does not interpret this descriptor kind.')
            return row
        digest = hashlib.sha256()
        ticks = 0
        for index in range(max_keys):
            key = read(value + index * 16, 16)
            a, b, c, base, frac, length = struct.unpack('>fffhBB', key)
            # Hold keys use only base/frac; linear keys use only a/b.
            used = () if length == 255 else (a, b, c) if kind == 1 else (a, b)
            if not all(math.isfinite(v) for v in used):
                raise ValueError('nonfinite evaluated animation coefficient')
            digest.update(key)
            if length == 255:
                row.update(status='keyed', interpolation='cubic' if kind == 1 else 'linear',
                           key_count=index + 1, active_key_count=index,
                           active_duration_ticks=ticks, active_duration_seconds=ticks / 30.0,
                           duration_scope='Nominal authored 30 Hz ticks before the terminal hold; not observed playback time.',
                           terminal_hold_value=base + frac / 256.0,
                           keys_rom_offset=archive_base + value, keys_size=(index + 1) * 16,
                           curve_sha256=digest.hexdigest())
                return row
            ticks += length
        raise ValueError('key budget exhausted before terminal hold')

    parts = []
    for index in range(max_parts + 1):
        offset = track_table_offset + index * 8
        kind, curves = struct.unpack('>ii', read(offset, 8))
        if kind <= 0:
            table_size = (index + 1) * 8
            table_hash = hashlib.sha256(read(track_table_offset, table_size)).hexdigest()
            break
        if index == max_parts:
            raise ValueError('part budget exhausted before table terminator')
        roles = [f'rotation_{axis}' for axis in 'xyz']
        if kind < 0x26:
            roles += [f'movement_{axis}' for axis in 'xyz']
        descriptors = [curve(curves + n * 8, role) for n, role in enumerate(roles)]
        parts.append(dict(index=index, object_kind=kind,
                          track_table_rom_offset=archive_base + offset,
                          curves_archive_offset=curves, root_motion=kind < 0x26,
                          descriptors=descriptors))
    raw = read(start_offset, end_offset - start_offset)
    return dict(status='decoded', rom_offset=archive_base + start_offset, size=len(raw),
                archive_sha256=hashlib.sha256(raw).hexdigest(), duration_seconds=duration,
                track_table_rom_offset=archive_base + track_table_offset,
                track_table_size=table_size, track_table_sha256=table_hash,
                terminator_object_kind=kind, parts=parts,
                unknown_descriptor_count=sum(d['status'] == 'unknown_kind'
                                             for p in parts for d in p['descriptors']),
                scope='Ordered original archive metadata; no rendered animation, physical gesture or reachability proof.')


def compare_archive_tracks(left, right):
    """Compare ordered channel identities without collapsing duplicate parts."""
    if left.get('status') != 'decoded' or right.get('status') != 'decoded':
        raise ValueError('comparison requires two decoded archives')

    def identity(part):
        descriptors = part['descriptors']
        if any(d['status'] == 'unknown_kind' for d in descriptors):
            raise ValueError('unknown descriptor prevents exact track comparison')
        return (part['object_kind'], tuple((d['kind'], d['axis_role'], d['curve_sha256'])
                                          for d in descriptors))

    original = [identity(p) for p in left['parts']]
    retained = [identity(p) for p in right['parts']]
    cursor = 0
    matches, omitted = [], []
    for index, entry in enumerate(original):
        if cursor < len(retained) and entry == retained[cursor]:
            matches.append(dict(left_index=index, right_index=cursor,
                                object_kind=entry[0]))
            cursor += 1
        else:
            omitted.append(dict(left_index=index, object_kind=entry[0]))
    return dict(right_is_ordered_exact_track_subsequence=cursor == len(retained),
                matched_parts=matches, omitted_left_parts=omitted,
                unmatched_right_count=len(retained) - cursor,
                conditional_omission_scope=OMISSION_SCOPE,
                scope='Exact descriptor kind/role and constant/full-key fingerprints; not pointer equality or anatomical inference.')


def motion_archive_catalog(image):
    """Read all 230 original resident archive references, failing closed on pins.

    image must be a LocalImage with a matching ELF. Source/table failures reject
    the catalog; individual resource failures are retained as unresolved rows.
    """
    bindings = source_bindings()
    if image.sha1 != ROM_SHA1 or hashlib.sha1(image.rom).hexdigest() != ROM_SHA1:
        raise ValueError('motion archives require authenticated original USA ROM')
    table_specs = [('offsets', 0x80077778, (ARCHIVE_COUNT + 1) * 4),
                   ('track_tables', 0x80077048, ARCHIVE_COUNT * 4),
                   ('durations', 0x800773E0, ARCHIVE_COUNT * 4)]
    data, tables = {}, []
    for name, address, size in table_specs:
        raw, location = image.read_address(address, size)
        data[name] = raw
        tables.append(dict(name=name, **location, size=size, sha256=hashlib.sha256(raw).hexdigest()))
    offsets = struct.unpack(f'>{ARCHIVE_COUNT + 1}i', data['offsets'])
    tracks = struct.unpack(f'>{ARCHIVE_COUNT}i', data['track_tables'])
    durations = struct.unpack(f'>{ARCHIVE_COUNT}f', data['durations'])
    rows = []
    for index in range(ARCHIVE_COUNT):
        try:
            row = decode_archive(image.rom, archive_base=ARCHIVE_ROM_BASE,
                                 start_offset=offsets[index], end_offset=offsets[index + 1],
                                 track_table_offset=tracks[index], duration=durations[index])
        except ValueError as error:
            row = dict(status='unresolved', reason=str(error),
                       start_archive_offset=offsets[index], end_archive_offset=offsets[index + 1])
        rows.append(dict(archive_id=index, **row))
    comparison = None
    if all(rows[i]['status'] == 'decoded' for i in (131, 132)):
        try:
            comparison = compare_archive_tracks(rows[131], rows[132])
        except ValueError as error:
            comparison = dict(status='unresolved', reason=str(error))
    return dict(archive_count=ARCHIVE_COUNT, archive_rom_base=ARCHIVE_ROM_BASE,
                rom_sha1=image.sha1, source_bindings=bindings, tables=tables,
                archives=rows, archive_131_132_comparison=comparison,
                scope='Authenticated static metadata; unresolved resources remain explicit. No coefficient/key payloads or debug strings are emitted.')
