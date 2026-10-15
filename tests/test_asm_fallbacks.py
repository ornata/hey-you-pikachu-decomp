from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from asm_fallbacks import inventory


class FallbackTests(unittest.TestCase):
    def test_pragma_blocks_data_and_standalone_units(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            files = {
                'src/game.c': '#pragma GLOBAL_ASM("build/asm/nonmatchings/f.s")\n#pragma GLOBAL_ASM("build/asm/data/d.s")\n',
                'build/asm/nonmatchings/f.s': 'glabel fallback\n',
                'build/asm/data/d.s': '.word 0\n',
                'build/asm/os/unit.s': 'nonmatching other, 0x10\nglabel other\nnonmatching next, 0x20\nglabel next\n',
            }
            for relative, text in files.items():
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text)
            records, data = inventory(root)
            self.assertEqual(sorted(row[2] for row in records), ['fallback', 'next', 'other'])
            self.assertEqual(data, {'build/asm/data/d.s'})
            (root / 'build/asm/nonmatchings/f.s').unlink()
            self.assertIn('(assembly not generated)', {row[2] for row in inventory(root)[0]})


if __name__ == '__main__':
    unittest.main()


def test_standalone_assembly_is_not_reported_as_a_c_fallback(monkeypatch, capsys):
    import asm_fallbacks
    monkeypatch.setattr(asm_fallbacks, 'inventory', lambda: ([('build/asm/game/stub.s', 'build/asm/game/stub.s', 'stub')], set()))
    monkeypatch.setattr(sys, 'argv', ['asm_fallbacks.py'])
    asm_fallbacks.main()
    output = capsys.readouterr().out
    assert 'GLOBAL_ASM fallbacks: 0 function entries' in output
    assert 'Standalone assembly: 1 function entries' in output
