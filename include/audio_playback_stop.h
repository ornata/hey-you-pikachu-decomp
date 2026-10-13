/// \file
/// Declares stopping and sequence fade requests for game audio playback handles.

#ifndef AUDIO_PLAYBACK_STOP_H
#define AUDIO_PLAYBACK_STOP_H

#include <ultra64.h>

/// Stop sound effects or fade out a sequence associated with a playback handle.
/// \param handle Playback handle returned by game audio services; zero selects no playback.
/// \param fadeSeconds Nonnegative sequence fade duration in seconds; sound effects stop immediately.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80020270 (0x80020270).
s32 audioStopPlayback(s32 handle, f64 fadeSeconds);

#endif
