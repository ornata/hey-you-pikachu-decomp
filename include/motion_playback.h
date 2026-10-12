/// \file
/// Declares actor and model-part motion playback reset, mode selection and frame updates.

#ifndef MOTION_PLAYBACK_H
#define MOTION_PLAYBACK_H

#include "ultra64.h"
#include "motion_playback_types.h"

/// Reset motion playback for an object and its animated children.
/// \param object Object whose motion playback is reset.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E15C (0x8000E15C).
void motionResetPlayback(u8 *object);

/// Set the motion playback mode for an object and its animated children.
/// \param object Object whose motion playback mode is updated.
/// \param mode Playback mode: 0 disables playback, 1 selects tracks, 2 selects timed playback, and 3 selects pose blending.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E190 (0x8000E190).
void motionSetPlaybackMode(u8 *object, s32 mode);

/// Advance the current action, its motion, callbacks, and transitions.
/// \param object Actor with initialized action state, part state, and transition map; pose blending requires a positive total transition duration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F14C (0x8000F14C).
/// \note Two pose reads retain checked member-offset addressing to preserve the original IDO floating-point operand order.
void actionUpdatePlayback(u8 *object);

/// Advance child motion playback using its parent action timing.
/// \param object Animated child part with initialized playback state and a valid parent action state; pose blending requires a positive parent transition duration.
/// \returns Nothing.
///
/// \note Portability/UB: the matrix result exceeds the declared three-float local array and relies on adjacent IDO frame padding retained by the unused local.
/// \note Original decompilation identifier: \c func_8000F33C (0x8000F33C).
void motionUpdateChildPlayback(u8 *object);

#endif
