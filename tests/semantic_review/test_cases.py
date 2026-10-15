"""Check review coverage and predeclared boundary fixtures without retail bytes."""

from tools.semantic_review.cases import experiments, inventory


def test_predeclared_cases_have_unique_names_and_observations():
    cases = experiments()
    assert cases
    assert len({case["id"] for case in cases}) == len(cases)
    assert all(case["observations"] for case in cases)


def test_mailbox_fixtures_cover_reject_force_and_nonboolean_pending():
    cases = {case["id"]: case for case in experiments()}
    assert cases["mailbox.post.7.0"]["expected_return"] == 0
    assert cases["mailbox.post.7.-1"]["expected_return"] == 1
    assert cases["mailbox.ack.7"]["expected_return"] == 7


def test_claims_record_evidence_or_missing_evidence():
    for claim in inventory():
        assert claim["claim"]
        if claim.get("status") == "supported":
            assert claim.get("evidence")
        else:
            assert claim.get("evidence") or claim.get("limitation")
