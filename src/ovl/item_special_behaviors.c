#include "math_scalar.h"
#include "pikachu_actor.h"
#include "terrain_surface.h"
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

#include "area_services.h"
#include "item_physics.h"
#include "item_special_behaviors.h"
/// Restore the baseball cap normal size when released by an actor.
/// \param object Item to update.
/// \param unused Unused release callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123060 (0x80123060).
void baseballCapResetReleasedScale(u8 *object, s32 unused) {
    u8 *objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (((ItemStatePrefix *)objectState)->state == ITEM_OWNERSHIP_PIKACHU) {
        itemSetScale(object, 1.0F);
    }
}

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

    if (((ItemStatePrefix *)rec)->state == ITEM_OWNERSHIP_PIKACHU) {
        if (!((ItemStatePrefix *)rec)->modeTime) {
            itemStartMaterialAnimation(object, 0xF4);
            ((ItemStatePrefix *)rec)->motionPhase.glowCountdown = 0.0F;
        }
        timer = &((ItemStatePrefix *)rec)->motionPhase.glowCountdown;
        *timer -= gRuntimeFrameSeconds;
        if (*timer <= 0.0F) {
            *timer = 1.0F;
            actorAdjustActionMeter((u8 *)((s32)gPikachuActor), -10.0F);
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
    if (((ItemStatePrefix *)objectState)->state == ITEM_OWNERSHIP_PIKACHU) {
        itemStartMaterialAnimation(object, 0xF3);
    }
}


/// Inflate an actor-held balloon and update its inflated state.
/// \param object Item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123178 (0x80123178).
void balloonUpdateHeldInflation(u8 *object) {
    u8 *rec = ((RuntimeObjectPrefix *)object)->state.bytes;
    f32 scale;

    if (((ItemStatePrefix *)rec)->state == ITEM_OWNERSHIP_PIKACHU) {
        scale = mathMapRangeClamped(0.0F, 1.0F, 6.0F, 10.0F, ((ItemStatePrefix *)rec)->modeTime);
        if (scale > 3.0F) {
            ((ItemStatePrefix *)rec)->flags |= 0x400000;
        } else {
            ((ItemStatePrefix *)rec)->flags &= ~0x400000;
        }
        itemSetScale(object, scale);
    }
}

#include "scene_configuration.h"

/// Begin upward flight for a sufficiently inflated balloon.
/// \param object Item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123220 (0x80123220).
void balloonBeginReleasedFlight(u8 *object) {
    u8 *sp1C;

    sp1C = ((RuntimeObjectPrefix *)object)->state.bytes;
    if ((gCurrentAreaConfiguration->objectScale * 7.0F) < ((RuntimeObjectPrefix *)object)->scale[0]) {
        itemSetMode(object, ITEM_MODE_DRIFTING_FLIGHT);
        ((ItemStatePrefix *)sp1C)->motionPhase.flightElapsed = 0.0F;
        ((ItemStatePrefix *)sp1C)->velocityZ = 0.0F;
        ((ItemStatePrefix *)sp1C)->velocityX = 0.0F;
        ((ItemStatePrefix *)sp1C)->downwardVelocity = -4.0F;
    }
}


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
        ((ItemStatePrefix *)st)->downwardVelocity = 0.0F;
    }
    terrainCreateContactEffect(((RuntimeObjectPrefix *)object)->position, hit);
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
    ((ItemStatePrefix *)objectState)->rotationVelocityX = 0.0F;
    ((ItemStatePrefix *)objectState)->rotationVelocityY = 600.0F;
}

/// Advance the item current behavior and apply its orientation rules.
/// \param object Item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123390 (0x80123390).
void itemUpdateModeAndOrientation(u8 *object) {
    u8 *state = ((RuntimeObjectPrefix *)object)->state.bytes;

    gItemModeUpdateHandlers[((ItemStatePrefix *)state)->mode](object);
    if (((ItemStatePrefix *)state)->flags & 0x800) {
        ((RuntimeObjectPrefix *)object)->rotation[0] = 0.0F;
        ((RuntimeObjectPrefix *)object)->rotation[1] = 0.0F;
        ((RuntimeObjectPrefix *)object)->rotation[2] = 0.0F;
    }
    if (((ItemStatePrefix *)state)->flags & 0x40000) {
        objectOrientTowardCamera(object);
    } else if (((ItemStatePrefix *)state)->flags & 0x80000) {
        objectFaceCameraHorizontally(object);
    }
}
