"""Assess a voice scenario's causal checkpoints independently of emulator success."""

import hashlib
import json

STAGES = (
    "prepared",
    "launch",
    "messenger",
    "posted",
    "message_read",
    "branch",
    "action",
)


def assess(captures: list[dict], controls: list[dict], expected: dict) -> dict:
    """Compare three command runs and three no-result runs with stated observations.

    Captures contain normalized events, a Pikachu actor address, and a completion
    status. This verdict requires message identity, actor identity, a recorded VM
    conditional decision and an expected action absent from the control window.
    It establishes this bounded scenario, not arbitrary speech or gameplay.
    """
    failures = []
    incomplete = len(captures) != 3 or len(controls) != 3
    for index, capture in enumerate(captures):
        if capture.get("status") != "completed":
            incomplete = True
            continue
        events = capture.get("events", [])
        chain = []
        cursor = -1
        for stage in STAGES:
            found = next(
                (
                    i
                    for i in range(cursor + 1, len(events))
                    if events[i].get("stage") == stage
                    and (
                        stage != "action"
                        or events[i].get("action_id") == expected["action_id"]
                    )
                    and (
                        stage != "branch"
                        or all(
                            events[i].get(k) == v for k, v in expected["branch"].items()
                        )
                    )
                ),
                None,
            )
            if found is None:
                failures.append(f"run {index}: missing ordered {stage} checkpoint")
                break
            cursor = found
            chain.append(events[found])
        if len(chain) != len(STAGES):
            continue
        prepared, launch, messenger, posted, read, branch, action = chain
        actor = capture.get("pikachu_actor")
        if (
            not isinstance(actor, int)
            or not 0x80000000 <= actor < 0x80800000
            or any(
                e.get("actor") != actor
                for e in (messenger, posted, read, branch, action)
            )
        ):
            failures.append(f"run {index}: recipient or VM/action actor differs")
        vm = posted.get("vm")
        if not isinstance(vm, int) or any(
            e.get("vm") != vm for e in (read, branch, action)
        ):
            failures.append(
                f"run {index}: message and action use different VM contexts"
            )
        message = prepared.get("message")
        if message != expected["prepared_message"]:
            failures.append(f"run {index}: translated message differs")
        launched = launch.get("message")
        if (
            not isinstance(launched, list)
            or len(launched) != 6
            or launched[:5] != expected["prepared_message"][:5]
        ):
            failures.append(f"run {index}: launch changed command payload")
        if (
            messenger.get("mode") != expected["messenger_mode"]
            or messenger.get("message") != launched
            or posted.get("message") != launched
            or posted.get("pending") != 1
            or posted.get("accepted") != 1
        ):
            failures.append(f"run {index}: messenger/mailbox delivery differs")
        if (
            read.get("field") != "type"
            or read.get("value") != expected["prepared_message"][0]
        ):
            failures.append(f"run {index}: VM message read differs")
        condition = branch.get("condition")
        destination = (
            branch.get("target") if condition == 0 else branch.get("fallthrough")
        )
        if (
            not isinstance(condition, int)
            or branch.get("next_offset") != destination
            or destination is None
        ):
            failures.append(
                f"run {index}: VM conditional decision is missing or inconsistent"
            )
        if any(branch.get(k) != v for k, v in expected["branch"].items()):
            failures.append(f"run {index}: expected message branch differs")
        if any(action.get(k) != v for k, v in expected["action_context"].items()):
            failures.append(
                f"run {index}: action was not started by the expected VM handler"
            )
        if action.get("vm_offset") != expected["action_vm_offset"]:
            failures.append(
                f"run {index}: action starts from a different VM instruction"
            )
        if action.get("action_id") != expected["action_id"]:
            failures.append(f"run {index}: action differs")
        positions = [e.get("hit") for e in chain]
        if any(not isinstance(n, int) for n in positions) or positions != sorted(
            set(positions)
        ):
            failures.append(f"run {index}: checkpoint order differs")
        if capture.get("valid_recognition_results") != 1:
            failures.append(f"run {index}: expected one consumed recognition result")
    for index, control in enumerate(controls):
        if control.get("status") != "completed":
            incomplete = True
        if control.get("valid_recognition_results") != 0 or any(
            e.get("stage") == "prepared" for e in control.get("events", [])
        ):
            failures.append(f"control {index}: unexpected recognized command")
        if any(
            e.get("stage") == "action"
            and e.get("actor") == control.get("pikachu_actor")
            and e.get("action_id") == expected["action_id"]
            for e in control.get("events", [])
        ):
            failures.append(f"control {index}: same action occurs without recognition")
    normalized = [json.dumps(c, sort_keys=True) for c in captures]
    repeatable = len(normalized) == 3 and len(set(normalized)) == 1
    control_repeatable = (
        len(controls) == 3
        and len({json.dumps(c, sort_keys=True) for c in controls}) == 1
    )
    if not repeatable or not control_repeatable:
        failures.append("normalized observations differ between repetitions")
    return {
        "status": (
            "inconclusive"
            if incomplete
            else "contradicted" if failures else "supported"
        ),
        "claim": "One injected VRU result reaches Pikachu's mailbox, VM conditional and expected action in the declared scenario.",
        "repeats": len(captures),
        "control_repeats": len(controls),
        "normalized_identical": repeatable,
        "control_normalized_identical": control_repeatable,
        "failures": failures,
        "normalized_sha256": (
            hashlib.sha256(normalized[0].encode()).hexdigest() if normalized else None
        ),
        "limitations": [
            "A sequential trace plus a no-result control supports this scenario; it does not prove every instruction's data dependency.",
            "Injected recognition does not establish physical microphone or speech-recognition fidelity.",
        ],
    }
