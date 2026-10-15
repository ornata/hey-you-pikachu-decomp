"""Numeric hypotheses cover distinguishing inputs before executing game code."""

from tools.semantic_review.numeric_domains import experiments


def test_operator_bytes_and_asymmetric_comparisons_are_covered():
    cases = experiments()
    assert {row["args"][1] for row in cases} == {
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        11,
        12,
        13,
        14,
        15,
        16,
        17,
        18,
        19,
        20,
        21,
        22,
        23,
    }
    assert len({row["id"] for row in cases}) == len(cases)
    for op in range(11, 17):
        selected = [
            row for row in cases if row["args"][1] == op and "integer" in row["id"]
        ]
        assert {
            (
                int.from_bytes(row["patches"][1][1][1:5], "big", signed=True),
                int.from_bytes(row["patches"][1][1][6:10], "big", signed=True),
            )
            for row in selected
        } >= {
            (7, 3), (3, 7), (7, 7)
        }
        assert {
            int.from_bytes(row["observations"][1][1][1:5], "big") for row in selected
        } == {0, 1}


def test_each_case_checks_stack_depth_payload_and_unused_bytes():
    for row in experiments():
        vm, values = row["observations"][0][1], row["observations"][1][1]
        assert int.from_bytes(vm[0x18:0x1C], "big") == 1
        assert values[10:] == b"\xa5" * 54
        assert vm[0x24:] == b"\xa5" * (0x2C8 - 0x24)
