/// \file
/// Defines recovered item motion and rendering state shared by item callbacks.

#ifndef ITEM_TYPES_H
#define ITEM_TYPES_H

#include <ultra64.h>
#include "search_tree_types.h"
#include "integer_animation_types.h"

/// Proven item-state flags; other bits retain their numeric representation.
/// Hierarchy and terrain-contact flags use separate domains.
typedef enum ItemStateFlag {
    /// Skip the stationary-mode hierarchy mark; mode selection still applies.
    ITEM_STATE_SKIP_STATIONARY_HIERARCHY_MARK = 0x2,
    /// Permit ground bounce when speed and terrain conditions allow it.
    ITEM_STATE_GROUND_BOUNCE_ENABLED = 0x4,
    /// Derive grounded rolling from horizontal speed and radius unless component rotation is selected.
    ITEM_STATE_ROLL_FROM_HORIZONTAL_SPEED = 0x8,
    /// Derive grounded angular rates from velocity components; takes precedence over speed-based rolling.
    ITEM_STATE_ROTATE_FROM_VELOCITY_COMPONENTS = 0x10,
    /// Enable the item's angular spin response.
    ITEM_STATE_SPIN_ENABLED = 0x20,
    /// Block player pickup, including otherwise permitted holder transfers.
    ITEM_STATE_BLOCK_PLAYER_PICKUP = 0x40,
    /// Block actor pickup, including otherwise permitted holder transfers.
    ITEM_STATE_BLOCK_ACTOR_PICKUP = 0x80,
    /// Support the runtime cooperation on/off state; does not enable cooperation by itself.
    ITEM_STATE_COOPERATION_SUPPORTED = 0x100,
    /// Enable the item material draw path.
    ITEM_STATE_MATERIAL_ENABLED = 0x200,
    /// Skip default disposal after contact with exhausted bounces; other destruction paths still apply.
    ITEM_STATE_SKIP_EXHAUSTED_BOUNCE_DISPOSAL = 0x400,
    /// Reset all rotation axes before applying either camera-facing rule.
    ITEM_STATE_RESET_ROTATION = 0x800,
    /// Permit the configured replacement when consumed; otherwise consumption removes the item.
    ITEM_STATE_CONSUMPTION_REPLACEMENT_ALLOWED = 0x1000,
    /// Permit the configured replacement for transformation cause one instead of the fallback kind.
    ITEM_STATE_CAUSE_1_REPLACEMENT_ALLOWED = 0x2000,
    /// Permit the configured replacement for transformation cause two; otherwise retain the item.
    ITEM_STATE_CAUSE_2_REPLACEMENT_ALLOWED = 0x4000,
    /// Suppress timed visual direction motion; its idle postponement still applies.
    ITEM_STATE_DIRECTION_MOTION_SUPPRESSED = 0x8000,
    /// Exclude world-owned items from collision-grid registration; physics still applies.
    ITEM_STATE_SKIP_COLLISION_GRID = 0x10000,
    /// Face the camera; takes precedence over horizontal camera facing.
    ITEM_STATE_FACE_CAMERA = 0x40000,
    /// Face the camera horizontally when full camera facing is disabled.
    ITEM_STATE_FACE_CAMERA_HORIZONTALLY = 0x80000,
    /// Let the material animation value govern child visibility while the parent is visible.
    ITEM_STATE_MATERIAL_CONTROLS_CHILD_VISIBILITY = 0x100000,
    /// Current movement step has a significant impact eligible for collision-response handling.
    ITEM_STATE_SIGNIFICANT_IMPACT = 0x200000,
    /// Halve the kind motion coefficient used for gravity and friction.
    ITEM_STATE_HALF_MOTION_COEFFICIENT = 0x400000,
    /// Block interaction responses and consumption; pickup has separate restrictions.
    ITEM_STATE_DISABLE_INTERACTION_RESPONSE_AND_CONSUMPTION = 0x800000,
    /// Airborne motion state, cleared on floor contact and set when no floor is contacted.
    ITEM_STATE_AIRBORNE = 0x1000000,
    /// Player-origin throw flight; also blocks player pickup until floor contact.
    ITEM_STATE_PLAYER_THROW_FLIGHT = 0x4000000,
    /// Pikachu-origin throw flight, selecting its terrain constraint policy until floor contact.
    ITEM_STATE_PIKACHU_THROW_FLIGHT = 0x8000000,
    /// Area contact event was handled; suppress the next settled-contact notification.
    ITEM_STATE_AREA_CONTACT_EVENT_HANDLED = 0x10000000,
    /// Shrink during each item update; other disposal conditions remain independent.
    ITEM_STATE_SHRINK_ON_UPDATE = 0x20000000,
    /// Bypass holder-specific restrictions on pickup and unforced release.
    /// Other item restrictions still apply; successful release clears this permission.
    ITEM_STATE_HELD_TRANSFER_ALLOWED = 0x40000000,
    /// Runtime cooperation on/off state; requires cooperation support for the query to succeed.
    ITEM_STATE_COOPERATION_ENABLED = 0x80000000
} ItemStateFlag;

