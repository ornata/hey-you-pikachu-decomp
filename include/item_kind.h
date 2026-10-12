/// \file
/// Defines shared item-kind configuration, model templates and display text.
#ifndef ITEM_KIND_H
#define ITEM_KIND_H
#include <ultra64.h>
#include "object_template.h"

/// Borrowed object hierarchy template used by an item kind.
typedef ObjectTemplate ItemModel;

/// Persistent item name and interaction-help text.
typedef struct {
    char *name; ///< Persistent display name.
    char *help; ///< Persistent interaction description.
} ItemText;

/// Update an item while held.
/// \param item Initialized item object.
/// \returns Nothing.
typedef void (*ItemHeldCallback)(u8 *item);
/// Respond when an item is released from a holder.
/// \param item Initialized item object being released.
/// \param holderAddress Original holder object address, or zero when unavailable.
/// \returns Nothing.
typedef void (*ItemReleaseCallback)(u8 *item, s32 holderAddress);
/// Prepare an item for launch.
/// \param item Initialized item receiving launch behavior.
/// \returns Nothing.
typedef void (*ItemLaunchCallback)(u8 *item);
/// Respond to an item's terrain collision.
/// \param item Initialized item experiencing the collision.
/// \param contactFlags Terrain contact mask describing the collision.
/// \returns Response status interpreted by item motion.
typedef s32 (*ItemCollisionCallback)(u8 *item, u16 contactFlags);

/// Per-kind motion, rendering, response and description configuration.
typedef struct {
    /* 0x00 */ s32 id;                        ///< Item kind identifier.
    /* 0x04 */ u32 stateFlags;                ///< Initial item behavior flags.
    /* 0x08 */ s32 unk8;                      ///< Initial bounce allowance; other uses remain unresolved.
    /* 0x0C */ s32 replacement;               ///< Replacement kind selected for transformation.
    /* 0x10 */ f32 growthMax;                 ///< Maximum replenishing search priority.
    /* 0x14 */ f32 growthRate;                ///< Search-priority replenishment rate.
    /* 0x18 */ u8 response[5];                ///< Transformation responses for causes one through five.
    /* 0x20 */ f32 scale;                     ///< Default scale relative to the scene scale.
    /* 0x24 */ f32 size;                      ///< Radius override, or the retained sentinel for the model default.
    /* 0x28 */ f32 unk28;                     ///< Motion coefficient whose complete interpretation remains unresolved.
    /* 0x2C */ f32 speed;                     ///< Multiplier applied to launch velocity.
    /* 0x30 */ f32 frictionCoefficient;       ///< Multiplier for terrain friction and horizontal motion thresholds.
    /* 0x34 */ f32 radiusX;                   ///< Horizontal collision extent along X.
    /* 0x38 */ f32 radiusZ;                   ///< Horizontal collision extent along Z.
    /* 0x3C */ s16 animation;                 ///< Initial material animation-track identifier.
    /* 0x3E */ s16 unk3E;                     ///< Offset 0x3E; purpose remains unresolved.
    /* 0x40 */ s16 effect[2];                 ///< Tracked sound identifiers selected by bounce allowance.
    /* 0x44 */ ItemHeldCallback hold;         ///< Optional behavior while held.
    /* 0x48 */ ItemReleaseCallback release;   ///< Optional release response.
    /* 0x4C */ ItemLaunchCallback launch;     ///< Optional launch preparation.
    /* 0x50 */ ItemCollisionCallback collide; ///< Optional terrain-collision response.
    /* 0x54 */ ItemModel *model;              ///< Borrowed model hierarchy template.
    /* 0x58 */ u8 color[4];                   ///< Environment red, green, blue and alpha components.
    /* 0x5C */ ItemText text[2];              ///< Persistent alternate names and help descriptions.
    /* 0x6C */ s32 unk6C;                     ///< Offset 0x6C; purpose remains unresolved.
    /* 0x70 */ u8 unk70;                      ///< Offset 0x70; purpose remains unresolved.
    /* 0x71 */ u8 itemClass;                  ///< Item classification used by registry queries.
    /* 0x72 */ s16 unk72;                     ///< Offset 0x72; purpose remains unresolved.
} ItemKind;                                   /* size 0x74 */

typedef char ItemKindSizeCheck[(sizeof(ItemKind) == 0x74) ? 1 : -1];
typedef char ItemKindFrictionOffsetCheck[(((u32) & ((ItemKind *)0)->frictionCoefficient) == 0x30) ? 1 : -1];
typedef char ItemKindEffectsOffsetCheck[(((u32) & ((ItemKind *)0)->effect) == 0x40) ? 1 : -1];
#endif
