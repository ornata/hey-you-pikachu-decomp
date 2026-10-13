/// \file
/// Declares sequence playback completion queries and tempo multipliers.

#ifndef SEQUENCE_PLAYBACK_H
#define SEQUENCE_PLAYBACK_H

#include "midi_sequence_types.h"

/// Check whether sequence playback and pending release events have ended.
/// \param player Sequence player to inspect.
/// \returns One when playback and pending releases have ended; otherwise zero.
/// \note Original decompilation identifier: \c func_80005B50 (0x80005B50).
s32 sequenceEngineIsDrained(SequenceState *player);

/// Check whether sequence playback is stopped.
/// \param player Sequence player to inspect.
/// \returns Nonzero when stopped, even if release events remain; otherwise zero.
/// \note Original decompilation identifier: \c func_80005B70 (0x80005B70).
s32 sequenceEngineIsStopped(SequenceState *player);

/// Set the sequence playback speed multiplier.
/// \param player Sequence player to update.
/// \param rate Positive playback multiplier; one preserves the original tempo.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80005B90 (0x80005B90).
s32 sequenceEngineSetRate(SequenceState *player, f64 rate);

/// Get the sequence playback speed multiplier.
/// \param player Sequence player to inspect.
/// \returns The configured playback multiplier.
/// \note Original decompilation identifier: \c func_80005BC0 (0x80005BC0).
f64 sequenceEngineGetRate(SequenceState *player);

#endif
