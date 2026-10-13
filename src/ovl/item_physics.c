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
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "object_lookup.h"
#include "actor_script.h"
#include "item_types.h"
#include "item_audio.h"
#include "item_kind.h"
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
    direction[1] = 0.2f;
    terrainGetDownhillHeightDifferences(((RuntimeObjectPrefix *)object)->position[0],
                                        ((RuntimeObjectPrefix *)object)->position[1],
                                        ((RuntimeObjectPrefix *)object)->position[2], 50.0f, 0.2f, direction,
                                        &direction[2], cachedNode);
    vector3Normalize(direction);
}

f32 terrainGetFriction(s32);
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
    if (!(((ItemStatePrefix *)state)->flags & ITEM_STATE_SPIN_ENABLED) || (((ItemStatePrefix *)state)->spinSpeed <= 0.0f)) {
        ((ItemStatePrefix *)state)->spinSpeed = 0.0f;
        return 0;
    }
    if (applyDecay != 0) {
        step = terrainGetFriction(surfaceAttributes);
        ((ItemStatePrefix *)state)->spinSpeed = ((ItemStatePrefix *)state)->spinSpeed - gRuntimeFrameSeconds * step;
        ((RuntimeObjectPrefix *)object)->rotation[0] = mathMapRangeClamped(0.8f, 60.0f, 4.0f, 0.0f, ((ItemStatePrefix *)state)->spinSpeed);
    }
    step = mathMapRangeClamped(0.0f, 0.0f, 3.2f, 900.0f, ((ItemStatePrefix *)state)->spinSpeed) * gRuntimeFrameSeconds;
    ((RuntimeObjectPrefix *)object)->rotation[1] += step;
    if (((RuntimeObjectPrefix *)object)->rotation[1] < 0.0f) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[1] = (f32)(((RuntimeObjectPrefix *)object)->rotation[1] + 360.0f);
        } while (((RuntimeObjectPrefix *)object)->rotation[1] < 0.0f);
    }
    if (((RuntimeObjectPrefix *)object)->rotation[1] >= 360.0f) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[1] = (f32)(((RuntimeObjectPrefix *)object)->rotation[1] - 360.0f);
        } while (((RuntimeObjectPrefix *)object)->rotation[1] >= 360.0f);
    }
    ((RuntimeObjectPrefix *)object)->rotation[2] = 0.0f;
    if (((ItemStatePrefix *)state)->spinSpeed < 4.0f)
        return 0;
    return 1;
}

f32 fabsf(f32);
#pragma intrinsic(fabsf)
f32 vector2NormalizeComponentsApprox(f32 *, f32 *);
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
        *z = 0.0f;
        *x = 0.0f;
        return;
    }
    rate *= scale * gRuntimeFrameSeconds;
    dx *= rate;
    dz *= rate;
    valueX = *x;
    if (fabsf(dx) > fabsf(valueX))
        dx = valueX;
    valueZ = *z;
    if (fabsf(dz) > fabsf(valueZ))
        dz = valueZ;
    *x -= dx;
    *z -= dz;
}


