"""Progress must report missing evidence rather than inventing completion."""

import os
from pathlib import Path
import sys
import time

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import progress
from progress import CDEF, strip_comments, definitions, inventory


def write(root, relative, content):
    path = root / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)
    return path


def fixture(root):
    write(
        root,
        "config/pikachu.yaml",
        "segments:\n  - subsegments:\n      - [0, hasm, game/leaf]\n      - [8, asm, os/sdk]\n",
    )
    write(
        root,
        "src/game/unit.c",
        'void current(void) {}\n#pragma GLOBAL_ASM("src/nonmatching/game/fallback.s")\n',
    )
    write(
        root,
        "src/nonmatching/game/fallback.s",
        "glabel fallback\n  abs.d $f0,$f12\n  nop\n",
    )
    write(root, "build/asm/game/leaf.s", "nonmatching leaf, 4\nglabel leaf\n  nop\n")
    write(root, "build/asm/os/sdk.s", "glabel sdk\n  nop\n")


def test_comments_are_not_functions():
    text = "/* void stale(void) {} */\nvoid current(void) {}\n"
    assert len(CDEF.findall(strip_comments(text))) == 1
    assert definitions(text) == ["current"]


def test_comment_tokens_in_strings_are_preserved():
    text = 'const char *s = "https://example/ /* literal */";\nvoid current(void) {}\n'
    assert '"https://example/ /* literal */"' in strip_comments(text)
    assert definitions(text) == ["current"]


def test_literals_macros_and_nested_control_flow_do_not_count():
    text = """const char *s = "void fake(void) {}";
#define FAKE(x) \\
void macro(void) {}
static
void real(void (*callback)(int)) { if (1) {} }
int old(a)
int a;
{ return a; }
"""
    assert definitions(text) == ["real", "old"]
    assert definitions("int/**/real(void) {}") == ["real"]


def test_pragma_whitespace_comments_and_quoted_pragmas():
    text = """// #pragma GLOBAL_ASM("ignored.s")
# pragma GLOBAL_ASM ( "real.s" )
const char *s = "\\
#pragma GLOBAL_ASM(\\"fake.s\\")";
"""
    assert progress.assembly_references(text) == ["real.s"]


def test_only_references_and_configured_units_count(tmp_path, monkeypatch):
    fixture(tmp_path)
    write(tmp_path, "src/os/sdk.c", "void excluded(void) {}")
    write(tmp_path, "build/asm/nonmatchings/stale.s", "glabel stale\n")
    write(tmp_path, "build/asm/game/unconfigured.s", "glabel stale2\n")
    write(
        tmp_path,
        "src/game/other.c",
        '# pragma GLOBAL_ASM ( "src/nonmatching/game/fallback.s" )\n',
    )
    monkeypatch.setattr(
        progress,
        "linked_sizes",
        lambda root: ({"fallback": 8}, time.time_ns() + 1000000000),
    )
    result = inventory(tmp_path)
    assert result["funcs_matched"] == 1
    assert result["funcs_total"] == 2
    assert result["funcs_unmatched"] == 1
    assert result["asm_bytes_remaining"] == 8
    assert result["funcs_standalone_game"] == 1
    assert result["funcs_excluded_sdk"] == 1
    assert result["inventory_complete"]


def test_multiple_function_labels_count_once_each_and_data_does_not(tmp_path):
    fixture(tmp_path)
    write(
        tmp_path,
        "src/nonmatching/game/fallback.s",
        "nonmatching a, 4\nglabel a\nglabel b\n",
    )
    write(tmp_path, "src/game/data.c", '#pragma GLOBAL_ASM("src/nonmatching/data.s")\n')
    write(tmp_path, "src/nonmatching/data.s", "dlabel table\n.word 0\n")
    result = inventory(tmp_path)
    assert result["funcs_unmatched"] == 2
    assert result["asm_bytes_remaining"] is None
    assert result["asm_functions_unknown_size"] == 2
    assert result["data_only_assembly"] == ["src/nonmatching/data.s"]


