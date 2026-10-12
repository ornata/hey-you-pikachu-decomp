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
/// \note Original decompilation identifier: \c func_8000F33C (0x8000F33C).
void motionUpdateChildPlayback(u8 *object);

/// Apply a motion root orientation and its heading change to an object.
/// \param angles Three root rotation angles in degrees whose orientation has a nonzero horizontal heading projection.
/// \param object Object with initialized motion state receiving the root orientation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E00C (0x8000E00C).
void motionApplyRootOrientation(f32 *angles, u8 *object);

/// Evaluate and apply the root orientation and movement of a motion.
/// \param object Object receiving the root motion.
/// \param state Initialized motion state containing root tracks and writable pending movement and velocity.
/// \param time Nonnegative time step in seconds by which to advance the root tracks.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E408 (0x8000E408).
void motionEvaluateRootTracks(u8 *object, u8 *state, f32 time);

#endif
