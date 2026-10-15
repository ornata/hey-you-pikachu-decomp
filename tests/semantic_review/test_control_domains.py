"""Control hypotheses retain branch contrasts, scheduling outcomes and guards."""

from tools.semantic_review.vm_control_domains import experiments
from tools.semantic_review.item_numeric_domains import experiments as item_experiments


def test_control_families_and_native_scheduling_outcomes():
    cases = {
        row["id"].removeprefix("numeric.vm.control."): row for row in experiments()
    }
    assert (
        cases["native-continue"]["expected_return"] == 5
    )  # Continues to the invalid sentinel.
    assert cases["native-yield"]["expected_return"] == 1
    assert cases["native-wait"]["expected_return"] == 2
    for name in [
        "load-variable",
        "store-variable",
        "load-element",
        "store-element",
        "call-frame-return",
    ]:
        assert name in cases
    assert (
        int.from_bytes(cases["jump-if-zero-0"]["observations"][2][1][0x14:0x18], "big")
        == 8
    )
    for condition in [1, -1]:
        assert (
            int.from_bytes(
                cases[f"jump-if-zero-{condition}"]["observations"][2][1][0x14:0x18],
                "big",
            )
            == 7
        )


def test_return_restores_frame_and_checks_unused_packed_storage():
    case = next(row for row in experiments() if row["id"].endswith("call-frame-return"))
    vm = case["observations"][2][1]
    assert int.from_bytes(vm[0x22C:0x230], "big") == 0x1234
    assert vm[0x24:0x28] == vm[0x128:0x12C] == b"\0" * 4
    assert case["observations"][3][1][25:] == b"\xa5" * (96 - 25)


def test_item_names_and_nominal_limit_have_distinguishing_inputs():
    cases = item_experiments()
    assert len(cases) == len({row["id"] for row in cases})
    assert {row["args"][1] for row in cases if ".name." in row["id"]} == {0, 1, 2}
    capacities = {
        row["id"]: row["expected_return"] for row in cases if ".capacity." in row["id"]
    }
    assert capacities == {
        "numeric.item.capacity.0": 60,
        "numeric.item.capacity.59": 1,
        "numeric.item.capacity.60": 0,
        "numeric.item.capacity.61": -1,
    }
