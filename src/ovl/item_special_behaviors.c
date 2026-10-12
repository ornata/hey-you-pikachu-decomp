#include "item_size.h"
#include "item_mode.h"
#include "actor_meters.h"
#include "object_billboard.h"
#include "object_movement.h"
#include "item_object.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "item_response.h"
#include "scene_types.h"
#include "item_types.h"
#include "object_types.h"

/// Restore the baseball cap normal size when released by an actor.
/// \param object Item to update.
/// \param unused Unused release callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123060 (0x80123060).
void baseballCapResetReleasedScale(u8 *object, s32 unused) {
    u8 *objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (((ItemStatePrefix *)objectState)->state == 2) {
        itemSetScale(object, 1.0f);
    }
}

/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern s32 gPikachuActor;
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Animate a light bulb held by an actor and consume the primary actor action resource while it glows.
/// \param object Item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123098 (0x80123098).
void lightBulbUpdateHeldGlow(u8 *object) {
    u8 *rec = ((RuntimeObjectPrefix *)object)->state.bytes;
    f32 *timer;

    if (((ItemStatePrefix *)rec)->state == 2) {
        if (!((ItemStatePrefix *)rec)->modeTime) {
            itemStartMaterialAnimation(object, 0xF4);
            ((ItemStatePrefix *)rec)->motionPhase.glowCountdown = 0.0f;
        }
        timer = &((ItemStatePrefix *)rec)->motionPhase.glowCountdown;
        *timer -= gRuntimeFrameSeconds;
        if (*timer <= 0.0f) {
            *timer = 1.0f;
            actorAdjustActionMeter((u8 *)gPikachuActor, -10.0f);
        }
    }
}

/// Restore the light bulb unlit appearance when released by an actor.
/// \param object Item to update.
/// \param unused Unused release callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123140 (0x80123140).
void lightBulbResetReleasedGlow(u8 *object, s32 unused) {
    u8 *objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (((ItemStatePrefix *)objectState)->state == 2) {
        itemStartMaterialAnimation(object, 0xF3);
    }
}

f32 mathMapRangeClamped(f32, f32, f32, f32, f32); /* extern */

/// Inflate an actor-held balloon and update its inflated state.
/// \param object Item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123178 (0x80123178).
void balloonUpdateHeldInflation(u8 *object) {
    u8 *rec = ((RuntimeObjectPrefix *)object)->state.bytes;
    f32 scale;

    if (((ItemStatePrefix *)rec)->state == 2) {
        scale = mathMapRangeClamped(0.0f, 1.0f, 6.0f, 10.0f, ((ItemStatePrefix *)rec)->modeTime);
        if (scale > 3.0f) {
            ((ItemStatePrefix *)rec)->flags |= 0x400000;
        } else {
            ((ItemStatePrefix *)rec)->flags &= ~0x400000;
        }
        itemSetScale(object, scale);
    }
}

/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

/// Begin upward flight for a sufficiently inflated balloon.
/// \param object Item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123220 (0x80123220).
void balloonBeginReleasedFlight(u8 *object) {
    u8 *sp1C;

    sp1C = ((RuntimeObjectPrefix *)object)->state.bytes;
    if ((((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale * 7.0f) <
        ((RuntimeObjectPrefix *)object)->scale[0]) {
        itemSetMode(object, ITEM_MODE_DRIFTING_FLIGHT);
        ((ItemStatePrefix *)sp1C)->motionPhase.flightElapsed = 0.0f;
        ((ItemStatePrefix *)sp1C)->velocityZ = 0.0f;
        ((ItemStatePrefix *)sp1C)->velocityX = 0.0f;
        ((ItemStatePrefix *)sp1C)->downwardVelocity = -4.0f;
    }
}

s8 terrainGetSurfaceAttributes(f32, f32, f32, s32);  /* extern */
void terrainCreateContactEffect(u8 *, s32);      /* extern */
void itemPlayImpactSounds(u8 *, u16, s32); /* extern */

/// Resolve item contact with the map and play the appropriate impact effects.
/// \param object Item to update.
/// \param contactFlags Contact flags supplied by the item collision system.
/// \returns One when map contact was processed, or zero when the item was replaced before processing.
/// \note Original decompilation identifier: \c func_80123294 (0x80123294).
s32 itemResolveMapCollision(u8 *object, u16 contactFlags) {
    u8 *st;
    s32 hit;
    s32 other;

    st = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (((ItemStatePrefix *)st)->bounceCount < 2) {
        other = itemTransform(((RuntimeObjectPrefix *)object)->handle, 2);
        if ((other != 0) && (other != ((RuntimeObjectPrefix *)object)->handle)) {
            return 0;
        }
    }
    hit = terrainGetSurfaceAttributes(((RuntimeObjectPrefix *)object)->position[0], ((RuntimeObjectPrefix *)object)->position[1], ((RuntimeObjectPrefix *)object)->position[2], 0);
    if ((hit & 0xF) == 5) {
        ((ItemStatePrefix *)st)->downwardVelocity = 0.0f;
    }
    terrainCreateContactEffect(object + 0x18, hit);
    itemPlayImpactSounds(object, contactFlags, hit);
    if (--((ItemStatePrefix *)st)->bounceCount < 0) {
        ((ItemStatePrefix *)st)->bounceCount = 0;
    }
    return 1;
}

/// Initialize spinning flight for a launched flying item.
/// \param object Item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123374 (0x80123374).
void itemInitializeFlightSpin(u8 *object) {
    u8 *objectState;

    objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    ((ItemStatePrefix *)objectState)->rotationVelocityX = 0.0f;
    ((ItemStatePrefix *)objectState)->rotationVelocityY = 600.0f;
}

/// Advance the item current behavior and apply its orientation rules.
/// \param object Item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123390 (0x80123390).
void itemUpdateModeAndOrientation(u8 *object) {
    u8 *state = ((RuntimeObjectPrefix *)object)->state.bytes;

    gItemModeUpdateHandlers[((ItemStatePrefix *)state)->mode](object);
    if (((ItemStatePrefix *)state)->flags & 0x800) {
        ((RuntimeObjectPrefix *)object)->rotation[0] = 0.0f;
        ((RuntimeObjectPrefix *)object)->rotation[1] = 0.0f;
        ((RuntimeObjectPrefix *)object)->rotation[2] = 0.0f;
    }
    if (((ItemStatePrefix *)state)->flags & 0x40000) {
        objectOrientTowardCamera(object);
    } else if (((ItemStatePrefix *)state)->flags & 0x80000) {
        objectFaceCameraHorizontally(object);
    }
}
