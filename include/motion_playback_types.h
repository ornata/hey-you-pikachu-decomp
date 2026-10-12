/// \file
/// Defines motion playback mode values shared by actors and animated model parts.

#ifndef MOTION_PLAYBACK_TYPES_H
#define MOTION_PLAYBACK_TYPES_H

/// Motion evaluation policy stored in a target 16-bit playback field.
/// The enum supplies names; it does not change the width of stored mode fields.
typedef enum MotionPlaybackMode {
    MOTION_PLAYBACK_DISABLED = 0, ///< Motion evaluation is inactive.
    MOTION_PLAYBACK_TRACKS = 1, ///< Evaluate motion archive tracks.
    MOTION_PLAYBACK_TIMED = 2, ///< Advance a timed motion without track evaluation.
    MOTION_PLAYBACK_POSE_BLEND = 3 ///< Blend captured source and destination poses.
} MotionPlaybackMode;

#endif
