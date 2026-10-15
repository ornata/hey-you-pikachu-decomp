#include "render_state.h"
#include "texture_setup.h"
#include "vector_metrics.h"
#include "math_angles.h"
#include "random.h"
/// \file
/// Maintains linked animated effects with neighbor constraints and target motion.

#include "linked_effect.h"
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

/// Compute a linked effect's requested velocity.
/// \param object Borrowed live effect with initialized private state.
/// \param velocity Borrowed writable X, Y, Z motion vector, valid only for this call.
/// The callback must not retain its address.
/// \returns One to use this motion alone, or another value to add damping and neighbor pulls.
typedef s32 (*LinkedEffectMotionCallback)(u8 *object, f32 *velocity);

/// Owned motion, neighbor constraints and animated appearance of one linked effect.
typedef struct LinkedEffectState {
    f32 elapsed; ///< Seconds accumulated since creation.
    u8 *previous; ///< Borrowed preceding effect, or null.
    u8 *next; ///< Borrowed following effect, or null.
    f32 spacing; ///< Allowed separation from the following effect.
    f32 previousPull; ///< Strength of correction toward the preceding effect.
    f32 nextPull; ///< Strength of correction toward the following effect.
    f32 damping; ///< Fraction of retained velocity used during constrained motion.
    f32 velocity[3]; ///< Retained motion vector.
    LinkedEffectMotionCallback callback; ///< Optional motion policy; null uses constrained motion alone.
    s32 targetHandle; ///< Object-manager target identifier.
    f32 speed; ///< Limit applied to target-directed motion.
    f32 playbackRate; ///< Time multiplier for both appearance tracks.
    ValueTrack frameTrack; ///< Texture-frame track borrowing resident keys.
    ValueTrack opacityTrack; ///< Opacity track borrowing resident keys.
    u8 color[4]; ///< Primitive RGBA; alpha follows the opacity track.
    s32 contactPending; ///< Nonzero enables target contact for the current leading effect.
} LinkedEffectState;

typedef char LinkedEffectStateSizeCheck[(sizeof(LinkedEffectState) == 0x58) ? 1 : -1];
#define LINKED_EFFECT_OFFSET_CHECK(name, member, offset) \
    typedef char name[(((u32)&((LinkedEffectState *)0)->member) == (offset)) ? 1 : -1]
LINKED_EFFECT_OFFSET_CHECK(LinkedEffectCallbackOffsetCheck, callback, 0x28);
LINKED_EFFECT_OFFSET_CHECK(LinkedEffectTargetOffsetCheck, targetHandle, 0x2C);
LINKED_EFFECT_OFFSET_CHECK(LinkedEffectFrameOffsetCheck, frameTrack, 0x38);
LINKED_EFFECT_OFFSET_CHECK(LinkedEffectOpacityOffsetCheck, opacityTrack, 0x44);
LINKED_EFFECT_OFFSET_CHECK(LinkedEffectColorOffsetCheck, color, 0x50);
LINKED_EFFECT_OFFSET_CHECK(LinkedEffectContactOffsetCheck, contactPending, 0x54);
#undef LINKED_EFFECT_OFFSET_CHECK

void _bzero(void *, s32); /* extern */

/// Allocate and initialize motion state for a linked effect.
/// \param object Effect object receiving its motion state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C860 (0x8001C860).
void linkedEffectInitialize(u8 *object) {
    LinkedEffectState *state;
    LinkedEffectState *allocation;

    allocation = heapAllocLocal(&gGameHeap, sizeof(LinkedEffectState), 0x38);
    state = allocation;
    _bzero(allocation, sizeof(LinkedEffectState));
    ((RuntimeObjectPrefix *)object)->state.pointer = state;
}

