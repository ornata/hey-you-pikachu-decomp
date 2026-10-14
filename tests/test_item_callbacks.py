"""Exercise item callback contract failures without ROM inputs."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from item_callbacks import TABLES, audit_table


class ItemCallbackTests(unittest.TestCase):
    def definition(self, returns, parameters):
        return {'file': 'src/item.c', 'line': 1, 'returns': returns, 'parameters': parameters}

    def test_each_kind_hook(self):
        _, stride, slots = TABLES['gItemKinds']
        data = bytearray(stride)
        names, definitions = {}, {}
        for address, (offset, (name, returns, parameters)) in enumerate(slots.items(), 1):
            struct.pack_into('>I', data, offset, address)
            names[address] = [name]
            definitions[name] = self.definition(returns, parameters)
        result = audit_table(data, 1, stride, slots, names, definitions)
        self.assertEqual(result['errors'], [])
        self.assertEqual(len(result['entries']), 4)

    def test_collision_width_and_release_address_contract(self):
        for offset, returns, parameters in [(0x50, 's32', ['u8*', 's32']),
                                            (0x48, 'void', ['u8*', 'u8*'])]:
            data = bytearray(0x74)
            struct.pack_into('>I', data, offset, 1)
            result = audit_table(data, 1, 0x74, TABLES['gItemKinds'][2],
                                 {1: ['hook']}, {'hook': self.definition(returns, parameters)})
            self.assertTrue(result['errors'])

    def test_optional_kind_hooks_and_required_mode(self):
        self.assertEqual(audit_table(bytes(0x74), 1, 0x74, TABLES['gItemKinds'][2], {}, {})['errors'], [])
        self.assertTrue(audit_table(bytes(4), 1, 4, TABLES['gItemModeUpdateHandlers'][2],
                                    {}, {}, require_all=True)['errors'])

    def test_mode_signature_and_unknown_target(self):
        slots = TABLES['gItemModeUpdateHandlers'][2]
        for definitions in [{}, {'hook': self.definition('void', [])},
                            {'hook': self.definition('s32', ['u8*'])}]:
            self.assertTrue(audit_table(struct.pack('>I', 1), 1, 4, slots,
                                        {1: ['hook']}, definitions, require_all=True)['errors'])

    def test_extent_and_slot_bounds(self):
        for data, count, stride, slots in [(bytes(3), 1, 4, {}), (bytes(4), 0, 4, {}),
                                          (bytes(4), 1, 4, {4: ('bad', 'void', [])}),
                                          (bytes(8), 1, 8, {1: ('bad', 'void', [])})]:
            with self.assertRaises(ValueError):
                audit_table(data, count, stride, slots, {}, {})
