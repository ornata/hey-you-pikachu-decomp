/// \file
/// Declares direction-pose playback shared by actor and item visual state.
#ifndef DIRECTION_ANIMATION_H
#define DIRECTION_ANIMATION_H

#include "direction_animation_types.h"

/// Set one directional light's orientation while preserving the pose's colors.
/// \param pitch Direction pitch in degrees.
/// \param yaw Direction yaw in degrees.
/// \param pose Borrowed complete two-light pose receiving the new orientation.
/// \param index Directional-light index, zero or one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80031F50 (0x80031F50).
void directionPoseSetAngles(f32 pitch, f32 yaw, DirectionPose *pose, s32 index);

/// Restore both poses and stop playback.
/// \param animation Borrowed storage for a complete DirectionAnimation.
/// \returns Nothing.
void directionAnimationReset(DirectionAnimation *animation);

/// Advance active playback for the current frame.
/// \param animation Borrowed initialized state with a valid mode and pose index zero or one.
/// \returns Nothing.
void directionAnimationUpdate(DirectionAnimation *animation);

/// Set one direction in the next pose.
/// \param animation Borrowed initialized state with a valid mode and pose index zero or one.
/// \param pitch Direction pitch in degrees.
/// \param yaw Direction yaw in degrees.
/// \param index Direction index, zero or one.
/// \returns Nothing.
void directionAnimationSetNextPoseAngles(DirectionAnimation *animation, f32 pitch, f32 yaw, s32 index);

/// Start timed motion from the selected pose.
/// \param animation Borrowed initialized state with a valid mode and pose index zero or one.
/// \param duration Nonnegative motion duration in seconds.
/// \param rates Two pitch/yaw rate records, borrowed only for this call.
/// \returns Nothing.
void directionAnimationStartMotion(DirectionAnimation *animation, f32 duration, DirectionAngularRates *rates);

#endif
