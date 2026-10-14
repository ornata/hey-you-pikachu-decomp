/// \file
/// Defines buffered cartridge and MIDI stream storage shared by sequence readers.
#ifndef MIDI_READER_TYPES_H
#define MIDI_READER_TYPES_H

#include <ultra64.h>

/// Active buffered cartridge-reader storage; callers may retain additional trailing workspace.
/// DMA and its completion queue belong to this reader. Reads on one reader must not overlap.
/// This prefix is not a standalone allocation-size contract; initialized storage must remain at a stable address.
typedef struct RomByteReaderPrefix {
    s32 windowStart;   ///< Cartridge address of the cached window, or the invalid initial sentinel.
    u8 storage[0x410]; ///< Reader-owned space for one internally aligned 0x400-byte DMA window.
    u8 *alignedBuffer; ///< Borrowed aligned view into storage.
    OSMesgQueue queue; ///< Owned single-read completion queue.
    OSMesg completion; ///< Message slot backing the completion queue.
} RomByteReaderPrefix;

/// MIDI stream cursor and buffered cartridge reader preceding sequence playback fields.
/// The backing sequence is borrowed and must remain readable while events are consumed.
/// This view is not a standalone allocation-size contract; initialized storage must remain at a stable address.
typedef struct MidiReader {
    u32 start;        ///< Address of the sequence header in backing storage.
    u32 position;     ///< Address of the next unread sequence byte.
    u32 length;       ///< Extent from the sequence start through its track end.
    u8 runningStatus; ///< Retained channel status for events omitting their status byte.
    u8 unknown00D[3]; ///< Unresolved bytes before the buffered reader; no role is assigned from alignment alone.
    RomByteReaderPrefix cartridge; ///< Owned cache and completion state for cartridge-backed reads.
} MidiReader;

typedef char RomByteReaderPrefixSizeCheck[(sizeof(RomByteReaderPrefix) == 0x434) ? 1 : -1];
typedef char RomByteReaderBufferCheck[((u32) & ((RomByteReaderPrefix *)0)->alignedBuffer == 0x414) ? 1 : -1];
typedef char RomByteReaderQueueCheck[((u32) & ((RomByteReaderPrefix *)0)->queue == 0x418) ? 1 : -1];
typedef char RomByteReaderCompletionCheck[((u32) & ((RomByteReaderPrefix *)0)->completion == 0x430) ? 1 : -1];
typedef char MidiReaderSizeCheck[(sizeof(MidiReader) == 0x444) ? 1 : -1];
typedef char MidiReaderStatusOffsetCheck[((u32) & ((MidiReader *)0)->runningStatus == 0xC) ? 1 : -1];
typedef char MidiReaderCartridgeCheck[((u32) & ((MidiReader *)0)->cartridge == 0x10) ? 1 : -1];
#endif
