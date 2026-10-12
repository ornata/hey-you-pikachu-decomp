/// \file
/// Defines recovered item motion and rendering state shared by item callbacks.

#ifndef ITEM_TYPES_H
#define ITEM_TYPES_H

#include <ultra64.h>
#include "search_tree_types.h"
#include "integer_animation_types.h"

/// Proven item-state flags; other bits retain their numeric representation.
typedef enum ItemStateFlag { ITEM_STATE_SPIN_ENABLED = 0x20, ITEM_STATE_MATERIAL_ENABLED = 0x200 } ItemStateFlag;

/// Recovered item state prefix, ending before direction-animation state.
/// Unknown fields remain offset-named. This prefix is not the full allocation
/// and must not be used to allocate, copy, or clear an item state.
typedef struct ItemStatePrefix {
    s32 flags;                   ///< Item behavior and lifecycle flags.
    s16 kind;                    ///< Item-kind table index.
    s8 state;                    ///< Current item lifecycle state.
    s8 mode;                     ///< Additional mode used by transformation responses.
    s32 owner;                   ///< Owner handle supplied to item effects.
    f32 baseRadius;              ///< Unscaled object radius retained for item resizing.
    f32 downwardVelocity;        ///< Downward-positive vertical velocity.
    f32 velocityX;               ///< Horizontal X velocity.
    f32 velocityZ;               ///< Horizontal Z velocity.
    f32 rotationVelocityX;       ///< X rotation rate; reused by kind-specific motion.
    f32 rotationVelocityY;       ///< Y rotation rate; reused by kind-specific motion.
    s32 soundHandle;             ///< Last tracked spatial sound handle.
    f32 spinSpeed;               ///< Spin magnitude used by terrain response.
    f32 modeTime;                ///< Elapsed time used by the active item mode.
    f32 wriggleAngle;            ///< Phase angle used by pinata motion.
    s32 bounceCount;             ///< Remaining automatic bounce attempts.
    f32 searchPriority;          ///< Replenishing priority consumed by weighted item searches.
    s16 materialAnimationActive; ///< Nonzero while the material track controls the frame.
    u16 renderMode;              ///< Material rendering mode.
    ValueTrack materialTrack;    ///< Material animation cursor and retained texture frame.
    u8 tint[4];                  ///< Red, green, blue and alpha material components.
    s32 collisionKey;            ///< Collision-grid registration key, or zero when unregistered.
    TreeNode registryNode;       ///< Embedded node linked into the item search tree.
    Gfx *lighting[2];            ///< Owned lighting command buffers for alternate frames.
    f32 idleMotionDelay;         ///< Countdown before another idle direction motion may begin.
} ItemStatePrefix;

typedef char ItemStatePrefixSizeCheck[(sizeof(ItemStatePrefix) == 0x6C) ? 1 : -1];
#define ITEM_OFFSET(field) ((u32) & (((ItemStatePrefix *)0)->field))
typedef char ItemKindOffsetCheck[(ITEM_OFFSET(kind) == 4) ? 1 : -1];
typedef char ItemDownwardVelocityOffsetCheck[(ITEM_OFFSET(downwardVelocity) == 0x10) ? 1 : -1];
typedef char ItemSpinOffsetCheck[(ITEM_OFFSET(spinSpeed) == 0x28) ? 1 : -1];
typedef char ItemMaterialTrackOffsetCheck[(ITEM_OFFSET(materialTrack) == 0x40) ? 1 : -1];
typedef char ItemMaterialFrameOffsetCheck[(ITEM_OFFSET(materialTrack.value) == 0x48) ? 1 : -1];
typedef char ItemRegistryNodeOffsetCheck[(ITEM_OFFSET(registryNode) == 0x54) ? 1 : -1];
typedef char ItemLightingOffsetCheck[(ITEM_OFFSET(lighting) == 0x60) ? 1 : -1];
#undef ITEM_OFFSET

#endif
