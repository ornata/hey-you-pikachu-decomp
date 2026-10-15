"""Guard against treating shared overlay addresses or short prefixes as identity."""

import pytest

from tools.script_decoder.live import compare_candidates
from tools.semantic_review.emulator.voice_runtime import capture_program


def test_short_capture_preserves_ambiguous_overlay_candidates():
    class Image:
        def read_address(self, program, size):
            data = b'abcX' if '.scene_one' in self.allowed else b'abcY'
            return data[:size], dict(address=program)

    candidates = [dict(program=0x80100000, address=0x80100004,
                       window_bytes=4, section=section)
                  for section in ('.scene_one', '.scene_two')]
    short = compare_candidates(Image(), candidates, 0x80100000, b'abc')
    assert len(short) == 2
    assert all(row['matches'] and not row['covers_candidate_window'] for row in short)
    full = compare_candidates(Image(), candidates, 0x80100000, b'abcY')
    assert [row['matches'] for row in full] == [False, True]
    assert full[0]['first_difference'] == 3
    with pytest.raises(ValueError, match='empty'):
        compare_candidates(Image(), candidates, 0x80100000, b'')


def test_program_capture_rejects_out_of_ram_and_short_reads(tmp_path):
    calls = []

    def read(address, size):
        calls.append((address, size))
        return b'abc'

    with pytest.raises(ValueError, match='RDRAM'):
        capture_program(read, 0x807FFFFF, 2, tmp_path)
    assert not calls
    with pytest.raises(ValueError, match='short'):
        capture_program(read, 0x80100000, 4, tmp_path)
    assert not list(tmp_path.iterdir())
    result = capture_program(read, 0x80100000, 3, tmp_path)
    assert (tmp_path / result['file']).read_bytes() == b'abc'
