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
/// \param item Borrowed live item with initialized state; its current holder remains available.
/// \returns Nothing.
typedef void (*ItemHeldCallback)(u8 *item);
/// Respond when an item is released from a holder.
/// \param item Borrowed live item being released, with its former hold state still visible.
/// \param holderAddress Borrowed former actor-holder address, or zero for a player-held or missing holder.
/// \note The target ABI represents this address as a 32-bit integer; it must not outlive the holder.
/// \returns Nothing.
typedef void (*ItemReleaseCallback)(u8 *item, s32 holderAddress);
/// Prepare an item for launch.
/// \param item Borrowed initialized item after release, placement and launch velocity setup.
/// \returns Nothing.
typedef void (*ItemLaunchCallback)(u8 *item);
/// Respond to an item's terrain collision.
/// \param item Borrowed initialized item experiencing contact; the hook may replace it.
/// \param contactFlags Low 16 bits of the terrain contact mask.
/// \returns Zero to suppress the optional automatic terrain response, or nonzero to allow it.
typedef s32 (*ItemCollisionCallback)(u8 *item, u16 contactFlags);

/// Per-kind motion, rendering, response and description configuration.
typedef struct {
    /* 0x00 */ s32 id;                        ///< Item kind identifier.
    /* 0x04 */ u32 stateFlags;                ///< Initial item behavior flags.
    /* 0x08 */ s32 initialBounceCount;                      ///< Initial automatic-bounce allowance copied into item state.
    /* 0x0C */ s32 replacement;               ///< Replacement kind selected for transformation.
    /* 0x10 */ f32 growthMax;                 ///< Maximum replenishing search priority.
    /* 0x14 */ f32 growthRate;                ///< Search-priority replenishment rate.
    /* 0x18 */ u8 response[5];                ///< Transformation responses for causes one through five.
    /* 0x20 */ f32 scale;                     ///< Default scale relative to the scene scale.
    /* 0x24 */ f32 size;                      ///< Radius override, or the retained sentinel for the model default.
    /* 0x28 */ f32 motionCoefficient;         ///< Base coefficient for gravity and horizontal friction.
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
    /* 0x72 */ s16 availabilityField;         ///< Saved progress-field selector controlling item availability.
} ItemKind;                                   /* size 0x74 */

typedef char ItemKindSizeCheck[(sizeof(ItemKind) == 0x74) ? 1 : -1];
typedef char ItemKindBounceCountOffsetCheck[(((u32) & ((ItemKind *)0)->initialBounceCount) == 8) ? 1 : -1];
typedef char ItemKindFrictionOffsetCheck[(((u32) & ((ItemKind *)0)->frictionCoefficient) == 0x30) ? 1 : -1];
typedef char ItemKindEffectsOffsetCheck[(((u32) & ((ItemKind *)0)->effect) == 0x40) ? 1 : -1];
typedef char ItemKindMotionCoefficientOffsetCheck[(((u32)&((ItemKind *)0)->motionCoefficient) == 0x28) ? 1 : -1];
typedef char ItemKindAvailabilityOffsetCheck[(((u32)&((ItemKind *)0)->availabilityField) == 0x72) ? 1 : -1];

/// Per-kind item motion, rendering, response and description configuration.
/// \note Original identifier: \c D_80125A40 (0x80125A40).
extern ItemKind gItemKinds[];

/// Borrowed interior view beginning at each kind's held-item hook.
/// The remaining bytes overlap adjacent catalog fields; never allocate, copy or clear this view.
typedef struct ItemKindHeldCallbackEntry {
    ItemHeldCallback callback; ///< Optional held-item behavior.
    u8 following[0x70]; ///< Storage through the next held-item hook view.
} ItemKindHeldCallbackEntry;

/// Borrowed interior view beginning at each kind's terrain-collision hook.
/// The remaining bytes overlap adjacent catalog fields; never allocate, copy or clear this view.
typedef struct ItemKindCollisionCallbackEntry {
    ItemCollisionCallback callback; ///< Optional terrain-contact response.
    u8 following[0x70]; ///< Storage through the next collision-hook view.
} ItemKindCollisionCallbackEntry;

typedef char ItemKindHeldViewStrideCheck[(sizeof(ItemKindHeldCallbackEntry) == sizeof(ItemKind)) ? 1 : -1];
typedef char ItemKindCollisionViewStrideCheck[(sizeof(ItemKindCollisionCallbackEntry) == sizeof(ItemKind)) ? 1 : -1];
typedef char ItemKindHeldOffsetCheck[(((u32) & ((ItemKind *)0)->hold) == 0x44) ? 1 : -1];
typedef char ItemKindReleaseOffsetCheck[(((u32) & ((ItemKind *)0)->release) == 0x48) ? 1 : -1];
typedef char ItemKindLaunchOffsetCheck[(((u32) & ((ItemKind *)0)->launch) == 0x4C) ? 1 : -1];
typedef char ItemKindCollisionOffsetCheck[(((u32) & ((ItemKind *)0)->collide) == 0x50) ? 1 : -1];

/// Held-item hook view of the persistent item catalog.
/// \note Original identifier: \c D_80125A84 (0x80125A84).
extern ItemKindHeldCallbackEntry gItemKindHeldCallbackEntries[];

/// Terrain-contact hook view of the same catalog.
/// \note Original identifier: \c D_80125A90 (0x80125A90).
extern ItemKindCollisionCallbackEntry gItemKindCollisionCallbackEntries[];

#endif
