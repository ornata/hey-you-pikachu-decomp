/// \file
/// Advances item motion and handles terrain contact, bouncing, and collision responses.

#include "random_scale.h"
#include "math_scalar.h"
#include "pikachu_actor.h"
#include "vector_metrics.h"
#include "math_angles.h"
#include "random.h"
#include "terrain_movement.h"
#include "terrain_surface.h"
#include "wisp_effect.h"
#include "actor_parts.h"
#include "item_size.h"
#include "item_participation.h"
#include "item_mode.h"
#include "item_queries.h"
#include "camera_pose.h"
#include "item_object.h"
#include "common.h"
#include "object_lookup.h"
#include "actor_script.h"
#include "item_types.h"
#include "item_audio.h"
#include "item_kind.h"
#include "item_kind_ids.h"
#include "object_types.h"

/// Start item disposal with its transition sound.
/// \param object Item to begin disposing of.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801217C0 (0x801217C0).
void itemBeginShrinkingDisposal(u8 *object) {
    itemPlayTrackedSound(object, (ItemStatePrefix *)((RuntimeObjectPrefix *)object)->state.bytes, 0xB7);
    itemSetMode(object, ITEM_MODE_SHRINKING_DISPOSAL);
}

/// Determine the normalized terrain response direction at an item.
/// \param object Item whose terrain is sampled.
/// \param direction Output three-component normalized terrain response direction.
/// \param cachedNode Address of a writable cached map-node pointer, initially null or valid, updated by the terrain query.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801217F4 (0x801217F4).
void itemGetTerrainResponseDirection(u8 *object, f32 *direction, u8 **cachedNode) {
    direction[1] = 0.2F;
    terrainGetDownhillHeightDifferences(
        ((RuntimeObjectPrefix *)object)->position[0], ((RuntimeObjectPrefix *)object)->position[1],
        ((RuntimeObjectPrefix *)object)->position[2], 50.0F, 0.2F, direction, &direction[2], cachedNode);
    vector3Normalize(direction);
}

#include "area_services.h"
#include "item_physics.h"
#include "vector_operations.h"
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
/// Advance item spin and optionally reduce it according to the contacted surface.
/// \param object Item whose spin is updated.
/// \param surfaceAttributes Terrain surface-attribute mask used to select the spin decay rate.
/// \param applyDecay Nonzero to reduce spin during this update.
/// \returns One when the item retains substantial spin, otherwise zero.
/// \note Original decompilation identifier: \c func_80121854 (0x80121854).
s32 itemUpdateSpin(u8 *object, s32 surfaceAttributes, s32 applyDecay) {
    u8 *state;
    f32 step;

    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (!(((ItemStatePrefix *)state)->flags & ITEM_STATE_SPIN_ENABLED) ||
        (((ItemStatePrefix *)state)->spinSpeed <= 0.0F)) {
        ((ItemStatePrefix *)state)->spinSpeed = 0.0F;
        return 0;
    }
    if (applyDecay != 0) {
        step = terrainGetFriction(surfaceAttributes);
        ((ItemStatePrefix *)state)->spinSpeed = ((ItemStatePrefix *)state)->spinSpeed - (gRuntimeFrameSeconds * step);
        ((RuntimeObjectPrefix *)object)->rotation[0] =
            mathMapRangeClamped(0.8F, 60.0F, 4.0F, 0.0F, ((ItemStatePrefix *)state)->spinSpeed);
    }
    step = mathMapRangeClamped(0.0F, 0.0F, 3.2F, 900.0F, ((ItemStatePrefix *)state)->spinSpeed) * gRuntimeFrameSeconds;
    ((RuntimeObjectPrefix *)object)->rotation[1] += step;
    if (((RuntimeObjectPrefix *)object)->rotation[1] < 0.0F) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[1] = (f32)(((RuntimeObjectPrefix *)object)->rotation[1] + 360.0F);
        } while (((RuntimeObjectPrefix *)object)->rotation[1] < 0.0F);
    }
    if (((RuntimeObjectPrefix *)object)->rotation[1] >= 360.0F) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[1] = (f32)(((RuntimeObjectPrefix *)object)->rotation[1] - 360.0F);
        } while (((RuntimeObjectPrefix *)object)->rotation[1] >= 360.0F);
    }
    ((RuntimeObjectPrefix *)object)->rotation[2] = 0.0F;
    if (((ItemStatePrefix *)state)->spinSpeed < 4.0F) {
        return 0;
    }
    return 1;
}

