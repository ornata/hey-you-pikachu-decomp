"""Reject plausible-looking voice traces that omit the command's actual outcome."""

from copy import deepcopy
import pytest
from tools.semantic_review.emulator.voice_verdict import assess


def example():
    actor = 0x80401000
    message = [2, 0, 0, 8000, 200, 0]
    launch = message[:5] + [17]
    events = [
        dict(stage="prepared", message=message),
        dict(stage="launch", message=launch),
        dict(stage="messenger", actor=actor, message=launch, mode=0),
        dict(stage="posted", actor=actor, message=launch, pending=1, accepted=1),
        dict(stage="message_read", actor=actor, field="type", value=2),
        dict(
            stage="branch",
            actor=actor,
            condition=0,
            target=84,
            fallthrough=19,
            next_offset=84,
        ),
        dict(stage="action", actor=actor, action_id=7, vm_offset=100),
    ]
    for hit, event in enumerate(events):
        event["hit"] = hit
        if event["stage"] == "action":
            event.update(
                caller_ra=0x80116838, vm_action_selector=2, action_initializing=1
            )
        if event["stage"] in ("posted", "message_read", "branch", "action"):
            event["vm"] = 0x80402000
    capture = dict(
        status="completed",
        pikachu_actor=actor,
        events=events,
        valid_recognition_results=1,
    )
    control = dict(
        status="completed", pikachu_actor=actor, events=[], valid_recognition_results=0
    )
    return (
        [deepcopy(capture) for _ in range(3)],
        [deepcopy(control) for _ in range(3)],
        dict(
            prepared_message=message,
            messenger_mode=0,
            action_id=7,
            action_vm_offset=100,
            action_context=dict(
                caller_ra=0x80116838, vm_action_selector=2, action_initializing=1
            ),
            branch=dict(condition=0, target=84, next_offset=84),
        ),
    )


def test_complete_chain_and_control_support_only_declared_scenario():
    captures, controls, expected = example()
    assert assess(captures, controls, expected)["status"] == "supported"


@pytest.mark.parametrize(
    "stage,field,value",
    [
        (0, "message", [9, 0, 0, 8000, 200, 0]),
        (1, "message", [2, 1, 0, 8000, 200, 17]),
        (2, "mode", 1),
        (3, "accepted", 0),
        (3, "pending", 0),
        (4, "actor", 0x80402000),
        (4, "value", 0),
        (5, "next_offset", 19),
        (6, "action_id", 8),
        (6, "hit", 1),
        (6, "caller_ra", 0x80110000),
        (6, "vm_action_selector", 3),
        (6, "action_initializing", 0),
        (6, "vm_offset", 101),
        (6, "vm", 0x80403000),
    ],
)
def test_wrong_command_recipient_branch_or_action_is_not_accepted(stage, field, value):
    captures, controls, expected = example()
    for capture in captures:
        capture["events"][stage][field] = value
    assert assess(captures, controls, expected)["status"] == "contradicted"


def test_mailbox_only_is_not_success():
    captures, controls, expected = example()
    for capture in captures:
        capture["events"] = capture["events"][:4]
    assert assess(captures, controls, expected)["status"] == "contradicted"


def test_unfinished_gameplay_is_inconclusive():
    captures, controls, expected = example()
    captures[0]["status"] = "inconclusive"
    assert assess(captures, controls, expected)["status"] == "inconclusive"


def test_ambient_action_does_not_count_as_command_response():
    captures, controls, expected = example()
    for control in controls:
        control["events"] = [deepcopy(captures[0]["events"][-1])]
    assert assess(captures, controls, expected)["status"] == "contradicted"


def test_three_repetitions_required():
    captures, controls, expected = example()
    assert assess(captures[:1], controls, expected)["status"] == "inconclusive"


def test_repeat_divergence_visible():
    captures, controls, expected = example()
    captures[2]["events"][-1]["action_id"] += 1
    assert assess(captures, controls, expected)["status"] == "contradicted"


def test_nonzero_branch_uses_fallthrough():
    captures, controls, expected = example()
    expected["branch"] = dict(condition=1, next_offset=19)
    for capture in captures:
        capture["events"][5].update(condition=1, next_offset=19)
    assert assess(captures, controls, expected)["status"] == "supported"


def test_intermediate_idle_action_does_not_hide_expected_action():
    captures, controls, expected = example()
    for capture in captures:
        capture["events"].insert(
            6, dict(stage="action", actor=capture["pikachu_actor"], action_id=99, hit=6)
        )
        capture["events"][-1]["hit"] = 7
    assert assess(captures, controls, expected)["status"] == "supported"
