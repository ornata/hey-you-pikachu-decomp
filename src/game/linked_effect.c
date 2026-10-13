#include "opacity_animation_programs.h"
#include "texture_animation_programs.h"
#include "actor_meters.h"
#include "actor_sound.h"
#include "object_matrix_buffers.h"
#include "object_movement.h"
#include "object_create.h"
#include "object_lifecycle.h"
#include "heap_free.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "object_audio.h"
#include "integer_animation.h"
#include "object_lookup.h"
#include "heap_alloc.h"
#include "game_heap.h"
#include "object_types.h"

/// Owned motion, neighbor constraints and animated appearance of one linked effect.
typedef struct MotionState {
    f32 elapsed;
    u8 *previous;
    u8 *next;
    f32 spacing;
    f32 previousPull;
    f32 nextPull;
    f32 damping;
    f32 velocity[3];
    s32 (*callback)(u8 *, f32 *);
    s32 target;
    f32 speed;
    f32 rate;
    f32 channelA[3];
    f32 channelB[3];
    u8 color[4];
    s32 head;
} MotionState;

typedef char LinkedEffectMotionStateSizeCheck[(sizeof(MotionState) == 0x58) ? 1 : -1];

void _bzero(void *, s32); /* extern */
void textureLoadForDraw(Gfx **, s32, s16);
/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;

/// Allocate and initialize motion state for a linked effect.
/// \param object Effect object receiving its motion state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C860 (0x8001C860).
void linkedEffectInitialize(u8 *object) {
    MotionState *state;
    MotionState *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x58, 0x38);
    state = allocation;
    _bzero(allocation, 0x58);
    ((RuntimeObjectPrefix *)object)->state.pointer = state;
}

/// Detach an effect from its neighbors and release its motion state.
/// \param object Linked effect object with initialized motion state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C8AC (0x8001C8AC).
void linkedEffectDestroy(u8 *object) {
    u8 *effectState;
    u8 *new_var;
    u8 *previousEffect;
    u8 *nextEffect;
    u8 *new_var2;
    effectState = *(u8 **)(object + 0x54);
    previousEffect = *(u8 **)(effectState + 4);
    nextEffect = *(u8 **)(effectState + 8);
    if (previousEffect != 0) {
        new_var = *(u8 **)(previousEffect + 0x54);
        *(u8 **)(new_var + 8) = nextEffect;
    }
    if (nextEffect != 0) {
        *(u8 **)((new_var2 = *(u8 **)(nextEffect + 0x54)) + 4) = previousEffect;
    }
    heapFreeLocal(&gGameHeap, effectState);
}

f32 vector3Normalize(f32 *);
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Add motion toward a neighbor when the allowed separation is exceeded.
/// \param object Effect whose motion is being adjusted.
/// \param neighbor Neighbor toward which the effect is pulled.
/// \param spacing Maximum separation that requires no corrective motion.
/// \param pull Strength of the separation correction.
/// \param velocity Writable three-component velocity whose existing values are augmented.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C8FC (0x8001C8FC).
void linkedEffectAddNeighborPull(u8 *object, u8 *neighbor, f32 spacing, f32 pull, f32 *velocity) {
    f32 d[3];
    f32 len;
    f32 k;
    s32 i;

    d[0] = *(f32 *)(neighbor + 0x18) - *(f32 *)(object + 0x18);
    d[1] = *(f32 *)(neighbor + 0x1C) - *(f32 *)(object + 0x1C);
    d[2] = *(f32 *)(neighbor + 0x20) - *(f32 *)(object + 0x20);
    len = vector3Normalize(d);
    k = len - spacing;
    if (len <= spacing) {
        return;
    }
    k *= pull / gRuntimeFrameSeconds;
    for (i = 0; i < 3; i++) {
        velocity[i] += d[i] * k;
    }
}

