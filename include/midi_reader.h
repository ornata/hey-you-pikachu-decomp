/// \file
/// Defines the MIDI stream cursor prefix and saved-position interface.

#ifndef MIDI_READER_H
#define MIDI_READER_H

#include "midi_sequence_types.h"

/// MIDI cursor prefix preceding reader-owned buffered cartridge storage.
/// This view is not the full reader allocation and must not be used to allocate a reader.
typedef struct MidiReader {
    u32 start;        ///< Address of the sequence header in its backing storage.
    u32 position;     ///< Address of the next unread sequence byte.
    u32 length;       ///< Extent from the sequence start through the end of its track.
    u8 runningStatus; ///< Last channel status retained for running-status events.
} MidiReader;

typedef char MidiReaderPrefixSizeCheck[(sizeof(MidiReader) == 0x10) ? 1 : -1];
typedef char MidiReaderStatusOffsetCheck[((u32) & ((MidiReader *)0)->runningStatus == 0xC) ? 1 : -1];

/// Open a single-track format-zero MIDI sequence using tick-based timing.
/// \param reader Caller-owned, word-aligned storage of at least 0x448 bytes, retained while the sequence is read.
/// \param address Complete sequence in cartridge ROM or readable memory; its track extent must fit in signed 32 bits.
/// \returns The ticks-per-quarter-note division, or zero when the header is unsupported or invalid.
/// \note Original decompilation identifier: \c func_80007808 (0x80007808).
s32 midiReaderOpen(u8 *reader, s32 address);

/// Restore a saved MIDI stream position and running status.
/// \param reader Opened reader to reposition within the same sequence as the saved mark.
/// \param mark Previously saved, word-aligned mark containing at least five readable bytes.
/// \returns 1.
/// \note Original decompilation identifier: \c func_8000790C (0x8000790C).
s32 midiReaderRestoreMark(MidiReader *reader, SequenceMark *mark);

/// Save a MIDI stream position and running status.
/// \param reader Reader whose current state is saved.
/// \param mark Word-aligned destination with room for at least five bytes; must not overlap the reader.
/// \returns 1.
/// \note Original decompilation identifier: \c func_80007924 (0x80007924).
s32 midiReaderSaveMark(MidiReader *reader, SequenceMark *mark);

/// Consume and decode the next supported MIDI event.
/// Requires complete channel-voice, SysEx, or meta events, valid running status, and meta lengths below 128.
/// The sequence storage must remain readable through the event, including any discarded payload.
/// \param reader Opened MIDI reader with its original backing storage and no overlapping reads.
/// \param event Output buffer, separate from reader and size, with room for at least capacity + 1 bytes.
/// \param size Receives the full event length, which may exceed the retained data; unchanged at end of track.
/// \param capacity Payload limit for SysEx events and total byte limit for meta events; must be at least three.
/// \returns The event delay in MIDI ticks, or -1 when already at the end of track.
/// \note Original decompilation identifier: \c func_8000793C (0x8000793C).
s32 midiReaderReadEvent(MidiReader *reader, u8 *event, s32 *size, s32 capacity);

#endif
