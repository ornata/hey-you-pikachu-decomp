#!/usr/bin/env python3
"""Audit a format-zero cue bank against the sequence reader's storage contract."""
import argparse
import json
from pathlib import Path


class SequenceBankError(ValueError):
    """A bank or event cannot satisfy the selected reader contract."""


def audit_sequence_bank(rom, base, event_limit, event_storage):
    """Return bounded track/event evidence; never modify ROM or build inputs."""
    def require(condition, message):
        if not condition:
            raise SequenceBankError(message)

    require(6 <= event_limit <= 255, "event limit must be 6..255")
    require(event_storage >= 8, "generated controls require eight destination bytes")
    require(0 <= base <= len(rom) - 4, "bank header outside ROM")
    revision = int.from_bytes(rom[base:base + 2], "big")
    count = int.from_bytes(rom[base + 2:base + 4], "big")
    require(revision == 1 and count > 0, "unsupported or empty cue bank")
    table_end = base + 4 + count * 8
    require(table_end <= len(rom), "truncated cue table")
    rows = []
    for index in range(count):
        slot = base + 4 + index * 8
        start = base + int.from_bytes(rom[slot:slot + 4], "big")
        size = int.from_bytes(rom[slot + 4:slot + 8], "big")
        require(start >= table_end and size >= 22 and start + size <= len(rom),
                f"cue {index}: resource outside bank storage")
        require(rom[start:start + 8] == b"MThd\0\0\0\6", f"cue {index}: invalid MIDI header")
        require(rom[start + 8:start + 12] == b"\0\0\0\1", f"cue {index}: expected format zero, one track")
        division = int.from_bytes(rom[start + 12:start + 14], "big")
        require(0 < division < 0x8000, f"cue {index}: unsupported timing division")
        require(rom[start + 14:start + 18] == b"MTrk", f"cue {index}: missing track")
        pos = start + 22
        end = pos + int.from_bytes(rom[start + 18:start + 22], "big")
        require(end <= start + size, f"cue {index}: track exceeds resource")
        running = 0
        events = 0
        finished = False
        sysex = []
        maximum_written = 8

        def byte():
            nonlocal pos
            require(pos < end, f"cue {index}: truncated event")
            value = rom[pos]
            pos += 1
            return value

        def vlq():
            value = 0
            for _ in range(4):
                item = byte()
                value = (value << 7) | (item & 127)
                if not item & 128:
                    return value
            raise SequenceBankError(f"cue {index}: variable quantity exceeds four bytes")

        while pos < end:
            vlq()
            status = byte()
            first_data = status < 128
            if first_data:
                require(running != 0, f"cue {index}: absent running status")
                status = running
            if 128 <= status < 240:
                running = status
                length = 2 if (status & 240) in (192, 208) else 3
                for _ in range(length - 1 - first_data):
                    require(byte() < 128, f"cue {index}: invalid channel data")
                written = length
            elif status in (240, 247):
                length = vlq()
                sysex.append(length)
                written = min(length, event_limit) + 1
                for _ in range(length):
                    byte()
            elif status == 255:
                meta_type = byte()
                length = byte()
                require(length < 128, f"cue {index}: unsupported meta-event length")
                running = 0
                finished = meta_type == 0x2F
                written = max(3, min(length + 3, event_limit))
                for _ in range(length):
                    byte()
            else:
                raise SequenceBankError(f"cue {index}: unsupported status {status:#x}")
            require(written <= event_storage,
                    f"cue {index}: event writes {written} bytes into {event_storage}-byte storage")
            maximum_written = max(maximum_written, written)
            events += 1
            if finished:
                break
        rows.append({"index": index, "rom_address": start, "resource_bytes": size,
                     "events": events, "sysex_payloads": sysex, "maximum_written": maximum_written})
    return {"bank_address": base, "revision": revision, "entries": count,
            "event_limit": event_limit, "event_storage": event_storage,
            "sysex_events": sum(len(row["sysex_payloads"]) for row in rows),
            "maximum_written": max(row["maximum_written"] for row in rows), "tracks": rows}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    parser.add_argument("--bank", type=lambda value: int(value, 0), required=True)
    parser.add_argument("--event-limit", type=int, required=True)
    parser.add_argument("--storage", type=int, required=True)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    try:
        result = audit_sequence_bank(args.rom.read_bytes(), args.bank, args.event_limit, args.storage)
    except (OSError, SequenceBankError) as error:
        parser.exit(1, f"ERROR: {error}\n")
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(f"OK: {result['entries']} cues, {result['sysex_events']} SysEx events; "
              f"maximum write {result['maximum_written']}/{args.storage} bytes")


if __name__ == "__main__":
    main()
