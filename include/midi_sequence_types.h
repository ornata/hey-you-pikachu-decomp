/// \file
/// Defines MIDI playback state and queued controls shared by sequence readers.

#ifndef MIDI_SEQUENCE_TYPES_H
#define MIDI_SEQUENCE_TYPES_H

#include <ultra64.h>
#include "midi_reader_types.h"

/// Saved MIDI reader position and running status for a sequence loop.
typedef struct SequenceMark {
    s32 position; ///< Saved address of the next unread sequence byte.
    u8 runningStatus; ///< Saved channel status for events that omit their status byte.
    u8 unknown5[3]; ///< Unresolved trailing storage; not written by mark save/restore.
} SequenceMark;
/// MIDI reader, tempo, held notes and pending playback releases.
/// Reader storage is shared with the MIDI stream and cartridge-cache interfaces.
typedef struct SequenceState {
    MidiReader reader; ///< Retained cursor and buffered cartridge storage.
    s16 division;      ///< Tick-based timing division returned by MIDI initialization.
    s16 unknown446;
    s32 tempo;
    s32 unknown44C;
    f64 rate; ///< Positive playback-speed multiplier.
    s32 noteCount;
    s32 noteChannels[32];
    s32 noteKeys[32];
    s32 sustain[16];
    u8 stopped;        ///< Nonzero after playback stops.
    u8 releaseNotes;   ///< Pending note-off release pass.
    u8 releaseSustain; ///< Pending sustain release pass.
    u8 hasLoop;
    SequenceMark loop;
    u16 channelMask; ///< Channels whose note events are enabled.
    u16 repetitions;
} SequenceState;

/// Queued engine reset, stop and volume requests for one MIDI sequence.
/// Negative request times or levels mean that no corresponding request is pending.
typedef struct SequenceControlState {
    SequenceState playback;
    s32 resetPending; ///< Emit an engine reset before subsequent MIDI events.
    u8 unknown5B4[4];
    f64 stopDelay;      ///< Seconds until playback must stop; zero stops immediately.
    f64 fadeDuration;   ///< Duration of a queued volume fade in seconds.
    f64 fadeLevel;      ///< Target volume fraction for the queued fade.
    f64 immediateLevel; ///< Volume fraction to apply before the next event.
} SequenceControlState;

typedef char SequenceMarkSizeCheck[(sizeof(SequenceMark) == 8) ? 1 : -1];
typedef char SequenceMarkStatusOffsetCheck[((u32) & ((SequenceMark *)0)->runningStatus == 4) ? 1 : -1];
typedef char SequenceStateSizeCheck[(sizeof(SequenceState) == 0x5B0) ? 1 : -1];
typedef char SequenceControlStateSizeCheck[(sizeof(SequenceControlState) == 0x5D8) ? 1 : -1];
#define SEQUENCE_OFFSET(type, member) ((u32) & ((type *)0)->member)
typedef char SequenceRateOffsetCheck[(SEQUENCE_OFFSET(SequenceState, rate) == 0x450) ? 1 : -1];
typedef char SequenceStoppedOffsetCheck[(SEQUENCE_OFFSET(SequenceState, stopped) == 0x59C) ? 1 : -1];
typedef char SequenceChannelsOffsetCheck[(SEQUENCE_OFFSET(SequenceState, channelMask) == 0x5A8) ? 1 : -1];
typedef char SequenceResetOffsetCheck[(SEQUENCE_OFFSET(SequenceControlState, resetPending) == 0x5B0) ? 1 : -1];
typedef char SequenceStopOffsetCheck[(SEQUENCE_OFFSET(SequenceControlState, stopDelay) == 0x5B8) ? 1 : -1];
typedef char SequenceDivisionOffsetCheck[(SEQUENCE_OFFSET(SequenceState, division) == 0x444) ? 1 : -1];
#undef SEQUENCE_OFFSET

#endif