/// Item ownership and world-participation states, stored as a signed byte.
/// These selectors are independent of the ItemMode behavior-update selector.
typedef enum ItemOwnershipState {
    ITEM_OWNERSHIP_WORLD = 0,       ///< Unheld world item; other flags still govern participation.
    ITEM_OWNERSHIP_PLAYER = 1,      ///< Held by the player; owner is zero.
    ITEM_OWNERSHIP_PIKACHU = 2,     ///< Held by the primary companion; owner is its object handle.
    ITEM_OWNERSHIP_OTHER_ACTOR = 3, ///< Held by another actor; owner is its object handle.
    ITEM_OWNERSHIP_TOOLBOX = 4,     ///< Retained in toolbox storage and excluded from item searches.
    ITEM_OWNERSHIP_SUSPENDED = 5    ///< Hidden item with world participation suspended.
} ItemOwnershipState;

/// Kind- and mode-specific storage reused for angular motion and timers.
typedef union ItemMotionPhase {
    f32 wriggleAngle;  ///< Piñata phase in degrees.
    f32 glowCountdown; ///< Seconds until the next held light-bulb glow update.
    f32 flightElapsed; ///< Seconds elapsed in drifting-flight mode.
} ItemMotionPhase;

typedef char ItemMotionPhaseSizeCheck[(sizeof(ItemMotionPhase) == 4) ? 1 : -1];

/// Recovered item state prefix, ending before direction-animation state.
/// Unknown fields remain offset-named. This prefix is not the full allocation
/// and must not be used to allocate, copy, or clear an item state.
typedef struct ItemStatePrefix {
    s32 flags;             ///< Item behavior and lifecycle flags.
    s16 kind;              ///< Item-kind table index.
    s8 state;              ///< ItemOwnershipState selector; retained as a signed byte.
    s8 mode;               ///< Independent ItemMode behavior-update selector.
    s32 owner;             ///< Borrowed actor-holder handle in actor-held states; zero when unheld or player-held.
    f32 baseRadius;        ///< Unscaled object radius retained for item resizing.
    f32 downwardVelocity;  ///< Downward-positive vertical velocity.
    f32 velocityX;         ///< Horizontal X velocity.
    f32 velocityZ;         ///< Horizontal Z velocity.
    f32 rotationVelocityX; ///< X rotation rate; reused by kind-specific motion.
    f32 rotationVelocityY; ///< Y rotation rate; reused by kind-specific motion.
    s32 soundHandle;       ///< Last tracked spatial sound handle.
    f32 spinSpeed;         ///< Spin magnitude used by terrain response.
    f32 modeTime;          ///< Elapsed time used by the active item mode.
    ItemMotionPhase motionPhase; ///< Phase or elapsed time selected by item kind and mode.
    s32 bounceCount;             ///< Remaining automatic bounce attempts.
    f32 searchPriority;          ///< Replenishing priority consumed by weighted item searches.
    s16 materialAnimationActive; ///< Nonzero while the material track controls the frame.
    u16 renderMode;              ///< Material rendering mode.
    ValueTrack materialTrack;    ///< Material animation cursor and retained texture frame.
    u8 tint[4];                  ///< Red, green, blue and alpha material components.
    s32 collisionKey;            ///< Collision-grid registration key, or zero when unregistered.
    TreeNode registryNode;       ///< Embedded node linked into the item search tree.
    Gfx *lighting[2];    ///< Alternate command-buffer views into one owned allocation; lighting[0] owns the base.
    f32 idleMotionDelay; ///< Countdown before another idle direction motion may begin.
} ItemStatePrefix;

typedef char ItemStatePrefixSizeCheck[(sizeof(ItemStatePrefix) == 0x6C) ? 1 : -1];
#define ITEM_OFFSET(field) ((u32) & (((ItemStatePrefix *)0)->field))
typedef char ItemOwnershipOffsetCheck[(ITEM_OFFSET(state) == 6) ? 1 : -1];
typedef char ItemModeOffsetCheck[(ITEM_OFFSET(mode) == 7) ? 1 : -1];
typedef char ItemOwnerOffsetCheck[(ITEM_OFFSET(owner) == 8) ? 1 : -1];
typedef char ItemKindOffsetCheck[(ITEM_OFFSET(kind) == 4) ? 1 : -1];
typedef char ItemDownwardVelocityOffsetCheck[(ITEM_OFFSET(downwardVelocity) == 0x10) ? 1 : -1];
typedef char ItemMotionPhaseOffsetCheck[(ITEM_OFFSET(motionPhase) == 0x30) ? 1 : -1];
typedef char ItemSpinOffsetCheck[(ITEM_OFFSET(spinSpeed) == 0x28) ? 1 : -1];
typedef char ItemMaterialTrackOffsetCheck[(ITEM_OFFSET(materialTrack) == 0x40) ? 1 : -1];
typedef char ItemMaterialFrameOffsetCheck[(ITEM_OFFSET(materialTrack.value) == 0x48) ? 1 : -1];
typedef char ItemTintOffsetCheck[(ITEM_OFFSET(tint) == 0x4C) ? 1 : -1];
typedef char ItemCollisionKeyOffsetCheck[(ITEM_OFFSET(collisionKey) == 0x50) ? 1 : -1];
typedef char ItemRegistryNodeOffsetCheck[(ITEM_OFFSET(registryNode) == 0x54) ? 1 : -1];
typedef char ItemLightingOffsetCheck[(ITEM_OFFSET(lighting) == 0x60) ? 1 : -1];
#undef ITEM_OFFSET

#endif
