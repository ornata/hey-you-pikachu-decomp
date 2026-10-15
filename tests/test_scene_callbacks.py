"""Protect scene callback resolution against overlapping overlay addresses."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from scene_callbacks import RUNTIME, configuration_locations, report


def catalog():
    return 'SceneConfigurationPrefix *gAreaConfigurations[23] = {' + ','.join(
        f'(SceneConfigurationPrefix *)(scene_test{i}_DATA_START + 0x0)' for i in range(23)) + '};'


class SceneCallbackTests(unittest.TestCase):
    def test_catalog_requires_complete_explicit_overlay_selection(self):
        self.assertEqual(len(configuration_locations(catalog())), 23)
        for source in [catalog().replace('[23]', '[24]'),
                       catalog().replace('scene_test1_DATA_START', 'scene_test0_DATA_START'),
                       catalog().replace('+ 0x0', '+ dynamicOffset', 1)]:
            with self.assertRaises(ValueError):
                configuration_locations(source)

    def test_symbolic_extent_still_requires_the_independent_retail_count(self):
        source = catalog().replace('[23]', '[SCENE_AREA_COUNT]')
        self.assertEqual(len(configuration_locations(source, 'enum { SCENE_AREA_COUNT = 23 };')), 23)
        for header in [None, 'enum { SCENE_AREA_COUNT = 24 };', '// SCENE_AREA_COUNT = 23',
                       'enum { SCENE_AREA_COUNT = 23, SCENE_AREA_COUNT = 23 };']:
            with self.assertRaises(ValueError):
                configuration_locations(source, header)

    def fixture(self, wrong_overlay=False, wrong_catalog=False):
        symbols, data = [], {}
        sections = [(0, 0, 0, 0, 0, 0)] * 26
        definitions = []
        def symbol(name, address, size, info, section):
            symbols.append((name, address, size, info, 0, section))
        def definition(name, returns, parameters):
            definitions.append(dict(name=name, returns=returns, parameters=parameters,
                                    file='src/test.c', line=1, definition=True))
        sections[1] = (0, 0, 0, 0x1000, 0, 12)
        data[1] = struct.pack('>3I', 0x1100, 0x1104, 0x1108)
        for i, name in enumerate(RUNTIME):
            symbol(name, 0x1000 + i * 4, 4, 17, 1)
            symbol('runtime' + str(i), 0x1100 + i * 4, 4, 18, 1)
            definition('runtime' + str(i), 'void', [])
        sections[2] = (0, 0, 0, 0x2000, 0, 92)
        data[2] = struct.pack('>23I', *([0x8000] * 22 + [0x8004 if wrong_catalog else 0x8000]))
        symbol('gAreaConfigurations', 0x2000, 92, 17, 2)
        for i in range(23):
            section = i + 3
            sections[section] = (0, 0, 0, 0x8000, 0, 0x7C)
            content = bytearray(0x7C)
            struct.pack_into('>I', content, 0x3C, 0x8100)
            data[section] = bytes(content)
            symbol(f'scene_test{i}_DATA_START', 0x8000, 0, 16, section)
            symbol('sampler' + str(i), 0x8100, 4, 18, section)
            definition('sampler' + str(i), 's32', [] if wrong_overlay and i == 22 else ['float'] * 3)
        class FakeElf:
            names = ['.symtab']
            def __init__(self, path):
                self.sections = sections
            def symbols(self, index):
                return iter(symbols)
            def data(self, index):
                return data[index]
        return FakeElf, {'declarations': definitions}

    def run_fixture(self, **kwargs):
        fake, declarations = self.fixture(**kwargs)
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source = root / 'src/ovl/area_terrain_maze.c'
            source.parent.mkdir(parents=True)
            source.write_text(catalog())
            with patch('scene_callbacks.Elf', fake), patch('scene_callbacks.interface_report', return_value=declarations):
                return report(root, root / 'unused.elf')

    def test_same_virtual_address_in_23_overlays_resolves_separately(self):
        result = self.run_fixture()
        self.assertEqual(result['errors'], [])
        self.assertEqual(len(result['entries']), 26)

    def test_wrong_signature_in_last_overlay_is_not_hidden_by_first(self):
        errors = self.run_fixture(wrong_overlay=True)['errors']
        self.assertEqual(len(errors), 1)
        self.assertIn('scene_test22_DATA_START', errors[0])

    def test_linked_catalog_must_agree_with_source_overlay_selection(self):
        with self.assertRaisesRegex(ValueError, 'linked catalog disagrees'):
            self.run_fixture(wrong_catalog=True)
