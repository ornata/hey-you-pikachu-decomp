"""Transfer hypotheses include rejection, force controls and unrelated-byte checks."""

from tools.semantic_review.participation_domains import experiments


def test_transfer_matrix_and_forced_release_controls():
    cases = experiments()
    assert len({row["id"] for row in cases}) == len(cases)
    transfers = [row for row in cases if row["id"].startswith("numeric.item.transfer.")]
    assert len(transfers) == 24
    releases = {
        row["id"]: row for row in cases if row["id"].startswith("numeric.item.release.")
    }
    assert releases["numeric.item.release.0.0"]["expected_return"] == 0
    assert releases["numeric.item.release.1.0"]["expected_return"] == 1
    assert releases["numeric.item.release.0.1"]["expected_return"] == 1
    for row in releases.values():
        assert (0x800EF724, b"\0\0\x12\x34") in row["patches"]
        if row["expected_return"]:
            assert row["observations"][1][1][:4] == b"\0\0\0\x20"


def test_actor_flags_cover_all_existing_low_bits_and_preserve_guards():
    cases = [
        row for row in experiments() if row["id"].startswith("numeric.actor.flags.")
    ]
    assert len(cases) == 16
    assert {row["args"][1] for row in cases} == {1, 2, 4, 8, 0x10, 0x20, 0x40, 0x7F}
    for row in cases:
        before, after = row["patches"][1][1], row["observations"][1][1]
        assert before[:0x1E8] == after[:0x1E8]
        assert before[0x1EA:] == after[0x1EA:]
