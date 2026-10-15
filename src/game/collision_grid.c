#include "random_scale.h"
#include "vector_metrics.h"
#include "random.h"
#include "runtime_control.h"
#include "keyed_pool.h"
#include "object_types.h"
#include "collision_registration.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "object_response.h"
#include "vector_direction.h"
#include "runtime_clock.h"
#include "text_window_images.h"
#include "text_image_create.h"
#include "keyed_pool_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on



/// Request a collision impulse for a target object.
/// \param source Object responsible for the collision.
/// \param target Object receiving the impulse request.
/// \param direction Displacement from source to target; may be modified, and must remain usable as a direction when distance is near zero.
/// \param distance Nonnegative distance between the objects.
/// \returns 0 when impulse-record capacity is insufficient, or 1 otherwise, including when no impulse is queued.
/// \note Original decompilation identifier: \c func_8002C8B0 (0x8002C8B0).
s32 collisionRequestImpulse(u8 *source, u8 *target, f32 *direction, f32 distance) {
    ObjectImpulse impulse;
    f32 randomDirection[3];
    f32 strength;
    f32 maximum = 10.0F;

    if (objectResponsesGetFreeCapacity() < 4) {
        return 0;
    }
    /* Original behavior: the fallback is stored locally, but direction remains the input pointer. */
    if (distance < 0.001F) {
        randomDirection[0] = (randomNext() * RANDOM_U32_UNIT_SCALE - 0.5) * 1.2F;
        randomDirection[1] = 0.5F;
        randomDirection[2] = (randomNext() * RANDOM_U32_UNIT_SCALE - 0.5) * 1.2F;
        strength = maximum;
    } else {
        strength = 1.0F / distance;
        direction[0] = (direction[0] * strength) + ((randomNext() * RANDOM_U32_UNIT_SCALE - 0.5) * 0.001F);
        direction[1] = (direction[1] * strength) + ((randomNext() * RANDOM_U32_UNIT_SCALE - 0.5) * 0.001F);
        direction[2] = (direction[2] * strength) + ((randomNext() * RANDOM_U32_UNIT_SCALE - 0.5) * 0.001F);
        strength *= maximum;
    }
    if (strength < 0.001F) {
        return 1;
    }
    vector3PackNormalBytes(direction, impulse.bytes);
    if (strength < maximum) {
        impulse.bytes[3] = (s8)(s32)strength;
    } else {
        impulse.bytes[3] = (s8)(s32)maximum;
    }
        objectResponseQueueImpulse(source, target, impulse);

    return 1;
}

/// Provide the inactive collision hook for this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CB98 (0x8002CB98).
void collisionInactiveHook(void) {
}

#include "collision_diagnostics.h"
/// Provide the inactive collision-grid diagnostic entry point for this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CBA0 (0x8002CBA0).
void collisionGridDebugReportNoOp(void) {
    TRACE("-------- collisionTbl Info --------\n");
}

/// Owned keyed pool grouping registered objects for collision candidate queries.
/// \note Original identifier: \c D_800F1B20 (0x800F1B20).
extern KeyedPool gCollisionCandidateRegistry;
/// X-axis alignment offset used by collision spatial hashing.
/// \note Original identifier: \c D_800F1B40 (0x800F1B40).
extern f32 gCollisionGridAlignmentX;
/// Z-axis alignment offset used by collision spatial hashing.
/// \note Original identifier: \c D_800F1B44 (0x800F1B44).
extern f32 gCollisionGridAlignmentZ;

/// Register an eligible object for collision candidate queries.
/// \param object Object with collision size, flags, and position to register.
/// \returns A nonzero registration key, or zero when ineligible or registration fails.
/// \note Original decompilation identifier: \c func_8002CBA8 (0x8002CBA8).
s32 collisionGridRegisterObject(u8 *object) {
    s32 key;
    s32 h;

    if ((((RuntimeObjectPrefix *)object)->flags & 0xF0) || ((RuntimeObjectPrefix *)object)->radius < 0.001F) {
        return 0;
    }
    h = ((s32)((((RuntimeObjectPrefix *)object)->worldPosition[0] + gCollisionGridAlignmentX) * 0.125F) * 0xAD) +
        ((s32)((((RuntimeObjectPrefix *)object)->worldPosition[2] + gCollisionGridAlignmentZ) * 0.125F) * 0x8B) +
        0x1E240;
    if (h == 0) {
        key = 1;
    } else {
        key = h;
    }
    if (keyedPoolInsert(&gCollisionCandidateRegistry, (s32)object, key) != 0) {
        return key;
    }
    return 0;
}