f32 fabsf(f32);
#pragma intrinsic(fabsf)
/// Reduce horizontal velocity toward rest for the current frame.
/// \param x Horizontal X velocity to update.
/// \param z Horizontal Z velocity to update.
/// \param rate Nonnegative deceleration rate before scaling.
/// \param threshold Nonnegative speed below which motion stops, before scaling.
/// \param scale Nonnegative multiplier applied to the stopping speed and deceleration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801219EC (0x801219EC).
void itemApplyHorizontalFriction(f32 *x, f32 *z, f32 rate, f32 threshold, f32 scale) {
    f32 dx;
    f32 dz;
    f32 valueX;
    f32 valueZ;
    dx = *x;
    dz = *z;
    if (vector2NormalizeComponentsApprox(&dx, &dz) < threshold * scale) {
        *z = 0.0F;
        *x = 0.0F;
        return;
    }
    rate *= scale * gRuntimeFrameSeconds;
    dx *= rate;
    dz *= rate;
    valueX = *x;
    if (fabsf(dx) > fabsf(valueX)) {
        dx = valueX;
    }
    valueZ = *z;
    if (fabsf(dz) > fabsf(valueZ)) {
        dz = valueZ;
    }
    *x -= dx;
    *z -= dz;
}

/// Separate friction-coefficient view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125A70 (0x80125A70).
extern u8 gItemKindFrictionEntries[];
/// Update item ground velocity and spin according to the terrain.
/// \param object Item whose terrain and motion are evaluated.
/// \param state Item state containing the velocity to update.
/// \param cachedNode Address of a writable cached map-node pointer, initially null or valid, updated by terrain queries.
/// \param spinning Output flag indicating whether substantial spin remains.
/// \returns Zero.
/// \note Two checked member-offset velocity reads preserve the original IDO scheduling and floating-point operands.
/// \note Original decompilation identifier: \c func_80121B18 (0x80121B18).
s32 itemUpdateGroundVelocity(u8 *object, u8 *state, s32 cachedNode, s32 *spinning) {
    s32 sp4C;
    f32 slope[3];
    s32 var_v1;
    f32 friction;
    f32 var_f12;
    f32 var_f14;

    sp4C = terrainGetSurfaceAttributes(((RuntimeObjectPrefix *)object)->position[0],
                                       ((RuntimeObjectPrefix *)object)->position[1],
                                       ((RuntimeObjectPrefix *)object)->position[2], (u8 **)cachedNode);
    var_v1 = ((sp4C & 0x100) || ((sp4C & 0x70) == 0x70)) ? 1 : 0;
    if (((ItemStatePrefix *)state)->flags & ITEM_STATE_SPIN_ENABLED) {
        *spinning = itemUpdateSpin(object, sp4C, 1);
    } else {
        *spinning = 0;
    }
    itemGetTerrainResponseDirection(object, slope, (u8 **)cachedNode);
    if ((var_v1 != 0) || (slope[0] > 0.01F) || (slope[0] < -0.01F) || (slope[2] > 0.01F) || (slope[2] < -0.01F)) {
        f32 accelerationStep;
        accelerationStep = 900.0F * gRuntimeFrameSeconds;
        ((ItemStatePrefix *)state)->velocityX =
            ((slope[0] * accelerationStep) + (*(f32 *)((s8 *)state + (u32) & ((ItemStatePrefix *)0)->velocityX)));
        ((ItemStatePrefix *)state)->velocityZ =
            ((slope[2] * accelerationStep) + (*(f32 *)((s8 *)state + (u32) & ((ItemStatePrefix *)0)->velocityZ)));
    }
    friction = terrainGetFriction(sp4C);
    friction = 60.0F * friction;
    var_f12 = ((ItemKind *)(gItemKindFrictionEntries - 0x30))[((ItemStatePrefix *)state)->kind].frictionCoefficient *
              friction;
    var_f14 =
        ((ItemKind *)(gItemKindFrictionEntries - 0x30))[((ItemStatePrefix *)state)->kind].frictionCoefficient * 3.0F;
    if (*spinning != 0) {
        var_f12 *= 0.8F;
        var_f14 *= 0.8F;
    }
    if (var_v1 != 0) {
        var_f12 *= 0.3F;
        var_f14 *= 0.3F;
    }
    itemApplyHorizontalFriction(&((ItemStatePrefix *)state)->velocityX, &((ItemStatePrefix *)state)->velocityZ, var_f12,
                                var_f14, itemGetMotionCoefficient(object));
    return 0;
}

