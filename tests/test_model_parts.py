"""Synthetic template topology and fail-closed role-consumer bindings."""
import math
import hashlib
import struct
import tempfile
import unittest
from unittest.mock import patch
from pathlib import Path

from tools.script_decoder.model_parts import SOURCE_PINS, decode_template_hierarchy, source_bindings, pikachu_model_catalog, reference_part_roles, cap_appearance_contract


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

    def test_reference_callbacks_and_topology_fail_closed(self):
        hooks = {
            63: (62, (0x80015110, 0x80014E54, 0x800155E0, 0, 0, 0)),
            66: (63, (0x80015110, 0x80014E54, 0x800155E0, 0, 0, 0)),
            118: (43, (0x800139A8, 0x800086E0, 0, 0x800148E0, 0, 0)),
            119: (39, (0x800139B0, 0x800086E0, 0, 0, 0, 0)),
            121: (43, (0x80013848, 0x80013968, 0x800140F0, 0x800146DC, 0, 0)),
        }
        nodes = {kind: [dict(address=kind * 56, parent_address=parent * 56)]
                 for kind, (parent, _) in hooks.items()}
        for kind in (62, 43, 39):
            nodes[kind] = [dict(address=kind * 56, parent_address=None)]
        data = {0x80078140 + kind * 24: struct.pack('>6I', *values)
                for kind, (_, values) in hooks.items()}
        self.assertEqual(len(reference_part_roles(reader(data), nodes)), 5)
        original = data[0x80078140 + 118 * 24]
        data[0x80078140 + 118 * 24] = b'\0' * 24
        with self.assertRaisesRegex(ValueError, 'callback binding'):
            reference_part_roles(reader(data), nodes)
        data[0x80078140 + 118 * 24] = original
        nodes[118][0]['parent_address'] = nodes[39][0]['address']
        with self.assertRaisesRegex(ValueError, 'topology'):
            reference_part_roles(reader(data), nodes)

    def test_cap_predicates_preserve_distinct_actor_scopes(self):
        contract = cap_appearance_contract()
        self.assertEqual(contract['carrier']['actor_scope'], 'carrier_own_hierarchy_root')
        self.assertEqual(contract['held_item_visibility']['actor_scope'], 'global_primary_pikachu')
        self.assertIn('resolved_item_kind_equals80', contract['carrier']['all_conditions'])
        self.assertEqual(contract['released_item_scale']['condition'], 'item_ownership_still_equals2')
        self.assertEqual(contract['released_item_scale']['otherwise'], 'no_scale_request')
        self.assertEqual(contract['carrier']['selected_display_list_word'], 0x01049F80)

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
