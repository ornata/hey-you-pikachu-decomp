#include "game_heap.h"
#include "math_angles.h"
#include "random.h"
#include "terrain_height.h"
#include "object_hierarchy_create.h"
#include "object_lifecycle.h"
#include "common.h"
#include "object_lookup.h"
#include "heap_alloc.h"
#include "object_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

void _bzero(void *, s32); /* extern */

/// Owned orbit parameters shared by initialization and the follower update.
typedef struct OrbitState {
    s32 ownerId;       ///< Borrowed owner object handle; a missing owner ends the follower lifetime.
    f32 timer;         ///< Seconds remaining before the next orbit reset; obstruction shortens this interval.
    f32 heading;       ///< Desired orbit angle around the owner in degrees.
    f32 radius;        ///< Horizontal clearance radius supplied to terrain queries.
    f32 height;        ///< Follower height above terrain and owner reset position.
    f32 ceiling;       ///< Clearance-query upper extent above the ground reference.
    f32 turnRate;      ///< Signed angular rate of the desired orbit heading in degrees per second.
    s32 collisionKind; ///< Terrain-query policy selected for the owner kind.
} OrbitState;
typedef char OrbitStateSizeCheck[(sizeof(OrbitState) == 0x20) ? 1 : -1];
typedef char OrbitRadiusOffsetCheck[((u32) & ((OrbitState *)0)->radius == 0xC) ? 1 : -1];
typedef char OrbitCollisionPolicyOffsetCheck[((u32) & ((OrbitState *)0)->collisionKind == 0x1C) ? 1 : -1];

/// Initialize the movement state for an object that circles its owner.
/// \param object Orbiting follower object to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80026DF0 (0x80026DF0).
void orbitFollowerInitialize(u8 *object) {
    OrbitState *state;
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x20, 0x3C);
    state = (OrbitState *)allocation;
    _bzero(allocation, 0x20);
    state->timer = 10.0F;
    state->turnRate = 45.0F;
    ((RuntimeObjectPrefix *)object)->state.bytes = (u8 *)state;
}




#include "orbit_follower.h"
#include "vector_operations.h"
/// Check horizontal clearance at a candidate world position under the requested collision policy.
/// \param type Collision policy from zero through nine; policy five also requires the active primary actor.
/// \param x Candidate world X coordinate.
/// \param y Candidate world Y coordinate.
/// \param z Candidate world Z coordinate.
/// \param width Horizontal clearance width.
/// \param height Vertical span used for clearance checks.
/// \param step Maximum permitted terrain world height.
/// \param dirX X movement sign: minus one, zero, or one.
/// \param dirZ Z movement sign: minus one, zero, or one.
/// \param previous Optional writable terrain-node cache; initialize to null or a live node in the current
/// area. Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns Directional obstruction flags, or zero when the tested position is clear.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
/// \note Original decompilation identifier: \c func_8010C788 (0x8010C788).
/// \note Matching: retain the signed-word return view in this caller. The canonical u16 return changes
/// allocated instructions even though this call discards its result.
s32 terrainCheckHorizontalObstruction(s32, f32, f32, f32, f32, f32, f32, f32, f32, u8 **);
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Move an orbiting follower around its owner while respecting scene obstacles and ground height.
/// \param obj Orbiting follower object to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80026E54 (0x80026E54).
void orbitFollowerUpdate(u8 *obj) {
    OrbitState *state = (OrbitState *)((RuntimeObjectPrefix *)obj)->state.pointer;
    RuntimeObjectPrefix *owner;
    f32 targetX;
    f32 targetZ;
    s32 signX;
    f32 speed = 75.0F;
    f32 dx;
    f32 dy = 0.0F;
    f32 dz;
    u16 flags;
    s32 collision;
    u8 *surface = NULL;
    f32 orbitRadius = 30.0F;
    f32 resetDuration = 10.0F;
    f32 collisionPenalty = 1.5F;

    owner = (RuntimeObjectPrefix *)objectFindLiveByHandle(state->ownerId);
    if (owner == NULL) {
        objectRequestHierarchyDestruction((u8 *)obj);
        return;
    }
    state->timer -= gRuntimeFrameSeconds;
    if (state->timer < 0.0F) {
        state->timer = resetDuration;
        state->turnRate = -state->turnRate;
        state->heading = (s32)randomBelow(360);
        ((RuntimeObjectPrefix *)obj)->position[0] = owner->position[0];
        ((RuntimeObjectPrefix *)obj)->position[1] = owner->position[1] + state->height;
        ((RuntimeObjectPrefix *)obj)->position[2] = owner->position[2];
        ((RuntimeObjectPrefix *)obj)->worldPosition[0] = ((RuntimeObjectPrefix *)obj)->position[0];
        ((RuntimeObjectPrefix *)obj)->worldPosition[1] = ((RuntimeObjectPrefix *)obj)->position[1];
        ((RuntimeObjectPrefix *)obj)->worldPosition[2] = ((RuntimeObjectPrefix *)obj)->position[2];
        return;
    }
    dx = mathSinDegrees(((RuntimeObjectPrefix *)obj)->rotation[1]) * speed * gRuntimeFrameSeconds;
    dz = mathCosDegrees(((RuntimeObjectPrefix *)obj)->rotation[1]) * speed * gRuntimeFrameSeconds;
    signX = dx > 0.0F ? 1 : dx < 0.0F ? -1 : 0;
    collision = terrainCheckHorizontalObstruction(
        state->collisionKind, ((RuntimeObjectPrefix *)obj)->worldPosition[0] + dx,
        ((RuntimeObjectPrefix *)obj)->worldPosition[1] + dy - state->height,
        ((RuntimeObjectPrefix *)obj)->worldPosition[2] + dz, state->radius, state->height,
        ((RuntimeObjectPrefix *)obj)->worldPosition[1] - state->height + state->ceiling, (f32)signX,
        (f32)(dz > 0.0F   ? 1
              : dz < 0.0F ? -1
                          : 0),
        &surface);
    flags = collision;
    if (collision & 9) {
        dx = 0.0F;
    } else {
        ((RuntimeObjectPrefix *)obj)->position[0] += dx;
    }
    if (flags & 0x24) {
        dz = 0.0F;
    } else {
        ((RuntimeObjectPrefix *)obj)->position[2] += dz;
    }
    ((RuntimeObjectPrefix *)obj)->position[1] =
        terrainGetWorldHeightForQuery(3, ((RuntimeObjectPrefix *)obj)->position[0],
                                      ((RuntimeObjectPrefix *)obj)->position[1],
                                      ((RuntimeObjectPrefix *)obj)->position[2], &surface) +
        state->height;
    if (flags != 0) {
        state->timer -= collisionPenalty;
        if (dx < 0.001F && dx > -0.001F && dz < 0.001F && dz > -0.001F) {
            state->heading += state->turnRate;
        }
    }
    state->heading += state->turnRate * gRuntimeFrameSeconds;
    while (state->heading < 0.0F) {
        state->heading += 360.0F;
    }
    while (state->heading >= 360.0F) {
        state->heading -= 360.0F;
    }
    targetX = owner->worldPosition[0] + (mathCosDegrees(state->heading) * orbitRadius) -
              ((RuntimeObjectPrefix *)obj)->worldPosition[0];
    targetZ = owner->worldPosition[2] + (mathSinDegrees(state->heading) * orbitRadius) -
              ((RuntimeObjectPrefix *)obj)->worldPosition[2];
    vector2NormalizeComponentsApprox(&targetX, &targetZ);
    ((RuntimeObjectPrefix *)obj)->rotation[1] = mathUnitDirectionDegrees(targetZ, targetX);
}