/// Advance item rotation according to its rolling and angular-motion settings.
/// \param object Item whose orientation is updated.
/// \param state Item state containing its motion settings and angular velocity.
/// \returns Nothing.
/// \note The checked member-offset radius read preserves the original IDO floating-point operand order.
/// \note Original decompilation identifier: \c func_80121D2C (0x80121D2C).
void itemUpdateRollingRotation(u8 *object, u8 *state) {
    s32 itemFlags;
    f32 sp38;
    f32 sp34;

    itemFlags = ((ItemStatePrefix *)state)->flags;
    if (!(itemFlags & ITEM_STATE_AIRBORNE)) {
        if (itemFlags & ITEM_STATE_ROTATE_FROM_VELOCITY_COMPONENTS) {
            ((ItemStatePrefix *)state)->rotationVelocityX =
                (f32)((2.0F * ((ItemStatePrefix *)state)->velocityX) / gRuntimeFrameSeconds);
            ((ItemStatePrefix *)state)->rotationVelocityY =
                (f32)((2.0F * ((ItemStatePrefix *)state)->velocityZ) / gRuntimeFrameSeconds);
        } else if (itemFlags & ITEM_STATE_ROLL_FROM_HORIZONTAL_SPEED) {
            sp38 = ((ItemStatePrefix *)state)->velocityX;
            sp34 = ((ItemStatePrefix *)state)->velocityZ;
            ((ItemStatePrefix *)state)->rotationVelocityX =
                (f32)((57.295776F * vector2NormalizeComponentsApprox(&sp38, &sp34)) /
                      (*(f32 *)((s8 *)object + (u32) & ((RuntimeObjectPrefix *)0)->radius)));
            if (!(((ItemStatePrefix *)state)->flags & ITEM_STATE_AIRBORNE)) {
                ((ItemStatePrefix *)state)->rotationVelocityY =
                    (mathApproachAngleDegrees(((RuntimeObjectPrefix *)object)->rotation[1],
                                              mathUnitDirectionDegrees(sp34, sp38), 0.2F, 60.0F, 300.0F) -
                     ((RuntimeObjectPrefix *)object)->rotation[1]);
            }
            ((RuntimeObjectPrefix *)object)->rotation[2] = 0.0F;
        } else {
            ((ItemStatePrefix *)state)->rotationVelocityX *= 0.9F;
            ((ItemStatePrefix *)state)->rotationVelocityY *= 0.9F;
        }
    }
    ((RuntimeObjectPrefix *)object)->rotation[0] +=
        (((ItemStatePrefix *)state)->rotationVelocityX * gRuntimeFrameSeconds);
    if (((RuntimeObjectPrefix *)object)->rotation[0] < 0.0F) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[0] += 360.0F;
        } while (((RuntimeObjectPrefix *)object)->rotation[0] < 0.0F);
    }
    if (((RuntimeObjectPrefix *)object)->rotation[0] >= 360.0F) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[0] -= 360.0F;
        } while (((RuntimeObjectPrefix *)object)->rotation[0] >= 360.0F);
    }
    ((RuntimeObjectPrefix *)object)->rotation[1] +=
        (((ItemStatePrefix *)state)->rotationVelocityY * gRuntimeFrameSeconds);
    if (((RuntimeObjectPrefix *)object)->rotation[1] < 0.0F) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[1] += 360.0F;
        } while (((RuntimeObjectPrefix *)object)->rotation[1] < 0.0F);
    }
    if (((RuntimeObjectPrefix *)object)->rotation[1] >= 360.0F) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[1] -= 360.0F;
        } while (((RuntimeObjectPrefix *)object)->rotation[1] >= 360.0F);
    }
}

/// Adjust item velocity for a bounce or stop against the ground.
/// \param object Item whose ground response is evaluated.
/// \param state Item motion state whose velocity is updated.
/// \param cachedNode Address of a writable cached map-node pointer, initially null or valid, updated by the terrain query.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80121F64 (0x80121F64).
void itemApplyGroundBounce(u8 *object, ItemStatePrefix *state, s32 cachedNode) {
    f32 reflection;
    f32 normal[3];
    if ((state->flags & ITEM_STATE_SPIN_ENABLED) && state->downwardVelocity > 9.0F) {
        state->velocityX *= 0.2F;
        state->velocityZ *= 0.2F;
    }
    if (state->downwardVelocity < 9.0F || !(state->flags & ITEM_STATE_GROUND_BOUNCE_ENABLED)) {
        state->downwardVelocity = 0.0F;
    } else {
        itemGetTerrainResponseDirection(object, normal, (u8 **)cachedNode);
        vector3Normalize((f32 *)normal);
        reflection =
            (state->velocityX * normal[0] - state->downwardVelocity * normal[1]) + (normal[2] * state->velocityZ);
        reflection *= -2.0F;
        state->velocityX = (normal[0] * reflection) + (state->velocityX);
        state->downwardVelocity = ((state->downwardVelocity) - normal[1] * reflection) * 0.7F;
        state->velocityZ = (normal[2] * reflection) + (state->velocityZ);
        if (-state->downwardVelocity < 9.0F) {
            state->downwardVelocity = 0.0F;
        }
    }
}