/// Detach an effect from its neighbors and release its motion state.
/// \param object Linked effect object with initialized motion state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C8AC (0x8001C8AC).
void linkedEffectDestroy(u8 *object) {
    u8 *effectState;
    u8 *precedingState;
    u8 *previousEffect;
    u8 *nextEffect;
    u8 *followingState;
    effectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    previousEffect = ((LinkedEffectState *)effectState)->previous;
    nextEffect = ((LinkedEffectState *)effectState)->next;
    if (previousEffect != 0) {
        precedingState = ((RuntimeObjectPrefix *)previousEffect)->state.bytes;
        ((LinkedEffectState *)precedingState)->next = nextEffect;
    }
    if (nextEffect != 0) {
        ((LinkedEffectState *)(followingState = ((RuntimeObjectPrefix *)nextEffect)->state.bytes))->previous = previousEffect;
    }
    heapFreeLocal(&gGameHeap, effectState);
}

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

    d[0] = ((RuntimeObjectPrefix *)neighbor)->position[0] - ((RuntimeObjectPrefix *)object)->position[0];
    d[1] = ((RuntimeObjectPrefix *)neighbor)->position[1] - ((RuntimeObjectPrefix *)object)->position[1];
    d[2] = ((RuntimeObjectPrefix *)neighbor)->position[2] - ((RuntimeObjectPrefix *)object)->position[2];
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
    LinkedEffectMotionCallback callback;

    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    velocity[2] = 0.0F;
    velocity[1] = 0.0F;
    velocity[0] = 0.0F;
    callback = ((LinkedEffectState *)state)->callback;
    if (callback == NULL || callback(object, velocity) != 1) {
        for (i = 0; i < 3; i++) {
            velocity[i] += ((LinkedEffectState *)state)->velocity[i] * ((LinkedEffectState *)state)->damping;
        }
        neighbor = ((LinkedEffectState *)state)->previous;
        if (neighbor != NULL) {
            linkedEffectAddNeighborPull(object, neighbor, ((LinkedEffectState *)((RuntimeObjectPrefix *)neighbor)->state.pointer)->spacing,
                                        ((LinkedEffectState *)state)->previousPull, velocity);
        }
        next = ((LinkedEffectState *)state)->next;
        if (next != NULL) {
            linkedEffectAddNeighborPull(object, next, ((LinkedEffectState *)state)->spacing, ((LinkedEffectState *)state)->nextPull, velocity);
        }
    }
    ((LinkedEffectState *)state)->velocity[0] = velocity[0];
    ((LinkedEffectState *)state)->velocity[1] = velocity[1];
    ((LinkedEffectState *)state)->velocity[2] = velocity[2];
    for (axis = 0; axis < 3; axis++) {
        ((RuntimeObjectPrefix *)object)->position[axis] += velocity[axis] * gRuntimeFrameSeconds;
    }
}


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
        (f32)(mathSinDegrees(((LinkedEffectState *)state)->elapsed * 540.0F) * 45.0F);
    if (((RuntimeObjectPrefix *)object)->handle & 1) {
        ((RuntimeObjectPrefix *)object)->rotation[0] = (-((RuntimeObjectPrefix *)object)->rotation[0]);
    }
    if (((RuntimeObjectPrefix *)object)->rotation[0] < 0.0F) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[0] = (f32)(((RuntimeObjectPrefix *)object)->rotation[0] + 360.0F);
        } while (((RuntimeObjectPrefix *)object)->rotation[0] < 0.0F);
    }
    if (((RuntimeObjectPrefix *)object)->rotation[0] >= 360.0F) {
        do {
            ((RuntimeObjectPrefix *)object)->rotation[0] = (f32)(((RuntimeObjectPrefix *)object)->rotation[0] - 360.0F);
        } while (((RuntimeObjectPrefix *)object)->rotation[0] >= 360.0F);
    }
    step = ((LinkedEffectState *)state)->playbackRate * gRuntimeFrameSeconds;
    animationIntegerTrackEvaluate(&((LinkedEffectState *)state)->frameTrack, &channelValue, step);
    animationIntegerTrackEvaluate(&((LinkedEffectState *)state)->opacityTrack, &channelValue, step);
    ((LinkedEffectState *)state)->color[3] = (s8)channelValue;
    ((LinkedEffectState *)state)->elapsed = (((LinkedEffectState *)state)->elapsed + gRuntimeFrameSeconds);
}