s32 itemUpdateSpin(u8 *, s32, s32);
void itemApplyHorizontalFriction(f32 *, f32 *, f32, f32, f32);
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
    if (((ItemStatePrefix *)state)->flags & 0x20) {
        *spinning = itemUpdateSpin(object, sp4C, 1);
    } else {
        *spinning = 0;
    }
    itemGetTerrainResponseDirection(object, slope, (u8 **)cachedNode);
    if ((var_v1 != 0) || (slope[0] > 0.01f) || (slope[0] < -0.01f) || (slope[2] > 0.01f) || (slope[2] < -0.01f)) {
        f32 accelerationStep;
        accelerationStep = 900.0f * gRuntimeFrameSeconds;
        ((ItemStatePrefix *)state)->velocityX =
            (f32)((slope[0] * accelerationStep) + (*(f32 *)((s8 *)state + (u32) & ((ItemStatePrefix *)0)->velocityX)));
        ((ItemStatePrefix *)state)->velocityZ =
            (f32)((slope[2] * accelerationStep) + (*(f32 *)((s8 *)state + (u32) & ((ItemStatePrefix *)0)->velocityZ)));
    }
    friction = terrainGetFriction(sp4C);
    friction = 60.0f * friction;
    var_f12 = ((ItemKind *)(gItemKindFrictionEntries - 0x30))[((ItemStatePrefix *)state)->kind].frictionCoefficient *
              friction;
    var_f14 =
        ((ItemKind *)(gItemKindFrictionEntries - 0x30))[((ItemStatePrefix *)state)->kind].frictionCoefficient * 3.0f;
    if (*spinning != 0) {
        var_f12 *= 0.8f;
        var_f14 *= 0.8f;
    }
    if (var_v1 != 0) {
        var_f12 *= 0.3f;
        var_f14 *= 0.3f;
    }
    itemApplyHorizontalFriction((f32 *)(state + 0x14), (f32 *)(state + 0x18), var_f12, var_f14, itemGetMotionCoefficient(object));
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
    if (!(itemFlags & 0x01000000)) {
        if (itemFlags & 0x10) {
            ((ItemStatePrefix *)state)->rotationVelocityX =
                (f32)((2.0f * ((ItemStatePrefix *)state)->velocityX) / gRuntimeFrameSeconds);
            ((ItemStatePrefix *)state)->rotationVelocityY =
                (f32)((2.0f * ((ItemStatePrefix *)state)->velocityZ) / gRuntimeFrameSeconds);
        } else if (itemFlags & 8) {
            sp38 = ((ItemStatePrefix *)state)->velocityX;
            sp34 = ((ItemStatePrefix *)state)->velocityZ;
            ((ItemStatePrefix *)state)->rotationVelocityX =
                (f32)((57.295776f * vector2NormalizeComponentsApprox(&sp38, &sp34)) /
                      (*(f32 *)((s8 *)object + (u32) & ((RuntimeObjectPrefix *)0)->radius)));
            if (!(((ItemStatePrefix *)state)->flags & 0x01000000)) {
                ((ItemStatePrefix *)state)->rotationVelocityY =
                    (f32)(mathApproachAngleDegrees(((RuntimeObjectPrefix *)object)->rotation[1],
                                                   mathUnitDirectionDegrees(sp34, sp38), 0.2f, 60.0f, 300.0f) -
                          ((RuntimeObjectPrefix *)object)->rotation[1]);
            }
            ((RuntimeObjectPrefix *)object)->rotation[2] = 0.0f;
        } else {
            ((ItemStatePrefix *)state)->rotationVelocityX *= 0.9f;
            ((ItemStatePrefix *)state)->rotationVelocityY *= 0.9f;
        }
    }
    ((RuntimeObjectPrefix *)object)->rotation[0] +=
        (((ItemStatePrefix *)state)->rotationVelocityX * gRuntimeFrameSeconds);
    if (((RuntimeObjectPrefix *)object)->rotation[0] < 0.0f) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[0] += 360.0f;
        } while (((RuntimeObjectPrefix *)object)->rotation[0] < 0.0f);
    }
    if (((RuntimeObjectPrefix *)object)->rotation[0] >= 360.0f) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[0] -= 360.0f;
        } while (((RuntimeObjectPrefix *)object)->rotation[0] >= 360.0f);
    }
    ((RuntimeObjectPrefix *)object)->rotation[1] +=
        (((ItemStatePrefix *)state)->rotationVelocityY * gRuntimeFrameSeconds);
    if (((RuntimeObjectPrefix *)object)->rotation[1] < 0.0f) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[1] += 360.0f;
        } while (((RuntimeObjectPrefix *)object)->rotation[1] < 0.0f);
    }
    if (((RuntimeObjectPrefix *)object)->rotation[1] >= 360.0f) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[1] -= 360.0f;
        } while (((RuntimeObjectPrefix *)object)->rotation[1] >= 360.0f);
    }
}

/* Item motion prefix: vertical speed at 0x10, horizontal components at 0x14/0x18. */

