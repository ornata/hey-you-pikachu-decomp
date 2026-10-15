"""Keep verdict windows explicit while retaining all relevant raw observations."""

from tools.semantic_review.emulator.voice_integration import normalize


def test_window_keeps_wrong_actions_and_native_context():
    raw = dict(
        status="completed",
        events=[
            dict(stage="voice_state", poll=1, dispatch=1, pikachu=0x80400000),
            dict(stage="action", poll=9, dispatch=2, action_id=14),
            dict(stage="action", poll=10, dispatch=3, action_id=999, caller_ra=123),
            dict(stage="action", poll=21, dispatch=4, action_id=168),
        ],
        device_events=[dict(event="recognition_result", b=1)],
    )
    normalized = normalize(raw, dict(window_start=10, poll_bound=20))
    assert normalized["events"] == [
        dict(stage="action", poll=10, hit=3, action_id=999, caller_ra=123)
    ]
    assert normalized["pikachu_actor"] == 0x80400000
    assert normalized["valid_recognition_results"] == 1
    assert len(raw["events"]) == 4


def test_empty_device_results_do_not_count_as_valid_recognition():
    raw = dict(
        status="inconclusive",
        events=[],
        device_events=[dict(event="recognition_result", b=0)],
    )
    normalized = normalize(raw, dict(window_start=0, poll_bound=20))
    assert normalized["valid_recognition_results"] == 0
    assert normalized["status"] == "inconclusive"
    assert normalized["pikachu_actor"] is None


def test_repetitions_use_process_boundaries(tmp_path, monkeypatch):
    import json
    from pathlib import Path
    from tools.semantic_review.emulator import voice_integration

    calls = []

    def child(command, check):
        assert check is True
        args = json.loads(command[-1])
        calls.append(args)
        directory = Path(args["output"])
        directory.mkdir(parents=True)
        (directory / "runtime.json").write_text(
            json.dumps(
                dict(
                    status="inconclusive",
                    events=[],
                    device_events=[],
                    rom_sha256="unavailable",
                    core_sha256="unavailable",
                    plugin_sha256={},
                )
            )
        )

    monkeypatch.setattr(voice_integration.subprocess, "run", child)
    scenario = dict(
        name="API-only isolation test",
        response=dict(num_results=1),
        max_seconds=1,
        expected=dict(window_start=0, poll_bound=1, action_id=168),
    )
    result = voice_integration.run(
        *(
            tmp_path / name
            for name in ("rom", "core", "source", "rsp", "gfx", "output")
        ),
        scenario,
    )
    assert len(calls) == 6
    assert len({c["output"] for c in calls}) == 6
    assert [c["response"] for c in calls] == [scenario["response"]] * 3 + [None] * 3
    assert result["status"] == "inconclusive"
