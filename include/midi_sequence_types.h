/// \file
/// Defines MIDI playback state and queued controls shared by sequence readers.

#ifndef MIDI_SEQUENCE_TYPES_H
#define MIDI_SEQUENCE_TYPES_H

#include <ultra64.h>
#include "midi_reader_types.h"

/// Game-authored MIDI controllers for sequence loop playback.
typedef enum SequencePlaybackMarker {
    SEQUENCE_MARK_LOOP_START = 0x66, ///< Save the stream position and running status for later repetitions.
    SEQUENCE_MARK_LOOP_END = 0x67    ///< Finish an iteration and restore the saved mark when available.
} SequencePlaybackMarker;

/// Saved MIDI reader position and running status for a sequence loop.
typedef struct SequenceMark {
    s32 position; ///< Saved address of the next unread sequence byte.
    u8 runningStatus; ///< Saved channel status for events that omit their status byte.
    u8 unknown5[3]; ///< Unresolved trailing storage; not written by mark save/restore.
} SequenceMark;
/// MIDI reader, tempo, held notes and pending playback releases.
/// Reader storage is shared with the MIDI stream and cartridge-cache interfaces.
/// Initialized storage must remain stable while the reader and playback retain it.
typedef struct SequenceState {
    MidiReader reader; ///< Retained cursor and buffered cartridge storage.
    s16 division;      ///< Tick-based timing division returned by MIDI initialization.
    s16 unknown446;    ///< Unresolved storage between timing division and tempo.
    s32 tempo;         ///< Microseconds per quarter note, updated by tempo events.
    s32 unknown44C;    ///< Unresolved word preceding the playback-rate value.
    f64 rate; ///< Positive playback-speed multiplier.
    s32 noteCount;        ///< Number of tracked held notes, bounded by 32.
    s32 noteChannels[32]; ///< MIDI channel of each tracked note, paired with noteKeys.
    s32 noteKeys[32];     ///< MIDI key of each tracked note, paired with noteChannels.
    s32 sustain[16];      ///< Retained sustain-controller level for each MIDI channel.
    u8 stopped;        ///< Nonzero after playback stops.
    u8 releaseNotes;   ///< Pending note-off release pass.
    u8 releaseSustain; ///< Pending sustain release pass.
    u8 hasLoop;        ///< Nonzero after a loop-start marker establishes a saved mark.
    SequenceMark loop; ///< Saved stream mark used by loop-end markers.
    u16 channelMask; ///< Channels whose note events are enabled.
    u16 repetitions; ///< Loop-end count used to select enabled channels on later iterations.
} SequenceState;

/// Queued engine reset, stop and volume requests for one MIDI sequence.
/// Negative request times or levels mean that no corresponding request is pending.
typedef struct SequenceControlState {
    SequenceState playback; ///< Owned MIDI playback state advanced by queued controls.
    s32 resetPending; ///< Emit an engine reset before subsequent MIDI events.
    u8 unknown5B4[4]; ///< Unresolved bytes between reset request and queued stop time.
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
typedef char SequenceNoteCountOffsetCheck[(SEQUENCE_OFFSET(SequenceState, noteCount) == 0x458) ? 1 : -1];
typedef char SequenceNoteChannelsOffsetCheck[(SEQUENCE_OFFSET(SequenceState, noteChannels) == 0x45C) ? 1 : -1];
typedef char SequenceNoteKeysOffsetCheck[(SEQUENCE_OFFSET(SequenceState, noteKeys) == 0x4DC) ? 1 : -1];
typedef char SequenceSustainOffsetCheck[(SEQUENCE_OFFSET(SequenceState, sustain) == 0x55C) ? 1 : -1];
typedef char SequenceLoopOffsetCheck[(SEQUENCE_OFFSET(SequenceState, loop) == 0x5A0) ? 1 : -1];
typedef char SequenceRepetitionsOffsetCheck[(SEQUENCE_OFFSET(SequenceState, repetitions) == 0x5AA) ? 1 : -1];
#undef SEQUENCE_OFFSET

#endif