/// Adjust item velocity for a bounce or stop against the ground.
/// \param object Item whose ground response is evaluated.
/// \param state Item motion state whose velocity is updated.
/// \param cachedNode Address of a writable cached map-node pointer, initially null or valid, updated by the terrain query.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80121F64 (0x80121F64).
void itemApplyGroundBounce(u8 *object, ItemStatePrefix *state, s32 cachedNode) {
    f32 reflection;
    f32 normal[3];
    if ((state->flags & ITEM_STATE_SPIN_ENABLED) && state->downwardVelocity > 9.0f) {
        state->velocityX *= 0.2f;
        state->velocityZ *= 0.2f;
    }
    if (state->downwardVelocity < 9.0f || !(state->flags & 4)) {
        state->downwardVelocity = 0.0f;
    } else {
        itemGetTerrainResponseDirection(object, normal, (u8 **)cachedNode);
        vector3Normalize((f32 *)normal);
        reflection = (state->velocityX * normal[0] - state->downwardVelocity * normal[1]) + normal[2] * state->velocityZ;
        reflection *= -2.0f;
        state->velocityX = normal[0] * reflection + (state->velocityX);
        state->downwardVelocity = ((state->downwardVelocity) - normal[1] * reflection) * 0.7f;
        state->velocityZ = normal[2] * reflection + (state->velocityZ);
        if (-state->downwardVelocity < 9.0f)
            state->downwardVelocity = 0.0f;
    }
}

s32 terrainGetMaterialSound(s32 arg0, s32 arg1); /* extern */
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
    if (((ItemStatePrefix *)sp1C)->bounceCount > 0) {
        itemPlayTrackedSound(
            object, sp1C, *((s16 *)((u8 *)&gItemKindPrimarySoundEntries + (((ItemStatePrefix *)sp1C)->kind * 0x74))));
        return;
    }
    itemPlayTrackedSound(object, sp1C,
                         *((s16 *)((u8 *)&gItemKindSecondarySoundEntries + (((ItemStatePrefix *)sp1C)->kind * 0x74))));
}

/// Separate terrain-collision callback view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125A90 (0x80125A90).
extern u8 gItemKindCollisionCallbackEntries[];
s32 actorPostItemImpactMessage();
s32 areaDispatchPositionEvent(f32 *, s32);

/// Handle item contact and invoke its kind-specific collision response.
/// \param object Item involved in the contact.
/// \param flags Collision contact flags supplied to the response callback.
/// \pre An active actor must be available for actor-contact notifications.
/// \returns The kind-specific callback result, or one when no callback is installed.
///
/// \note NONPORTABLE: the impact-message call retains its legacy implicit-int declaration although the callee returns void; this preserves the original register-clobber contract.
/// \note Original decompilation identifier: \c func_80122184 (0x80122184).
s32 itemDispatchCollisionResponse(u8 *object, s32 flags) {
    u8 *state;
    ItemCollisionCallback callback;

    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    callback = *(ItemCollisionCallback *)(gItemKindCollisionCallbackEntries + ((ItemStatePrefix *)state)->kind * sizeof(ItemKind));
    if (flags & 0x40) {
        actorPostItemImpactMessage(((u8 *)gPikachuActor), object);
    }
    if (!(flags & 0x40) && !(((ItemStatePrefix *)state)->flags & 0x10000000)) {
        if (areaDispatchPositionEvent(((RuntimeObjectPrefix *)object)->worldPosition, 6) != 0) {
            ((ItemStatePrefix *)state)->flags |= 0x10000000;
        }
    }
    if (callback != NULL) {
        return callback(object, flags);
    }
    return 1;
}

