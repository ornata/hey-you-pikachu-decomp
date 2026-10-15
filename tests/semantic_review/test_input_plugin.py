"""Optional native device-ABI checks; no ROM or SDK is required."""

import ctypes as C
import os
from pathlib import Path
import sys

import pytest
from tools.semantic_review.emulator.input import build, configure


@pytest.fixture
def plugin(tmp_path):
    source = os.environ.get("SEMANTIC_MUPEN_SOURCE")
    if not source:
        pytest.skip("set SEMANTIC_MUPEN_SOURCE to the pinned Mupen core checkout")
    path = build(
        Path(source),
        tmp_path / ("input.dylib" if sys.platform == "darwin" else "input.so"),
    )
    return path


def test_vru_results_use_native_uint16_plugin_contract(plugin, tmp_path):
    lib = configure(
        plugin,
        {
            "vru": {
                "results": [{"num_results": 1, "mic_level": 0x1234, "matches": [3, 42]}]
            }
        },
        tmp_path / "events.jsonl",
    )
    buf = (C.c_uint16 * 15)()
    pointers = [C.byref(buf, i * 2) for i in range(5)] + [C.byref(buf, 10)]
    lib.ReadVRUResults(*pointers)
    assert buf[1] == 1 and buf[2] == 0x1234
    assert buf[5] == 3 and buf[6] == 42
    lib.ReadVRUResults(*pointers)
    assert bytes(buf) == bytes(30)
    lib.PluginShutdown()


def test_controller_poll_sequence_holds_final_input(plugin, tmp_path):
    lib = configure(plugin, {"controller": [0, 0x80, 0x20]}, tmp_path / "events.jsonl")
    keys = C.c_uint32()
    observed = []
    for _ in range(4):
        lib.GetKeys(0, C.byref(keys))
        observed.append(keys.value)
    assert observed == [0, 0x80, 0x20, 0x20]
    lib.GetKeys(1, C.byref(keys))
    assert keys.value == 0
    lib.PluginShutdown()


def test_rejects_more_than_five_vru_results(plugin, tmp_path):
    with pytest.raises(ValueError, match="num_results"):
        configure(
            plugin,
            {"vru": {"results": [{"num_results": 6}]}},
            tmp_path / "events.jsonl",
        )


def test_vru_speech_signal_releases_and_does_not_repeat_after_result(plugin, tmp_path):
    lib = configure(
        plugin,
        {"vru": {"talk_polls": 3, "results": [{"num_results": 1}]}},
        tmp_path / "events.jsonl",
    )
    keys = C.c_uint32()
    lib.SetMicState(1)
    values = []
    for _ in range(4):
        lib.GetKeys(3, C.byref(keys))
        values.append(keys.value)
    assert values == [0x20, 0x20, 0x20, 0]
    buf = (C.c_uint16 * 15)()
    lib.ReadVRUResults(*[C.byref(buf, i * 2) for i in range(5)], C.byref(buf, 10))
    lib.SetMicState(1)
    lib.GetKeys(3, C.byref(keys))
    assert keys.value == 0
    lib.PluginShutdown()


def test_adaptive_navigation_uses_start_only_at_title_prompt():
    from tools.semantic_review.emulator.voice_runtime import controller_value

    assert controller_value(30, 12, 18) == 0x10
    assert controller_value(30, 13, 18) == 0x80
    assert controller_value(30, 0, 3) == 0x80
    assert controller_value(30, 0, 0) == 0x20
    assert controller_value(50, 0, 0) == 0
