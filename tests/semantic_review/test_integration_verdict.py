"""Completed emulator execution must not hide a contradicted dictionary claim."""

from copy import deepcopy
from tools.semantic_review.emulator.integration import assess


def example():
    capture = {
        "status": "completed",
        "checkpoints": [{"memory": [{"data_hex": "000000010000000100000000"}]}],
        "device_events": [
            {"event": "dictionary_clear", "a": 1},
            {"event": "dictionary_word"},
        ],
        "traces": [
            {
                "registers": {"gpr_5": 10},
                "memory": [{"data_hex": "00000000"}],
                "dictionary_words": 0,
            }
        ],
        "dispatches": 10,
        "core_sha256": "synthetic",
        "plugin_sha256": {},
        "limitations": [],
    }
    expected = {
        "dictionary_clear_count": 1,
        "dictionary_words": 1,
        "batch_size": 10,
        "final_cursor": 1,
        "between_uploads": 1,
    }
    return [deepcopy(capture) for _ in range(3)], expected


def test_observed_dictionary_disagreement_is_a_contradiction():
    captures, expected = example()
    assert assess(captures, expected)["status"] == "supported"
    expected["dictionary_words"] = 2
    assert assess(captures, expected)["status"] == "contradicted"


def test_unreached_checkpoint_is_inconclusive():
    captures, expected = example()
    captures[0]["status"] = "inconclusive"
    captures[0]["traces"] = []
    assert assess(captures, expected)["status"] == "inconclusive"


def test_repeat_divergence_is_visible():
    captures, expected = example()
    captures[2]["dispatches"] += 1
    result = assess(captures, expected)
    assert result["status"] == "contradicted"
    assert result["normalized_identical"] is False
