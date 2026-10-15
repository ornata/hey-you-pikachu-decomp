"""Check review coverage and predeclared boundary fixtures without retail bytes."""

from tools.semantic_review.cases import experiments, inventory


def test_predeclared_cases_have_unique_names_and_observations():
    cases = experiments()
    assert len(cases) >= 20
    assert len({case["id"] for case in cases}) == len(cases)
    assert all(case["observations"] for case in cases)


def test_mailbox_fixtures_cover_reject_force_and_nonboolean_pending():
    cases = {case["id"]: case for case in experiments()}
    assert cases["mailbox.post.7.0"]["expected_return"] == 0
    assert cases["mailbox.post.7.-1"]["expected_return"] == 1
    assert cases["mailbox.ack.7"]["expected_return"] == 7


def test_remaining_integration_claims_stay_explicitly_inconclusive():
    claims = {claim["id"]: claim for claim in inventory()}
    for name in (
        "voice.device",
        "vm.dispatch",
        "actor.ownership",
        "item.physics",
        "scene.resources",
        "save.persistence",
        "presentation",
    ):
        assert claims[name]["status"] == "inconclusive"
        assert claims[name]["limitation"]
