/// \file
/// Declares initialization and playback of integer-valued animation tracks.

#ifndef INTEGER_ANIMATION_H
#define INTEGER_ANIMATION_H

#include "integer_animation_types.h"

/// Initialize integer animation playback and prime the first key.
/// \param track Writable track state.
/// \param keys Borrowed first key, or null for a held zero value; keys must outlive playback.
/// \returns Nothing.
/// \note Initial one-shot output is consumed without becoming the retained value.
/// \note Original decompilation identifier: \c func_8000A5CC (0x8000A5CC).
void animationIntegerTrackInitialize(ValueTrack *track, ValueKey *keys);

/// Evaluate an integer animation track and advance its time.
/// \param state Initialized track to update; its key stream must contain a reachable value or termination.
/// \param out Writable destination for the current or emitted value.
/// \param step Time step in seconds applied after evaluation.
/// \returns INTEGER_ANIMATION_ACTIVE, INTEGER_ANIMATION_HELD, or INTEGER_ANIMATION_EMITTED.
/// \pre Timed range keys have positive duration and representable arithmetic; relative jumps stay within live keys and reach an output or hold.
/// \note Original decompilation identifier: \c func_8000A600 (0x8000A600).
s32 animationIntegerTrackEvaluate(ValueTrack *state, s32 *out, f32 step);

#endif
