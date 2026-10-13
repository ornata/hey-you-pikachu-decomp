#include "render_state.h"
#include "texture_setup.h"
#include "camera_storage.h"
#include "vector_metrics.h"
/// \file
/// Creates and advances actor-attached effects with frame and opacity animation.

#include "attached_effect.h"
#include "camera_types.h"
#include "actor_script.h"
#include "object_types.h"
#include "opacity_animation_programs.h"
#include "texture_animation_programs.h"
#include "object_movement.h"
#include "object_flags.h"
#include "object_hierarchy_create.h"
#include "object_lifecycle.h"
#include "common.h"
#include "actor_parts.h"
#include "integer_animation.h"
#include "object_lookup.h"
#include "heap_alloc.h"
#include "game_heap.h"

/// Separate view of the default texture-frame animation program for attached effects.
/// \note Original identifier: \c D_8006B320 (0x8006B320).
extern ValueKey *gAttachedEffectDefaultFrameProgram;

/// Resident appearance and animation defaults for one attached-effect kind.
typedef struct AttachedEffectDefinition {
    ObjectTemplate *objectTemplate; ///< Borrowed hierarchy template.
    f32 baseScale; ///< Scale multiplier for view-dependent sizing.
    s32 opacityProgramIndex; ///< Opacity key-program table index.
} AttachedEffectDefinition;
typedef char AttachedEffectDefinitionSizeCheck[(sizeof(AttachedEffectDefinition) == 0xC) ? 1 : -1];

/// Four byte color components used by the effect's primitive color.
typedef struct Rgba8 {
    u8 r, g, b, a;
} Rgba8;

/// Owned animation and appearance state for an actor-attached effect.
typedef struct AttachedEffectState {
    u8 kind; ///< Effect-definition index selected at creation.
    s8 reverseFacing; ///< Nonzero turns the effect away from its camera-facing orientation.
    u8 unknown002[2];
    u8 *owner; ///< Borrowed actor that anchors the effect.
    f32 playbackRate; ///< Time multiplier shared by frame and opacity tracks.
    ValueTrack frameTrack; ///< Texture-frame animation borrowing resident keys.
    ValueTrack opacityTrack; ///< Opacity animation borrowing resident keys.
    Rgba8 color; ///< Primitive color; alpha follows the opacity track.
    f32 baseScale; ///< Multiplier applied to view-dependent scale.
} AttachedEffectState;
typedef char AttachedEffectStateSizeCheck[(sizeof(AttachedEffectState) == 0x2C) ? 1 : -1];
#define ATTACHED_EFFECT_OFFSET_CHECK(name, member, offset) \
    typedef char name[(((u32)&((AttachedEffectState *)0)->member) == (offset)) ? 1 : -1]
