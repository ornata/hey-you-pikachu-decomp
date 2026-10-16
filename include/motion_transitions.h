/// \file
/// Declares actor pose-transition map setup and recovered entry encodings.

#ifndef MOTION_TRANSITIONS_H
#define MOTION_TRANSITIONS_H

#include <ultra64.h>

/// Encodings used by the pose-transition map and its explicit source table.
/// Map values encode action IDs with a bias; decoded negative IDs select special
/// transitions. Capacity is storage capacity, not a count of poses or transitions.
typedef enum PoseTransitionEncoding {
    POSE_TRANSITION_TABLE_END = 48,       ///< Source-pose sentinel terminating the explicit table.
    POSE_TRANSITION_MAP_CAPACITY = 0x125, ///< Allocated word-map entry capacity.
    POSE_TRANSITION_ACTION_BIAS = 0x100,  ///< Bias added to action IDs stored in the map.
    POSE_TRANSITION_BLEND_VALUE = 0xFE,   ///< Map value selecting a generated pose blend.
    POSE_TRANSITION_DIRECT_ACTION = -3    ///< Decoded action ID accepting the requested record without a bridge.
} PoseTransitionEncoding;

/// Initialize the pose-transition lookup map from explicit transitions and action definitions.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F00C (0x8000F00C).
void actionInitializeTransitionMap(void);

#endif