/// Separate first tracked-sound view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125A80 (0x80125A80).
extern s16 gItemKindPrimarySoundEntries;
/// Separate second tracked-sound view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125A82 (0x80125A82).
extern s16 gItemKindSecondarySoundEntries;

/// Play the surface and item impact sounds appropriate to a contact.
/// \param object Item producing the impact.
/// \param contactFlags Collision flags controlling surface sound playback.
/// \param surface Surface properties used to select the impact sound.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801220B8 (0x801220B8).
void itemPlayImpactSounds(u8 *object, s32 contactFlags, s32 surface) {
    ItemStatePrefix *sp1C;
    s32 surfaceSoundId;

    sp1C = (ItemStatePrefix *)((RuntimeObjectPrefix *)object)->state.pointer;
    if (!(contactFlags & 0x2D)) {
        surfaceSoundId = terrainGetMaterialSound(6, surface);
        if (surfaceSoundId >= 0) {
            itemPlayTrackedSound(object, sp1C, surfaceSoundId);
        }
    }
    if ((sp1C)->bounceCount > 0) {
        itemPlayTrackedSound(object, sp1C, *((s16 *)((u8 *)&gItemKindPrimarySoundEntries + ((sp1C)->kind * 0x74))));
        return;
    }
    itemPlayTrackedSound(object, sp1C, *((s16 *)((u8 *)&gItemKindSecondarySoundEntries + ((sp1C)->kind * 0x74))));
}

/// Handle item contact and invoke its kind-specific collision response.
/// \param object Item involved in the contact.
/// \param flags Collision contact flags supplied to the response callback.
/// \pre An active actor must be available for actor-contact notifications.
/// \returns The kind-specific callback result, or one when no callback is installed.
///
/// \note Original decompilation identifier: \c func_80122184 (0x80122184).
s32 itemDispatchCollisionResponse(u8 *object, s32 flags) {
    u8 *state;
    ItemCollisionCallback callback;

    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    callback = gItemKindCollisionCallbackEntries[((ItemStatePrefix *)state)->kind].callback;
    if (flags & 0x40) {
        actorPostItemImpactMessage(((u8 *)gPikachuActor), object);
    }
    if (!(flags & 0x40) && !(((ItemStatePrefix *)state)->flags & ITEM_STATE_AREA_CONTACT_EVENT_HANDLED)) {
        if (areaDispatchPositionEvent(((RuntimeObjectPrefix *)object)->worldPosition, 6) != 0) {
            ((ItemStatePrefix *)state)->flags |= ITEM_STATE_AREA_CONTACT_EVENT_HANDLED;
        }
    }
    if (callback != NULL) {
        return callback(object, flags);
    }
    return 1;
}