ATTACHED_EFFECT_OFFSET_CHECK(AttachedEffectOwnerOffsetCheck, owner, 0x4);
ATTACHED_EFFECT_OFFSET_CHECK(AttachedEffectRateOffsetCheck, playbackRate, 0x8);
ATTACHED_EFFECT_OFFSET_CHECK(AttachedEffectFrameOffsetCheck, frameTrack, 0xC);
ATTACHED_EFFECT_OFFSET_CHECK(AttachedEffectOpacityOffsetCheck, opacityTrack, 0x18);
ATTACHED_EFFECT_OFFSET_CHECK(AttachedEffectColorOffsetCheck, color, 0x24);
ATTACHED_EFFECT_OFFSET_CHECK(AttachedEffectScaleOffsetCheck, baseScale, 0x28);
#undef ATTACHED_EFFECT_OFFSET_CHECK
/// Attached-effect templates, base scales and opacity-program selections indexed by effect kind.
/// \note Original identifier: \c D_80074EB8 (0x80074EB8).
extern AttachedEffectDefinition gAttachedEffectDefinitions[];
/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
extern s32 gRuntimeGameMode;
/// Create an animated effect for an owner, replacing its previous attached effect.
/// \param owner Object that owns and anchors the effect.
/// \param kind Effect-definition index selecting its appearance.
/// \param track Animation-track index selecting the effect animation.
/// \returns The created effect object.
/// \note Original decompilation identifier: \c func_80013130 (0x80013130).
u8 *attachedEffectCreate(u8 *owner, s32 kind, s32 track) {
    u8 *obj;
    u8 *v;
    u8 *ov;
    AttachedEffectDefinition *def;
    def = &gAttachedEffectDefinitions[kind];
    obj = objectCreateHierarchyFromTemplate(def->objectTemplate);
    if (gRuntimeGameMode == 8 || gRuntimeGameMode == 0x12) {
        objectSetHierarchyFlags(obj, 0x6000);
    }
    v = ((RuntimeObjectPrefix *)obj)->state.bytes;
    ((AttachedEffectState *)v)->kind = kind;
    ((AttachedEffectState *)v)->baseScale = def->baseScale;
    animationIntegerTrackInitialize(&((AttachedEffectState *)v)->opacityTrack, gOpacityAnimationPrograms[def->opacityProgramIndex]);
    animationIntegerTrackInitialize(&((AttachedEffectState *)v)->frameTrack, (gTextureFrameAnimationPrograms[track]));
    ((AttachedEffectState *)v)->owner = owner;
    ((AttachedEffectState *)v)->reverseFacing = 0;
    objectRegisterActiveHierarchy(obj);
    ov = ((RuntimeObjectPrefix *)owner)->state.bytes;
    owner = objectFindLiveByHandle(((ActorScriptState *)ov)->attachedEffectHandle);
    if (owner != NULL) {
        objectRequestHierarchyDestruction(owner);
    }
    ((ActorScriptState *)ov)->attachedEffectHandle = ((RuntimeObjectPrefix *)obj)->handle;
    return obj;
}

/// Initialize an attached effect object with its default animation and appearance.
/// \param object Effect object to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013254 (0x80013254).
void attachedEffectInitialize(u8 *object) {
    u8 *obj;

    obj = heapAllocLocal(&gGameHeap, sizeof(AttachedEffectState), 0x7A);
    animationIntegerTrackInitialize(&((AttachedEffectState *)obj)->frameTrack, gAttachedEffectDefaultFrameProgram);
    animationIntegerTrackInitialize(&((AttachedEffectState *)obj)->opacityTrack, gOpacityAnimationPrograms[0]);
    ((AttachedEffectState *)obj)->color.r = ((AttachedEffectState *)obj)->color.g =
        ((AttachedEffectState *)obj)->color.b = ((AttachedEffectState *)obj)->color.a = 0xFF;
    ((AttachedEffectState *)obj)->owner = NULL;
    ((AttachedEffectState *)obj)->playbackRate = 1.0f;
    ((AttachedEffectState *)obj)->baseScale = 1.0f;
    ((RuntimeObjectPrefix *)object)->state.bytes = obj;
}