/// Advance a linked effect according to its motion behavior and neighbor constraints.
/// \param object Linked effect object with initialized motion state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C9D8 (0x8001C9D8).
void linkedEffectUpdateMotion(u8 *object) {
    s32 axis;
    f32 velocity[3];
    u8 *state;
    s32 i;
    u8 *neighbor;
    u8 *next;
    s32 (*callback)(u8 *, f32 *);

    state = *(u8 **)(object + 0x54);
    velocity[2] = 0.0f;
    velocity[1] = 0.0f;
    velocity[0] = 0.0f;
    callback = *(s32(**)(u8 *, f32 *))(state + 0x28);
    if (callback == NULL || callback(object, velocity) != 1) {
        for (i = 0; i < 3; i++) {
            velocity[i] += ((f32 *)(state + 0x1C))[i] * *(f32 *)(state + 0x18);
        }
        neighbor = *(u8 **)(state + 4);
        if (neighbor != NULL) {
            linkedEffectAddNeighborPull(object, neighbor, *(f32 *)(*(u8 **)(neighbor + 0x54) + 0xC),
                                        *(f32 *)(state + 0x10), velocity);
        }
        next = *(u8 **)(state + 8);
        if (next != NULL) {
            linkedEffectAddNeighborPull(object, next, *(f32 *)(state + 0xC), *(f32 *)(state + 0x14), velocity);
        }
    }
    *(f32 *)(state + 0x1C) = velocity[0];
    *(f32 *)(state + 0x20) = velocity[1];
    *(f32 *)(state + 0x24) = velocity[2];
    for (axis = 0; axis < 3; axis++) {
        ((f32 *)(object + 0x18))[axis] += velocity[axis] * gRuntimeFrameSeconds;
    }
}

void linkedEffectUpdateMotion(u8 *);                      /* extern */
f32 mathSinDegrees(f32);                                  /* extern */

/// Advance a linked effect's motion and appearance.
/// \param object Linked effect object with initialized motion state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001CB54 (0x8001CB54).
void linkedEffectUpdate(u8 *object) {
    u8 *state;
    s32 channelValue;
    f32 step;

    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    linkedEffectUpdateMotion(object);
    objectOrientAlongCameraView(object);
    ((RuntimeObjectPrefix *)object)->rotation[0] =
        (f32)(mathSinDegrees(((MotionState *)state)->elapsed * 540.0f) * 45.0f);
    if (((RuntimeObjectPrefix *)object)->handle & 1) {
        ((RuntimeObjectPrefix *)object)->rotation[0] = (f32)-((RuntimeObjectPrefix *)object)->rotation[0];
    }
    if (((RuntimeObjectPrefix *)object)->rotation[0] < 0.0f) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[0] = (f32)(((RuntimeObjectPrefix *)object)->rotation[0] + 360.0f);
        } while (((RuntimeObjectPrefix *)object)->rotation[0] < 0.0f);
    }
    if (((RuntimeObjectPrefix *)object)->rotation[0] >= 360.0f) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[0] = (f32)(((RuntimeObjectPrefix *)object)->rotation[0] - 360.0f);
        } while (((RuntimeObjectPrefix *)object)->rotation[0] >= 360.0f);
    }
    step = ((MotionState *)state)->rate * gRuntimeFrameSeconds;
    animationIntegerTrackEvaluate((ValueTrack *)(f32 *)(state + 0x38), &channelValue, step);
    animationIntegerTrackEvaluate((ValueTrack *)(f32 *)(state + 0x44), &channelValue, step);
    ((MotionState *)state)->color[3] = (s8)channelValue;
    ((MotionState *)state)->elapsed = (f32)(((MotionState *)state)->elapsed + gRuntimeFrameSeconds);
}

typedef struct Rgba8 {
    u8 r, g, b, a;
} Rgba8;

/// Append drawing setup for a linked effect using its animated appearance.
/// \param object Linked effect object with initialized motion state.
/// \returns The borrowed effect model display-list address.
/// \note Original decompilation identifier: \c func_8001CC9C (0x8001CC9C).
s32 linkedEffectPrepareDraw(u8 *object) {
    u8 *v = *(u8 **)(object + 0x54);
    Rgba8 *c;

    textureLoadForDraw(&gRenderDisplayListCursor, 5, *(s16 *)(v + 0x40));
    c = (Rgba8 *)(v + 0x50);
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, c->r, c->g, c->b, c->a);
    return *(s32 *)(object + 0x30);
}

/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern u8 *gPikachuActor;

