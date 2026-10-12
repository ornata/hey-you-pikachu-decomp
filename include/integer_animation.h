/// \file
/// Declares initialization and playback of integer-valued animation tracks.

#ifndef INTEGER_ANIMATION_H
#define INTEGER_ANIMATION_H

#include "integer_animation_types.h"

/// Initialize an integer animation track and select its initial value.
/// \param track Writable track state.
/// \param keys Borrowed first key, or null for a held zero value; keys must outlive playback.
/// \returns Nothing.
void animationIntegerTrackInitialize(ValueTrack *track, ValueKey *keys);

/// Evaluate an integer animation track and advance its time.
/// \param state Initialized track to update; its key stream must contain a reachable value or termination.
/// \param out Writable destination for the current or emitted value.
/// \param step Time step in seconds applied after evaluation.
/// \returns Zero for an active value, one for a held or absent track, or two for a one-shot value.
s32 animationIntegerTrackEvaluate(ValueTrack *state, s32 *out, f32 step);

#endif