/// Minimum view-dependent scale factor for an attached effect.
/// \note Original identifier: \c D_800A26A0 (0x800A26A0).
extern f32 gAttachedEffectMinimumViewScale;
/// Scale coefficient applied to camera distance and owner size for attached effects.
/// \note Original identifier: \c D_800A26A4 (0x800A26A4).
extern f32 gAttachedEffectViewScaleCoefficient;
/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Advance an attached effect animation and keep its placement and size appropriate to its owner and the view.
/// \param obj Attached effect object to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800132E0 (0x800132E0).
void attachedEffectUpdate(u8 *obj) {
    u8 *state;
    f32 minimum;
    s32 value;
    f32 step;
    u8 *owner;
    f32 *view;
    f32 factor;
    f32 delta[3];

    state = ((RuntimeObjectPrefix *)obj)->state.bytes;
    step = ((AttachedEffectState *)state)->playbackRate * gRuntimeFrameSeconds;
    owner = ((AttachedEffectState *)state)->owner;
    view = (f32 *)actorFindPart(owner, 0x79);
    ((RuntimeObjectPrefix *)obj)->position[0] = ((RuntimeObjectPrefix *)view)->worldPosition[0];
    ((RuntimeObjectPrefix *)obj)->position[1] = ((RuntimeObjectPrefix *)view)->worldPosition[1];
    ((RuntimeObjectPrefix *)obj)->position[2] = ((RuntimeObjectPrefix *)view)->worldPosition[2];
    ((RuntimeObjectPrefix *)obj)->worldPosition[0] = ((RuntimeObjectPrefix *)obj)->position[0];
    ((RuntimeObjectPrefix *)obj)->worldPosition[1] = ((RuntimeObjectPrefix *)obj)->position[1];
    ((RuntimeObjectPrefix *)obj)->worldPosition[2] = ((RuntimeObjectPrefix *)obj)->position[2];
    objectOrientAlongCameraView(obj);
    delta[0] = ((RuntimeObjectPrefix *)obj)->worldPosition[0] - gRenderCameras[gRenderCameraIndex].eye[0];
    delta[1] = ((RuntimeObjectPrefix *)obj)->worldPosition[1] - gRenderCameras[gRenderCameraIndex].eye[1];
    delta[2] = ((RuntimeObjectPrefix *)obj)->worldPosition[2] - gRenderCameras[gRenderCameraIndex].eye[2];
    factor = vector3Length(delta);
    minimum = gAttachedEffectMinimumViewScale;
    factor = (factor + ((RuntimeObjectPrefix *)owner)->scale[0] * 1000.0f) * gAttachedEffectViewScaleCoefficient;
    if (factor < minimum) {
        factor = minimum;
    } else if (factor > 4.0f) {
        factor = 4.0f;
    }
    ((RuntimeObjectPrefix *)obj)->position[1] += ((RuntimeObjectPrefix *)owner)->radius * factor * 40.0f;
    factor *= ((AttachedEffectState *)state)->baseScale;
    ((RuntimeObjectPrefix *)obj)->scale[0] = factor;
    ((RuntimeObjectPrefix *)obj)->scale[1] = factor;
    ((RuntimeObjectPrefix *)obj)->scale[2] = factor;
    if (((AttachedEffectState *)state)->reverseFacing != 0) {
        ((RuntimeObjectPrefix *)obj)->rotation[1] += 180.0f;
        while (((RuntimeObjectPrefix *)obj)->rotation[1] < 0.0f) {
            ((RuntimeObjectPrefix *)obj)->rotation[1] += 360.0f;
        }
        while (((RuntimeObjectPrefix *)obj)->rotation[1] >= 360.0f) {
            ((RuntimeObjectPrefix *)obj)->rotation[1] -= 360.0f;
        }
    }
    animationIntegerTrackEvaluate(&((AttachedEffectState *)state)->frameTrack, &value, step);
    if (animationIntegerTrackEvaluate(&((AttachedEffectState *)state)->opacityTrack, &value, step) == 1) {
        objectRequestHierarchyDestruction(obj);
        return;
    }
    ((AttachedEffectState *)state)->color.a = value;
}

/// Prepare the current animation frame and color for rendering an attached effect.
/// \param object Attached effect object to render.
/// \returns The effect geometry display-list address.
/// \note Original decompilation identifier: \c func_80013530 (0x80013530).
s32 attachedEffectPrepareDraw(u8 *object) {
    u8 *v = ((RuntimeObjectPrefix *)object)->state.bytes;
    Rgba8 *c;

    textureLoadForDraw(&gRenderDisplayListCursor, 4, ((AttachedEffectState *)v)->frameTrack.value);
    c = &((AttachedEffectState *)v)->color;
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, c->r, c->g, c->b, c->a);
    return (s32)((RuntimeObjectPrefix *)object)->displayList;
}