typedef struct Rgba8 {
    u8 r, g, b, a;
} Rgba8;

/// Append drawing setup for a linked effect using its animated appearance.
/// \param object Linked effect object with initialized motion state.
/// \returns Borrowed object display list, or null when no drawing continuation is supplied.
/// \note Original decompilation identifier: \c func_8001CC9C (0x8001CC9C).
Gfx *linkedEffectPrepareDraw(u8 *object) {
    u8 *v = ((RuntimeObjectPrefix *)object)->state.bytes;
    Rgba8 *c;

    textureLoadForDraw(&gRenderDisplayListCursor, 5, ((LinkedEffectState *)v)->frameTrack.value);
    c = (Rgba8 *)((LinkedEffectState *)v)->color;
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, c->r, c->g, c->b, c->a);
    return ((RuntimeObjectPrefix *)object)->displayList;
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

    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (((RuntimeObjectPrefix *)object)->position[1] > 100.0F) {
        objectRequestHierarchyDestruction(((RuntimeObjectPrefix *)object)->root.bytes);
        velocity[0] = 0.0F;
        velocity[1] = 0.0F;
        velocity[2] = 0.0F;
        return 1;
    }
    target = objectResolveAvailableHandle(((LinkedEffectState *)state)->targetHandle);
    if (target == NULL) {
        velocity[0] = 0.0F;
        velocity[2] = 0.0F;
        velocity[1] = 10.0F;
        return 1;
    }
    velocity[0] = (((RuntimeObjectPrefix *)target)->worldPosition[0] - ((RuntimeObjectPrefix *)object)->position[0]);
    velocity[1] = (((RuntimeObjectPrefix *)target)->worldPosition[1] - ((RuntimeObjectPrefix *)object)->position[1]);
    velocity[2] = (((RuntimeObjectPrefix *)target)->worldPosition[2] - ((RuntimeObjectPrefix *)object)->position[2]);
    distance = vector3Normalize(velocity);
    if ((((LinkedEffectState *)state)->contactPending != 0) && (distance < (((RuntimeObjectPrefix *)target)->radius + ((RuntimeObjectPrefix *)object)->radius))) {
        next = ((LinkedEffectState *)state)->next;
        ((LinkedEffectState *)state)->contactPending = 0;
        flags = ((RuntimeObjectPrefix *)object)->flags;
        if (!(flags & 2)) {
            ((RuntimeObjectPrefix *)object)->flags = (u16)(flags | 2);
            do {
                if (gPikachuActor == ((RuntimeObjectPrefix *)target)->root.bytes) {
                    actorPlayTrackedSound(gPikachuActor, 0x8F);
                    actorAdjustActionMeter(gPikachuActor, 2.5F);
                } else {
                    objectPlaySpatialSound(target, 0x8F, 20.0F, 100.0F);
                }
            } while (0);
        }
        if (next != NULL) {
            nextState = ((RuntimeObjectPrefix *)next)->state.bytes;
            ((LinkedEffectState *)nextState)->callback = linkedEffectComputeTargetVelocity;
            ((LinkedEffectState *)nextState)->contactPending = 1;
        } else {
            objectRequestHierarchyDestruction(((RuntimeObjectPrefix *)object)->root.bytes);
        }
        velocity[0] = 0.0F;
        velocity[1] = 0.0F;
        velocity[2] = 0.0F;
        return 1;
    }
    maxStep = ((LinkedEffectState *)state)->speed;
    if (maxStep < distance) {
        step = maxStep;
    } else {
        step = distance;
    }
    distance = step / gRuntimeFrameSeconds;
    velocity[0] = (velocity[0] * distance);
    velocity[1] = (velocity[1] * distance);
    velocity[2] = (velocity[2] * distance);
    return 2;
}

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

