"""A replay boundary or matching fixture must not become semantic evidence by accident."""

from pathlib import Path
import pytest
from tools.semantic_review.__main__ import save, summarize, merge_interpreter_evidence, needs_cross_engine_validation
from tools.semantic_review.crosscheck import evaluate


def test_completed_boundary_is_not_enough():
    case = {"observations": [(0x1000, b"\x01")], "expected_return": 7}
    observed = {
        "status": "completed",
        "registers": {"gpr_2": 7},
        "memory": [{"address": 0x1000, "data_hex": "00"}],
    }
    assert evaluate(case, observed) is False
    observed["memory"][0]["data_hex"] = "01"
    assert evaluate(case, observed) is True
    observed["status"] = "unsupported"
    assert evaluate(case, observed) is None


def test_missing_memory_is_not_a_match():
    case = {"observations": [(0x1000, b"\x00")], "expected_return": None}
    assert evaluate(case, {"status": "completed", "memory": []}) is False


def test_report_does_not_hide_inconclusive_and_unsupported():
    assert summarize(
        [{"status": "supported"}, {"status": "unsupported"}, {"status": "inconclusive"}]
    ) == {"supported": 1, "contradicted": 0, "inconclusive": 1, "unsupported": 1}


def test_reports_cannot_escape_build_directory(tmp_path):
    with pytest.raises(ValueError, match="build/semantic-review"):
        save(tmp_path / "report.json", {})
    assert not (tmp_path / "report.json").exists()


@pytest.mark.parametrize("first,second", [("contradicted", "supported"), ("supported", "contradicted")])
def test_agreement_from_one_engine_cannot_hide_a_contradiction(first, second):
    rows = merge_interpreter_evidence(
        [dict(id="case", status=first)], dict(experiments=[dict(id="case", status=second)])
    )
    assert rows[0]["status"] == "contradicted"
    assert needs_cross_engine_validation(dict(cross_validated=True), rows)


def test_integer_probes_do_not_certify_unsupported_function_paths():
    rows = merge_interpreter_evidence(
        [dict(id="case", status="unsupported")],
        dict(experiments=[dict(id="case", status="supported")]),
    )
    assert rows[0]["status"] == "supported"
    assert rows[0]["unicorn_status"] == "unsupported"
    assert needs_cross_engine_validation(dict(cross_validated=True), rows)
    rows[0]["unicorn_status"] = "supported"
    assert not needs_cross_engine_validation(dict(cross_validated=True), rows)
