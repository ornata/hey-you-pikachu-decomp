/// \file
/// Defines paired direction poses and angular playback state shared by actor and item rendering.

#ifndef DIRECTION_ANIMATION_TYPES_H
#define DIRECTION_ANIMATION_TYPES_H

#include <ultra64.h>

/// Packed pose containing the two directions consumed by lighting commands.
/// Raw storage preserves the original four-byte-compatible enclosing layout.
typedef struct DirectionPose {
    u8 raw[0x28]; ///< Packed pose bytes, including direction vectors at 0x10 and 0x20.
} DirectionPose;

/// Angular rates supplied for one direction during timed motion.
typedef struct DirectionAngularRates {
    f32 pitch; ///< Pitch rate in degrees per second.
    f32 yaw;   ///< Yaw rate in degrees per second.
} DirectionAngularRates;

/// Angles and angular rates of one animated direction.
typedef struct DirectionAngularMotion {
    f32 pitch;     ///< Current pitch in degrees.
    f32 yaw;       ///< Current yaw in degrees.
    f32 pitchRate; ///< Pitch rate in degrees per second.
    f32 yawRate;   ///< Yaw rate in degrees per second.
} DirectionAngularMotion;

/// Two retained poses with the angular motion used to update the selected pose.
/// The enclosing actor or item owns this state and its pose storage.
typedef struct DirectionAnimation {
    DirectionPose poses[2];
    DirectionAngularMotion directions[2];
    s16 mode;      ///< Zero when stopped, one for timed motion, two while retargeting.
    u16 poseIndex; ///< Selected pose, zero or one.
    f32 duration;  ///< Requested timed-motion duration in seconds.
    f32 remaining; ///< Seconds until the current motion phase ends.
} DirectionAnimation;

/// Advance a direction-animation mode for one selected pose.
/// \param animation Borrowed initialized direction-animation storage.
/// \param poseIndex Selected pose, zero or one.
/// \returns Nothing.
typedef void (*DirectionAnimationUpdateHandler)(u8 *animation, s32 poseIndex);

typedef char DirectionAngularRatesSizeCheck[(sizeof(DirectionAngularRates) == 8) ? 1 : -1];
typedef char DirectionPoseSizeCheck[(sizeof(DirectionPose) == 0x28) ? 1 : -1];
typedef char DirectionAnimationSizeCheck[(sizeof(DirectionAnimation) == 0x7C) ? 1 : -1];
#define DIRECTION_OFFSET_CHECK(name, member, offset)                                                                   \
    typedef char name[(((u32) & ((DirectionAnimation *)0)->member) == (offset)) ? 1 : -1]
DIRECTION_OFFSET_CHECK(DirectionMotionOffsetCheck, directions, 0x50);
DIRECTION_OFFSET_CHECK(DirectionAnimationModeOffsetCheck, mode, 0x70);
DIRECTION_OFFSET_CHECK(DirectionPoseIndexOffsetCheck, poseIndex, 0x72);
DIRECTION_OFFSET_CHECK(DirectionAnimationRemainingOffsetCheck, remaining, 0x78);
#undef DIRECTION_OFFSET_CHECK

#endif
