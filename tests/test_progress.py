import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from progress import CDEF, strip_comments


class CensusTests(unittest.TestCase):
    def test_comments_are_not_functions(self):
        text = "/* void stale(void) {} */\nvoid current(void) {}\n"
        self.assertEqual(len(CDEF.findall(strip_comments(text))), 1)

    def test_comment_tokens_in_strings_are_preserved(self):
        text = 'const char *s = "https://example/ /* literal */";\nvoid current(void) {}\n'
        self.assertIn('"https://example/ /* literal */"', strip_comments(text))
        self.assertEqual(len(CDEF.findall(strip_comments(text))), 1)


if __name__ == "__main__":
    unittest.main()