#pragma intrinsic(fabsf)
/// Advance item movement and its terrain interactions, including bouncing, settling, and disposal.
/// \param object Moving item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80122268 (0x80122268).
void itemUpdateGroundAndAirMotion(u8 *object) {
    u8 *state;
    f32 dx;
    f32 dy;
    f32 dz;
    u16 flags;
    u8 *node;
    u32 collision;
    s32 drag;
    s32 ground;
    f32 speed;

    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    node = NULL;
    if (areaIsPositionOutsideBounds(((RuntimeObjectPrefix *)object)->worldPosition[0],
                                    ((RuntimeObjectPrefix *)object)->worldPosition[1],
                                    ((RuntimeObjectPrefix *)object)->worldPosition[2]) != 0) {
        itemDestroy(object);
        return;
    }
    ((ItemStatePrefix *)state)->flags &= ~ITEM_STATE_SIGNIFICANT_IMPACT;
    if (!(((ItemStatePrefix *)state)->flags & ITEM_STATE_AIRBORNE)) {
        itemUpdateGroundVelocity(object, state, (s32)&node, &drag);
    } else {
        drag = itemUpdateSpin(object, 0, 0);
    }
    speed = itemGetMotionCoefficient(object);
    ((ItemStatePrefix *)state)->downwardVelocity =
        (f32)(((ItemStatePrefix *)state)->downwardVelocity + (3.2666667F * gRuntimeFrameSeconds * speed * 30.0F));
    dx = ((ItemStatePrefix *)state)->velocityX * gRuntimeFrameSeconds;
    dy = -((ItemStatePrefix *)state)->downwardVelocity * gRuntimeFrameSeconds;
    dz = ((ItemStatePrefix *)state)->velocityZ * gRuntimeFrameSeconds;
    if (((ItemStatePrefix *)state)->flags & ITEM_STATE_PIKACHU_THROW_FLIGHT) {
        flags = terrainConstrainMovement(
            7, ((RuntimeObjectPrefix *)object)->position[0],
            ((RuntimeObjectPrefix *)object)->position[1] - ((RuntimeObjectPrefix *)object)->radius,
            ((RuntimeObjectPrefix *)object)->position[2], ((RuntimeObjectPrefix *)object)->radius * 2,
            ((RuntimeObjectPrefix *)object)->radius * 2, ((RuntimeObjectPrefix *)object)->radius, &dx, &dy, &dz, &node);
    } else {
        flags = terrainConstrainMovement(
            5, ((RuntimeObjectPrefix *)object)->position[0],
            ((RuntimeObjectPrefix *)object)->position[1] - ((RuntimeObjectPrefix *)object)->radius,
            ((RuntimeObjectPrefix *)object)->position[2], ((RuntimeObjectPrefix *)object)->radius * 2,
            ((RuntimeObjectPrefix *)object)->radius * 2, ((RuntimeObjectPrefix *)object)->radius, &dx, &dy, &dz, &node);
    }
    collision = flags;
    if (collision != 0) {
        if (((flags & 0x6D) || ((flags & 0x12) && (((ItemStatePrefix *)state)->flags & ITEM_STATE_AIRBORNE) &&
                                (fabsf(((ItemStatePrefix *)state)->downwardVelocity) > 18.0F))) &&
            ((fabsf(((ItemStatePrefix *)state)->velocityX) > 9.0F) ||
             (fabsf(((ItemStatePrefix *)state)->downwardVelocity) > 9.0F) ||
             (fabsf(((ItemStatePrefix *)state)->velocityZ) > 9.0F))) {
            ((ItemStatePrefix *)state)->flags |= ITEM_STATE_SIGNIFICANT_IMPACT;
        }
        if (flags & 0x10) {
            itemApplyGroundBounce(object, (ItemStatePrefix *)state, (s32)&node);
        }
        if (flags & 2) {
            ((ItemStatePrefix *)state)->downwardVelocity *= -0.7F;
        }
        if (flags & 9) {
            ((ItemStatePrefix *)state)->velocityX *= -0.7F;
        }
        if (flags & 0x24) {
            ((ItemStatePrefix *)state)->velocityZ *= -0.7F;
        }
    }
    ((RuntimeObjectPrefix *)object)->position[0] += dx;
    ((RuntimeObjectPrefix *)object)->position[1] += dy;
    ((RuntimeObjectPrefix *)object)->position[2] += dz;
    if (!(((ItemStatePrefix *)state)->flags & ITEM_STATE_SIGNIFICANT_IMPACT) ||
        (itemDispatchCollisionResponse(object, collision) != 0)) {
        if ((collision != 0) && !(((ItemStatePrefix *)state)->flags & ITEM_STATE_SKIP_EXHAUSTED_BOUNCE_DISPOSAL) &&
            (((ItemStatePrefix *)state)->bounceCount <= 0)) {
            itemBeginShrinkingDisposal(object);
            return;
        }
        if ((dx != 0.0F) || (dy != 0.0F) || (dz != 0.0F) || (((ItemStatePrefix *)state)->flags & ITEM_STATE_AIRBORNE) ||
            (((ItemStatePrefix *)state)->spinSpeed > 0.0F)) {
            if (drag == 0) {
                itemUpdateRollingRotation(object, state);
            }
        } else {
            ((ItemStatePrefix *)state)->velocityZ = 0.0F;
            ((ItemStatePrefix *)state)->downwardVelocity = 0.0F;
            ((ItemStatePrefix *)state)->velocityX = 0.0F;
            ((ItemStatePrefix *)state)->rotationVelocityY = 0.0F;
            ((ItemStatePrefix *)state)->rotationVelocityX = 0.0F;
            if (((ItemStatePrefix *)state)->flags & ITEM_STATE_AREA_CONTACT_EVENT_HANDLED) {
                ((ItemStatePrefix *)state)->flags &= ~ITEM_STATE_AREA_CONTACT_EVENT_HANDLED;
            } else {
                areaDispatchPositionEvent(((RuntimeObjectPrefix *)object)->worldPosition, 5);
            }
            ground = terrainGetSurfaceAttributes(((RuntimeObjectPrefix *)object)->position[0],
                                                 ((RuntimeObjectPrefix *)object)->position[1],
                                                 ((RuntimeObjectPrefix *)object)->position[2], &node);
            if ((ground & 0x70) == 0x70) {
                ((ItemStatePrefix *)state)->velocityX = (f32)((randomNext() * RANDOM_U32_UNIT_SCALE * 20.0) + 20.0);
                if ((randomNext() * RANDOM_U32_UNIT_SCALE) < 0.5) {
                    ((ItemStatePrefix *)state)->velocityX = -((ItemStatePrefix *)state)->velocityX;
                }
                ((ItemStatePrefix *)state)->velocityZ = (f32)((randomNext() * RANDOM_U32_UNIT_SCALE * 20.0) + 20.0);
                if ((randomNext() * RANDOM_U32_UNIT_SCALE) < 0.5) {
                    ((ItemStatePrefix *)state)->velocityZ = -((ItemStatePrefix *)state)->velocityZ;
                }
                ((ItemStatePrefix *)state)->bounceCount = (s32)((u32)((ItemStatePrefix *)state)->bounceCount - 1U);
                ((ItemStatePrefix *)state)->downwardVelocity = -60.0F;
                if (((ItemStatePrefix *)state)->bounceCount < 0) {
                    ((ItemStatePrefix *)state)->bounceCount = 0;
                }
                if (((ground & 0xF) == 5) || ((ground & 0xF) == 7)) {
                    wispEffectCreate(9, ((RuntimeObjectPrefix *)object)->position[0],
                                     ((RuntimeObjectPrefix *)object)->position[1],
                                     ((RuntimeObjectPrefix *)object)->position[2]);
                    return;
                }
                wispEffectCreate(8, ((RuntimeObjectPrefix *)object)->position[0],
                                 ((RuntimeObjectPrefix *)object)->position[1],
                                 ((RuntimeObjectPrefix *)object)->position[2]);
                return;
            }
            itemSetMode(object, ITEM_MODE_STATIONARY);
        }
        if (flags & 0x10) {
            ((ItemStatePrefix *)state)->flags &=
                ~(ITEM_STATE_AIRBORNE | ITEM_STATE_PLAYER_THROW_FLIGHT | ITEM_STATE_PIKACHU_THROW_FLIGHT);
            return;
        }
        ((ItemStatePrefix *)state)->flags |= ITEM_STATE_AIRBORNE;
    }
}