def test_missing_fallback_makes_denominator_unknown(tmp_path):
    fixture(tmp_path)
    (tmp_path / "src/nonmatching/game/fallback.s").unlink()
    result = inventory(tmp_path)
    assert result["funcs_total"] is None
    assert result["asm_bytes_remaining"] is None
    assert not result["inventory_complete"]


def test_stale_linked_sizes_are_not_accepted(tmp_path, monkeypatch):
    fixture(tmp_path)
    monkeypatch.setattr(progress, "linked_sizes", lambda root: ({"fallback": 8}, 0))
    result = inventory(tmp_path)
    assert result["asm_bytes_remaining"] is None


def test_text_and_json_strict_exit(tmp_path, monkeypatch, capsys):
    fixture(tmp_path)
    monkeypatch.setattr(progress, "ROOT", tmp_path)
    # inventory's default is bound at definition time; use the explicit fixture root.
    original = progress.inventory
    monkeypatch.setattr(progress, "inventory", lambda: original(tmp_path))
    monkeypatch.setattr(sys, "argv", ["progress.py", "--json", "--strict"])
    assert progress.main() == 1
    import json

    assert json.loads(capsys.readouterr().out)["asm_bytes_remaining"] is None
    monkeypatch.setattr(sys, "argv", ["progress.py"])
    assert progress.main() == 0
    assert "bytes: unknown" in capsys.readouterr().out


def test_linked_duplicate_overlay_names_have_unknown_sizes(tmp_path, monkeypatch):
    from types import SimpleNamespace

    write(tmp_path, "build/pikachu.elf", "API fixture")
    fake = SimpleNamespace(
        sections=[(0, 2)],
        symbols=lambda index: [
            ("ambiguous", 0x80100000, 8, 18, 0, 3),
            ("ambiguous", 0x80100000, 8, 18, 0, 4),
            ("unique", 0x80000000, 8, 18, 0, 3),
            ("data", 0x80010000, 8, 17, 0, 3),
        ],
    )
    monkeypatch.setitem(
        sys.modules, "compare_objects", SimpleNamespace(Elf=lambda path: fake)
    )
    sizes, timestamp = progress.linked_sizes(tmp_path)
    assert sizes == {"unique": 8}
    assert timestamp is not None


def test_small_remaining_fraction_is_never_printed_as_complete(monkeypatch, capsys):
    monkeypatch.setattr(
        progress,
        "inventory",
        lambda: dict(
            funcs_matched=99999,
            funcs_total=100000,
            funcs_unmatched=1,
            asm_bytes_remaining=8,
            funcs_standalone_game=0,
            funcs_excluded_sdk=0,
            inventory_complete=True,
        ),
    )
    monkeypatch.setattr(sys, "argv", ["progress.py", "--strict"])
    assert progress.main() == 0
    output = capsys.readouterr().out
    assert "99.99%" in output
    assert "100.00%" not in output


def test_invalid_build_is_an_error_not_a_zero_byte_report(monkeypatch, capsys):
    def invalid():
        raise ValueError("invalid linked ELF")

    monkeypatch.setattr(progress, "inventory", invalid)
    monkeypatch.setattr(sys, "argv", ["progress.py", "--json"])
    with pytest.raises(SystemExit) as error:
        progress.main()
    assert error.value.code == 2
    assert "invalid linked ELF" in capsys.readouterr().err


@pytest.mark.parametrize(
    "configuration",
    [
        "segments: [",
        "null",
        "segments:\n  - subsegments:\n      - [0, asm]\n",
        "segments:\n  - type: hasm\n",
    ],
)
def test_invalid_configuration_cannot_report_completion(tmp_path, configuration):
    write(tmp_path, "config/pikachu.yaml", configuration)
    with pytest.raises(ValueError):
        inventory(tmp_path)
