"""Synthetic template topology and fail-closed role-consumer bindings."""
import math
import hashlib
import struct
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path

from tools.script_decoder.model_parts import SOURCE_PINS, decode_template_hierarchy, source_bindings, pikachu_model_catalog


def template(kind=38, sibling=0, child=0, gfx=0x0A001234, x=1.):
    return struct.pack('>10fIIIHH', 1., 1., 1., 0., 0., 0., x, 0., 0., 2.,
                       sibling, child, gfx, kind, 0)


def reader(nodes):
    def read(address, size):
        if address not in nodes:
            raise ValueError('unsupported template mapping')
        return nodes[address], dict(address=address)
    return read


class ModelPartTests(unittest.TestCase):
    def test_root_sibling_parent_and_order(self):
        nodes = {0x1000: template(37, 0x1100, 0x1200),
                 0x1100: template(122), 0x1200: template(38, 0x1300),
                 0x1300: template(38)}
        rows = decode_template_hierarchy(reader(nodes), 0x1000)
        self.assertEqual([r['kind'] for r in rows], [37, 38, 38, 122])
        self.assertEqual([r['parent_address'] for r in rows], [None, 0x1000, 0x1000, None])
        self.assertEqual([r['path'] for r in rows], [[0], [0, 0], [0, 1], [1]])
        self.assertEqual(rows[1]['display_list_word'], 0x0A001234)
        self.assertNotIn('vertices', rows[1])

    def test_cycle_and_shared_links_rejected(self):
        for nodes in [{0x1000: template(child=0x1000)},
                      {0x1000: template(sibling=0x1100, child=0x1100), 0x1100: template()}]:
            with self.assertRaisesRegex(ValueError, 'cycle or shared'):
                decode_template_hierarchy(reader(nodes), 0x1000)

    def test_node_depth_budget(self):
        nodes = {0x1000: template(child=0x1100), 0x1100: template()}
        for kwargs in [dict(max_nodes=1), dict(max_depth=1)]:
            with self.assertRaisesRegex(ValueError, 'budget'):
                decode_template_hierarchy(reader(nodes), 0x1000, **kwargs)
        for kwargs in [dict(max_nodes=True), dict(max_depth=1.), dict(max_nodes=4097), dict(max_depth=0)]:
            with self.assertRaises(ValueError):
                decode_template_hierarchy(reader(nodes), 0x1000, **kwargs)

    def test_unsupported_mapping_alignment_and_incomplete_node(self):
        for root in [0, True, 0x1001, -1]:
            with self.assertRaises(ValueError):
                decode_template_hierarchy(reader({}), root)
        with self.assertRaisesRegex(ValueError, 'incomplete'):
            decode_template_hierarchy(reader({0x1000: b'\0' * 55}), 0x1000)
        with self.assertRaisesRegex(ValueError, 'unsupported template mapping'):
            decode_template_hierarchy(reader({0x1000: template(child=0x1100)}), 0x1000)

    def test_nonfinite_bind_rejected(self):
        for x in [math.inf, math.nan]:
            with self.assertRaisesRegex(ValueError, 'nonfinite'):
                decode_template_hierarchy(reader({0x1000: template(x=x)}), 0x1000)

    def test_changed_configuration_roles_fail_closed(self):
        class Image:
            rom = b'synthetic image'
            sha1 = hashlib.sha1(rom).hexdigest()

            def read_address(self, address, size):
                data = bytearray(84)
                struct.pack_into('>I', data, 24, 0x800797B0)
                # Incorrect first attachment; do not assign recovered roles.
                struct.pack_into('>5i', data, 64, 63, 59, 53, 0, 0)
                return bytes(data), dict(address=address)
        with patch('tools.script_decoder.model_parts.source_bindings', return_value=[]), \
             patch('tools.script_decoder.model_parts.ROM_SHA1', Image.sha1):
            with self.assertRaisesRegex(ValueError, 'configuration role'):
                pikachu_model_catalog(Image())

    def test_source_stale_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for path in SOURCE_PINS:
                (root / path).parent.mkdir(parents=True, exist_ok=True)
                (root / path).write_text('changed consumer')
            with self.assertRaisesRegex(ValueError, 'stale model part'):
                source_bindings(root)


if __name__ == '__main__':
    unittest.main()
