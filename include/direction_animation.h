/// \file
/// Declares direction-pose playback shared by actor and item visual state.
#ifndef DIRECTION_ANIMATION_H
#define DIRECTION_ANIMATION_H

#include "direction_animation_types.h"

/// Restore both poses and stop playback.
/// \param animation Borrowed storage for a complete DirectionAnimation.
/// \returns Nothing.
void directionAnimationReset(u8 *animation);

/// Advance active playback for the current frame.
/// \param animation Borrowed initialized DirectionAnimation storage.
/// \returns Nothing.
void directionAnimationUpdate(u8 *animation);

/// Set one direction in the next pose.
/// \param animation Borrowed initialized DirectionAnimation storage.
/// \param pitch Direction pitch in degrees.
/// \param yaw Direction yaw in degrees.
/// \param index Direction index, zero or one.
/// \returns Nothing.
void directionAnimationSetNextPoseAngles(u8 *animation, f32 pitch, f32 yaw, s32 index);

/// Start timed motion from the selected pose.
/// \param animation Borrowed initialized DirectionAnimation storage.
/// \param duration Nonnegative motion duration in seconds.
/// \param rates Two pitch/yaw rate records, borrowed only for this call.
/// \returns Nothing.
void directionAnimationStartMotion(u8 *animation, f32 duration, DirectionAngularRates *rates);

#endif
