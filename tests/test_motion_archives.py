"""Synthetic archive boundaries and metadata; no game payload fixtures."""

import math
import struct
import tempfile
import unittest
from pathlib import Path

from tools.script_decoder.motion_archives import (
    SOURCE_PINS, compare_archive_tracks, decode_archive, source_bindings,
)


def fixture(kinds=(38,), descriptor_kind=0, key_offset=0):
    data = bytearray(512)
    # Offset zero is an ordinary keyed reference, ending in one held key.
    struct.pack_into('>fffhBB', data, 0, 1., 2., 3., -2, 128, 255)
    cursor = 160
    for i, kind in enumerate(kinds):
        struct.pack_into('>ii', data, 64 + i * 8, kind, cursor)
        count = 6 if kind < 38 else 3
        for axis in range(count):
            struct.pack_into('>ii', data, cursor + axis * 8, descriptor_kind,
                             key_offset if descriptor_kind else 65536 + axis)
        cursor += count * 8
    struct.pack_into('>ii', data, 64 + len(kinds) * 8, 0, 0)
    return data


def decode(data, **kwargs):
    args = dict(archive_base=0, start_offset=0, end_offset=len(data),
                track_table_offset=64, duration=1.)
    args.update(kwargs)
    return decode_archive(data, **args)


class MotionArchiveTests(unittest.TestCase):
    def test_root_six_child_three_and_duplicates(self):
        row = decode(fixture((37, 38, 38)))
        self.assertEqual([len(p['descriptors']) for p in row['parts']], [6, 3, 3])
        self.assertEqual([p['object_kind'] for p in row['parts']], [37, 38, 38])
        self.assertEqual(row['parts'][0]['descriptors'][3]['axis_role'], 'movement_x')
        self.assertEqual(row['parts'][0]['descriptors'][0]['constant_value'], 1.)

    def test_zero_offset_is_valid_key(self):
        d = decode(fixture(descriptor_kind=1))['parts'][0]['descriptors'][0]
        self.assertEqual(d['keys_rom_offset'], 0)
        self.assertEqual(d['terminal_hold_value'], -1.5)
        self.assertEqual(d['key_count'], 1)
        self.assertNotIn('coefficients', d)

    def test_unknown_kind_explicit_not_dereferenced(self):
        row = decode(fixture(descriptor_kind=99, key_offset=-100))
        self.assertEqual(row['unknown_descriptor_count'], 3)
        self.assertEqual(row['parts'][0]['descriptors'][0]['status'], 'unknown_kind')
        with self.assertRaises(ValueError):
            compare_archive_tracks(row, row)

    def test_invalid_extents_rejected(self):
        for args in [dict(end_offset=513), dict(start_offset=-1),
                     dict(start_offset=100, end_offset=50), dict(track_table_offset=-8)]:
            with self.subTest(args=args), self.assertRaises(ValueError):
                decode(fixture(), **args)
        data = fixture()
        struct.pack_into('>ii', data, 64, 38, 500)
        with self.assertRaisesRegex(ValueError, 'outside'):
            decode(data)

    def test_missing_table_terminator_and_budget(self):
        with self.assertRaisesRegex(ValueError, 'part budget'):
            decode(fixture((38, 39)), max_parts=1)
        with self.assertRaisesRegex(ValueError, 'outside'):
            decode(fixture(), track_table_offset=508)

    def test_missing_hold_and_key_budget(self):
        data = fixture(descriptor_kind=2)
        struct.pack_into('>fffhBB', data, 0, 1., 2., 3., 0, 0, 1)
        with self.assertRaisesRegex(ValueError, 'key budget'):
            decode(data, max_keys=1)
        data = fixture(descriptor_kind=1, key_offset=504)
        with self.assertRaisesRegex(ValueError, 'outside'):
            decode(data)

    def test_duration_and_used_coefficients_finite(self):
        for value in [math.nan, math.inf, -1., True, None, '1']:
            with self.assertRaises(ValueError):
                decode(fixture(), duration=value)
        data = fixture(descriptor_kind=1)
        struct.pack_into('>fffhBB', data, 0, math.nan, 0., 0., 0, 0, 1)
        with self.assertRaisesRegex(ValueError, 'nonfinite'):
            decode(data)
        # A terminal hold does not evaluate its unused coefficient bytes.
        data[15] = 255
        self.assertEqual(decode(data)['status'], 'decoded')

    def test_ordered_omission_preserves_duplicate_occurrences(self):
        left = decode(fixture((38, 39, 38)))
        right = decode(fixture((38, 38)))
        result = compare_archive_tracks(left, right)
        self.assertTrue(result['right_is_ordered_exact_track_subsequence'])
        self.assertEqual(result['omitted_left_parts'], [dict(left_index=1, object_kind=39)])
        self.assertEqual(result['matched_parts'][1]['left_index'], 2)
        reverse = decode(fixture((39, 38, 38)))
        self.assertFalse(compare_archive_tracks(left, reverse)['right_is_ordered_exact_track_subsequence'])

    def test_all_budgets_bounded(self):
        for args in [dict(max_parts=0), dict(max_parts=4097), dict(max_keys=0), dict(max_keys=65537), dict(max_parts=True), dict(max_keys=True),
                     dict(max_parts=1.0), dict(max_keys=math.inf)]:
            with self.assertRaises(ValueError):
                decode(fixture(), **args)

    def test_stale_source_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for path in SOURCE_PINS:
                (root / path).parent.mkdir(parents=True, exist_ok=True)
                (root / path).write_text('changed format consumer')
            with self.assertRaisesRegex(ValueError, 'stale'):
                source_bindings(root)


if __name__ == '__main__':
    unittest.main()
