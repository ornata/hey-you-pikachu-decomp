import copy
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from tools.script_decoder.native_frames import (
    compact_review_dependencies, expand_review_dependencies, fingerprint,
    load_review_manifest, native_frame_reviews, review_manifest_fingerprint,
    write_review_manifest,
)


def fixture(root, version=1):
    """Author small independent source reviews without any ROM or SDK inputs."""
    layout = root / 'include/layout.h'
    layout.parent.mkdir(parents=True)
    layout.write_text('#define LAYOUT 1\n')
    dependency = dict(kind='file', file='include/layout.h', sha256=fingerprint(layout.read_text()))
    callbacks = []
    for name, file in [('alpha', 'src/a.c'), ('beta', 'src/b.c'), ('gamma', 'src/a.c')]:
        source = f's32 {name}(u8 *actor) {{ return 0; }}'
        path = root / file
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text((path.read_text() if path.exists() else '') + source + '\n')
        callbacks.append(dict(function=name, file=file, sha256=fingerprint(source),
                              effect='append_scalar', arguments=0, stack_delta=1,
                              result_tag='integer', scheduling='continue', dependencies=[dict(dependency)]))
    manifest = dict(schema_version=1, dependencies=[dependency], callbacks=callbacks,
                    deferred_reviews=[], preconditions=[])
    if version == 2:
        manifest = compact_review_dependencies(manifest)
    path = root / 'tools/script_decoder/native_frame_reviews.json'
    path.parent.mkdir(parents=True)
    path.write_text(json.dumps(manifest))
    return path, manifest


class NativeReviewStorageTests(unittest.TestCase):
    def test_inline_versions_preserve_activation_order_and_fingerprints(self):
        for version in (1, 2):
            with self.subTest(version=version), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                path, original = fixture(root, version)
                preserved = copy.deepcopy(original)
                before = native_frame_reviews(root)
                digest = review_manifest_fingerprint(root)
                write_review_manifest(original, root)
                self.assertEqual(original, preserved)
                self.assertEqual(expand_review_dependencies(load_review_manifest(root)),
                                 expand_review_dependencies(original))
                self.assertEqual(native_frame_reviews(root), before)
                self.assertEqual(list(native_frame_reviews(root)['active']), ['alpha', 'beta', 'gamma'])
                self.assertEqual(review_manifest_fingerprint(root), digest)
                indexed = path.read_bytes()
                write_review_manifest(load_review_manifest(root), root)
                self.assertEqual(path.read_bytes(), indexed)

    def test_missing_and_altered_parts_fail_before_activation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path, manifest = fixture(root)
            write_review_manifest(manifest, root)
            index = json.loads(path.read_text())
            part = path.parent / index['callback_files'][0]['file']
            saved = part.read_bytes()
            part.write_bytes(saved + b'\n')
            with self.assertRaisesRegex(ValueError, 'checksum'):
                native_frame_reviews(root)
            part.unlink()
            with self.assertRaises(FileNotFoundError):
                native_frame_reviews(root)

    def test_duplicate_identities_and_directory_escape_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path, manifest = fixture(root)
            write_review_manifest(manifest, root)
            index = json.loads(path.read_text())
            for change in ('order', 'parts', 'escape'):
                altered = copy.deepcopy(index)
                if change == 'order':
                    altered['callback_order'].append('alpha')
                elif change == 'parts':
                    altered['callback_files'].append(altered['callback_files'][0])
                else:
                    altered['callback_files'][0]['file'] = '../outside.json'
                with self.subTest(change=change), self.assertRaises(ValueError):
                    native_frame_reviews(root, altered)
            duplicate = copy.deepcopy(manifest)
            duplicate['callbacks'].append(duplicate['callbacks'][0])
            with self.assertRaisesRegex(ValueError, 'duplicate'):
                write_review_manifest(duplicate, root)

    def test_part_edits_require_an_index_update_and_change_the_logical_digest(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path, manifest = fixture(root)
            write_review_manifest(manifest, root)
            digest = review_manifest_fingerprint(root)
            index = json.loads(path.read_text())
            record = index['callback_files'][0]
            part = path.parent / record['file']
            content = json.loads(part.read_text())
            content['callbacks'][0]['preconditions'] = ['An explicitly changed review condition.']
            data = json.dumps(content).encode()
            part.write_bytes(data)
            with self.assertRaises(ValueError):
                review_manifest_fingerprint(root)
            record['sha256'] = hashlib.sha256(data).hexdigest()
            path.write_text(json.dumps(index))
            self.assertNotEqual(review_manifest_fingerprint(root), digest)

    def test_source_changes_revoke_reviews_and_distinct_frozen_hashes_survive(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            _, manifest = fixture(root)
            manifest['callbacks'][1]['dependencies'][0]['sha256'] = '0' * 64
            before = native_frame_reviews(root, manifest)
            write_review_manifest(manifest, root)
            self.assertEqual(native_frame_reviews(root), before)
            self.assertIn('beta', before['stale_callback_dependencies'])
            (root / 'src/a.c').write_text('s32 alpha(u8 *actor) { return 1; }\n'
                                         's32 gamma(u8 *actor) { return 0; }\n')
            after = native_frame_reviews(root)
            self.assertNotIn('alpha', after['active'])
            self.assertIn('gamma', after['active'])

    def test_directory_and_temporary_symlinks_cannot_escape(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path, manifest = fixture(root)
            parts = path.parent / 'native_frame_reviews'
            parts.mkdir()
            victim = root / 'victim'
            victim.write_text('untouched')
            (parts / 'src__a.json.tmp').symlink_to(victim)
            path.with_suffix('.json.tmp').symlink_to(victim)
            write_review_manifest(manifest, root)
            self.assertEqual(victim.read_text(), 'untouched')
            outside = root / 'elsewhere'
            parts.rename(outside)
            parts.symlink_to(outside, target_is_directory=True)
            with self.assertRaisesRegex(ValueError, 'escapes'):
                native_frame_reviews(root)
            with self.assertRaisesRegex(ValueError, 'escapes'):
                write_review_manifest(manifest, root)

    def test_empty_callback_index_cannot_follow_an_outside_tools_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            root = base / 'project'
            path, manifest = fixture(root)
            outside = base / 'outside'
            (root / 'tools').rename(outside)
            (root / 'tools').symlink_to(outside, target_is_directory=True)
            before = path.read_bytes()
            manifest['callbacks'] = []
            with self.assertRaisesRegex(ValueError, 'escapes'):
                write_review_manifest(manifest, root)
            with self.assertRaisesRegex(ValueError, 'escapes'):
                load_review_manifest(root)
            self.assertEqual(path.read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
