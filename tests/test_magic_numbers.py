"""The numeric inventory must retain scope, provenance and uncertainty."""

import hashlib
import pytest
from tools.magic_numbers import inventory


def report(tmp_path, source, positions, status="complete"):
    path = tmp_path / "src/game/example.c"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(source)
    return {
        "source_hashes": {
            "src/game/example.c": hashlib.sha256(path.read_bytes()).hexdigest()
        },
        "coverage": {"complete": int(status == "complete"), "total": 1},
        "units": [
            {
                "status": status,
                "findings": [
                    {
                        "check": "readability-magic-numbers",
                        "path": "src/game/example.c",
                        "offset": len(source[:position].encode()),
                        "message": "magic number",
                    }
                    for position in positions
                ],
            }
        ],
    }


def test_comments_strings_and_repeated_findings(tmp_path):
    source = '/* 42 */\nchar *s = "42";\nint example(void) { return 42 + 42; }\n'
    positions = [source.index("42 +"), source.index("42; }")]
    data = report(tmp_path, source, positions)
    data["units"].append(data["units"][0])
    result = inventory(data, tmp_path)
    assert result["occurrences"] == 2
    assert result["review_groups"] == 1
    assert all(
        e["function"] == "example" and e["literal"] == "42" for e in result["entries"]
    )
    for invalid in [source.index("42"), source.index('42"')]:
        with pytest.raises(ValueError, match="does not point at code"):
            inventory(report(tmp_path, source, [invalid]), tmp_path)


def test_same_number_in_different_functions_is_not_one_domain(tmp_path):
    source = "int first(void) { return 17; }\nint second(void) { return 17; }\n"
    result = inventory(
        report(tmp_path, source, [source.index("17"), source.rindex("17")]), tmp_path
    )
    assert result["review_groups"] == 2


def test_data_definitions_assertions_and_masks(tmp_path):
    source = (
        "enum Mode { MODE = 17 };\nint table[] = {17, 18};\n"
        "typedef char SizeCheck[(sizeof(int) == 17) ? 1 : -1];\n"
        "int example(int value) { return value & 17; }\n"
    )
    positions = [i for i in range(len(source)) if source.startswith("17", i)]
    entries = inventory(report(tmp_path, source, positions), tmp_path)["entries"]
    assert [e["category"] for e in entries] == [
        "definition_literal",
        "data_literal",
        "verification_literal",
        "mask_candidate",
    ]


def test_macro_expansion_is_not_a_source_literal(tmp_path):
    source = "void example(void) { SDK_MACRO(); }\n"
    entry = inventory(report(tmp_path, source, [source.index("SDK_MACRO")]), tmp_path)[
        "entries"
    ][0]
    assert entry["literal"] is None
    assert entry["category"] == "macro_expansion_candidate"


def test_partial_ast_remains_explicit_and_complete_header_report_wins(tmp_path):
    source = "int example(void) { return 17; }\n"
    data = report(tmp_path, source, [source.index("17")], "unsupported")
    assert inventory(data, tmp_path)["entries"][0]["analysis"] == "partial-ast"
    data["units"].append(dict(data["units"][0], status="complete"))
    assert inventory(data, tmp_path)["entries"][0]["analysis"] == "complete"


def test_stale_snapshot_and_outside_ownership_are_rejected(tmp_path):
    source = "int example(void) { return 17; }\n"
    data = report(tmp_path, source, [source.index("17")])
    (tmp_path / "src/game/example.c").write_text(source + "\n")
    with pytest.raises(ValueError, match="stale"):
        inventory(data, tmp_path)
    data["source_hashes"] = {"../outside.c": "anything"}
    with pytest.raises(ValueError, match="non-owned"):
        inventory(data, tmp_path)


def test_utf8_byte_offsets_and_pointer_arithmetic_are_review_candidates(tmp_path):
    source = "// 文\nint *example(int *buffer) { return buffer + 17; }\n"
    entry = inventory(report(tmp_path, source, [source.index("17")]), tmp_path)[
        "entries"
    ][0]
    assert entry["line"] == 2 and entry["category"] == "numeric_candidate"
    assert entry["offset"] == len(source[: source.index("17")].encode())
