"""Scene replay must retain overlay identity and explicit paired-register inputs."""

import pytest
from tools.semantic_review.cases import initial_registers, selected_code_sections
from tools.semantic_review.scene_grid_numeric_domains import experiments


def test_same_virtual_address_does_not_authorize_another_scene():
    assert selected_code_sections(".boot") == [".boot", ".ovl"]
    assert selected_code_sections(".scene_bedroom", ".scene_bedroom") == [
        ".boot", ".ovl", ".scene_bedroom"
    ]
    for section, overlay in [
        (".scene_bedroom", None), (".scene_bedroom", ".scene_entryway"),
        (".boot", ".scene_bedroom"), (".scene_", ".scene_"), (".data", None)
    ]:
        with pytest.raises(ValueError, match="overlay identity"):
            selected_code_sections(section, overlay)


def test_paired_float_bits_are_distinct_from_integer_argument_words():
    regs = initial_registers([0, 0, 0xC1200000], {12: 0x41200000, 14: 0x41A00000})
    assert regs["gpr_6"] == 0xFFFFFFFFC1200000
    assert regs["fpr_12"] == 0x41200000
    assert regs["fpr_14"] == 0x41A00000
    for pairs in [{13: 0}, {32: 0}, {-2: 0}, {True: 0}, {12: -1}, {12: 1 << 64}, {12: 1.0}]:
        with pytest.raises(ValueError, match="FPR pairs"):
            initial_registers([], pairs)


def test_negative_cell_boundary_contrasts_have_nonzero_retail_witnesses():
    cases = experiments()
    near = [case for case in cases if case["id"].endswith("near-negative-y")]
    assert len(cases) == len({case["id"] for case in cases}) == 207
    assert len(near) == len({case["overlay"] for case in near}) == 23
    assert all(case["expected_return"] != 0 and case["fpr_pairs"][14] & 0x80000000 for case in near)
    assert all(case["expected_return"] == 0 for case in cases if case["id"].endswith("below-y"))
