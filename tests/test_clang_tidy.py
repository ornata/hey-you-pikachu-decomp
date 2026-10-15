"""Clang analysis must preserve target settings and reject unsafe edits."""

import hashlib
import json
from pathlib import Path
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import clang_tidy as tidy


def test_ido_flags_preserve_abi_defines_and_per_unit_isa():
    flags = [
        "-c",
        "-Wab,-r4300_mul",
        "-non_shared",
        "-G0",
        "-Xcpluscomm",
        "-Xfullwarn",
        "-signed",
        "-O2",
        "-mips1",
        "-32",
        "-nostdinc",
        "-D_LANGUAGE_C",
        "-Iinclude",
        "-I",
        "src",
        "-o",
        "build/unit.o",
        "src/unit.c",
    ]
    result = tidy.adapt_flags(flags, "src/unit.c")
    assert result[:4] == ["-target", "mips-unknown-linux-gnu", "-mabi=32", "-std=gnu89"]
    assert "-march=mips1" in result
    assert "-fsigned-char" in result
    assert "-nostdinc" in result and "-D_LANGUAGE_C" in result
    assert result[result.index("-I") + 1] == "src"
    assert "-Wab,-r4300_mul" not in result


@pytest.mark.parametrize("flags", [["-mips2", "-unknown"], ["-O2"], ["-mips2", "-I"]])
def test_unmapped_or_incomplete_flags_fail(flags):
    with pytest.raises(ValueError):
        tidy.adapt_flags(flags, "src/unit.c")


def test_only_reviewed_units_downgrade_legacy_conversions():
    assert "-Wno-error=int-conversion" in tidy.adapt_flags(
        ["-mips2"], "src/game/texture_cache.c"
    )
    assert "-Wno-error=int-conversion" not in tidy.adapt_flags(
        ["-mips2"], "src/game/new.c"
    )


def test_binasm_wrapper_and_make_commands_are_understood(tmp_path):
    source = tmp_path / "src/game/action_script_commands.c"
    source.parent.mkdir(parents=True)
    source.write_text("void f(void) {}")
    command = "python3 tools/asm-processor/build.py -O2 python3 tools/ido_binasm_patch.py patches.json ido -- as -mabi=32 -- -c -mips2 -signed -Iinclude -o build/src/game/action_script_commands.o src/game/action_script_commands.c"
    result = tidy.commands_from_make(command, tmp_path, {source})
    assert len(result) == 1
    assert "-march=mips2" in result[0]["arguments"]
    assert result[0]["file"] == str(source)


def test_missing_units_cannot_be_silently_skipped(tmp_path):
    with pytest.raises(ValueError, match="incomplete compilation inventory"):
        tidy.commands_from_make("", tmp_path, {tmp_path / "src/forgotten.c"})


def test_ownership_excludes_sdk_generated_and_external_symlinks(tmp_path, monkeypatch):
    manifest = tmp_path / "docs/LOCAL_SDK_FILES.json"
    manifest.parent.mkdir()
    manifest.write_text(json.dumps(dict(files=[dict(path="include/PR/os.h")])))
    monkeypatch.setattr(
        tidy.subprocess,
        "check_output",
        lambda *a, **k: b"src/game/good.c\0include/owned.h\0include/PR/os.h\0src/os/sdk.c\0build/generated.c\0",
    )
    # The Git query is scoped, but filter generated paths even if a caller supplies them.
    actual = tidy.owned_files(tmp_path)
    assert tmp_path / "src/game/good.c" in actual
    assert tmp_path / "include/owned.h" in actual
    assert tmp_path / "include/PR/os.h" not in actual
    assert tmp_path / "src/os/sdk.c" not in actual


def edit_fixture(tmp_path):
    source = tmp_path / "src/game/unit.c"
    source.parent.mkdir(parents=True)
    source.write_bytes(b"void f(void) {}\n")
    hashes = {"src/game/unit.c": hashlib.sha256(source.read_bytes()).hexdigest()}
    edit = dict(FilePath=str(source), Offset=0, Length=0, ReplacementText=" ")
    return source, hashes, edit


def test_replacements_are_deduplicated_without_writing(tmp_path):
    source, hashes, edit = edit_fixture(tmp_path)
    before = source.read_bytes()
    result = tidy.validated_replacements(
        [dict(replacements=[edit, edit])], tmp_path, {source}, hashes
    )
    assert result == {source: [edit]}
    assert source.read_bytes() == before


