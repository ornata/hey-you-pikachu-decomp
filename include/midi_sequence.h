/// \file
/// Declares ROM-backed MIDI playback, channel selection, and pending-release queries.

#ifndef MIDI_SEQUENCE_H
#define MIDI_SEQUENCE_H

#include "midi_sequence_types.h"

/// Initialize the sequence playback and pending-release state.
/// \param state Sequence state to initialize.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800051A8 (0x800051A8).
s32 midiSequenceInitialize(SequenceState *state);

/// Vary enabled MIDI channels while keeping the lowest eight channels enabled.
/// \param state Sequence state whose channel mask is changed.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800051EC (0x800051EC).
s32 midiSequenceRandomizeUpperChannels(SequenceState *state);

/// Enable note playback on every MIDI channel.
/// \param state Sequence state whose channel mask is changed.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_8000521C (0x8000521C).
s32 midiSequenceEnableAllChannels(SequenceState *state);

/// Start reading a ROM-backed MIDI sequence when playback is stopped.
/// \param state Initialized sequence state to start.
/// \param romAddress ROM address of the sequence data.
/// \returns 1 when playback starts, or 0 when already playing or the sequence cannot be opened.
/// \note Original decompilation identifier: \c func_8000522C (0x8000522C).
s32 midiSequenceStart(SequenceState *state, s32 romAddress);

/// Stop sequence playback and request release of held notes and sustain.
/// \param state Sequence state to stop.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800052BC (0x800052BC).
s32 midiSequenceStop(SequenceState *state);

/// Get the next playable MIDI event and its delay, including pending playback releases.
/// \param state Sequence state to advance.
/// \param event Destination with room for at least capacity + 1 event bytes.
/// \param capacity Event data limit in bytes, at least six to retain tempo events.
/// \param delta Receives the delay in seconds before the returned event, or -1 when playback and pending releases have ended.
/// \returns The event length in bytes, or zero when playback and pending releases have ended.
/// \note Original decompilation identifier: \c func_8000530C (0x8000530C).
s32 midiSequenceReadEvent(SequenceState *state, u8 *event, s32 capacity, f64 *delta);

/// Check whether playback is stopped and all requested releases have been emitted.
/// \param state Sequence state to inspect.
/// \returns 1 when playback and pending releases have ended, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80005768 (0x80005768).
s32 midiSequenceIsDrained(SequenceState *state);

/// Check whether sequence playback is stopped.
/// \param state Sequence state to inspect.
/// \returns Nonzero when playback is stopped, even if release events remain, or zero while playing.
/// \note Original decompilation identifier: \c func_800057A0 (0x800057A0).
u8 midiSequenceIsStopped(SequenceState *state);

/// Set the sequence playback speed multiplier.
/// \param state Sequence state to update.
/// \param rate Positive playback multiplier; 1 preserves the original tempo.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_800057A8 (0x800057A8).
s32 midiSequenceSetRate(SequenceState *state, f64 rate);

/// Get the sequence playback speed multiplier.
/// \param state Sequence state to inspect.
/// \returns The configured playback multiplier.
/// \note Original decompilation identifier: \c func_800057BC (0x800057BC).
f64 midiSequenceGetRate(SequenceState *state);

#endif