s32 areaIsPositionOutsideBounds(f32, f32, f32);
s32 itemUpdateGroundVelocity(u8 *, u8 *, s32, s32 *);
s32 itemDispatchCollisionResponse(u8 *, s32);
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
    if (areaIsPositionOutsideBounds(((RuntimeObjectPrefix *)object)->worldPosition[0], ((RuntimeObjectPrefix *)object)->worldPosition[1], ((RuntimeObjectPrefix *)object)->worldPosition[2]) != 0) {
        itemDestroy(object);
        return;
    }
    ((ItemStatePrefix *)state)->flags &= 0xFFDFFFFF;
    if (!(((ItemStatePrefix *)state)->flags & 0x01000000)) {
        itemUpdateGroundVelocity(object, state, (s32)&node, &drag);
    } else {
        drag = itemUpdateSpin(object, 0, 0);
    }
    speed = itemGetMotionCoefficient(object);
    ((ItemStatePrefix *)state)->downwardVelocity =
        (f32)(((ItemStatePrefix *)state)->downwardVelocity + (3.2666667f * gRuntimeFrameSeconds * speed * 30.0f));
    dx = ((ItemStatePrefix *)state)->velocityX * gRuntimeFrameSeconds;
    dy = -((ItemStatePrefix *)state)->downwardVelocity * gRuntimeFrameSeconds;
    dz = ((ItemStatePrefix *)state)->velocityZ * gRuntimeFrameSeconds;
    if (((ItemStatePrefix *)state)->flags & 0x08000000) {
        flags =
            terrainConstrainMovement(7, ((RuntimeObjectPrefix *)object)->position[0], ((RuntimeObjectPrefix *)object)->position[1] - ((RuntimeObjectPrefix *)object)->radius,
                          ((RuntimeObjectPrefix *)object)->position[2], ((RuntimeObjectPrefix *)object)->radius * 2,
                          ((RuntimeObjectPrefix *)object)->radius * 2, ((RuntimeObjectPrefix *)object)->radius, &dx, &dy, &dz, &node);
    } else {
        flags =
            terrainConstrainMovement(5, ((RuntimeObjectPrefix *)object)->position[0], ((RuntimeObjectPrefix *)object)->position[1] - ((RuntimeObjectPrefix *)object)->radius,
                          ((RuntimeObjectPrefix *)object)->position[2], ((RuntimeObjectPrefix *)object)->radius * 2,
                          ((RuntimeObjectPrefix *)object)->radius * 2, ((RuntimeObjectPrefix *)object)->radius, &dx, &dy, &dz, &node);
    }
    collision = flags;
    if (collision != 0) {
        if (((flags & 0x6D) || ((flags & 0x12) && (((ItemStatePrefix *)state)->flags & 0x01000000) &&
                                (fabsf(((ItemStatePrefix *)state)->downwardVelocity) > 18.0f))) &&
            ((fabsf(((ItemStatePrefix *)state)->velocityX) > 9.0f) ||
             (fabsf(((ItemStatePrefix *)state)->downwardVelocity) > 9.0f) ||
             (fabsf(((ItemStatePrefix *)state)->velocityZ) > 9.0f))) {
            ((ItemStatePrefix *)state)->flags |= 0x200000;
        }
        if (flags & 0x10) {
            itemApplyGroundBounce(object, (ItemStatePrefix *)state, (s32)&node);
        }
        if (flags & 2) {
            ((ItemStatePrefix *)state)->downwardVelocity *= -0.7f;
        }
        if (flags & 9) {
            ((ItemStatePrefix *)state)->velocityX *= -0.7f;
        }
        if (flags & 0x24) {
            ((ItemStatePrefix *)state)->velocityZ *= -0.7f;
        }
    }
    ((RuntimeObjectPrefix *)object)->position[0] += dx;
    ((RuntimeObjectPrefix *)object)->position[1] += dy;
    ((RuntimeObjectPrefix *)object)->position[2] += dz;
    if (!(((ItemStatePrefix *)state)->flags & 0x200000) || (itemDispatchCollisionResponse(object, collision) != 0)) {
        if ((collision != 0) && !(((ItemStatePrefix *)state)->flags & 0x400) &&
            (((ItemStatePrefix *)state)->bounceCount <= 0)) {
            itemBeginShrinkingDisposal(object);
            return;
        }
        if ((dx != 0.0f) || (dy != 0.0f) || (dz != 0.0f) || (((ItemStatePrefix *)state)->flags & 0x01000000) ||
            (((ItemStatePrefix *)state)->spinSpeed > 0.0f)) {
            if (drag == 0) {
                itemUpdateRollingRotation(object, state);
            }
        } else {
            ((ItemStatePrefix *)state)->velocityZ = 0.0f;
            ((ItemStatePrefix *)state)->downwardVelocity = 0.0f;
            ((ItemStatePrefix *)state)->velocityX = 0.0f;
            ((ItemStatePrefix *)state)->rotationVelocityY = 0.0f;
            ((ItemStatePrefix *)state)->rotationVelocityX = 0.0f;
            if (((ItemStatePrefix *)state)->flags & 0x10000000) {
                ((ItemStatePrefix *)state)->flags &= 0xEFFFFFFF;
            } else {
                areaDispatchPositionEvent((f32 *)(object + 0x44), 5);
            }
            ground = terrainGetSurfaceAttributes(((RuntimeObjectPrefix *)object)->position[0], ((RuntimeObjectPrefix *)object)->position[1],
                                   ((RuntimeObjectPrefix *)object)->position[2], &node);
            if ((ground & 0x70) == 0x70) {
                ((ItemStatePrefix *)state)->velocityX = (f32)((randomNext() * 2.3283064365386963e-10 * 20.0) + 20.0);
                if ((randomNext() * 2.3283064365386963e-10) < 0.5) {
                    ((ItemStatePrefix *)state)->velocityX = -((ItemStatePrefix *)state)->velocityX;
                }
                ((ItemStatePrefix *)state)->velocityZ = (f32)((randomNext() * 2.3283064365386963e-10 * 20.0) + 20.0);
                if ((randomNext() * 2.3283064365386963e-10) < 0.5) {
                    ((ItemStatePrefix *)state)->velocityZ = -((ItemStatePrefix *)state)->velocityZ;
                }
                ((ItemStatePrefix *)state)->bounceCount = (s32)((u32)((ItemStatePrefix *)state)->bounceCount - 1U);
                ((ItemStatePrefix *)state)->downwardVelocity = -60.0f;
                if (((ItemStatePrefix *)state)->bounceCount < 0) {
                    ((ItemStatePrefix *)state)->bounceCount = 0;
                }
                if (((ground & 0xF) == 5) || ((ground & 0xF) == 7)) {
                    wispEffectCreate(9, ((RuntimeObjectPrefix *)object)->position[0], ((RuntimeObjectPrefix *)object)->position[1],
                                  ((RuntimeObjectPrefix *)object)->position[2]);
                    return;
                }
                wispEffectCreate(8, ((RuntimeObjectPrefix *)object)->position[0], ((RuntimeObjectPrefix *)object)->position[1],
                              ((RuntimeObjectPrefix *)object)->position[2]);
                return;
            }
            itemSetMode(object, ITEM_MODE_STATIONARY);
        }
        if (flags & 0x10) {
            ((ItemStatePrefix *)state)->flags &= 0xF2FFFFFF;
            return;
        }
        ((ItemStatePrefix *)state)->flags |= 0x01000000;
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
    if (((ItemStatePrefix *)state)->motionPhase.flightElapsed > 10.0f) {
        itemSetMode(object, ITEM_MODE_SHRINKING_DISPOSAL);
        return;
    }
    wave = 2.0f * mathCosDegrees(((ItemStatePrefix *)state)->motionPhase.flightElapsed * 120.0f);
    dx = (((ItemStatePrefix *)state)->velocityX + wave) * gRuntimeFrameSeconds;
    dz = (((ItemStatePrefix *)state)->velocityZ + wave) * gRuntimeFrameSeconds;
    dy = -((ItemStatePrefix *)state)->downwardVelocity * gRuntimeFrameSeconds;
    radius = ((RuntimeObjectPrefix *)object)->radius;
    diameter = radius * 2;
    if (terrainConstrainMovement(5, ((RuntimeObjectPrefix *)object)->position[0], ((RuntimeObjectPrefix *)object)->position[1] - radius, ((RuntimeObjectPrefix *)object)->position[2], diameter,
                      diameter, radius, &dx, &dy, &dz, &node) &
        0x12) {
        itemPlayTrackedSound(object, (ItemStatePrefix *)state, 0xBD);
        itemSetMode(object, ITEM_MODE_SHRINKING_DISPOSAL);
        return;
    }
    ((RuntimeObjectPrefix *)object)->position[0] += dx;
    ((RuntimeObjectPrefix *)object)->position[1] += dy;
    ((RuntimeObjectPrefix *)object)->position[2] += dz;
}