@pytest.mark.parametrize(
    "case", ["outside", "stale", "bounds", "overlap", "conflicting-insertion"]
)
def test_invalid_replacements_are_rejected(tmp_path, case):
    source, hashes, edit = edit_fixture(tmp_path)
    edits = [dict(edit)]
    if case == "outside":
        edits[0]["FilePath"] = str(tmp_path / "include/PR/os.h")
    if case == "stale":
        source.write_text("changed")
    if case == "bounds":
        edits[0]["Length"] = 1000
    if case == "overlap":
        edits = [dict(edit, Length=5), dict(edit, Offset=2, Length=1)]
    if case == "conflicting-insertion":
        edits.append(dict(edit, ReplacementText="different"))
    with pytest.raises(ValueError):
        tidy.validated_replacements(
            [dict(replacements=edits)], tmp_path, {source}, hashes
        )


def test_sdk_findings_are_filtered_but_sdk_parse_errors_remain_visible(tmp_path):
    source = tmp_path / "src/game/unit.c"

    def diagnostic(name, path, level):
        return dict(
            DiagnosticName=name,
            Level=level,
            DiagnosticMessage=dict(
                Message="diagnostic", FilePath=str(path), FileOffset=0, Replacements=[]
            ),
        )

    document = dict(
        Diagnostics=[
            diagnostic("readability-braces-around-statements", source, "Warning"),
            diagnostic(
                "readability-identifier-length", tmp_path / "include/PR/os.h", "Warning"
            ),
            diagnostic("clang-diagnostic-error", tmp_path / "include/PR/os.h", "Error"),
        ]
    )
    findings, errors, excluded = tidy.collect_diagnostics(document, tmp_path, {source})
    assert len(findings) == 1 and len(errors) == 1 and excluded == 1


def test_known_abi_failure_does_not_hide_other_parse_errors():
    name = "src/game/save_progress.c"
    error = dict(
        message="too few arguments to function call, expected 2, have 1",
        notes=["'progressReadFieldValue' declared here"],
    )
    assert tidy.parse_status(name, 1, [error]) == "unsupported"
    assert (
        tidy.parse_status(name, 1, [error, dict(message="new syntax error")])
        == "parse-failed"
    )
    assert (
        tidy.parse_status(name, 1, [dict(message="new syntax error")]) == "parse-failed"
    )
    assert tidy.parse_status("src/game/new.c", 1, [error]) == "parse-failed"
    assert tidy.parse_status(name, 0, []) == "complete"


def test_scan_reports_real_coverage_and_keeps_sources_unchanged(tmp_path, monkeypatch):
    from types import SimpleNamespace
    import yaml

    config = tmp_path / "config/clang-tidy.yaml"
    config.parent.mkdir()
    config.write_text("Checks: '-*,readability-*'\n")
    output = tmp_path / "build/clang-tidy"
    output.mkdir(parents=True)
    sources = [
        tmp_path / "src/game/good.c",
        tmp_path / "src/game/save_progress.c",
        tmp_path / "src/game/broken.c",
    ]
    for source in sources:
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_text("void f(void) {}\n")
    monkeypatch.setattr(
        tidy.subprocess, "check_output", lambda *a, **k: "LLVM version 21.1.8"
    )

    def clang(command, **kwargs):
        source = Path(command[1])
        fixes = Path(
            next(a.split("=", 1)[1] for a in command if a.startswith("--export-fixes="))
        )
        diagnostics = []
        if source.name != "good.c":
            message = (
                "too few arguments to function call, expected 2, have 1"
                if source.name == "save_progress.c"
                else "unexpected syntax"
            )
            diagnostics = [
                dict(
                    DiagnosticName="clang-diagnostic-error",
                    Level="Error",
                    DiagnosticMessage=dict(Message=message, FilePath=str(source)),
                    Notes=[dict(Message="'progressReadFieldValue' declared here")],
                )
            ]
        fixes.write_text(yaml.safe_dump(dict(Diagnostics=diagnostics)))
        return SimpleNamespace(returncode=int(bool(diagnostics)), stdout="", stderr="")

    monkeypatch.setattr(tidy.subprocess, "run", clang)
    before = {p: p.read_bytes() for p in sources}
    report = tidy.scan(
        tmp_path,
        output,
        [dict(file=str(p)) for p in sources],
        set(sources),
        "clang-tidy",
        2,
        None,
    )
    assert report["coverage"] == dict(total=3, complete=1)
    assert [u["status"] for u in report["units"]] == [
        "complete",
        "unsupported",
        "parse-failed",
    ]
    assert before == {p: p.read_bytes() for p in sources}
