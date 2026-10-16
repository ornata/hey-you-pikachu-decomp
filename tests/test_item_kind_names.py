"""Authored item-kind metadata preserves retained labels and creator operands."""

import hashlib
import json
import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.script_decoder import items
from tools.script_decoder.formats import decode_vm
from tests.test_script_argument_recovery import literal, native, NAMES


class ItemKindNameTests(unittest.TestCase):
    synthetic_names = {81: 'ITEM_KIND_AUTHORED_A', 106: 'ITEM_KIND_AUTHORED_B'}

    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        (self.root / 'include').mkdir()
        (self.root / 'include/item_kind_ids.h').write_text((items.ROOT / 'include/item_kind_ids.h').read_text())
        (self.root / 'include/item_kind.h').write_text('enum { ITEM_STORAGE_CLASS_AUTHORED = 1 };')
        (self.root / 'include/item_types.h').write_text('typedef enum ItemStateFlag { ITEM_STATE_AUTHORED = 1 } ItemStateFlag;')
        (self.root / 'include/item_response.h').write_text('typedef enum ItemInteractionResponseCategory { ITEM_RESPONSE_AUTHORED = 1 } ItemInteractionResponseCategory; typedef enum ItemReactionCode { ITEM_REACTION_AUTHORED = 1 } ItemReactionCode;')

    def parse(self, body='ITEM_KIND_AUTHORED_A=81,ITEM_KIND_AUTHORED_B=106', extra=''):
        (self.root / 'include/item_kind_ids.h').write_text(f'typedef enum ItemKindId {{ {body} }} ItemKindId; {extra}')
        digest = hashlib.sha256(json.dumps(self.synthetic_names, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
        with patch.object(items, 'ROOT', self.root), patch.object(items, '_KIND_NAMES_SHA256', digest):
            return items.item_kind_names()

    def test_strict_scoped_mapping_and_nonnegative_domain(self):
        self.assertEqual(self.parse(), self.synthetic_names)
        self.assertEqual(self.parse('ITEM_KIND_AUTHORED_A=0x51,ITEM_KIND_AUTHORED_B=0x6A,'), self.synthetic_names)
        extra = 'enum { ITEM_KIND_OUTSIDE_ALIAS=81 }; /* ITEM_KIND_COMMENT=99 */ const char *label="ITEM_KIND_STRING=2";'
        self.assertEqual(self.parse(extra=extra), self.synthetic_names)
        for body in ('ITEM_KIND_AUTHORED_A=81,ITEM_KIND_AUTHORED_B=81',
                     'ITEM_KIND_AUTHORED_A=81,ITEM_KIND_AUTHORED_A=106',
                     'ITEM_KIND_AUTHORED_A=81,ITEM_KIND_AUTHORED_B=106+0',
                     'ITEM_KIND_AUTHORED_A=81,ITEM_KIND_AUTHORED_B=ITEM_KIND_AUTHORED_A',
                     'ITEM_KIND_AUTHORED_A=81,ITEM_KIND_AUTHORED_B',
                     'ITEM_KIND_AUTHORED_A=-81,ITEM_KIND_AUTHORED_B=106',
                     'ITEM_KIND_AUTHORED_A=81,ITEM_KIND_CHANGED_B=106'):
            with self.subTest(body=body), self.assertRaises(ValueError):
                self.parse(body)
        with self.assertRaises(ValueError):
            self.parse(extra='typedef enum ItemKindId { ITEM_KIND_OTHER=81 } ItemKindId;')

    def catalog(self):
        data = bytearray(166 * 0x74)
        for kind in range(166):
            struct.pack_into('>i', data, kind * 0x74, kind)
        for kind in (81, 106):
            struct.pack_into('>I', data, kind * 0x74 + 0x64, 0x90000000)
        struct.pack_into('>I', data, 81 * 0x74 + 0x54, 0x81234560)
        label = b'synthetic retained label\0'
        class Image:
            def symbol(self, name):
                if name != 'gItemKinds':
                    raise AssertionError('unexpected symbol')
                return name, 0x80000000
            def read_address(self, address, size):
                if (address, size) == (0x80000000, len(data)):
                    return bytes(data), dict(address=address)
                if size == 1 and 0x90000000 <= address < 0x90000000 + len(label):
                    return label[address - 0x90000000:address - 0x90000000 + 1], dict(address=address)
                raise AssertionError('escaped authored table/text extent')
        with patch.object(items, 'ROOT', self.root), patch.object(items, '_TABLE_SHA256', hashlib.sha256(data).hexdigest()):
            return items.item_kind_catalog(Image())

    def test_catalog_only_known_ids_named_without_replacing_display_labels(self):
        catalog = self.catalog()
        names = items.item_kind_names()
        self.assertEqual(len(names), 18)
        self.assertEqual(set(names), {81, 138, *range(106, 122)})
        self.assertEqual([row['item_kind'] for row in catalog['entries']], list(range(166)))
        for row in catalog['entries']:
            if row['item_kind'] in names:
                self.assertEqual(row['recovered_kind_name'], names[row['item_kind']])
            else:
                self.assertNotIn('recovered_kind_name', row)
        for kind in (81, 106):
            self.assertEqual(catalog['entries'][kind]['display_names']['default']['text'], 'synthetic retained label')
            self.assertEqual(catalog['entries'][kind]['display_names']['alternate']['status'], 'null')
        self.assertTrue(catalog['entries'][81]['has_model_template'])
        self.assertFalse(catalog['entries'][106]['has_model_template'])
        self.assertEqual(catalog['entries'][138]['display_names']['default']['status'], 'null')

    def test_creation_argument_roles_are_unchanged_by_recovered_names(self):
        catalog = self.catalog()
        plain = dict(catalog, entries=[{key:value for key,value in row.items() if key != 'recovered_kind_name'} for row in catalog['entries']])
        for function in ('actorScriptCreateItem', 'actorScriptCreateStationaryItem'):
            raw = b''.join(literal(number) for number in (1, 2, 3, 4, 5, 106)) + native(777) + bytes([35])
            definitions = {777: dict(address=0x80000000, functions=[function], inline_operands=[])}
            instructions = decode_vm(raw, NAMES, natives=definitions)['instructions']
            named = items.item_creation_requests(instructions, catalog)[0]
            unnamed = items.item_creation_requests(instructions, plain)[0]
            self.assertEqual(named['argument_expressions'], unnamed['argument_expressions'])
            self.assertEqual(named['constant_item_kind'], 106)
            self.assertEqual(named['kind_expression'], unnamed['kind_expression'])
            self.assertEqual(named['item_configuration']['display_names'], unnamed['item_configuration']['display_names'])
            self.assertEqual(named['item_configuration']['recovered_kind_name'], 'ITEM_KIND_TASTY_STEW')


if __name__ == '__main__':
    unittest.main()