/// Separate held-item callback view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125A84 (0x80125A84).
extern u8 gItemKindHeldCallbackEntries;

/// Run the held-item behavior supplied by the item kind, when present.
/// \param object Address of the held item.
/// \param state Borrowed initialized item state identifying the callback kind.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80122A6C (0x80122A6C).
void itemRunHeldKindCallback(u8 *object, u8 *state) {
    ItemHeldCallback heldCallback;

    heldCallback = *(ItemHeldCallback *)(&gItemKindHeldCallbackEntries + ((ItemStatePrefix *)state)->kind * sizeof(ItemKind));
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

    itemRunHeldKindCallback(object, state);
    ((ItemStatePrefix *)state)->modeTime += gRuntimeFrameSeconds;
    pos = ((RuntimeObjectPrefix *)object)->position;
    pos[0] = 1.25f;
    pos[1] = -0.9f;
    pos[2] = -3.0f;
    cameraViewPointToWorld(pos);
    ((RuntimeObjectPrefix *)object)->worldPosition[0] = ((RuntimeObjectPrefix *)object)->position[0];
    ((RuntimeObjectPrefix *)object)->worldPosition[1] = ((RuntimeObjectPrefix *)object)->position[1];
    ((RuntimeObjectPrefix *)object)->worldPosition[2] = ((RuntimeObjectPrefix *)object)->position[2];
    ((RuntimeObjectPrefix *)object)->rotation[0] = gPlayerPitchOffset + gPlayerPitch;
    ((RuntimeObjectPrefix *)object)->rotation[1] = gPlayerYaw + gPlayerViewShake;
    ((RuntimeObjectPrefix *)object)->rotation[2] = 0.0f;
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
    if ((randomBelow(0x64) == 0) || (((ItemStatePrefix *)state)->rotationVelocityX < 10.0f)) {
        ((ItemStatePrefix *)state)->rotationVelocityX = (f32)(s32)(randomBelow(0x32) + 0xA);
    }
    ((ItemStatePrefix *)state)->motionPhase.wriggleAngle += ((ItemStatePrefix *)state)->rotationVelocityX * gRuntimeFrameSeconds;
    if (((ItemStatePrefix *)state)->motionPhase.wriggleAngle < 0.0f) {
        do {
            ((ItemStatePrefix *)state)->motionPhase.wriggleAngle = (f32)(((ItemStatePrefix *)state)->motionPhase.wriggleAngle + 360.0f);
        } while (((ItemStatePrefix *)state)->motionPhase.wriggleAngle < 0.0f);
    }
    if (((ItemStatePrefix *)state)->motionPhase.wriggleAngle >= 360.0f) {
        do {
            ((ItemStatePrefix *)state)->motionPhase.wriggleAngle = (f32)(((ItemStatePrefix *)state)->motionPhase.wriggleAngle - 360.0f);
        } while (((ItemStatePrefix *)state)->motionPhase.wriggleAngle >= 360.0f);
    }
    ((RuntimeObjectPrefix *)model)->rotation[1] =
        (f32)(mathSinDegrees(((ItemStatePrefix *)state)->motionPhase.wriggleAngle) * 40.0f);
    if ((randomBelow(0x64) == 0) || (((ItemStatePrefix *)state)->rotationVelocityY < 10.0f)) {
        ((ItemStatePrefix *)state)->rotationVelocityY = (f32)(s32)(randomBelow(0x64) + 0x190);
    }
    ((ItemStatePrefix *)state)->spinSpeed += ((ItemStatePrefix *)state)->rotationVelocityY * gRuntimeFrameSeconds;
    if (((ItemStatePrefix *)state)->spinSpeed < 0.0f) {
        do {
            ((ItemStatePrefix *)state)->spinSpeed = (f32)(((ItemStatePrefix *)state)->spinSpeed + 360.0f);
        } while (((ItemStatePrefix *)state)->spinSpeed < 0.0f);
    }
    if (((ItemStatePrefix *)state)->spinSpeed >= 360.0f) {
        do {
            ((ItemStatePrefix *)state)->spinSpeed = (f32)(((ItemStatePrefix *)state)->spinSpeed - 360.0f);
        } while (((ItemStatePrefix *)state)->spinSpeed >= 360.0f);
    }
    angle = mathSinDegrees(((ItemStatePrefix *)state)->spinSpeed) * 15.0f;

    /* clang-format off */
    index = 0; do {
        /* clang-format on */
        model = ((RuntimeObjectPrefix *)model)->child.bytes;
        ((RuntimeObjectPrefix *)model)->rotation[1] = angle;
        if (!(index & 1)) {
            angle = -angle * 0.9f;
        }
        index += 1;
    } while (index != 3);
}

