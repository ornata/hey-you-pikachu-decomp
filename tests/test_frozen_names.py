import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from check_frozen_names import check, defined_names, hasm_ranges, symbols

YAML = """
segments:
  - name: boot
    type: code
    start: 0x1000
    vram: 0x80000400
    subsegments:
      - [0x1000, hasm, entry]
      - [0x1050, c, main]
      - [0x58D80, hasm, mgu/mtxidentf]
      - [0x58DD0, hasm, mgu/mtxf2l]
      - [0x58E40, c, os/other]
"""


class FakeTree:
    def __init__(self, files):
        self.files_ = files

    def files(self, directory, suffix):
        return sorted(p for p in self.files_ if p.startswith(directory + "/") and p.endswith(suffix))

    def read(self, path):
        return self.files_.get(path)


MANIFEST = {
    "frozen_paths": {"src/os": "libultra", "asm/mgu": "sdk"},
    "frozen_functions": {
        "func_80057B5C": {"severity": "error", "reason": "exception fragment"},
        "proutSprintf": {"severity": "error", "reason": "manual func"},
        "gameOverride": {"severity": "warn", "reason": "port binds it", "port_fix": "voice.cpp"},
    },
}


def tree(src="void osGetTime(void) {\n}\nvoid func_80100000(void) {\n}\n", syms="", extra=None):
    files = {
        "src/os/time.c": src,
        "src/game/p.c": "void proutSprintf(void) {\n}\nvoid gameOverride(void) {\n}\n",
        "config/symbol_addrs.txt": "guMtxIdentF = 0x80058180;\nguMtxF2L = 0x800581D0;\n" + syms,
        "config/pikachu.yaml": YAML,
    }
    files.update(extra or {})
    return FakeTree(files)


