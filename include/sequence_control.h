/// \file
/// Declares queued MIDI playback reset, stop, and volume controls.

#ifndef SEQUENCE_CONTROL_H
#define SEQUENCE_CONTROL_H

#include "midi_sequence_types.h"

/// Initialize sequence playback with no pending control requests.
/// \param state Sequence control state to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800057D0 (0x800057D0).
void sequenceControlInitialize(SequenceControlState *state);

/// Request playback and an engine reset for a stopped sequence.
/// \param state Initialized sequence control state.
/// \param romAddress ROM address of the MIDI sequence.
/// \returns 1 when playback starts, or 0 when already playing or the sequence cannot be opened.
/// \note Original decompilation identifier: \c func_80005808 (0x80005808).
s32 sequenceControlStart(SequenceControlState *state, s32 romAddress);

/// Stop playback immediately or shorten its pending stop deadline.
/// \param state Sequence control state to update.
/// \param delay Nonnegative delay in seconds; zero stops playback immediately.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80005864 (0x80005864).
s32 sequenceControlScheduleStop(SequenceControlState *state, f64 delay);

/// Queue an immediate volume change or a timed volume fade.
/// \param state Sequence control state receiving the request.
/// \param duration Nonnegative fade duration in seconds; zero requests an immediate change.
/// \param level Target volume fraction, clamped to [0, 1] when the event is produced.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800058F4 (0x800058F4).
s32 sequenceControlRequestVolume(SequenceControlState *state, f64 duration, f64 level);

/// Encode a sequence-engine volume change event.
/// \param duration Fade duration in seconds, quantized downward to sixteenths and clamped to [0, 127/16].
/// \param level Target volume fraction, quantized downward to 127ths and clamped to [0, 1].
/// \param event Destination with room for eight event bytes.
/// \returns 8, the encoded event length.
/// \note Original decompilation identifier: \c func_80005938 (0x80005938).
s32 sequenceControlEncodeVolumeEvent(f64 duration, f64 level, u8 *event);

/// Get the next engine control or MIDI event and update the playback stop deadline.
/// \param state Sequence control state to advance.
/// \param event Destination for the event; must have room for at least eight bytes.
/// \param capacity Event buffer capacity in bytes.
/// \param delta Receives the delay in seconds, zero for an immediate control event, or -1 after playback releases have ended.
/// \returns The event length, including a one-byte reset marker when requested, or zero when no event remains.
/// \note Original decompilation identifier: \c func_800059E4 (0x800059E4).
s32 sequenceControlReadEvent(SequenceControlState *state, u8 *event, s32 capacity, f64 *delta);

#endif