void pinataUpdateWriggle(u8 *);
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
    itemRunHeldKindCallback(object, state);
    ((ItemStatePrefix *)state)->modeTime += gRuntimeFrameSeconds;
    parentState = ((RuntimeObjectPrefix *)parent)->state.bytes;
    actorGetConfiguredAttachmentPosition(parent, ((RuntimeObjectPrefix *)object)->radius, ((RuntimeObjectPrefix *)object)->position);
    ((RuntimeObjectPrefix *)object)->worldPosition[0] = ((RuntimeObjectPrefix *)object)->position[0];
    ((RuntimeObjectPrefix *)object)->worldPosition[1] = ((RuntimeObjectPrefix *)object)->position[1];
    ((RuntimeObjectPrefix *)object)->worldPosition[2] = ((RuntimeObjectPrefix *)object)->position[2];
    ((RuntimeObjectPrefix *)object)->rotation[0] = ((ActorScriptState *)parentState)->orientation[0];
    ((RuntimeObjectPrefix *)object)->rotation[1] = ((ActorScriptState *)parentState)->orientation[1];
    ((RuntimeObjectPrefix *)object)->rotation[2] = ((ActorScriptState *)parentState)->orientation[2];
    kind = ((ItemStatePrefix *)state)->kind;
    if ((kind == 0x4E || kind == 0x4F) && ((RuntimeObjectPrefix *)object)->scale[0] < 0.049999997f)
        pinataUpdateWriggle(object);
}

