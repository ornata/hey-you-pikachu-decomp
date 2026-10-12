/// \file
/// Declares current action attributes and movement scaling for initialized actors.
#ifndef ACTION_QUERY_H
#define ACTION_QUERY_H
#include <ultra64.h>

/// Retrieve the current action's behavior attributes.
/// \param object Live actor with initialized action state.
/// \returns Current attributes, or the default mask 0x20100400 when playback is inactive.
/// \note Original decompilation identifier: \c func_8000F594 (0x8000F594).
s32 actionGetAttributes(u8 *object);

/// Retrieve the current action's movement scale.
/// \param object Live actor with initialized action state.
/// \returns Current action movement scale, or one when playback is inactive.
/// \note Original decompilation identifier: \c func_8000F5BC (0x8000F5BC).
f32 actionGetMovementScale(u8 *object);

#endif