/// Advance a rebounding effect and select its subsequent motion behavior.
/// \param object Linked effect object with initialized motion state.
/// \param velocity Storage for three floating-point output velocity components.
/// \returns One, indicating that the output fully controls motion.
/// \note Original decompilation identifier: \c func_8001CF3C (0x8001CF3C).
s32 linkedEffectComputeReboundVelocity(u8 *object, f32 *velocity) {
    u8 *objectState;
    f32 turn;

    objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (((LinkedEffectState *)objectState)->previous != 0 &&
        ((f64)randomNext() * 2.3283064365386963e-10 * gLinkedReboundReleaseTimeRange) +
                gLinkedReboundReleaseTimeMinimum <
            (f64)((LinkedEffectState *)objectState)->elapsed) {
        ((LinkedEffectState *)objectState)->callback = NULL;
        ((LinkedEffectState *)objectState)->velocity[1] = -((LinkedEffectState *)objectState)->velocity[1];
    } else if (gLinkedReboundTargetTime < ((LinkedEffectState *)objectState)->elapsed) {
        ((LinkedEffectState *)objectState)->callback = &linkedEffectComputeTargetVelocity;
        ((LinkedEffectState *)objectState)->velocity[1] = -((LinkedEffectState *)objectState)->velocity[1];
    }
    turn = gLinkedReboundTurnCoefficient;
    velocity[0] =
        (((LinkedEffectState *)objectState)->velocity[0] - (((LinkedEffectState *)objectState)->velocity[2] * turn));
    velocity[1] = (f32)(((LinkedEffectState *)objectState)->velocity[1] - (gRuntimeFrameSeconds * 70.0F));
    velocity[2] =
        (((LinkedEffectState *)objectState)->velocity[2] + (((LinkedEffectState *)objectState)->velocity[0] * turn));
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
    if (((LinkedEffectState *)objectState)->previous != 0 &&
        ((f64)randomNext() * 2.3283064365386963e-10 * gLinkedFallingReleaseTimeRange) +
                gLinkedFallingReleaseTimeMinimum <
            (f64)((LinkedEffectState *)objectState)->elapsed) {
        ((LinkedEffectState *)objectState)->callback = NULL;
    } else if (0.5F < ((LinkedEffectState *)objectState)->elapsed) {
        ((LinkedEffectState *)objectState)->callback = &linkedEffectComputeTargetVelocity;
    }
    turn = gLinkedFallingTurnCoefficient;
    velocity[0] =
        (((LinkedEffectState *)objectState)->velocity[0] - (((LinkedEffectState *)objectState)->velocity[2] * turn));
    velocity[1] = (f32)(((LinkedEffectState *)objectState)->velocity[1] - (gRuntimeFrameSeconds * 30.0F));
    velocity[2] =
        (((LinkedEffectState *)objectState)->velocity[2] + (((LinkedEffectState *)objectState)->velocity[0] * turn));
    return 1;
}

/// Angular separation in degrees between successive effect launch directions.
/// \note Original identifier: \c D_800A3A4C (0x800A3A4C).
extern f32 gLinkedEffectLaunchAngleStep;

/// Assign an effect its initial outward motion for the selected effect kind.
/// \param kind Motion kind: zero preserves vertical velocity; one, two, and three choose distinct upward launch ranges.
/// \param object Initialized effect whose velocity is assigned.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001D194 (0x8001D194).
void linkedEffectInitializeVelocity(s32 kind, u8 *object) {
    static f32 angle = 0.0F;
    u8 *state = ((RuntimeObjectPrefix *)object)->state.bytes;

    angle += gLinkedEffectLaunchAngleStep;
    while (angle < 0.0F) {
        angle += 360.0F;
    }
    while (angle >= 360.0F) {
        angle -= 360.0F;
    }
    ((LinkedEffectState *)state)->velocity[0] = mathCosDegrees(angle) * 10.0F;
    ((LinkedEffectState *)state)->velocity[2] = mathSinDegrees(angle) * 10.0F;
    switch (kind) {
    case LINKED_EFFECT_RISING_TARGET:
        ((LinkedEffectState *)state)->velocity[1] = ((f64)randomNext() * 2.3283064365386963e-10 * 1.0F) + 3.0;
        return;
    case LINKED_EFFECT_REBOUND:
        ((LinkedEffectState *)state)->velocity[1] = ((f64)randomNext() * 2.3283064365386963e-10 * 5.0) + 30.0;
        return;
    case LINKED_EFFECT_FALLING:
        ((LinkedEffectState *)state)->velocity[1] = ((f64)randomNext() * 2.3283064365386963e-10 * 2.0F) + 8.0;
        return;
    }
}

