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
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

/// Owned orbit parameters shared by initialization and the follower update.
typedef struct OrbitState {
    s32 ownerId;
    f32 timer;
    f32 heading;
    f32 radius;
    f32 height;
    f32 ceiling;
    f32 turnRate;
    s32 collisionKind;
} OrbitState;
typedef char OrbitStateSizeCheck[(sizeof(OrbitState) == 0x20) ? 1 : -1];

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
    state->timer = 10.0f;
    state->turnRate = 45.0f;
    ((RuntimeObjectPrefix *)object)->state.bytes = (u8 *)state;
}



typedef struct OrbitObject {
    f32 scale[3];
    f32 rotation[3];
    f32 position[3];
    u8 unknown24[0x20];
    f32 world[3];
    struct OrbitObject *parent;
    OrbitState *state;
} OrbitObject;

f32 vector2NormalizeComponentsApprox(f32 *, f32 *);
u32 randomBelow(s32);
f32 mathUnitDirectionDegrees(f32, f32);
f32 mathSinDegrees(f32);
f32 mathCosDegrees(f32);
f32 terrainGetWorldHeightForQuery(s32, f32, f32, f32, u8 **);
s32 terrainCheckHorizontalObstruction(s32, f32, f32, f32, f32, f32, f32, f32, f32, u8 **);
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Move an orbiting follower around its owner while respecting scene obstacles and ground height.
/// \param obj Orbiting follower object to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80026E54 (0x80026E54).
void orbitFollowerUpdate(OrbitObject *obj) {
    OrbitState *state = obj->state;
    OrbitObject *owner;
    f32 targetX;
    f32 targetZ;
    s32 signX;
    f32 speed = 75.0f;
    f32 dx;
    f32 dy = 0.0f;
    f32 dz;
    u16 flags;
    s32 collision;
    u8 *surface = NULL;
    f32 orbitRadius = 30.0f;
    f32 resetDuration = 10.0f;
    f32 collisionPenalty = 1.5f;

    owner = (OrbitObject *)objectFindLiveByHandle(state->ownerId);
    if (owner == NULL) {
        objectRequestHierarchyDestruction((u8 *)obj);
        return;
    }
    state->timer -= gRuntimeFrameSeconds;
    if (state->timer < 0.0f) {
        state->timer = resetDuration;
        state->turnRate = -state->turnRate;
        state->heading = (s32)randomBelow(360);
        obj->position[0] = owner->position[0];
        obj->position[1] = owner->position[1] + state->height;
        obj->position[2] = owner->position[2];
        obj->world[0] = obj->position[0];
        obj->world[1] = obj->position[1];
        obj->world[2] = obj->position[2];
        return;
    }
    dx = mathSinDegrees(obj->rotation[1]) * speed * gRuntimeFrameSeconds;
    dz = mathCosDegrees(obj->rotation[1]) * speed * gRuntimeFrameSeconds;
    signX = dx > 0.0f ? 1 : dx < 0.0f ? -1 : 0;
    collision =
        terrainCheckHorizontalObstruction(state->collisionKind, obj->world[0] + dx, obj->world[1] + dy - state->height, obj->world[2] + dz,
                      state->radius, state->height, obj->world[1] - state->height + state->ceiling, (f32)signX,
                      (f32)(dz > 0.0f   ? 1
                            : dz < 0.0f ? -1
                                        : 0),
                      &surface);
    flags = collision;
    if (collision & 9) {
        dx = 0.0f;
    } else {
        obj->position[0] += dx;
    }
    if (flags & 0x24) {
        dz = 0.0f;
    } else {
        obj->position[2] += dz;
    }
    obj->position[1] = terrainGetWorldHeightForQuery(3, obj->position[0], obj->position[1], obj->position[2], &surface) + state->height;
    if (flags != 0) {
        state->timer -= collisionPenalty;
        if (dx < 0.001f && dx > -0.001f && dz < 0.001f && dz > -0.001f) {
            state->heading += state->turnRate;
        }
    }
    state->heading += state->turnRate * gRuntimeFrameSeconds;
    while (state->heading < 0.0f) {
        state->heading += 360.0f;
    }
    while (state->heading >= 360.0f) {
        state->heading -= 360.0f;
    }
    targetX = owner->world[0] + mathCosDegrees(state->heading) * orbitRadius - obj->world[0];
    targetZ = owner->world[2] + mathSinDegrees(state->heading) * orbitRadius - obj->world[2];
    vector2NormalizeComponentsApprox(&targetX, &targetZ);
    obj->rotation[1] = mathUnitDirectionDegrees(targetZ, targetX);
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
    d = *(u8 **)(obj + 0x54);
    *(s32 *)(d + 0) = *(s32 *)(src + 0x38);
    *(f32 *)(d + 0xC) = radius;
    *(f32 *)(d + 0x10) = height;
    *(f32 *)(d + 0x14) = ceiling;
    if (*(u16 *)(src + 0x34) == 0x25) {
        TRACE("actor type : ACTOR_PIKACHU\n");
        *(s32 *)(d + 0x1C) = 3;
    } else {
        TRACE("actor type : ACTOR_POKEMON\n");
        *(s32 *)(d + 0x1C) = 4;
    }
    *(f32 *)(obj + 0x18) = *(f32 *)(src + 0x18);
    *(f32 *)(obj + 0x1C) = *(f32 *)(d + 0x10) + *(f32 *)(src + 0x1C);
    *(f32 *)(obj + 0x20) = *(f32 *)(src + 0x20);
    *(f32 *)(obj + 0x44) = *(f32 *)(obj + 0x18);
    *(f32 *)(obj + 0x48) = *(f32 *)(obj + 0x1C);
    *(f32 *)(obj + 0x4C) = *(f32 *)(obj + 0x20);
    *(f32 *)(obj + 0x10) = (s32)randomBelow(360);
    objectRegisterActiveHierarchy(obj);
    return obj;
}