/// Remove an object collision registration and clear its registration key.
/// \param object Address of the registered object.
/// \param registrationKey Stored registration key, cleared by this operation; zero means no registration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CCB4 (0x8002CCB4).
void collisionGridUnregisterObject(u8 *object, s32 *registrationKey) {
    s32 registeredKey;

    registeredKey = *registrationKey;
    if (registeredKey != 0) {
        keyedPoolRemoveKeyAndValue(&gCollisionCandidateRegistry, (s32)object, registeredKey);
        *registrationKey = 0;
    }
}

/// X-axis alignment offset used by collision spatial hashing.
/// \note Original identifier: \c D_800F1B40 (0x800F1B40).
/// Z-axis alignment offset used by collision spatial hashing.
/// \note Original identifier: \c D_800F1B44 (0x800F1B44).
extern f32 gCollisionGridAlignmentX, gCollisionGridAlignmentZ;

/* Advance a spatial-hash bucket iterator to the next entry with the same key. */
#define HASH_NEXT(var, table, key)                                                                                     \
    do {                                                                                                               \
        var = (u8 *)keyedPoolIterateNextForKey(table, key);                                                            \
    } while (0)

/// Find an overlapping collision candidate and request applicable object responses.
/// \param object Object whose collision size and position are queried.
/// \returns The last overlapping candidate visited in the current collision
/// group, or NULL when that group contains no eligible overlap.
/// \note The checked query-radius member offset preserves the matching target compiler representation.
/// \note Original decompilation identifier: \c func_8002CCF8 (0x8002CCF8).
u8 *collisionGridFindOverlap(u8 *object) {
    f32 distance;
    s32 hash;
    s32 key;
    f32 delta[3];
    u8 *candidate;
    u8 *hit;

    if (*(f32 *)(object + (u32) & ((RuntimeObjectPrefix *)0)->radius) < 0.001F) {
        return NULL;
    }
    hash = ((s32)((((RuntimeObjectPrefix *)object)->worldPosition[0] + gCollisionGridAlignmentX) * 0.125F) * 0xAD) +
           ((s32)((((RuntimeObjectPrefix *)object)->worldPosition[2] + gCollisionGridAlignmentZ) * 0.125F) * 0x8B) +
           0x1E240;
    if (hash == 0) {
        key = 1;
    } else {
        key = hash;
    }
    hit = NULL;
    candidate = (u8 *)keyedPoolIterateFirstForKey(&gCollisionCandidateRegistry, key);
    if (candidate != NULL) {
        do {
            if (candidate != object && !(((RuntimeObjectPrefix *)candidate)->flags & 0xC0)) {
                delta[0] = ((RuntimeObjectPrefix *)candidate)->worldPosition[0] -
                           ((RuntimeObjectPrefix *)object)->worldPosition[0];
                delta[1] = ((RuntimeObjectPrefix *)candidate)->worldPosition[1] -
                           ((RuntimeObjectPrefix *)object)->worldPosition[1];
                delta[2] = ((RuntimeObjectPrefix *)candidate)->worldPosition[2] -
                           ((RuntimeObjectPrefix *)object)->worldPosition[2];
                distance = vector3Length(delta);
                if (!(((RuntimeObjectPrefix *)candidate)->radius +
                          *(f32 *)(object + (u32) & ((RuntimeObjectPrefix *)0)->radius) <
                      distance)) {
                    hit = candidate;
                    switch (((RuntimeObjectPrefix *)candidate)->kind) {
                    case 0x86:
                        if (collisionRequestImpulse(object, candidate, delta, distance) == 0) {
                            goto done;
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
            HASH_NEXT(candidate, &gCollisionCandidateRegistry, key);
        } while (candidate != NULL);
    }
done:
    return hit;
}


/// Randomize the alignment used to group collision candidates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CED0 (0x8002CED0).
void collisionGridRandomizeAlignment(void) {
    gCollisionGridAlignmentX = (0.5 - randomNext() * RANDOM_U32_UNIT_SCALE) * 8.0;
    gCollisionGridAlignmentZ = (0.5 - randomNext() * RANDOM_U32_UNIT_SCALE) * 8.0;
}


/// Initialize the collision candidate registry.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CFA8 (0x8002CFA8).
void collisionGridInitialize(void) {
    keyedPoolInitialize(&gCollisionCandidateRegistry, 0x1F3);
    gCollisionGridAlignmentX = 0.0F;
    gCollisionGridAlignmentZ = 0.0F;
}


/// Release the collision candidate registry.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002CFE0 (0x8002CFE0).
void collisionGridDestroy(void) {
    keyedPoolDestroy(&gCollisionCandidateRegistry);
}
