/// \file
/// Names recovered motion-action definitions used by actor behavior and continuation chains.

#ifndef MOTION_ACTION_IDS_H
#define MOTION_ACTION_IDS_H

/// Selected indices in the 330-entry ActionDefinition table.
/// These are distinct from timed action-script IDs and motion archive IDs.
/// Names describe retained resource identities and reviewed consumers; unreviewed
/// variants and global pose identities remain explicit numeric values.
/// Failure names identify authored variants and do not guarantee gameplay outcomes.
typedef enum MotionActionId {
    MOTION_ACTION_THUNDER = 18,              ///< Thunder motion.
    MOTION_ACTION_THUNDER_FAILURE = 19,      ///< Thunder motion failure variant.
    MOTION_ACTION_THUNDERBOLT = 20,          ///< Thunderbolt motion.
    MOTION_ACTION_THUNDERBOLT_FAILURE = 21,  ///< Thunderbolt motion failure variant.
    MOTION_ACTION_THUNDERSHOCK = 22,         ///< Thundershock motion.
    MOTION_ACTION_THUNDERSHOCK_FAILURE = 23, ///< Thundershock motion failure variant.
    MOTION_ACTION_HEAD_LOOK_RIGHT = 34, ///< Turn the head to the actor's right; this does not describe world-space yaw.
    MOTION_ACTION_FORWARD_JUMP_COLLISION =
        61, ///< Forward-jump collision continuation, distinct from subsequent falling and landing definitions.
    MOTION_ACTION_TURN_TOWARD_TARGET = 190,                     ///< Turn toward target direction.
    MOTION_ACTION_TURN_TOWARD_TARGET_SHOW_PICTURE = 191,        ///< Separate show-picture target-turn variant.
    MOTION_ACTION_TURN_TOWARD_TARGET_SIDEWAYS_READY = 192,      ///< Separate sideways-ready target-turn variant.
    MOTION_ACTION_TURN_TOWARD_TARGET_POSITION_ADJUSTMENT = 193, ///< Separate position-adjusting target turn.
    MOTION_ACTION_WALK_TO_TARGET_START = 198,                   ///< Begin walking toward the destination.
    MOTION_ACTION_WALK_TO_TARGET_LOOP = 199,                    ///< Repeat destination walking.
    MOTION_ACTION_WALK_TO_TARGET_END = 200                      ///< Finish destination walking.
} MotionActionId;

#endif
