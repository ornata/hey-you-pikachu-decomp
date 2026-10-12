import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from source_quality import code_only, inspect_source, inventory, recovery_ledger


class QualityTests(unittest.TestCase):
    def test_comments_strings_and_character_literals_are_excluded(self):
        source = '''/* M2C_FIELD(p, s32 *, 0) */
// M2C_UNK D_80000000 temp_v0
const char *s = "M2C_UNK32 D_80000000 temp_v0 p + 4 \\\"";
char c = '\\'';
M2C_UNK32 temp_v0 = M2C_FIELD(p, s32 *, 4);
'''
        items = inspect_source(source)
        self.assertEqual(len(items['field_accesses']), 1)
        self.assertEqual(len(items['unknown_types']), 1)
        self.assertEqual(len(items['temporary_names']), 1)
        self.assertEqual(items['address_globals'], [])
        self.assertEqual(items['unknown_types'][0]['line'], 5)
        self.assertEqual(len(code_only(source)), len(source))

    def test_repetitions_are_not_unique_symbols(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'src').mkdir()
            (root / 'src/a.c').write_text('D_80000000 = D_80000000; temp_v0 = temp_v0;')
            (root / 'src/b.c').write_text('D_80000000;')
            report = inventory(root)
            self.assertEqual(report['totals']['address_globals'],
                             {'occurrences': 3, 'unique_spellings': 1, 'files': 2})

    def test_raw_offsets_remain_candidates_including_legitimate_arithmetic(self):
        items = inspect_source('read(buffer + 4); index = index + 1; M2C_FIELD(p, s32 *, 0x10);')
        self.assertEqual([x['token'] for x in items['raw_offset_candidates']],
                         ['buffer + 4', 'index + 1'])
        self.assertEqual(len(items['field_accesses']), 1)

    def test_function_context_and_ledger_locations(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'src').mkdir()
            (root / 'src/a.c').write_text('void f(void) {\n M2C_UNK temp_v0; temp_v0 = 1;\n}\n')
            report = inventory(root)
            items = report['files']['src/a.c']['temporary_names']
            self.assertEqual([item['function'] for item in items], ['f', 'f'])
            ledger = recovery_ledger(root, report)
            self.assertEqual(ledger['files']['src/a.c']['locations']['temporary_names']['temp_v0'],
                             ['2:10', '2:19'])
            self.assertIn('next_evidence', ledger['policies']['unknown_types'])

    def test_identifier_boundaries(self):
        items = inspect_source('NOT_M2C_UNK M2C_UNK128 D_800000000 not_temp_v0;')
        self.assertFalse(any(items.values()))