/// Advance an item drifting through the air and begin disposal when its flight ends.
/// \param object Flying item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801228F4 (0x801228F4).
void itemUpdateDriftingFlight(u8 *object) {
    f32 wave;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 radius;
    u8 *node;
    f32 diameter;
    u8 *state;
    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    node = NULL;
    ((ItemStatePrefix *)state)->motionPhase.flightElapsed += gRuntimeFrameSeconds;
    if (((ItemStatePrefix *)state)->motionPhase.flightElapsed > 10.0F) {
        itemSetMode(object, ITEM_MODE_SHRINKING_DISPOSAL);
        return;
    }
    wave = 2.0F * mathCosDegrees(((ItemStatePrefix *)state)->motionPhase.flightElapsed * 120.0F);
    dx = (((ItemStatePrefix *)state)->velocityX + wave) * gRuntimeFrameSeconds;
    dz = (((ItemStatePrefix *)state)->velocityZ + wave) * gRuntimeFrameSeconds;
    dy = -((ItemStatePrefix *)state)->downwardVelocity * gRuntimeFrameSeconds;
    radius = ((RuntimeObjectPrefix *)object)->radius;
    diameter = radius * 2;
    if (terrainConstrainMovement(
            5, ((RuntimeObjectPrefix *)object)->position[0], ((RuntimeObjectPrefix *)object)->position[1] - radius,
            ((RuntimeObjectPrefix *)object)->position[2], diameter, diameter, radius, &dx, &dy, &dz, &node) &
        0x12) {
        itemPlayTrackedSound(object, (ItemStatePrefix *)state, 0xBD);
        itemSetMode(object, ITEM_MODE_SHRINKING_DISPOSAL);
        return;
    }
    ((RuntimeObjectPrefix *)object)->position[0] += dx;
    ((RuntimeObjectPrefix *)object)->position[1] += dy;
    ((RuntimeObjectPrefix *)object)->position[2] += dz;
}

/// Run the held-item behavior supplied by the item kind, when present.
/// \param object Address of the held item.
/// \param state Borrowed initialized item state identifying the callback kind.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80122A6C (0x80122A6C).
void itemRunHeldKindCallback(u8 *object, ItemStatePrefix *state) {
    ItemHeldCallback heldCallback;

    heldCallback = gItemKindHeldCallbackEntries[state->kind].callback;
    if (heldCallback != NULL) {
        heldCallback(object);
    }
}

/// Separate symbol view of the primary player record's view yaw.
/// \note Original identifier: \c D_800EF6AC (0x800EF6AC).
extern f32 gPlayerYaw;
/// Separate symbol view of the primary player record's additional view pitch.
/// \note Original identifier: \c D_800EF6B0 (0x800EF6B0).
extern f32 gPlayerPitchOffset;
/// Separate symbol view of the primary player record's view pitch.
/// \note Original identifier: \c D_800EF6B4 (0x800EF6B4).
extern f32 gPlayerPitch;
/// Separate symbol view of the primary player record's horizontal view shake.
/// \note Original identifier: \c D_800EF6B8 (0x800EF6B8).
extern f32 gPlayerViewShake;

