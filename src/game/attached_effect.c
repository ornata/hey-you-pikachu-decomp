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
extern s32 gAttachedEffectDefaultFrameProgram;
/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;
void textureLoadForDraw(Gfx **, s32, s16);
/// Shared integer animation programs controlling effect and marker opacity.
/// \note Original identifier: \c D_8006B4B0 (0x8006B4B0).
extern s32 gOpacityAnimationPrograms;

typedef struct PartDef {
    u8 *tmpl;
    f32 scale;
    s32 track;
} PartDef;
/// Attached-effect templates, base scales and opacity-program selections indexed by effect kind.
/// \note Original identifier: \c D_80074EB8 (0x80074EB8).
extern PartDef gAttachedEffectDefinitions[];
/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
extern s32 gRuntimeGameMode;
/// Shared integer animation programs selecting texture frames for actors, effects and interface elements.
/// \note Original identifier: \c D_8006B090 (0x8006B090).
extern s32 gTextureFrameAnimationPrograms[];
void objectSetHierarchyFlags(u8 *, u16);
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
    PartDef *def;
    def = &gAttachedEffectDefinitions[kind];
    obj = objectCreateHierarchyFromTemplate((ObjectTemplate *)def->tmpl);
    if (gRuntimeGameMode == 8 || gRuntimeGameMode == 0x12) {
        objectSetHierarchyFlags(obj, 0x6000);
    }
    v = *(u8 **)(obj + 0x54);
    v[0] = kind;
    *(f32 *)(v + 0x28) = def->scale;
    animationIntegerTrackInitialize((ValueTrack *)(v + 0x18), (ValueKey *)((&gOpacityAnimationPrograms)[def->track]));
    animationIntegerTrackInitialize((ValueTrack *)(v + 0xC), (ValueKey *)(gTextureFrameAnimationPrograms[track]));
    *(u8 **)(v + 4) = owner;
    v[1] = 0;
    objectRegisterActiveHierarchy(obj);
    ov = *(u8 **)(owner + 0x54);
    owner = objectFindLiveByHandle(*(s32 *)(ov + 0x404));
    if (owner != NULL) {
        objectRequestHierarchyDestruction(owner);
    }
    *(s32 *)(ov + 0x404) = *(s32 *)(obj + 0x38);
    return obj;
}

/// Initialize an attached effect object with its default animation and appearance.
/// \param object Effect object to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013254 (0x80013254).
void attachedEffectInitialize(u8 *object) {
    u8 *obj;

    obj = heapAllocLocal(&gGameHeap, 0x2C, 0x7A);
    animationIntegerTrackInitialize((ValueTrack *)(obj + 0xC), (ValueKey *)(gAttachedEffectDefaultFrameProgram));
    animationIntegerTrackInitialize((ValueTrack *)(obj + 0x18), (ValueKey *)(gOpacityAnimationPrograms));
    obj[0x24] = obj[0x25] = obj[0x26] = obj[0x27] = 0xFF;
    *(s32 *)(obj + 4) = 0;
    *(f32 *)(obj + 8) = 1.0f;
    *(f32 *)(obj + 0x28) = 1.0f;
    *(u8 **)(object + 0x54) = obj;
}

void objectOrientAlongCameraView(u8 *);
f32 vector3Length(f32 *);
/// Minimum view-dependent scale factor for an attached effect.
/// \note Original identifier: \c D_800A26A0 (0x800A26A0).
extern f32 gAttachedEffectMinimumViewScale;
/// Scale coefficient applied to camera distance and owner size for attached effects.
/// \note Original identifier: \c D_800A26A4 (0x800A26A4).
extern f32 gAttachedEffectViewScaleCoefficient;
/// Camera storage used for view transforms, viewports and color overlays.
/// \note Original identifier: \c D_800ABCF0 (0x800ABCF0).
extern f32 gRenderCameras[][56];
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

    state = *(u8 **)(obj + 0x54);
    step = *(f32 *)(state + 8) * gRuntimeFrameSeconds;
    owner = *(u8 **)(state + 4);
    view = (f32 *)actorFindPart(owner, 0x79);
    *(f32 *)(obj + 0x18) = view[17];
    *(f32 *)(obj + 0x1C) = view[18];
    *(f32 *)(obj + 0x20) = view[19];
    *(f32 *)(obj + 0x44) = *(f32 *)(obj + 0x18);
    *(f32 *)(obj + 0x48) = *(f32 *)(obj + 0x1C);
    *(f32 *)(obj + 0x4C) = *(f32 *)(obj + 0x20);
    objectOrientAlongCameraView(obj);
    delta[0] = *(f32 *)(obj + 0x44) - gRenderCameras[gRenderCameraIndex][0];
    delta[1] = *(f32 *)(obj + 0x48) - gRenderCameras[gRenderCameraIndex][1];
    delta[2] = *(f32 *)(obj + 0x4C) - gRenderCameras[gRenderCameraIndex][2];
    factor = vector3Length(delta);
    minimum = gAttachedEffectMinimumViewScale;
    factor = (factor + *(f32 *)owner * 1000.0f) * gAttachedEffectViewScaleCoefficient;
    if (factor < minimum) {
        factor = minimum;
    } else if (factor > 4.0f) {
        factor = 4.0f;
    }
    *(f32 *)(obj + 0x1C) += *(f32 *)(owner + 0x24) * factor * 40.0f;
    factor *= *(f32 *)(state + 0x28);
    *(f32 *)(obj + 0) = factor;
    *(f32 *)(obj + 4) = factor;
    *(f32 *)(obj + 8) = factor;
    if (*(s8 *)(state + 1) != 0) {
        *(f32 *)(obj + 0x10) += 180.0f;
        while (*(f32 *)(obj + 0x10) < 0.0f) {
            *(f32 *)(obj + 0x10) += 360.0f;
        }
        while (*(f32 *)(obj + 0x10) >= 360.0f) {
            *(f32 *)(obj + 0x10) -= 360.0f;
        }
    }
    animationIntegerTrackEvaluate((ValueTrack *)(state + 0xC), &value, step);
    if (animationIntegerTrackEvaluate((ValueTrack *)(state + 0x18), &value, step) == 1) {
        objectRequestHierarchyDestruction(obj);
        return;
    }
    state[0x27] = value;
}

typedef struct Rgba8 {
    u8 r, g, b, a;
} Rgba8;

/// Prepare the current animation frame and color for rendering an attached effect.
/// \param object Attached effect object to render.
/// \returns The effect geometry display-list address.
/// \note Original decompilation identifier: \c func_80013530 (0x80013530).
s32 attachedEffectPrepareDraw(u8 *object) {
    u8 *v = *(u8 **)(object + 0x54);
    Rgba8 *c;

    textureLoadForDraw(&gRenderDisplayListCursor, 4, *(s16 *)(v + 0x14));
    c = (Rgba8 *)(v + 0x24);
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, c->r, c->g, c->b, c->a);
    return *(s32 *)(object + 0x30);
}