class FrozenNameTests(unittest.TestCase):
    def test_unchanged_tree_is_clean(self):
        self.assertEqual(check(MANIFEST, tree(), tree()), [])

    def test_alias_script_move_preserves_protection(self):
        base, now = tree(), tree()
        for snapshot in (base, now):
            snapshot.files_["config/symbol_addrs.txt"] = "guMtxF2L = 0x800581D0;\n"
        base.files_["rodata_aliases.ld"] = "guMtxIdentF = 0x80058180;\n"
        now.files_["linker/rodata_aliases.ld"] = base.files_["rodata_aliases.ld"]
        self.assertEqual(check(MANIFEST, base, now), [])
        now.files_["linker/rodata_aliases.ld"] = "guMtxIdentFast = 0x80058180;\n"
        self.assertTrue(any(severity == "error" and "guMtxIdentF" in message
                            for severity, message in check(MANIFEST, base, now)))

    def test_config_move_preserves_checks_against_old_commits(self):
        base, now = tree(), tree()
        for name in ("pikachu.yaml", "symbol_addrs.txt"):
            base.files_[name] = base.files_.pop("config/" + name)
        self.assertEqual(check(MANIFEST, base, now), [])
        now.files_["config/symbol_addrs.txt"] = now.files_["config/symbol_addrs.txt"].replace(
            "guMtxIdentF", "guMtxIdentFast")
        self.assertTrue(any(severity == "error" and "guMtxIdentF" in message
                            for severity, message in check(MANIFEST, base, now)))

    def test_renamed_source_function_is_reported(self):
        now = tree(src="void osGetTimeNew(void) {\n}\nvoid func_80100000(void) {\n}\n")
        problems = check(MANIFEST, tree(), now)
        self.assertEqual(len(problems), 1)
        self.assertEqual(problems[0][0], "error")  # libultra is never a warning
        self.assertIn("osGetTime is no longer defined", problems[0][1])

    def test_anonymous_function_may_be_renamed(self):
        now = tree(src="void osGetTime(void) {\n}\nvoid rename_me(void) {\n}\n")
        self.assertEqual(check(MANIFEST, tree(), now), [])

    def test_function_moved_between_files_is_fine(self):
        now = tree(src="", extra={"src/os/other.c": "void osGetTime(void) {\n}\n"})
        self.assertEqual(check(MANIFEST, tree(), now), [])

    def test_asm_symbol_rename_is_reported_by_address(self):
        now = tree()
        now.files_["config/symbol_addrs.txt"] = "guMtxIdentFast = 0x80058180;\nguMtxF2L = 0x800581D0;\n"
        problems = check(MANIFEST, tree(), now)
        self.assertEqual(len(problems), 1)
        self.assertEqual(problems[0][0], "error")
        self.assertIn("guMtxIdentF @ 0x80058180 is now guMtxIdentFast", problems[0][1])

    def test_symbol_outside_hasm_ranges_is_free(self):
        base = tree(syms="osSomething = 0x80059000;\n")
        now = tree(syms="osRenamed = 0x80059000;\n")
        self.assertEqual(check(MANIFEST, base, now), [])

    def test_frozen_anonymous_name_taken_by_symbol_addrs(self):
        now = tree(syms="exceptionFragment = 0x80057B5C;\n")
        problems = check(MANIFEST, tree(), now)
        self.assertEqual(len(problems), 1)
        self.assertEqual(problems[0][0], "error")
        self.assertIn("func_80057B5C @ 0x80057b5c is now exceptionFragment", problems[0][1])

    def test_missing_libultra_function_is_an_error(self):
        now = tree(extra={"src/game/p.c": "void gameOverride(void) {\n}\n"})
        problems = check(MANIFEST, tree(), now)
        self.assertEqual([p[0] for p in problems], ["error"])
        self.assertIn("proutSprintf is not defined anywhere", problems[0][1])

    def test_game_side_port_binding_is_a_warning(self):
        now = tree(extra={"src/game/p.c": "void proutSprintf(void) {\n}\n"})
        problems = check(MANIFEST, tree(), now)
        self.assertEqual([p[0] for p in problems], ["warn"])
        self.assertIn("gameOverride is not defined anywhere", problems[0][1])
        self.assertIn("[port: voice.cpp]", problems[0][1])

    def test_anonymous_function_defined_in_c_is_caught_when_renamed(self):
        manifest = {"frozen_functions": {"func_80002810": {"severity": "warn", "reason": "port", "port_fix": "voice.cpp"}}}
        base = tree(extra={"src/main.c": "int func_80002810(int a) {\n}\n"})
        now = tree(extra={"src/main.c": "int controllerStatus(int a) {\n}\n"})
        problems = check(manifest, base, now)
        self.assertEqual([p[0] for p in problems], ["warn"])
        self.assertIn("func_80002810 is not defined anywhere", problems[0][1])
        self.assertEqual(check(manifest, base, base), [])

    def test_sdk_c_may_become_generated_assembly(self):
        base = tree(syms="osGetTime = 0x80058240;\n")
        now = tree(src="", syms="osGetTime = 0x80058240;\n")
        now.files_["config/pikachu.yaml"] = YAML.replace("c, os/other", "asm, os/other")
        self.assertEqual(check(MANIFEST, base, now), [])
        now.files_["config/symbol_addrs.txt"] = now.files_["config/symbol_addrs.txt"].replace("80058240", "80058244")
        self.assertTrue(any("osGetTime" in p[1] for p in check(MANIFEST, base, now)))

    def test_generated_sdk_names_stay_frozen_in_a_fresh_history(self):
        base = tree(src="", syms="osGetTime = 0x80058240;\n")
        base.files_["config/pikachu.yaml"] = YAML.replace("c, os/other", "asm, os/other")
        now = FakeTree(dict(base.files_))
        now.files_["config/symbol_addrs.txt"] = now.files_["config/symbol_addrs.txt"].replace("osGetTime", "renamed")
        self.assertTrue(any("generated SDK symbol osGetTime" in p[1] for p in check(MANIFEST, base, now)))

    def test_helpers(self):
        self.assertEqual(defined_names("/* void x(void) {} */\nint f(int a) {\n}\n", lambda p: None), {"f"})
        self.assertEqual(symbols("a = 0x10;\nb = 0x20;"), {"a": 16, "b": 32})
        r = hasm_ranges(YAML, "mgu")
        self.assertEqual(len(r), 2)
        self.assertEqual(r[0][0], 0x58D80 + 0x7FFFF400)
        self.assertEqual(r[1][1], 0x58E40 + 0x7FFFF400)


if __name__ == "__main__":
    unittest.main()