/// Guide an effect toward its target and transfer arrival handling to the next linked effect.
/// If the target is unavailable, let the effect rise until its group is retired.
/// \param object Linked effect object with initialized motion state.
/// \param velocity Three-component output velocity for the effect.
/// \returns One when the output fully controls motion; two when normal damping and neighbor constraints also apply.
/// \note Original decompilation identifier: \c func_8001CD24 (0x8001CD24).
s32 linkedEffectComputeTargetVelocity(u8 *object, f32 *velocity) {
    u8 *state;
    f32 distance;
    f32 maxStep;
    u8 *next;
    f32 step;
    u16 flags;
    u8 *target;
    u8 *nextState;

    state = *(u8 **)(object + 0x54);
    if (*(f32 *)(object + 0x1C) > 100.0f) {
        objectRequestHierarchyDestruction(*(u8 **)(object + 0x50));
        velocity[0] = 0.0f;
        velocity[1] = 0.0f;
        velocity[2] = 0.0f;
        return 1;
    }
    target = objectResolveAvailableHandle(*(s32 *)(state + 0x2C));
    if (target == NULL) {
        velocity[0] = 0.0f;
        velocity[2] = 0.0f;
        velocity[1] = 10.0f;
        return 1;
    }
    velocity[0] = (f32)(*(f32 *)(target + 0x44) - *(f32 *)(object + 0x18));
    velocity[1] = (f32)(*(f32 *)(target + 0x48) - *(f32 *)(object + 0x1C));
    velocity[2] = (f32)(*(f32 *)(target + 0x4C) - *(f32 *)(object + 0x20));
    distance = vector3Normalize(velocity);
    if ((*(s32 *)(state + 0x54) != 0) && (distance < (*(f32 *)(object + 0x24) + *(f32 *)(target + 0x24)))) {
        next = *(u8 **)(state + 0x8);
        *(s32 *)(state + 0x54) = 0;
        flags = *(u16 *)(object + 0x36);
        if (!(flags & 2)) {
            *(u16 *)(object + 0x36) = (u16)(flags | 2);
            do {
                if (gPikachuActor == *(u8 **)(target + 0x50)) {
                    actorPlayTrackedSound(gPikachuActor, 0x8F);
                    actorAdjustActionMeter(gPikachuActor, 2.5f);
                } else {
                    objectPlaySpatialSound(target, 0x8F, 20.0f, 100.0f);
                }
            } while (0);
        }
        if (next != NULL) {
            nextState = *(u8 **)(next + 0x54);
            *(s32(**)(u8 *, f32 *))(nextState + 0x28) = linkedEffectComputeTargetVelocity;
            *(s32 *)(nextState + 0x54) = 1;
        } else {
            objectRequestHierarchyDestruction(*(u8 **)(object + 0x50));
        }
        velocity[0] = 0.0f;
        velocity[1] = 0.0f;
        velocity[2] = 0.0f;
        return 1;
    }
    maxStep = *(f32 *)(state + 0x30);
    if (maxStep < distance) {
        step = maxStep;
    } else {
        step = distance;
    }
    distance = step / gRuntimeFrameSeconds;
    velocity[0] = (f32)(velocity[0] * distance);
    velocity[1] = (f32)(velocity[1] * distance);
    velocity[2] = (f32)(velocity[2] * distance);
    return 2;
}

u32 randomNext(void); /* extern */
/// Range of randomized elapsed-time thresholds for release from rebound motion.
/// \note Original identifier: \c D_800A3A20 (0x800A3A20).
extern f64 gLinkedReboundReleaseTimeRange;
/// Minimum randomized elapsed-time threshold for release from rebound motion.
/// \note Original identifier: \c D_800A3A28 (0x800A3A28).
extern f64 gLinkedReboundReleaseTimeMinimum;
/// Elapsed-time threshold for switching from rebound to target-directed motion.
/// \note Original identifier: \c D_800A3A30 (0x800A3A30).
extern f32 gLinkedReboundTargetTime;
/// Horizontal velocity turning coefficient during rebound motion.
/// \note Original identifier: \c D_800A3A34 (0x800A3A34).
extern f32 gLinkedReboundTurnCoefficient;
s32 linkedEffectComputeTargetVelocity(u8 *, f32 *);

/// Advance a rebounding effect and select its subsequent motion behavior.
/// \param object Linked effect object with initialized motion state.
/// \param velocity Storage for three floating-point output velocity components.
/// \returns One, indicating that the output fully controls motion.
/// \note Original decompilation identifier: \c func_8001CF3C (0x8001CF3C).
s32 linkedEffectComputeReboundVelocity(u8 *object, f32 *velocity) {
    u8 *objectState;
    f32 turn;

    objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (((MotionState *)objectState)->previous != 0 &&
        (f64)randomNext() * 2.3283064365386963e-10 * gLinkedReboundReleaseTimeRange + gLinkedReboundReleaseTimeMinimum <
            (f64)((MotionState *)objectState)->elapsed) {
        ((MotionState *)objectState)->callback = NULL;
        ((MotionState *)objectState)->velocity[1] = -((MotionState *)objectState)->velocity[1];
    } else if (gLinkedReboundTargetTime < ((MotionState *)objectState)->elapsed) {
        ((MotionState *)objectState)->callback = &linkedEffectComputeTargetVelocity;
        ((MotionState *)objectState)->velocity[1] = -((MotionState *)objectState)->velocity[1];
    }
    turn = gLinkedReboundTurnCoefficient;
    velocity[0] = (f32)(((MotionState *)objectState)->velocity[0] - (((MotionState *)objectState)->velocity[2] * turn));
    velocity[1] = (f32)(((MotionState *)objectState)->velocity[1] - (gRuntimeFrameSeconds * 70.0f));
    velocity[2] = (f32)(((MotionState *)objectState)->velocity[2] + (((MotionState *)objectState)->velocity[0] * turn));
    return 1;
}

