"""Exercise the cue-bank reader's actual event-storage boundaries."""
import struct

import pytest

from tools.verify_sequence_bank import SequenceBankError, audit_sequence_bank


def bank(track):
    midi = b"MThd" + struct.pack(">IHHH", 6, 0, 1, 96)
    midi += b"MTrk" + struct.pack(">I", len(track)) + track
    return struct.pack(">HHII", 1, 1, 12, len(midi)) + midi


def audit(track, limit=20, storage=20):
    return audit_sequence_bank(bank(track), 0, limit, storage)


def test_channel_running_status_and_end_marker():
    result = audit(b"\0\x90\x3c\x40\0\x3d\x40\0\xff\x2f\0")
    assert result["entries"] == 1
    assert result["tracks"][0]["events"] == 3
    assert result["sysex_events"] == 0


def test_sysex_status_byte_fits_exact_storage():
    result = audit(b"\0\xf0\x13" + b"\x01" * 19)
    assert result["maximum_written"] == 20


def test_sysex_status_byte_exceeds_storage():
    with pytest.raises(SequenceBankError, match="writes 21 bytes"):
        audit(b"\0\xf0\x14" + b"\x01" * 20)


def test_reader_limit_can_leave_space_for_status():
    result = audit(b"\0\xf0\x14" + b"\x01" * 20, limit=19)
    assert result["maximum_written"] == 20


def test_truncated_resource_is_rejected():
    rom = bank(b"\0\x90\x3c\x40")[:-1]
    with pytest.raises(SequenceBankError, match="outside bank storage"):
        audit_sequence_bank(rom, 0, 20, 20)


def test_unsupported_meta_length_is_rejected():
    with pytest.raises(SequenceBankError, match="meta-event length"):
        audit(b"\0\xff\x03\x80\0")


def test_generated_controls_require_eight_bytes():
    with pytest.raises(SequenceBankError, match="eight destination bytes"):
        audit(b"", storage=7)


def test_end_of_track_stops_before_unused_tail():
    result = audit(b"\0\xff\x2f\0\0\0\0")
    assert result["tracks"][0]["events"] == 1
