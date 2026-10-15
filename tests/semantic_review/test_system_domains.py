"""System scenarios preserve boundary contrasts and distinguish CPU slices from I/O."""

import pytest
from tools.semantic_review.cases import bounded_slice
from tools.semantic_review.system_numeric_domains import experiments


def test_slice_boundaries_cannot_escape_the_authenticated_function():
    assert bounded_slice(
        0x80003438, 0x108, {"entry_offset": 0x50, "stop_offset": 0xF8}
    ) == (0x80003488, 0x80003530)
    for entry, stop in [(-4, 8), (4, 4), (4, 0x108), (1, 8), (4, 7), (True, 8)]:
        with pytest.raises(ValueError):
            bounded_slice(
                0x80003438, 0x108, {"entry_offset": entry, "stop_offset": stop}
            )
    with pytest.raises(ValueError):
        bounded_slice(0, 32, {"entry_offset": 4})


def test_eeprom_failures_are_preselected_cpu_branches_not_device_claims():
    cases = [row for row in experiments() if ".eeprom.failure." in row["id"]]
    assert {row["patches"][0][1][4:8] for row in cases} == {
        v.to_bytes(4, "big") for v in [9, 10, 11]
    }
    assert all(
        row["slice"]
        and row["scope"] == "original failure-branch CPU slice"
        and row["limitations"]
        for row in cases
    )


def test_audio_invalid_values_and_scene_boundary_inputs_remain_covered():
    cases = experiments()
    assert len(cases) == len({row["id"] for row in cases})
    assert {row["args"][0] for row in cases if ".audio.forward." in row["id"]} == {
        0,
        1,
        2,
        3,
        258,
    }
    assert {row["args"][0] for row in cases if ".scene.wrap." in row["id"]} == {
        -24,
        -23,
        -1,
        0,
        22,
        23,
        46,
    }