/// Update an item held by the player and its placement relative to the active camera.
/// \param object Player-held item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80122AB8 (0x80122AB8).
void itemUpdatePlayerHeld(u8 *object) {
    f32 *pos;
    u8 *state = ((RuntimeObjectPrefix *)object)->state.bytes;

    itemRunHeldKindCallback(object, (ItemStatePrefix *)state);
    ((ItemStatePrefix *)state)->modeTime += gRuntimeFrameSeconds;
    pos = ((RuntimeObjectPrefix *)object)->position;
    pos[0] = 1.25F;
    pos[1] = -0.9F;
    pos[2] = -3.0F;
    cameraViewPointToWorld(pos);
    ((RuntimeObjectPrefix *)object)->worldPosition[0] = ((RuntimeObjectPrefix *)object)->position[0];
    ((RuntimeObjectPrefix *)object)->worldPosition[1] = ((RuntimeObjectPrefix *)object)->position[1];
    ((RuntimeObjectPrefix *)object)->worldPosition[2] = ((RuntimeObjectPrefix *)object)->position[2];
    ((RuntimeObjectPrefix *)object)->rotation[0] = gPlayerPitchOffset + gPlayerPitch;
    ((RuntimeObjectPrefix *)object)->rotation[1] = gPlayerYaw + gPlayerViewShake;
    ((RuntimeObjectPrefix *)object)->rotation[2] = 0.0F;
}

/// Animate the piñata wriggling while held.
/// \param item Piñata item with an initialized model and three successive child links to animate.
/// \returns Nothing.
///
/// \note NONPORTABLE: IDO schedules the damping load before initialization when these share a line.
/// \note Original decompilation identifier: \c func_80122B7C (0x80122B7C).
void pinataUpdateWriggle(u8 *item) {
    u8 *state;
    f32 angle;
    u8 *model;
    s32 index;

    state = ((RuntimeObjectPrefix *)item)->state.bytes;
    model = ((RuntimeObjectPrefix *)item)->child.bytes;
    if ((randomBelow(0x64) == 0) || (((ItemStatePrefix *)state)->rotationVelocityX < 10.0F)) {
        ((ItemStatePrefix *)state)->rotationVelocityX = (f32)(s32)(randomBelow(0x32) + 0xA);
    }
    ((ItemStatePrefix *)state)->motionPhase.wriggleAngle +=
        ((ItemStatePrefix *)state)->rotationVelocityX * gRuntimeFrameSeconds;
    if (((ItemStatePrefix *)state)->motionPhase.wriggleAngle < 0.0F) {
        do {
            ((ItemStatePrefix *)state)->motionPhase.wriggleAngle =
                (f32)(((ItemStatePrefix *)state)->motionPhase.wriggleAngle + 360.0F);
        } while (((ItemStatePrefix *)state)->motionPhase.wriggleAngle < 0.0F);
    }
    if (((ItemStatePrefix *)state)->motionPhase.wriggleAngle >= 360.0F) {
        do {
            ((ItemStatePrefix *)state)->motionPhase.wriggleAngle =
                (f32)(((ItemStatePrefix *)state)->motionPhase.wriggleAngle - 360.0F);
        } while (((ItemStatePrefix *)state)->motionPhase.wriggleAngle >= 360.0F);
    }
    ((RuntimeObjectPrefix *)model)->rotation[1] =
        (f32)(mathSinDegrees(((ItemStatePrefix *)state)->motionPhase.wriggleAngle) * 40.0F);
    if ((randomBelow(0x64) == 0) || (((ItemStatePrefix *)state)->rotationVelocityY < 10.0F)) {
        ((ItemStatePrefix *)state)->rotationVelocityY = (f32)(s32)(randomBelow(0x64) + 0x190);
    }
    ((ItemStatePrefix *)state)->spinSpeed += ((ItemStatePrefix *)state)->rotationVelocityY * gRuntimeFrameSeconds;
    if (((ItemStatePrefix *)state)->spinSpeed < 0.0F) {
        do {
            ((ItemStatePrefix *)state)->spinSpeed = (f32)(((ItemStatePrefix *)state)->spinSpeed + 360.0F);
        } while (((ItemStatePrefix *)state)->spinSpeed < 0.0F);
    }
    if (((ItemStatePrefix *)state)->spinSpeed >= 360.0F) {
        do {
            ((ItemStatePrefix *)state)->spinSpeed = (f32)(((ItemStatePrefix *)state)->spinSpeed - 360.0F);
        } while (((ItemStatePrefix *)state)->spinSpeed >= 360.0F);
    }
    angle = mathSinDegrees(((ItemStatePrefix *)state)->spinSpeed) * 15.0F;

    /* clang-format off */
    index = 0; do {
        /* clang-format on */
        model = ((RuntimeObjectPrefix *)model)->child.bytes;
        ((RuntimeObjectPrefix *)model)->rotation[1] = angle;
        if (!(index & 1)) {
            angle = -angle * 0.9F;
        }
        index += 1;
    } while (index != 3);
}