/// Range of randomized elapsed-time thresholds for release from falling motion.
/// \note Original identifier: \c D_800A3A38 (0x800A3A38).
extern f64 gLinkedFallingReleaseTimeRange;
/// Minimum randomized elapsed-time threshold for release from falling motion.
/// \note Original identifier: \c D_800A3A40 (0x800A3A40).
extern f64 gLinkedFallingReleaseTimeMinimum;
/// Horizontal velocity turning coefficient during falling motion.
/// \note Original identifier: \c D_800A3A48 (0x800A3A48).
extern f32 gLinkedFallingTurnCoefficient;
/// Advance a falling effect and select its subsequent motion behavior.
/// \param object Linked effect object with initialized motion state.
/// \param velocity Storage for three floating-point output velocity components.
/// \returns One, indicating that the output fully controls motion.
/// \note Original decompilation identifier: \c func_8001D074 (0x8001D074).
s32 linkedEffectComputeFallingVelocity(u8 *object, f32 *velocity) {
    u8 *objectState;
    f32 turn;

    objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (((MotionState *)objectState)->previous != 0 &&
        (f64)randomNext() * 2.3283064365386963e-10 * gLinkedFallingReleaseTimeRange + gLinkedFallingReleaseTimeMinimum <
            (f64)((MotionState *)objectState)->elapsed) {
        ((MotionState *)objectState)->callback = NULL;
    } else if (0.5f < ((MotionState *)objectState)->elapsed) {
        ((MotionState *)objectState)->callback = &linkedEffectComputeTargetVelocity;
    }
    turn = gLinkedFallingTurnCoefficient;
    velocity[0] = (f32)(((MotionState *)objectState)->velocity[0] - (((MotionState *)objectState)->velocity[2] * turn));
    velocity[1] = (f32)(((MotionState *)objectState)->velocity[1] - (gRuntimeFrameSeconds * 30.0f));
    velocity[2] = (f32)(((MotionState *)objectState)->velocity[2] + (((MotionState *)objectState)->velocity[0] * turn));
    return 1;
}

f32 mathCosDegrees(f32);
/// Angular separation in degrees between successive effect launch directions.
/// \note Original identifier: \c D_800A3A4C (0x800A3A4C).
extern f32 gLinkedEffectLaunchAngleStep;

/// Assign an effect its initial outward motion for the selected effect kind.
/// \param kind Motion kind: zero preserves vertical velocity; one, two, and three choose distinct upward launch ranges.
/// \param object Initialized effect whose velocity is assigned.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001D194 (0x8001D194).
void linkedEffectInitializeVelocity(s32 kind, u8 *object) {
    static f32 angle = 0.0f;
    u8 *state = *(u8 **)(object + 0x54);

    angle += gLinkedEffectLaunchAngleStep;
    while (angle < 0.0f) {
        angle += 360.0f;
    }
    while (angle >= 360.0f) {
        angle -= 360.0f;
    }
    *(f32 *)(state + 0x1C) = mathCosDegrees(angle) * 10.0f;
    *(f32 *)(state + 0x24) = mathSinDegrees(angle) * 10.0f;
    switch (kind) {
    case 1:
        *(f32 *)(state + 0x20) = (f64)randomNext() * 2.3283064365386963e-10 * 1.0f + 3.0;
        return;
    case 2:
        *(f32 *)(state + 0x20) = (f64)randomNext() * 2.3283064365386963e-10 * 5.0 + 30.0;
        return;
    case 3:
        *(f32 *)(state + 0x20) = (f64)randomNext() * 2.3283064365386963e-10 * 2.0f + 8.0;
        return;
    }
}

#define SET_MOTION_VECTOR(v, x, y, z) ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))

