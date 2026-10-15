"""Predeclared MIDI byte streams, running-status behavior and payload boundaries."""


def experiments():
    """Check retail decoding and write footprints with directly readable streams."""
    from .cases import RAM, word

    cases = []
    stream, event, size = RAM + 0x500, RAM + 0x600, RAM + 0x700

    def reader(data, consumed, running=0x92, final_running=None):
        before = bytearray(b"\xa5" * 0x444)
        before[:12] = word(stream) + word(stream) + word(len(data))
        before[12] = running
        after = bytearray(before)
        after[4:8] = word(stream + consumed)
        after[12] = running if final_running is None else final_running
        return bytes(before), bytes(after)

    for name, data, expected in [
        ("zero", b"\x00", 0), ("single-max", b"\x7f", 127),
        ("two-min", b"\x81\x00", 128), ("two-max", b"\xff\x7f", 16383),
        ("three-min", b"\x81\x80\x00", 16384),
        ("four-max", b"\xff\xff\xff\x7f", 268435455),
    ]:
        before, after = reader(data, len(data))
        cases.append(dict(
            id="numeric.midi.vlq." + name, symbol="midiReaderReadVariableLength",
            args=[RAM], patches=[(RAM, before), (stream, data)],
            observations=[(RAM, after), (stream, data)], expected_return=expected,
        ))

    for name, data, output, logical_size, running, capacity in [
        ("note-on", b"\0\x92\x3c\x64", b"\x92\x3c\x64", 3, 0x92, 8),
        ("running", b"\0\x3d\0", b"\x92\x3d\0", 3, 0x92, 8),
        ("note-off", b"\0\x8f\x0c\x40", b"\x8f\x0c\x40", 3, 0x8F, 8),
        ("program", b"\0\xc3\x07", b"\xc3\x07", 2, 0xC3, 8),
        ("pressure", b"\0\xd5\x7b", b"\xd5\x7b", 2, 0xD5, 8),
        ("tempo", b"\0\xff\x51\x03\x07\xa1\x20", b"\xff\x51\x03\x07\xa1\x20", 6, 0, 8),
        ("end", b"\0\xff\x2f\0", b"\xff\x2f\0", 3, 0, 8),
        ("sysex", b"\0\xf0\x03\x7f\x04\x01", b"\xf0\x7f\x04\x01", 4, 0x92, 8),
        ("eox", b"\0\xf7\x02\x01\x02", b"\xf7\x01\x02", 3, 0x92, 8),
        ("meta-truncated", b"\0\xff\x51\x03\x07\xa1\x20", b"\xff\x51\x03", 6, 0, 3),
        ("sysex-truncated", b"\0\xf0\x05\x7f\x04\x01\0\0", b"\xf0\x7f\x04\x01", 6, 0x92, 3),
    ]:
        before, after = reader(data, len(data), final_running=running)
        cases.append(dict(
            id="numeric.midi.event." + name, symbol="midiReaderReadEvent",
            args=[RAM, event, size, capacity],
            patches=[(RAM, before), (stream, data), (event, b"\xa5" * 9), (size, word(99))],
            observations=[(RAM, after), (stream, data),
                          (event, output + b"\xa5" * (9 - len(output))), (size, word(logical_size))],
            expected_return=0,
            limitations=["SysEx retains the original running status; no standards-correction rewrite is implied."],
        ))
    before, _ = reader(b"", 0)
    patches = [(RAM, before), (event, b"\xa5" * 9), (size, word(99))]
    cases.append(dict(
        id="numeric.midi.event.exhausted", symbol="midiReaderReadEvent",
        args=[RAM, event, size, 8], patches=patches, observations=patches, expected_return=-1,
    ))
    return cases