#define SET_MOTION_VECTOR(v, x, y, z) ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))

/// Model display list used by linked effect objects.
/// \note Original identifier: \c D_1000B08 (0x01000B08).
extern u8 gLinkedEffectModelDisplayList[];
/// Mutable object template used to create each member of a linked effect chain.
/// \note Original identifier: \c D_80079224 (0x80079224).
f32 gLinkedEffectObjectTemplate[9] = {0.1F, 0.1F, 0.1F};
/// Separate bounds portion of the linked-effect object template, beginning with its radius.
/// \note Original identifier: \c D_80079248 (0x80079248).
f32 gLinkedEffectTemplateBounds[3] = {10.0F};
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
    LinkedEffectState *firstState;
    s32 i;
    LinkedEffectState initial;
    LinkedEffectState *state;
    LinkedEffectState *previousState;
    u8 *object;
    u8 *previous;
    gLinkedEffectObjectTemplate[0] = scale;
    gLinkedEffectObjectTemplate[1] = scale;
    gLinkedEffectObjectTemplate[2] = scale;
    gLinkedEffectTemplateBounds[0] = 0.5F * scale;
    _bzero(&initial, sizeof(LinkedEffectState));
    initial.spacing = spacing;
    initial.previousPull = previousPull;
    initial.nextPull = nextPull;
    initial.damping = damping;
    initial.targetHandle = target;
    initial.speed = speed;
    initial.playbackRate = 1.0F;
    animationIntegerTrackInitialize(&initial.frameTrack, gTextureFrameAnimationPrograms[channelA]);
    animationIntegerTrackInitialize(&initial.opacityTrack, gOpacityAnimationPrograms[channelB]);
    initial.color[0] = 255;
    initial.color[1] = 255;
    initial.color[2] = 255;
    initial.color[3] = 255;
    gLinkedEffectTemplateModel[0] = gLinkedEffectModelDisplayList;
    switch (kind) {
    case LINKED_EFFECT_TARGET:
    case LINKED_EFFECT_RISING_TARGET:
        initial.callback = linkedEffectComputeTargetVelocity;
        break;
    case LINKED_EFFECT_REBOUND:
        initial.callback = linkedEffectComputeReboundVelocity;
        break;
    case LINKED_EFFECT_FALLING:
        initial.callback = linkedEffectComputeFallingVelocity;
        break;
    }
    group = objectCreateDefault();
    first = objectCreateFromTemplate((ObjectTemplate *)gLinkedEffectObjectTemplate);
    ((RuntimeObjectPrefix *)group)->child.bytes = first;
    firstState = (LinkedEffectState *)((RuntimeObjectPrefix *)first)->state.pointer;
    SET_MOTION_VECTOR(((RuntimeObjectPrefix *)first)->position, x, y, z);
    SET_MOTION_VECTOR(((RuntimeObjectPrefix *)first)->worldPosition, x, y, z);
    *firstState = initial;
    firstState->speed = speed;
    firstState->contactPending = 1;
    linkedEffectInitializeVelocity(kind, first);
    previousState = firstState;
    previous = first;
    count--;
    i = 0;
    if (count > 0) {
        do {
            object = objectCreateFromTemplate((ObjectTemplate *)gLinkedEffectObjectTemplate);
            ((RuntimeObjectPrefix *)previous)->sibling.bytes = object;
            state = (LinkedEffectState *)((RuntimeObjectPrefix *)object)->state.pointer;
            SET_MOTION_VECTOR(((RuntimeObjectPrefix *)object)->position, x, y, z);
            SET_MOTION_VECTOR(((RuntimeObjectPrefix *)object)->worldPosition, x, y, z);
            *state = initial;
            state->contactPending = 0;
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