/// Animate an item shrinking away and remove it when finished.
/// \param object Item being disposed of.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80122ED8 (0x80122ED8).
void itemUpdateShrinkingDisposal(u8 *object) {
    RuntimeObjectPrefix *transform = (RuntimeObjectPrefix *)object;
    u8 *state;
    f32 value;
    transform->scale[0] -= gRuntimeFrameSeconds * 0.01f;
    state = *((u8 **)(object + 0x54));
    value = transform->scale[0];
    if (value < 0.0f) {
        transform->scale[2] = 0.0f;
        transform->scale[1] = 0.0f;
        transform->scale[0] = 0.0f;
        itemPlayTrackedSound(object, (ItemStatePrefix *)state, 0xBD);
        itemDestroy(object);
        return;
    }
    transform->scale[2] = (transform->scale[1] = transform->scale[0]);
    transform->rotation[2] = 0.0f;
    transform->rotation[1] = 0.0f;
    transform->rotation[0] += gRuntimeFrameSeconds * 400.0f;
    while (transform->rotation[0] < 0.0f) {
        transform->rotation[0] += 360.0f;
    }

    while (transform->rotation[0] >= 360.0f) {
        transform->rotation[0] -= 360.0f;
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
    if ((((ItemStatePrefix *)objectState)->state == 2) &&
        (((ActorScriptState *)((RuntimeObjectPrefix *)((u8 *)gPikachuActor))->state.pointer)->attachmentMode == 3)) {
        itemSetScale(object, 0.0f);
        return;
    }
    itemSetScale(object, 1.0f);
}
