/// \file
/// Names captured predicates in the reviewed companion-script status snapshots.

#ifndef COMPANION_SNAPSHOT_FLAGS_H
#define COMPANION_SNAPSHOT_FLAGS_H

/// Selected bits authored by the reviewed companion snapshot-builder template.
/// These describe sampled script conditions, separate from RuntimeObjectFlags,
/// message quality flags and device status. Root-slot locations vary by program.
/// Consumers require the matching producer, valid storage and preserved samples.
/// Counter predicates 0x800 and 0x1000 remain unnamed; later extensions are separate.
typedef enum CompanionSnapshotFlag {
    COMPANION_SNAPSHOT_MOVEMENT_COLLISION = 1, ///< Movement collision flags were nonzero.
    COMPANION_SNAPSHOT_ORDINARY_MESSAGE_CAPTURED =
        2, ///< Acknowledged pending value one with a category other than 13, 14 or 15.
    COMPANION_SNAPSHOT_COLLISION_RADIAL_REPEAT_CHECK =
        4, ///< Collision checks recorded repeated nearly unchanged origin radius.
    COMPANION_SNAPSHOT_TARGET_APPROACH_BELOW_5 = 8,   ///< Remaining target approach distance below five.
    COMPANION_SNAPSHOT_TARGET_APPROACH_BELOW_15 = 16, ///< Remaining target approach distance below fifteen.
    COMPANION_SNAPSHOT_PLAYER_DISTANCE_BELOW_15 = 32, ///< Approximate player distance below fifteen.
    COMPANION_SNAPSHOT_PLAYER_DISTANCE_BELOW_30 = 64, ///< Approximate player distance below thirty.
    COMPANION_SNAPSHOT_PLAYER_HELD_KIND_NEAR_30 =
        128, ///< Nonzero sampled player-held kind and player distance below thirty.
    COMPANION_SNAPSHOT_PLAYER_DISTANCE_AT_LEAST_60 = 256, ///< Approximate player distance at least sixty.
    COMPANION_SNAPSHOT_NEARBY_WORLD_ITEM = 512, ///< Actor-origin world-item selection satisfies the height criterion.
    COMPANION_SNAPSHOT_NEARBY_SPINNER_SPEED_POSITIVE = 1024, ///< Nearby spinner query reports positive speed.
    COMPANION_SNAPSHOT_SPECIAL_MESSAGE_CAPTURED = 8192, ///< Acknowledged pending value one with category 13, 14 or 15.
    COMPANION_SNAPSHOT_PLAYER_VIEW_WORLD_ITEM =
        16384, ///< Player-view world-item selection satisfies the actor-relative height criterion.
    COMPANION_SNAPSHOT_FIRST_ELAPSED_LIMIT_EXCEEDED = 32768,  ///< First enabled elapsed limit exceeded.
    COMPANION_SNAPSHOT_SECOND_ELAPSED_LIMIT_EXCEEDED = 65536, ///< Second enabled elapsed limit exceeded.
} CompanionSnapshotFlag;

#endif
