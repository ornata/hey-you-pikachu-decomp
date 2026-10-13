/// \file
/// Declares queries for active game audio playback handles.

#ifndef AUDIO_PLAYBACK_QUERY_H
#define AUDIO_PLAYBACK_QUERY_H

#include <ultra64.h>

/// Count active playback associated with a game audio handle.
/// \param handle Playback handle returned by game audio services; zero selects no playback.
/// \returns One or zero for a sequence handle, otherwise the number of matching sound-effect slots.
/// \note Original decompilation identifier: \c func_8001FD2C (0x8001FD2C).
s32 audioCountPlayback(s32 handle);

#endif