void linkedEffectInitializeVelocity(s32, u8 *);
/// Model display list used by linked effect objects.
/// \note Original identifier: \c D_1000B08 (0x01000B08).
extern u8 gLinkedEffectModelDisplayList[];
/// Mutable object template used to create each member of a linked effect chain.
/// \note Original identifier: \c D_80079224 (0x80079224).
f32 gLinkedEffectObjectTemplate[9] = {0.1f, 0.1f, 0.1f};
/// Separate bounds portion of the linked-effect object template, beginning with its radius.
/// \note Original identifier: \c D_80079248 (0x80079248).
f32 gLinkedEffectTemplateBounds[3] = {10.0f};
/// Separate model and kind/flags portion of the linked-effect object template.
/// \note Original identifier: \c D_80079254 (0x80079254).
void *gLinkedEffectTemplateModel[3] = {gLinkedEffectModelDisplayList, (void *)0x00A80000};
/// Create and register a chain of animated effects directed toward a target.
/// \param kind Motion kind: zero or one approaches the target, two rebounds, and three falls before changing behavior.
/// \param x Initial world-space X coordinate.
/// \param y Initial world-space Y coordinate.
/// \param z Initial world-space Z coordinate.
/// \param scale Uniform size of each effect.
/// \param target Target object identifier.
/// \param speed Nonnegative maximum target-directed displacement per update.
/// \param damping Fraction of previous velocity retained during normal linked motion.
/// \param spacing Allowed separation from the next effect.
/// \param previousPull Strength of attraction toward the previous effect when separated.
/// \param nextPull Strength of attraction toward the next effect when separated.
/// \param channelA Valid animation index for the effect's appearance frame.
/// \param channelB Valid animation index for the effect's opacity.
/// \param count Requested number of effects; at least one is created.
/// \returns The registered effect group.
/// \note Original decompilation identifier: \c func_8001D3B8 (0x8001D3B8).
u8 *linkedEffectCreateChain(s32 kind, f32 x, f32 y, f32 z, f32 scale, s32 target, f32 speed, f32 damping, f32 spacing,
                            f32 previousPull, f32 nextPull, s32 channelA, s32 channelB, s32 count) {
    u8 *group;
    u8 *first;
    MotionState *firstState;
    s32 i;
    MotionState initial;
    MotionState *state;
    MotionState *previousState;
    u8 *object;
    u8 *previous;
    gLinkedEffectObjectTemplate[0] = scale;
    gLinkedEffectObjectTemplate[1] = scale;
    gLinkedEffectObjectTemplate[2] = scale;
    gLinkedEffectTemplateBounds[0] = 0.5f * scale;
    _bzero(&initial, 0x58);
    initial.spacing = spacing;
    initial.previousPull = previousPull;
    initial.nextPull = nextPull;
    initial.damping = damping;
    initial.target = target;
    initial.speed = speed;
    initial.rate = 1.0f;
    animationIntegerTrackInitialize((ValueTrack *)initial.channelA, gTextureFrameAnimationPrograms[channelA]);
    animationIntegerTrackInitialize((ValueTrack *)initial.channelB, gOpacityAnimationPrograms[channelB]);
    initial.color[0] = 255;
    initial.color[1] = 255;
    initial.color[2] = 255;
    initial.color[3] = 255;
    gLinkedEffectTemplateModel[0] = gLinkedEffectModelDisplayList;
    switch (kind) {
    case 0:
    case 1:
        initial.callback = linkedEffectComputeTargetVelocity;
        break;
    case 2:
        initial.callback = linkedEffectComputeReboundVelocity;
        break;
    case 3:
        initial.callback = linkedEffectComputeFallingVelocity;
        break;
    }
    group = objectCreateDefault();
    first = objectCreateFromTemplate((ObjectTemplate *)gLinkedEffectObjectTemplate);
    *(u8 **)(group + 0x2C) = first;
    firstState = *(MotionState **)(first + 0x54);
    SET_MOTION_VECTOR((f32 *)(first + 0x18), x, y, z);
    SET_MOTION_VECTOR((f32 *)(first + 0x44), x, y, z);
    *firstState = initial;
    firstState->speed = speed;
    firstState->head = 1;
    linkedEffectInitializeVelocity(kind, first);
    previousState = firstState;
    previous = first;
    count--;
    i = 0;
    if (count > 0) {
        do {
            object = objectCreateFromTemplate((ObjectTemplate *)gLinkedEffectObjectTemplate);
            *(u8 **)(previous + 0x28) = object;
            state = *(MotionState **)(object + 0x54);
            SET_MOTION_VECTOR((f32 *)(object + 0x18), x, y, z);
            SET_MOTION_VECTOR((f32 *)(object + 0x44), x, y, z);
            *state = initial;
            state->head = 0;
            previousState->next = object;
            state->previous = previous;
            linkedEffectInitializeVelocity(kind, object);
            i++;
            previousState = state;
            previous = object;
        } while (i != count);
    }
    objectMakeHierarchyRoot(group);
    objectAllocateHierarchyMatrixBuffers(group);
    objectRegisterActiveHierarchy(group);
    return group;
}

#undef SET_MOTION_VECTOR
