import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import materialize_ultralib as sdk


class LocalSdkTests(unittest.TestCase):
    def test_missing_and_changed_headers_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            entry = {'path': 'include/test.h', 'sha256': hashlib.sha256(b'expected').hexdigest()}
            self.assertIn('missing', sdk.verify([entry], root)[0])
            (root / 'include').mkdir()
            (root / entry['path']).write_bytes(b'changed')
            self.assertIn('checksum differs', sdk.verify([entry], root)[0])
            (root / entry['path']).write_bytes(b'expected')
            self.assertEqual(sdk.verify([entry], root), [])

    def test_failed_import_does_not_copy_valid_subset(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, dest = root / 'source', root / 'dest'
            (source / 'include').mkdir(parents=True)
            (source / 'include/a.h').write_bytes(b'expected')
            entries = [{'path': f'include/{name}.h', 'sha256': hashlib.sha256(b'expected').hexdigest()}
                       for name in ('a', 'b')]
            manifest = root / 'manifest.json'
            manifest.write_text(json.dumps({'files': entries}))
            with patch.object(sdk, 'ROOT', dest), patch.object(sdk, 'MANIFEST', manifest), \
                    patch.object(sys, 'argv', ['sdk', '--from', str(source)]):
                self.assertEqual(sdk.main(), 1)
                self.assertFalse(dest.exists())
                (source / 'include/b.h').write_bytes(b'expected')
                self.assertEqual(sdk.main(), 0)
                self.assertEqual((dest / 'include/a.h').read_bytes(), b'expected')
