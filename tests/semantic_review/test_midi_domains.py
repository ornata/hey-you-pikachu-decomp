"""MIDI fixtures must retain truncation witnesses and the O32 fifth argument."""

from tools.semantic_review.cases import STACK
from tools.semantic_review.midi_numeric_domains import experiments
from tools.semantic_review.midi_controller_numeric_domains import experiments as controllers


def test_event_boundaries_include_retained_and_discarded_payloads():
    cases = {case["id"]: case for case in experiments()}
    assert len(cases) == 18
    meta = cases["numeric.midi.event.meta-truncated"]
    sysex = cases["numeric.midi.event.sysex-truncated"]
    assert meta["args"][3] == sysex["args"][3] == 3
    assert meta["observations"][-1][1] == sysex["observations"][-1][1] == b"\0\0\0\6"
    assert meta["observations"][0][1][12] == 0
    assert sysex["observations"][0][1][12] == 0x92


def test_controller_value_is_supplied_in_the_stack_argument_slot():
    cases = controllers()
    assert len(cases) == len({case["id"] for case in cases}) == 23
    for case in cases:
        assert len(case["args"]) == 4
        assert any(address == STACK + 0x10 and len(data) == 4 for address, data in case["patches"])
        assert "stack_args" not in case