/// Hierarchy template used to create objects that orbit a creature owner.
/// \note Original identifier: \c D_800999B0 (0x800999B0).
extern u8 gOrbitFollowerObjectTemplate[];

/// Create an orbiting follower for a creature.
/// \param owner Creature object to follow.
/// \param radius Horizontal collision radius of the follower.
/// \param height Follower height above the ground and owner spawn position.
/// \param ceiling Upper extent used for scene collision checks.
/// \returns The created follower object.
/// \note Original decompilation identifier: \c func_8002728C (0x8002728C).
u8 *orbitFollowerCreate(u8 *owner, f32 radius, f32 height, f32 ceiling) {
    u8 *obj;
    u8 *src = owner;
    u8 *d;

    obj = objectCreateHierarchyFromTemplate((ObjectTemplate *)gOrbitFollowerObjectTemplate);
    d = ((RuntimeObjectPrefix *)obj)->state.bytes;
    ((OrbitState *)d)->ownerId = ((RuntimeObjectPrefix *)src)->handle;
    ((OrbitState *)d)->radius = radius;
    ((OrbitState *)d)->height = height;
    ((OrbitState *)d)->ceiling = ceiling;
    if (((RuntimeObjectPrefix *)src)->kind == 0x25) {
        TRACE("actor type : ACTOR_PIKACHU\n");
        ((OrbitState *)d)->collisionKind = 3;
    } else {
        TRACE("actor type : ACTOR_POKEMON\n");
        ((OrbitState *)d)->collisionKind = 4;
    }
    ((RuntimeObjectPrefix *)obj)->position[0] = ((RuntimeObjectPrefix *)src)->position[0];
    ((RuntimeObjectPrefix *)obj)->position[1] = ((OrbitState *)d)->height + ((RuntimeObjectPrefix *)src)->position[1];
    ((RuntimeObjectPrefix *)obj)->position[2] = ((RuntimeObjectPrefix *)src)->position[2];
    ((RuntimeObjectPrefix *)obj)->worldPosition[0] = ((RuntimeObjectPrefix *)obj)->position[0];
    ((RuntimeObjectPrefix *)obj)->worldPosition[1] = ((RuntimeObjectPrefix *)obj)->position[1];
    ((RuntimeObjectPrefix *)obj)->worldPosition[2] = ((RuntimeObjectPrefix *)obj)->position[2];
    ((RuntimeObjectPrefix *)obj)->rotation[1] = (s32)randomBelow(360);
    objectRegisterActiveHierarchy(obj);
    return obj;
}