/// Update an actor-held item attachment, or release it when its holder is absent.
/// \param object Actor-held item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80122DE0 (0x80122DE0).
void itemUpdateActorHeld(u8 *object) {
    s32 parentId;
    u8 *parent;
    u8 *parentState;
    u8 *state;
    s16 kind;
    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    parentId = ((ItemStatePrefix *)state)->owner;
    parent = objectFindLiveByHandle(parentId);
    if (parent == NULL) {
        itemTryRelease(object, 1);
        return;
    }
    itemRunHeldKindCallback(object, (ItemStatePrefix *)state);
    ((ItemStatePrefix *)state)->modeTime += gRuntimeFrameSeconds;
    parentState = ((RuntimeObjectPrefix *)parent)->state.bytes;
    actorGetConfiguredAttachmentPosition(parent, ((RuntimeObjectPrefix *)object)->radius,
                                         ((RuntimeObjectPrefix *)object)->position);
    ((RuntimeObjectPrefix *)object)->worldPosition[0] = ((RuntimeObjectPrefix *)object)->position[0];
    ((RuntimeObjectPrefix *)object)->worldPosition[1] = ((RuntimeObjectPrefix *)object)->position[1];
    ((RuntimeObjectPrefix *)object)->worldPosition[2] = ((RuntimeObjectPrefix *)object)->position[2];
    ((RuntimeObjectPrefix *)object)->rotation[0] = ((ActorScriptState *)parentState)->orientation[0];
    ((RuntimeObjectPrefix *)object)->rotation[1] = ((ActorScriptState *)parentState)->orientation[1];
    ((RuntimeObjectPrefix *)object)->rotation[2] = ((ActorScriptState *)parentState)->orientation[2];
    kind = ((ItemStatePrefix *)state)->kind;
    if ((kind == ITEM_KIND_PINATA || kind == ITEM_KIND_CRACKED_PINATA) &&
        ((RuntimeObjectPrefix *)object)->scale[0] < 0.049999997F) {
        pinataUpdateWriggle(object);
    }
}

/// Animate an item shrinking away and remove it when finished.
/// \param object Item being disposed of.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80122ED8 (0x80122ED8).
void itemUpdateShrinkingDisposal(u8 *object) {
    RuntimeObjectPrefix *transform = (RuntimeObjectPrefix *)object;
    u8 *state;
    f32 value;
    transform->scale[0] -= gRuntimeFrameSeconds * 0.01F;
    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    value = transform->scale[0];
    if (value < 0.0F) {
        transform->scale[2] = 0.0F;
        transform->scale[1] = 0.0F;
        transform->scale[0] = 0.0F;
        itemPlayTrackedSound(object, (ItemStatePrefix *)state, 0xBD);
        itemDestroy(object);
        return;
    }
    transform->scale[2] = (transform->scale[1] = transform->scale[0]);
    transform->rotation[2] = 0.0F;
    transform->rotation[1] = 0.0F;
    transform->rotation[0] += gRuntimeFrameSeconds * 400.0F;
    while (transform->rotation[0] < 0.0F) {
        transform->rotation[0] += 360.0F;
    }

    while (transform->rotation[0] >= 360.0F) {
        transform->rotation[0] -= 360.0F;
    }
}

/// Provide the behavior update for an item without autonomous motion.
/// \param unused Unused item callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80122FF8 (0x80122FF8).
void itemUpdateStationaryMode(u8 *unused) {
}

/// Update baseball-cap visibility for the primary actor held-item appearance.
/// \param object Baseball-cap item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123000 (0x80123000).
void baseballCapUpdateHeldVisibility(u8 *object) {
    u8 *objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if ((((ItemStatePrefix *)objectState)->state == ITEM_OWNERSHIP_PIKACHU) &&
        (((ActorScriptState *)((RuntimeObjectPrefix *)((u8 *)gPikachuActor))->state.pointer)->attachmentMode ==
         ACTOR_ATTACHMENT_RAISED)) {
        itemSetScale(object, 0.0F);
        return;
    }
    itemSetScale(object, 1.0F);
}
