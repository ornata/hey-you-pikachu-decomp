/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "object_lookup.h"
#include "heap_alloc.h"
#include "object_types.h"

/// Emit particles from a live emitter during its update.
/// \param emitter Borrowed emitter object with initialized state.
/// \returns Nothing.
typedef void (*EffectEmissionCallback)(u8 *emitter);

/// State owned by a timed, target-following effect emitter.
/// Targets are handles rather than retained object pointers. The callback must remain callable until destruction.
typedef struct EffectEmitterState {
    f32 remaining; ///< Remaining lifetime; nonpositive initial values disable automatic expiry.
    s32 targetHandle; ///< Object to follow, or an unavailable handle to use the fallback position.
    s32 referenceHandle; ///< Object defining the offset direction when both targets are available.
    f32 offset; ///< Signed horizontal distance from the target away from the reference.
    f32 cooldown; ///< Seconds until the emission callback may emit again.
    EffectEmissionCallback emit; ///< Optional per-update emission callback.
} EffectEmitterState;

typedef char EffectEmitterStateSizeCheck[(sizeof(EffectEmitterState) == 0x18) ? 1 : -1];
typedef char EffectEmitterCallbackOffsetCheck[((u32)&((EffectEmitterState *)0)->emit == 0x14) ? 1 : -1];

u32 randomBelow(s32);
void objectRequestHierarchyDestruction(u8 *);
u32 randomNext(void);
void animationIntegerTrackInitialize(u8 *, s32);
/// Default opacity animation used when wisp state is initialized.
/// \note Original identifier: \c D_8006B4C4 (0x8006B4C4).
extern s32 gWispDefaultOpacityAnimation;
void heapFreeLocal();
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;
void objectAdjustPitchAlongCameraView(u8 *);
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
s32 animationIntegerTrackEvaluate(void *, s32 *, f32);
void effectEmitterEmitColoredParticle(u8 *, f32, f32, f32, u8, u8, u8);
/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;
u8 *objectCreateFromTemplate(void *);
void objectMakeHierarchyRoot(u8 *);
u8 *objectRegisterActiveHierarchy(u8 *);
/// Object template for emitters of expanding orange effects.
/// \note Original identifier: \c D_80077E30 (0x80077E30).
extern u8 gOrangePulseEmitterObjectTemplate[];
u8 *objectCreateHierarchyFromTemplate(void *);
/// Hierarchy template for expanding orange effects.
/// \note Original identifier: \c D_80077E68 (0x80077E68).
extern u8 gExpandingOrangeEffectTemplate[];
/// Integer texture-frame animation used by expanding orange effects.
/// \note Original identifier: \c D_8006B3A8 (0x8006B3A8).
extern s32 gExpandingOrangeEffectFrameAnimation;
/// Framebuffer and command-buffer index for the current frame.
/// \note Original identifier: \c D_800F298C (0x800F298C).
extern s32 gGraphicsFramebufferIndex;
void textureLoadForDraw(Gfx **, s32, s32);
s32 wispEffectUpdateDrawAnimation(u8 *, s32);
u8 *wispEffectCreate(s32, f32, f32, f32);

typedef struct Color173 {
    u8 r, g, b, a;
} Color173;
typedef struct AnimatedBranches173 {
    s32 count;
    f32 speed;
    Gfx *lists[2];
    u8 *tracks;
    u8 alphaTrack[12];
    Color173 color;
} AnimatedBranches173;

/// Advance a wisp effect's animated appearance and prepare its selected drawing buffer.
/// \param effect Wisp effect with initialized animation tracks and drawing resources.
/// \param buffer Drawing buffer index, zero or one; the selected buffer must not be in use by rendering.
/// \returns One when the animation has finished and destruction is requested; zero while active.
/// \note Original decompilation identifier: \c func_80017300 (0x80017300).
s32 wispEffectUpdateDrawAnimation(u8 *effect, s32 buffer) {
    s32 rendererTable = 5;
    s32 trackStride = 12;
    s32 alpha;
    f32 step;
    s32 frame;
    Gfx *branch;
    s32 i;
    Gfx *commands;
    AnimatedBranches173 *state;

    state = *(AnimatedBranches173 **)(effect + 0x54);
    step = state->speed * gRuntimeFrameSeconds;
    if (animationIntegerTrackEvaluate(state->alphaTrack, &alpha, step) == 1) {
        objectRequestHierarchyDestruction(effect);
        return 1;
    }
    state->color.a = alpha;
    branch = state->lists[buffer];
    buffer = 0;
    commands = branch + state->count;
    i = 0;
    if (state->count > 0) {
        do {
            animationIntegerTrackEvaluate(state->tracks + buffer, &frame, step);
            gSPBranchList(branch++, commands);
            gDPPipeSync(commands++);
            gDPSetPrimColor(commands++, 0, 0, state->color.r, state->color.g, state->color.b, state->color.a);
            textureLoadForDraw(&commands, rendererTable, frame);
            gSPEndDisplayList(commands++);
            i++;
            buffer += trackStride;
        } while (i < state->count);
    }
    return 0;
}

void objectOrientAlongCameraView(u8 *); /* extern */

/// Keep a wisp effect facing the current view.
/// \param effect Live wisp effect whose orientation is updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800174B0 (0x800174B0).
void wispEffectUpdateFacing(u8 *effect) {
    objectOrientAlongCameraView(effect);
}


/// Keep a wisp effect facing the current view.
/// \param effect Live wisp effect whose orientation is updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800174D0 (0x800174D0).
void wispEffectUpdateFacingAlternate(u8 *effect) {
    objectOrientAlongCameraView(effect);
}


/// Animate a wisp that drifts upward, expands, and turns toward the view.
/// \param effect Live wisp effect to update.
/// \returns Nothing.
///
/// \note Portability: integer factors preserve the original IDO floating-point code generation.
/// \note Original decompilation identifier: \c func_800174F0 (0x800174F0).
void wispEffectUpdateDriftingMotion(u8 *effect) {
    *(f32 *)(effect + 24) += (1.0 - (u32)randomNext() * 2.3283064365386963e-10 * 2) * gRuntimeFrameSeconds;
    *(f32 *)(effect + 28) += ((u32)randomNext() * 2.3283064365386963e-10 * 6) * gRuntimeFrameSeconds;
    *(f32 *)(effect + 32) += (1.0 - (u32)randomNext() * 2.3283064365386963e-10 * 2) * gRuntimeFrameSeconds;
    *(f32 *)(effect + 0) += ((u32)randomNext() * 2.3283064365386963e-10 * 0.01f) * gRuntimeFrameSeconds;
    *(f32 *)(effect + 4) += ((u32)randomNext() * 2.3283064365386963e-10 * 0.01f) * gRuntimeFrameSeconds;
    *(f32 *)(effect + 8) += ((u32)randomNext() * 2.3283064365386963e-10 * 0.01f) * gRuntimeFrameSeconds;
    objectOrientAlongCameraView(effect);
    *(f32 *)(effect + 0xC) += 50.0f * *(f32 *)effect;
}


/// Move a wisp upward while keeping it facing the view.
/// \param effect Live wisp effect to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001777C (0x8001777C).
void wispEffectUpdateRisingMotion(u8 *effect) {
    ((RuntimeObjectPrefix *)effect)->position[1] =
        (f32)(((RuntimeObjectPrefix *)effect)->position[1] + (18.0f * gRuntimeFrameSeconds));
    objectOrientAlongCameraView(effect);
}

/// Prepare an active wisp effect for rendering.
/// \param effect Initialized wisp effect whose selected drawing buffer is available for writing.
/// \returns Address of the effect's display list, or zero when its animation has finished.
///
/// \note Requires the effect texture bank and an active display-list cursor with sufficient capacity.
/// \note Original decompilation identifier: \c func_800177B8 (0x800177B8).
s32 wispEffectPrepareDraw(u8 *effect) {
    u8 *obj;

    obj = *(u8 **)(effect + 0x54);
    if (wispEffectUpdateDrawAnimation(effect, gGraphicsFramebufferIndex) != 0) {
        return 0;
    }
    gSPSegment(gRenderDisplayListCursor++, 6, osVirtualToPhysical(((void **)(obj + 8))[gGraphicsFramebufferIndex]));
    return *(s32 *)(effect + 0x30);
}

/// Initialize a wisp effect's appearance and animation state.
/// \param effect Wisp effect to initialize; must not already own state.
/// \returns Nothing.
///
/// \note Portability: the grouped initial stores preserve the original IDO register allocation.
/// \note Original decompilation identifier: \c func_80017844 (0x80017844).
void wispEffectInitialize(u8 *effect) {
    u8 *allocation = heapAllocLocal(&gGameHeap, 0x24, 0x189);
    animationIntegerTrackInitialize(allocation + 0x14, gWispDefaultOpacityAnimation);
    do {
        allocation[0x20] = 0xFF;
        allocation[0x21] = 0xFF;
        allocation[0x22] = 0xFF;
    } while (0);
    allocation[0x23] = 0xFF;
    *(s32 *)(allocation + 0) = 0;
    *(s32 *)(allocation + 0xC) = 0;
    *(s32 *)(allocation + 8) = 0;
    *(s32 *)(allocation + 0x10) = 0;
    *(f32 *)(allocation + 4) = 1.0f;
    *(u8 **)(effect + 0x54) = allocation;
}

/// Release the resources owned by a wisp effect.
/// \param effect Wisp effect owning its initialized state and drawing resources; these become invalid after
/// release.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800178C4 (0x800178C4).
void wispEffectDestroy(u8 *effect) {
    u8 *effectState = *(u8 **)(effect + 0x54);
    heapFreeLocal(&gGameHeap, *(u8 **)(effectState + 8));
    heapFreeLocal(&gGameHeap, *(u8 **)(effectState + 0x10));
    heapFreeLocal(&gGameHeap, effectState);
}

/* Release-build debug print: compiled out, but cfe still emits its format
 * strings at the head of this unit's rodata. uopt still sees the dead blocks
 * when ranking ranges. */
#define DEBUG_PRINTF(args)                                                                                             \
    do {                                                                                                               \
        if (0) {                                                                                                       \
            osSyncPrintf args;                                                                                         \
        }                                                                                                              \
    } while (0)
/// Hierarchy templates indexed by wisp style.
/// \note Original identifier: \c D_80077EA0 (0x80077EA0).
extern void *gWispObjectTemplates[];
/// Number of texture-frame tracks required by each wisp style.
/// \note Original identifier: \c D_80077EC8 (0x80077EC8).
extern s32 gWispTextureTrackCounts[];
/// Shared opacity animation program index for each wisp style.
/// \note Original identifier: \c D_80077EF0 (0x80077EF0).
extern s32 gWispOpacityProgramIndices[];
/// Shared integer animation programs controlling effect and marker opacity.
/// \note Original identifier: \c D_8006B4B0 (0x8006B4B0).
extern s32 gOpacityAnimationPrograms[];
/// Integer texture-frame animation for wisp style 0.
/// \note Original identifier: \c D_8006B370 (0x8006B370).
extern s32 gWispStyle0FrameAnimation;
/// Integer texture-frame animation for wisp style 1 channel second.
/// \note Original identifier: \c D_8006B374 (0x8006B374).
extern s32 gWispStyle1SecondFrameAnimation;
/// Integer texture-frame animation for wisp style 1 channel first.
/// \note Original identifier: \c D_8006B378 (0x8006B378).
extern s32 gWispStyle1FirstFrameAnimation;
/// Integer texture-frame animation for wisp style 2.
/// \note Original identifier: \c D_8006B380 (0x8006B380).
extern s32 gWispStyle2FrameAnimation;
/// Integer texture animation program used by animated billboards.
/// \note Original identifier: \c D_8006B384 (0x8006B384).
extern s32 gBillboardTextureAnimation;
/// Integer texture-frame animation for wisp style 6.
/// \note Original identifier: \c D_8006B388 (0x8006B388).
extern s32 gWispStyle6FrameAnimation;
/// Integer texture-frame animation for wisp style 7.
/// \note Original identifier: \c D_8006B38C (0x8006B38C).
extern s32 gWispStyle7FrameAnimation;
/// Integer texture-frame animation for wisp style 3.
/// \note Original identifier: \c D_8006B3A0 (0x8006B3A0).
extern s32 gWispStyle3FrameAnimation;
/// Integer texture-frame animation for wisp style 8.
/// \note Original identifier: \c D_8006B3AC (0x8006B3AC).
extern s32 gWispStyle8FrameAnimation;
/// Integer texture-frame animation for wisp style 9.
/// \note Original identifier: \c D_8006B3B0 (0x8006B3B0).
extern s32 gWispStyle9FrameAnimation;
/// Integer texture-frame animation for wisp style 4.
/// \note Original identifier: \c D_8006B3B4 (0x8006B3B4).
extern s32 gWispStyle4FrameAnimation;

typedef struct {
    s32 count;
    f32 frame;
    Gfx *dl[2];
    u8 *tracks;
    u8 animation[12];
    u8 color[4];
} EffectState;

typedef struct {
    f32 scale[3];
    f32 rotation[3];
    f32 position[3];
    u8 unk24[0x20];
    f32 world[3];
    u8 unk50[4];
    EffectState *state;
} EffectObject;

/// Create an animated wisp effect of the selected style at a world position.
/// \param kind Wisp style from zero through nine.
/// \param x World X coordinate.
/// \param y World Y coordinate.
/// \param z World Z coordinate.
/// \returns Registered wisp effect, managed by the object system.
/// \note Original decompilation identifier: \c func_80017918 (0x80017918).
u8 *wispEffectCreate(s32 kind, f32 x, f32 y, f32 z) {
    EffectState *state;
    EffectObject *obj;
    s32 count;
    s32 commands;
    Gfx *gdl;
    s32 i;
    EffectObject *allocated;

    allocated = (EffectObject *)objectCreateHierarchyFromTemplate(gWispObjectTemplates[kind]);
    allocated->position[0] = x;
    allocated->world[0] = x;
    allocated->position[1] = y;
    allocated->world[1] = y;
    allocated->position[2] = z;
    allocated->world[2] = z;
    state = allocated->state;
    obj = allocated;
    count = gWispTextureTrackCounts[kind];
    state->count = count;
    commands = count * 20;
    state->dl[0] = (Gfx *)heapAllocLocal(&gGameHeap, commands * 16, 0x1FF);
    state->dl[1] = state->dl[0] + commands;
    gdl = state->dl[0];
    i = 0;
    while (i < (u32)(commands * 2)) {
        i++;
        gSPEndDisplayList(gdl++);
    }
    state->tracks = heapAllocLocal(&gGameHeap, count * 12, 0x20A);
    animationIntegerTrackInitialize(state->animation, gOpacityAnimationPrograms[gWispOpacityProgramIndices[kind]]);
    switch (kind) {
    case 0:
        obj->scale[0] = 0.01f;
        obj->scale[1] = 0.01f;
        obj->scale[2] = 0.01f;
        animationIntegerTrackInitialize(state->tracks, gWispStyle0FrameAnimation);
        break;
    case 1:
        obj->scale[0] = 0.01f;
        obj->scale[1] = 0.01f;
        obj->scale[2] = 0.01f;
        animationIntegerTrackInitialize(state->tracks, gWispStyle1FirstFrameAnimation);
        animationIntegerTrackInitialize(state->tracks + 0xC, gWispStyle1SecondFrameAnimation);
        break;
    case 2:
        obj->scale[0] = 0.01f;
        obj->scale[1] = 0.01f;
        obj->scale[2] = 0.01f;
        animationIntegerTrackInitialize(state->tracks, gWispStyle2FrameAnimation);
        state->color[2] = 0;
        state->color[1] = 0;
        state->color[0] = 0;
        break;
    case 3:
        obj->scale[0] = 0.1f;
        obj->scale[1] = (f64)randomNext() * 2.3283064365386963e-10 * 0.10000000149011612 + 0.05000000074505806;
        obj->scale[2] = (f64)randomNext() * 2.3283064365386963e-10 * 0.05000000074505806 + 0.05000000074505806;
        animationIntegerTrackInitialize(state->tracks, gWispStyle3FrameAnimation);
        state->color[0] = 0xFF;
        state->color[1] = 0xFF;
        state->color[2] = 0x99;
        break;
    case 4:
        obj->scale[0] = 0.003f;
        obj->scale[1] = 0.003f;
        obj->scale[2] = 0.003f;
        animationIntegerTrackInitialize(state->tracks, gWispStyle4FrameAnimation);
        break;
    case 5:
        obj->scale[0] = 0.02f;
        obj->scale[1] = 0.02f;
        obj->scale[2] = 0.02f;
        animationIntegerTrackInitialize(state->tracks, gBillboardTextureAnimation);
        break;
    case 6:
        obj->scale[0] = 0.03f;
        obj->scale[1] = 0.03f;
        obj->scale[2] = 0.03f;
        animationIntegerTrackInitialize(state->tracks, gWispStyle6FrameAnimation);
        break;
    case 7:
        obj->scale[0] = 0.03f;
        obj->scale[1] = 0.03f;
        obj->scale[2] = 0.03f;
        animationIntegerTrackInitialize(state->tracks, gWispStyle7FrameAnimation);
        break;
    case 8:
        obj->scale[0] = 0.2f;
        obj->scale[1] = 0.2f;
        obj->scale[2] = 0.2f;
        animationIntegerTrackInitialize(state->tracks, gWispStyle8FrameAnimation);
        break;
    case 9:
        obj->scale[0] = 0.2f;
        obj->scale[1] = 0.2f;
        obj->scale[2] = 0.2f;
        animationIntegerTrackInitialize(state->tracks, gWispStyle9FrameAnimation);
        break;
    default:
        DEBUG_PRINTF(("Unknown smokeType(%d)\n", kind));
        break;
    }
    DEBUG_PRINTF(("wispType:%d\n", kind));
    objectRegisterActiveHierarchy((u8 *)obj);
    return (u8 *)obj;
}
/// Texture-frame animation shared by ribbon controllers and particle-cluster renderers.
/// \note Original identifier: \c D_8006B37C (0x8006B37C).
extern s32 gRibbonAndClusterFrameAnimation;

/// Owned steering target, lifetime, ground-contact state and appearance track.
typedef struct RibbonControllerState {
    f32 destination[3];
    f32 remaining;
    s32 steeringFrame;
    s32 grounded;
    u8 animation[12];
} RibbonControllerState;

typedef char RibbonControllerStateSizeCheck[(sizeof(RibbonControllerState) == 0x24) ? 1 : -1];
typedef char RibbonAnimationOffsetCheck[((u32) & ((RibbonControllerState *)0)->animation == 0x18) ? 1 : -1];

/// Initialize a ribbon effect's motion and animation state.
/// \param controller Ribbon effect controller to initialize; must not already own state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80017D18 (0x80017D18).
void ribbonEffectControllerInitialize(u8 *controller) {
    u8 *sp1C;
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x24, 0x25F);
    sp1C = allocation;
    ((RibbonControllerState *)allocation)->steeringFrame = 0;
    ((RibbonControllerState *)allocation)->grounded = 0;
    ((RibbonControllerState *)allocation)->destination[2] = 0.0f;
    ((RibbonControllerState *)allocation)->destination[1] = 0.0f;
    ((RibbonControllerState *)allocation)->destination[0] = 0.0f;
    animationIntegerTrackInitialize(allocation + 0x18, gRibbonAndClusterFrameAnimation);
    ((RuntimeObjectPrefix *)controller)->state.bytes = sp1C;
}

extern Vtx D_100CA78[];
/// Create the drawing resources for a ribbon effect.
/// \param count Nonnegative number of intermediate ribbon segments; allocation size and matrix offsets must
/// fit in 32 bits.
/// \returns Owned display list, to be released with the ribbon controller.
/// \note Original decompilation identifier: \c func_80017D80 (0x80017D80).
Gfx *ribbonEffectCreateDisplayList(s32 count) {
    Gfx *head;
    Gfx *g;
    s32 i;
    f32 alphaRamp;
    u8 alpha;
    count += 2;
    head = (Gfx *)heapAllocLocal(&gGameHeap, (count * 6 + 30) * sizeof(Gfx), 0x27C);
    g = head;
    gDPPipeSync(g++);
    gSPGeometryMode(g++, 0xFFFFFF, 5);
    gDPSetRenderMode(g++, 0x0C184A50, 0);
    gDPSetCombine(g++, 0x30B3FF, 0x4FFE7E38);
    gDPSetTextureLOD(g++, G_TL_TILE);
    gDPSetTextureDetail(g++, G_TD_CLAMP);
    gSPTexture(g++, 0x7FFF, 0, 0, 0, G_ON);
    gMoveWd(g++, G_MW_FOG, 0x10, 0x7FFF);
    gSPMatrix(g++, 0x07000040, G_MTX_LOAD);
    gSPVertex(g++, D_100CA78, 2, 0);
    i = 1;
    if (count >= 2) {
        do {
            gSPMatrix(g++, (i << 6) + 0x07000040, G_MTX_LOAD);
            alphaRamp = i * 239.0f / count + 16.0f;
            alpha = (u32)alphaRamp;
            gDPSetPrimColor(g++, 0, 0, 255, 255, 255, alpha);
            if (i & 1) {
                gSPVertex(g++, D_100CA78, 2, 2);
                gSPModifyVertex(g++, 0, G_MWO_POINT_ST, 0);
                gSPModifyVertex(g++, 1, G_MWO_POINT_ST, 0x04000000);
                gSP2Triangles(g++, 0, 1, 2, 0, 1, 3, 2, 0);
            } else {
                gSPVertex(g++, D_100CA78, 2, 0);
                gSPModifyVertex(g++, 2, G_MWO_POINT_ST, 0);
                gSPModifyVertex(g++, 3, G_MWO_POINT_ST, 0x04000000);
                gSP2Triangles(g++, 2, 3, 0, 0, 3, 1, 0, 0);
            }
            i++;
        } while (i != count);
    }
    gSPTexture(g++, 0x7FFF, 0, 0, 0, G_OFF);
    gMoveWd(g++, G_MW_FOG, 0x10, 0x7FFF);
    gSPEndDisplayList(g++);
    return head;
}

/// Provide a no-operation hook for a ribbon effect.
/// \param unused Unused hook argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800180F4 (0x800180F4).
void ribbonEffectNoOp(s32 unused) {
}

/// Initialize a ribbon effect's anchor state.
/// \param anchor Ribbon effect anchor to initialize; must not already own state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800180FC (0x800180FC).
void ribbonEffectAnchorInitialize(void *anchor) {
    s32 *anchorState;

    anchorState = heapAllocLocal(&gGameHeap, 4, 0x2D1);
    *anchorState = 0;
    ((RuntimeObjectPrefix *)anchor)->state.words = anchorState;
}

/// Release the resources owned by a ribbon effect controller.
/// \param controller Ribbon effect controller with owned drawing resources and state; these become invalid
/// after release.
/// \returns Nothing.
///
/// \note Portability: the unprototyped deallocator call passes an extra argument ignored by the original MIPS
/// ABI.
/// \note Original decompilation identifier: \c func_80018138 (0x80018138).
void ribbonEffectControllerDestroy(u8 *controller) {
    s32 sp1C;

    sp1C = *(s32 *)(controller + 0x54);
    heapFreeLocal(&gGameHeap, *(s32 *)(controller + 0x30), controller);
    heapFreeLocal(&gGameHeap, sp1C);
}

void vector3GetZAxisAnglesDegrees(f32 *, f32 *, f32 *);
f32 mathLerpAngleDegrees(f32, f32, f32);
f32 objectGetHeightAboveGround(u8 *);

/// Advance a ribbon effect's steering, ground-contact effects, animation, and lifetime.
/// \param controller Initialized ribbon controller with its live motion child, tip sibling, and hierarchy
/// root.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001817C (0x8001817C).
void ribbonEffectControllerUpdate(u8 *controller) {
    u8 *v;
    u8 *src;
    s32 tmp;
    f32 d[3];
    f32 yaw;
    f32 pitch;
    f32 h;

    v = *(u8 **)(controller + 0x54);
    animationIntegerTrackEvaluate(v + 0x18, &tmp, gRuntimeFrameSeconds);
    src = *(u8 **)(controller + 0x2C);
    *(f32 *)(controller + 0xC) = *(f32 *)(src + 0xC);
    *(f32 *)(controller + 0x10) = *(f32 *)(src + 0x10);
    *(f32 *)(controller + 0x14) = *(f32 *)(src + 0x14);
    *(f32 *)(controller + 0x18) = *(f32 *)(src + 0x44);
    *(f32 *)(controller + 0x1C) = *(f32 *)(src + 0x48);
    *(f32 *)(controller + 0x20) = *(f32 *)(src + 0x4C);
    objectAdjustPitchAlongCameraView(controller);
    d[0] = *(f32 *)(v + 0) - *(f32 *)(controller + 0x18);
    d[1] = *(f32 *)(v + 4) - *(f32 *)(controller + 0x1C);
    d[2] = *(f32 *)(v + 8) - *(f32 *)(controller + 0x20);
    if (++*(s32 *)(v + 0x10) >= 5) {
        *(s32 *)(v + 0x10) = 0;
        *(f32 *)(src + 0x10) = (s32)randomBelow(360);
        *(f32 *)(src + 0x14) = (s32)randomBelow(360);
    } else {
        vector3GetZAxisAnglesDegrees(d, &yaw, &pitch);
        *(f32 *)(src + 0x10) = mathLerpAngleDegrees(*(f32 *)(src + 0x10), yaw, 0.3f);
        *(f32 *)(src + 0x14) = mathLerpAngleDegrees(*(f32 *)(src + 0x14), pitch, 0.3f);
    }
    *(f32 *)(*(u8 **)(controller + 0x28) + 0x18) = *(f32 *)(controller + 0x18);
    *(f32 *)(*(u8 **)(controller + 0x28) + 0x1C) = *(f32 *)(controller + 0x1C);
    *(f32 *)(*(u8 **)(controller + 0x28) + 0x20) = *(f32 *)(controller + 0x20);
    h = objectGetHeightAboveGround(controller);
    if (h < 1.2000000476837158f) {
        if (*(s32 *)(v + 0x14) == 0) {
            if (h < -1.0f) {
                h = 0.0f;
            }
            wispEffectCreate(4, *(f32 *)(controller + 0x44), *(f32 *)(controller + 0x48) - h, *(f32 *)(controller + 0x4C));
        }
        *(s32 *)(v + 0x14) = 1;
        *(f32 *)(v + 0xC) -= gRuntimeFrameSeconds;
    } else {
        *(s32 *)(v + 0x14) = 0;
    }
    *(f32 *)(v + 0xC) -= gRuntimeFrameSeconds;
    if (*(f32 *)(v + 0xC) < 0.0f) {
        objectRequestHierarchyDestruction(*(u8 **)(controller + 0x50));
    }
}


/// Keep a ribbon effect's tip facing the current view.
/// \param tip Ribbon effect tip to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800183A8 (0x800183A8).
void ribbonEffectTipUpdateFacing(u8 *tip) {
    objectOrientAlongCameraView(tip);
}

/// Align a mirrored ribbon segment with the next segment and the current view.
/// \param segment Ribbon effect segment with a live next segment.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800183C8 (0x800183C8).
void ribbonEffectSegmentUpdateMirrored(u8 *segment) {
    u8 *nextSegment;

    nextSegment = *(u8 **)(segment + 0x28);
    *(f32 *)(segment + 0x18) = *(f32 *)(nextSegment + 0x18);
    *(f32 *)(segment + 0x1C) = *(f32 *)(nextSegment + 0x1C);
    *(f32 *)(segment + 0x20) = *(f32 *)(nextSegment + 0x20);
    *(f32 *)(segment + 0xC) = *(f32 *)(nextSegment + 0xC);
    *(f32 *)(segment + 0x10) = *(f32 *)(nextSegment + 0x10);
    *(f32 *)(segment + 0x14) = *(f32 *)(nextSegment + 0x14);
    *(f32 *)(segment + 8) = -*(f32 *)(segment + 8);
    objectAdjustPitchAlongCameraView(segment);
}

/// Align a ribbon segment with the next segment and the current view.
/// \param segment Ribbon effect segment with a live next segment.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80018424 (0x80018424).
void ribbonEffectSegmentUpdate(u8 *segment) {
    u8 *nextSegment;

    nextSegment = ((RuntimeObjectPrefix *)segment)->sibling.bytes;
    ((RuntimeObjectPrefix *)segment)->position[0] = (f32)((RuntimeObjectPrefix *)nextSegment)->position[0];
    ((RuntimeObjectPrefix *)segment)->position[1] = (f32)((RuntimeObjectPrefix *)nextSegment)->position[1];
    ((RuntimeObjectPrefix *)segment)->position[2] = (f32)((RuntimeObjectPrefix *)nextSegment)->position[2];
    ((RuntimeObjectPrefix *)segment)->rotation[0] = (f32)((RuntimeObjectPrefix *)nextSegment)->rotation[0];
    ((RuntimeObjectPrefix *)segment)->rotation[1] = (f32)((RuntimeObjectPrefix *)nextSegment)->rotation[1];
    ((RuntimeObjectPrefix *)segment)->rotation[2] = (f32)((RuntimeObjectPrefix *)nextSegment)->rotation[2];
    objectAdjustPitchAlongCameraView(segment);
}

/// Prepare a ribbon effect for drawing with its current animated appearance.
/// \param controller Ribbon effect controller to draw.
/// \returns Address of the ribbon's display list.
///
/// \note Requires initialized effect resources, valid active matrix buffers where used, and an active
/// display-list cursor with sufficient capacity. Drawing resources remain owned by the effect and must stay
/// live until rendering finishes. The effect texture bank must be loaded.
/// \note Original decompilation identifier: \c func_80018474 (0x80018474).
s32 ribbonEffectControllerPrepareDraw(u8 *controller) {
    u8 *obj;
    Gfx *g;
    s32 idx;

    obj = *(u8 **)(controller + 0x54);
    g = gRenderDisplayListCursor++;
    g->words.w0 = 0xDB06001C;
    idx = (*(u16 *)(*(u8 **)(controller + 0x50) + 0x36) & 0x8000) ? 1 : 0;
    g->words.w1 = (u32)osVirtualToPhysical(((void **)(*(u8 **)(controller + 0x50) + 0x3C))[idx]);
    textureLoadForDraw(&gRenderDisplayListCursor, 5, *(s16 *)(obj + 0x20));
    return *(s32 *)(controller + 0x30);
}

u8 *objectCreateDefault(void);
void objectAllocateHierarchyMatrixBuffers(u8 *);
f32 mathSinDegrees(f32);
Gfx *ribbonEffectCreateDisplayList(s32);
/// Object template for the ribbon hierarchy anchor.
/// \note Original identifier: \c D_80077F18 (0x80077F18).
extern u8 gRibbonAnchorObjectTemplate[];
/// Object template for intermediate ribbon segments.
/// \note Original identifier: \c D_80077F50 (0x80077F50).
extern u8 gRibbonSegmentObjectTemplate[];
/// Object template for the ribbon destination and lifetime controller.
/// \note Original identifier: \c D_80077F88 (0x80077F88).
extern u8 gRibbonControllerObjectTemplate[];
/// Object template for the ribbon tip supplying its moving orientation.
/// \note Original identifier: \c D_80077FC0 (0x80077FC0).
extern u8 gRibbonTipObjectTemplate[];
/// Object template for the scaled decoration attached at the ribbon end.
/// \note Original identifier: \c D_80077C70 (0x80077C70).
extern u8 gRibbonTerminalDecorationTemplate[];
/// Create a moving ribbon effect that steers toward a world position.
/// \param count Nonnegative number of intermediate ribbon segments; drawing and matrix allocation sizes must
/// fit in 32 bits.
/// \param duration Effect lifetime in seconds.
/// \param baseWidth Base ribbon width scale.
/// \param widthVariation Amount of variation in the ribbon width.
/// \param widthStyle Width variation style: zero for waves or one for random variation.
/// \param x Initial world X coordinate.
/// \param y Initial world Y coordinate.
/// \param z Initial world Z coordinate.
/// \param pitch Initial motion-node Y-axis rotation in degrees.
/// \param yaw Initial motion-node Z-axis rotation in degrees.
/// \param destination Readable three-component world position toward which the ribbon steers; copied during
/// creation.
/// \returns Registered ribbon hierarchy root, managed by the object system.
/// \note Original decompilation identifier: \c func_80018514 (0x80018514).
u8 *ribbonEffectCreate(s32 count, f32 duration, f32 baseWidth, f32 widthVariation, s32 widthStyle, f32 x, f32 y, f32 z, f32 pitch, f32 yaw,
                  f32 *destination) {
    u8 *root;
    Gfx *displayList;
    u8 *head;
    u8 *tail;
    u8 *part;
    f32 *state;
    s32 i;
    root = objectCreateDefault();
    head = objectCreateFromTemplate(gRibbonAnchorObjectTemplate);
    *(f32 *)(head + 0x18) = x;
    *(f32 *)(head + 0x1C) = y;
    *(f32 *)(head + 0x20) = z;
    *(f32 *)(head + 0x44) = x;
    *(f32 *)(head + 0x48) = y;
    *(f32 *)(head + 0x4C) = z;
    *(f32 *)(head + 8) = baseWidth;
    *(u8 **)(root + 0x2C) = head;
    tail = head;
    for (i = 0; i < count; i++) {
        part = objectCreateFromTemplate(gRibbonSegmentObjectTemplate);
        *(f32 *)(part + 0x18) = x;
        *(f32 *)(part + 0x1C) = y;
        *(f32 *)(part + 0x20) = z;
        *(f32 *)(part + 0x44) = x;
        *(f32 *)(part + 0x48) = y;
        *(f32 *)(part + 0x4C) = z;
        *(u8 **)(tail + 0x28) = part;
        tail = part;
        switch (widthStyle) {
        case 0:
            *(f32 *)(part + 8) = mathSinDegrees(i * 30) * widthVariation + baseWidth;
            break;
        case 1:
            *(f32 *)(part + 8) =
                (1.0 - ((f64)randomNext() * 2.3283064365386963e-10) * 2) * (f64)widthVariation + (f64)baseWidth;
            break;
        }
        if (i & 1)
            *(f32 *)(part + 8) = -*(f32 *)(part + 8);
    }
    part = objectCreateFromTemplate(gRibbonControllerObjectTemplate);
    state = *(f32 **)(part + 0x54);
    *(f32 *)(part + 0x18) = x;
    *(f32 *)(part + 0x1C) = y;
    *(f32 *)(part + 0x20) = z;
    *(f32 *)(part + 0x44) = x;
    *(f32 *)(part + 0x48) = y;
    *(f32 *)(part + 0x4C) = z;
    *(f32 *)(part + 8) = baseWidth;
    *(u8 **)(tail + 0x28) = part;
    tail = part;
    state[0] = destination[0];
    state[1] = destination[1];
    state[2] = destination[2];
    state[3] = duration;
    **(u8 ***)(head + 0x54) = part;
    displayList = ribbonEffectCreateDisplayList(count);
    *(Gfx **)(part + 0x30) = displayList;
    head = objectCreateFromTemplate(gRibbonTipObjectTemplate);
    *(f32 *)(head + 0x44) = x;
    *(f32 *)(head + 0x48) = y;
    *(f32 *)(head + 0x4C) = z;
    *(f32 *)(head + 0xC) = 0.0f;
    *(f32 *)(head + 0x10) = pitch;
    *(f32 *)(head + 0x14) = yaw;
    *(u8 **)(part + 0x2C) = head;
    part = objectCreateFromTemplate(gRibbonTerminalDecorationTemplate);
    *(f32 *)(part) = baseWidth * 0.01f;
    *(f32 *)(part + 4) = baseWidth * 0.01f;
    *(f32 *)(part + 8) = baseWidth * 0.01f;
    *(u8 **)(tail + 0x28) = part;
    objectMakeHierarchyRoot(root);
    objectAllocateHierarchyMatrixBuffers(root);
    objectRegisterActiveHierarchy(root);
    return root;
}

extern Vtx D_100CA98[];
/// Create the drawing resources for a particle cluster.
/// \param count Nonnegative number of particles; allocation size and matrix offsets must fit in 32 bits.
/// \returns Address of an owned display list, to be released with its renderer.
///
/// \note Portability: paired texture setup commands preserve the original IDO scheduling.
/// \note Original decompilation identifier: \c func_80018838 (0x80018838).
s32 particleClusterCreateDisplayList(s32 count) {
    Gfx *base;
    Gfx *dl;
    s32 i;
    base = (Gfx *)heapAllocLocal(&gGameHeap, count * 3 * sizeof(Gfx) + 240, 0x427);
    dl = base;
    gDPPipeSync(dl++);
    gSPGeometryMode(dl++, -1, 0x200005);
    gDPSetRenderMode(dl++, 0x0C184A50, 0);
    gDPSetCombine(dl++, 0x3099FF, 0x4FFE7E38);
    gDPSetPrimColor(dl++, 0, 0, 255, 255, 255, 255);
    gDPSetTextureLOD(dl++, G_TL_TILE);
    gDPSetTextureDetail(dl++, G_TD_CLAMP);

    // clang-format off
    gSPTexture(dl++, 0x7FFF, 0, 0, 0, G_ON); gMoveWd(dl++, G_MW_FOG, 0x10, 0x7FFF);
    // clang-format on
    for (i = 0; i < count; i++) {
        gSPMatrix(dl++, (Mtx *)(0x07000040 + (i << 6)), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPVertex(dl++, D_100CA98, 4, 0);
        gSP2Triangles(dl++, 0, 1, 2, 0, 1, 3, 2, 0);
    }
    gSPTexture(dl++, 0x7FFF, 0, 0, 0, G_OFF);
    gMoveWd(dl++, G_MW_FOG, 0x10, 0x7FFF);
    gSPEndDisplayList(dl++);
    return (s32)base;
}

/// Emit black wisps when the emitter is ready.
/// \param emitter Initialized emitter supplying the origin and emission timing.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80018B18 (0x80018B18).
void effectEmitterEmitBlackWisps(u8 *emitter) {
    u8 *obj;

    obj = *(u8 **)(emitter + 0x54);
    if (((EffectEmitterState *)obj)->cooldown == 0.0f) {
        ((EffectEmitterState *)obj)->cooldown =
            (f32)(((f64)randomNext() * 2.3283064365386963e-10 * 0.10000000149011612) + 0.10000000149011612);
        wispEffectCreate(2, *(f32 *)(emitter + 0x44), *(f32 *)(emitter + 0x48), *(f32 *)(emitter + 0x4C));
    }
}

/// Emit white wisps when the emitter is ready.
/// \param emitter Initialized emitter supplying the origin and emission timing.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80018BC0 (0x80018BC0).
void effectEmitterEmitWhiteWisps(u8 *emitter) {
    u8 *obj;

    obj = *(u8 **)(emitter + 0x54);
    if (((EffectEmitterState *)obj)->cooldown == 0.0f) {
        ((EffectEmitterState *)obj)->cooldown =
            (f32)(((f64)randomNext() * 2.3283064365386963e-10 * 0.6000000238418579) + 0.800000011920929);
        wispEffectCreate(5, *(f32 *)(emitter + 0x44), *(f32 *)(emitter + 0x48), *(f32 *)(emitter + 0x4C));
    }
}

/// Emit scattered particles when the emitter is ready.
/// \param emitter Initialized effect emitter to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80018C6C (0x80018C6C).
void effectEmitterEmitScatteredParticles(u8 *emitter) {
    u8 *v;
    s32 n;

    v = *(u8 **)(emitter + 0x54);
    if (((EffectEmitterState *)v)->cooldown == 0.0f) {
        ((EffectEmitterState *)v)->cooldown =
            (f64)randomNext() * 2.3283064365386963e-10 * 0.009999999776482582 + 0.009999999776482582;
        for (n = randomBelow(3); n != 0; n--) {
            wispEffectCreate(3, *(f32 *)(emitter + 0x44) + ((f64)randomNext() * 2.3283064365386963e-10 - 0.5) * 5.0,
                          *(f32 *)(emitter + 0x48),
                          *(f32 *)(emitter + 0x4C) + ((f64)randomNext() * 2.3283064365386963e-10 - 0.5) * 5.0);
        }
    }
}

s32 objectPlaySpatialSound(u8 *, s32, f32, f32);
void audioSetSoundEffectPitch(s32, f32);
u8 *fallingParticleCreate(s32, s32, f32 *, f32 *, f32);

/// Emit a falling-particle burst with accompanying sound when the emitter is ready.
/// \param emitter Initialized effect emitter to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80018E00 (0x80018E00).
void effectEmitterEmitAudibleBurst(u8 *emitter) {
    u8 *state;
    s32 n;
    s32 count;
    s32 sound;
    f32 direction[3];
    s32 kind;
    s32 i;

    state = *(u8 **)(emitter + 0x54);
    if (((EffectEmitterState *)state)->cooldown == 0.0f) {
        n = randomBelow(3);
        ((EffectEmitterState *)state)->cooldown =
            (f64)randomNext() * 2.3283064365386963e-10 * 0.009999999776482582 + 0.009999999776482582;
        count = n + 1;
        sound = objectPlaySpatialSound(emitter, 0xA3, (f32)count * 20.0f, (f32)count * 100.0f);
        if (sound != 0) {
            audioSetSoundEffectPitch(sound, ((f64)randomNext() * 2.3283064365386963e-10 - 0.5) * 3.0);
        }
        i = n;
        if (count != 1) {
            state = emitter + 0x44;
            do {
                direction[0] = ((f64)randomNext() * 2.3283064365386963e-10 - 0.5) * 15.0;
                direction[1] = (f64)randomNext() * 2.3283064365386963e-10 * 5.0 + 8.0;
                direction[2] = ((f64)randomNext() * 2.3283064365386963e-10 - 0.5) * 15.0;
                if (randomBelow(2) != 0) {
                    kind = 0;
                } else {
                    kind = 1;
                }
                fallingParticleCreate(6, kind, (f32 *)state, direction,
                              (f64)randomNext() * 2.3283064365386963e-10 * 0.4000000059604645 + 1.600000023841858);
            } while (--i != 0);
        }
    }
}

/// Emit a colored particle near the emitter when it is ready.
/// \param emitter Initialized effect emitter providing the origin and emission timing.
/// \param intervalRange Range of random variation in the emission interval, in seconds.
/// \param minimumInterval Minimum interval between emissions, in seconds.
/// \param spread Spatial spread of emitted particles.
/// \param red Red color component.
/// \param green Green color component.
/// \param blue Blue color component.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800190FC (0x800190FC).
void effectEmitterEmitColoredParticle(u8 *emitter, f32 intervalRange, f32 minimumInterval, f32 spread, u8 red, u8 green, u8 blue) {
    u8 *v;
    u8 *e;
    f32 half;

    v = *(u8 **)(emitter + 0x54);
    if (((EffectEmitterState *)v)->cooldown == 0.0f) {
        ((EffectEmitterState *)v)->cooldown = (f64)randomNext() * 2.3283064365386963e-10 * intervalRange + minimumInterval;
        half = 0.5f * spread;
        e = wispEffectCreate(2, *(f32 *)(emitter + 0x44) + ((f64)randomNext() * 2.3283064365386963e-10 - 0.5) * spread,
                          *(f32 *)(emitter + 0x48) + (f64)randomNext() * 2.3283064365386963e-10 * half,
                          *(f32 *)(emitter + 0x4C) + ((f64)randomNext() * 2.3283064365386963e-10 - 0.5) * spread);
        v = *(u8 **)(e + 0x54);
        v[0x20] = red;
        v[0x21] = green;
        v[0x22] = blue;
        *(f32 *)(e + 8) = 0.0f;
        *(f32 *)(e + 4) = 0.0f;
        *(f32 *)(e + 0) = 0.0f;
    }
}

/// Emit pale green particles when the emitter is ready.
/// \param emitter Initialized effect emitter to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800192F0 (0x800192F0).
void effectEmitterEmitPaleGreenParticles(u8 *emitter) {
    effectEmitterEmitColoredParticle(emitter, 0.1f, 0.03f, 1.0f, 0xB7, 0xFF, 0xC1);
}

/// Emit yellow particles when the emitter is ready.
/// \param emitter Initialized effect emitter to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80019338 (0x80019338).
void effectEmitterEmitYellowParticles(u8 *emitter) {
    effectEmitterEmitColoredParticle(emitter, 0.1f, 0.03f, 1.0f, 0xFF, 0xFF, 0);
}

/// Emit pink particles when the emitter is ready.
/// \param emitter Initialized effect emitter to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001937C (0x8001937C).
void effectEmitterEmitPinkParticles(u8 *emitter) {
    effectEmitterEmitColoredParticle(emitter, 0.1f, 0.03f, 1.0f, 0xE0, 0x44, 0x77);
}

/// Emit gray particles when the emitter is ready.
/// \param emitter Initialized effect emitter to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800193C4 (0x800193C4).
void effectEmitterEmitGrayParticles(u8 *emitter) {
    effectEmitterEmitColoredParticle(emitter, 0.15f, 0.04f, 1.0f, 0xE0, 0xE0, 0xE0);
}

void effectEmitterCreateWithCallback(f32 *, f32, s32, s32, f32, EffectEmissionCallback);
void effectEmitterEmitAudibleBurst(u8 *);

/// Create a target-following effect emitter with the selected visual style.
/// \param kind Emission style from zero through seven; other values create no emitter.
/// \param position Readable three-component fallback world position, copied when the target is unavailable.
/// \param lifetime Lifetime in seconds; nonpositive values disable automatic expiry.
/// \param targetHandle Target object to follow, if available.
/// \param referenceHandle Object defining the horizontal offset direction, if available.
/// \param offset Signed horizontal offset from the target, positive away from the reference object; used when
/// both objects are available.
/// \returns Nothing.
///
/// \note NONPORTABLE: the callback is passed as s32 (32-bit function pointers).
/// \note Original decompilation identifier: \c func_8001940C (0x8001940C).
void effectEmitterCreateForStyle(u32 kind, f32 *position, f32 lifetime, s32 targetHandle, s32 referenceHandle, f32 offset) {
    switch (kind) {
    case 0:
        effectEmitterCreateWithCallback(position, lifetime, targetHandle, referenceHandle, offset, effectEmitterEmitBlackWisps);
        break;
    case 1:
        effectEmitterCreateWithCallback(position, lifetime, targetHandle, referenceHandle, offset, effectEmitterEmitPaleGreenParticles);
        break;
    case 2:
        effectEmitterCreateWithCallback(position, lifetime, targetHandle, referenceHandle, offset, effectEmitterEmitYellowParticles);
        break;
    case 3:
        effectEmitterCreateWithCallback(position, lifetime, targetHandle, referenceHandle, offset, effectEmitterEmitPinkParticles);
        break;
    case 4:
        effectEmitterCreateWithCallback(position, lifetime, targetHandle, referenceHandle, offset, effectEmitterEmitGrayParticles);
        break;
    case 5:
        effectEmitterCreateWithCallback(position, lifetime, targetHandle, referenceHandle, offset, effectEmitterEmitWhiteWisps);
        break;
    case 6:
        effectEmitterCreateWithCallback(position, lifetime, targetHandle, referenceHandle, offset, effectEmitterEmitScatteredParticles);
        break;
    case 7:
        effectEmitterCreateWithCallback(position, lifetime, targetHandle, referenceHandle, offset, effectEmitterEmitAudibleBurst);
        break;
    }
}

/// Object template for callback-driven particle emitters.
/// \note Original identifier: \c D_80077FF8 (0x80077FF8).
extern u8 gEffectEmitterObjectTemplate[];

/// Create a target-following effect emitter with a custom emission callback.
/// \param position Readable three-component fallback world position, copied when the target is unavailable.
/// \param lifetime Lifetime in seconds; nonpositive values disable automatic expiry.
/// \param targetHandle Target object to follow, if available.
/// \param referenceHandle Object defining the horizontal offset direction, if available.
/// \param offset Signed horizontal offset from the target, positive away from the reference object; used when
/// both objects are available.
/// \param callback Emission callback, or null to disable emissions;
/// must remain callable throughout the emitter lifetime.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_800195B0 (0x800195B0).
void effectEmitterCreateWithCallback(f32 *position, f32 lifetime, s32 targetHandle, s32 referenceHandle, f32 offset, EffectEmissionCallback callback) {
    u8 *o = objectCreateFromTemplate(gEffectEmitterObjectTemplate);
    u8 *v = *(u8 **)(o + 0x54);
    u8 *t;

    ((EffectEmitterState *)v)->remaining = lifetime;
    ((EffectEmitterState *)v)->targetHandle = targetHandle;
    ((EffectEmitterState *)v)->referenceHandle = referenceHandle;
    ((EffectEmitterState *)v)->offset = offset;
    ((EffectEmitterState *)v)->emit = callback;
    ((EffectEmitterState *)v)->cooldown = 0.0f;
    t = objectResolveAvailableHandle(targetHandle);
    if (t != NULL) {
        *(f32 *)(o + 0x18) = *(f32 *)(t + 0x44);
        *(f32 *)(o + 0x1C) = *(f32 *)(t + 0x48);
        *(f32 *)(o + 0x20) = *(f32 *)(t + 0x4C);
    } else {
        *(f32 *)(o + 0x18) = position[0];
        *(f32 *)(o + 0x1C) = position[1];
        *(f32 *)(o + 0x20) = position[2];
    }
    *(f32 *)(o + 0x44) = *(f32 *)(o + 0x18);
    *(f32 *)(o + 0x48) = *(f32 *)(o + 0x1C);
    *(f32 *)(o + 0x4C) = *(f32 *)(o + 0x20);
    objectMakeHierarchyRoot(o);
    objectRegisterActiveHierarchy(o);
}

void _bzero(void *, s32); /* extern */

/// Initialize the state of a timed effect emitter.
/// \param emitter Effect emitter to initialize; must not already own state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001968C (0x8001968C).
void effectEmitterInitialize(void *emitter) {
    u8 *sp1C;
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x18, 0x548);
    sp1C = allocation;
    _bzero((void *)allocation, 0x18);
    ((RuntimeObjectPrefix *)emitter)->state.bytes = sp1C;
}

f32 vector2NormalizeComponentsApprox(f32 *, f32 *);

/// Update an effect emitter's target-relative position, emissions, and lifetime.
/// \param emitter Initialized effect emitter to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800196D8 (0x800196D8).
void effectEmitterUpdate(u8 *emitter) {
    u8 *v;
    u8 *a;
    u8 *b;
    f32 dx;
    f32 dz;

    v = *(u8 **)(emitter + 0x54);
    a = objectResolveAvailableHandle(((EffectEmitterState *)v)->targetHandle);
    if (a != NULL) {
        *(f32 *)(emitter + 0x18) = *(f32 *)(a + 0x44);
        *(f32 *)(emitter + 0x1C) = *(f32 *)(a + 0x48);
        *(f32 *)(emitter + 0x20) = *(f32 *)(a + 0x4C);
        b = objectResolveAvailableHandle(((EffectEmitterState *)v)->referenceHandle);
        if (b != NULL) {
            dx = *(f32 *)(a + 0x44) - *(f32 *)(b + 0x44);
            dz = *(f32 *)(a + 0x4C) - *(f32 *)(b + 0x4C);
            vector2NormalizeComponentsApprox(&dx, &dz);
            dx *= ((EffectEmitterState *)v)->offset;
            dz *= ((EffectEmitterState *)v)->offset;
            *(f32 *)(emitter + 0x18) += dx;
            *(f32 *)(emitter + 0x20) += dz;
        }
    }
    if (((EffectEmitterState *)v)->emit != NULL) {
        (((EffectEmitterState *)v)->emit)(emitter);
    }
    if (((EffectEmitterState *)v)->remaining > 0.0f) {
        ((EffectEmitterState *)v)->remaining -= gRuntimeFrameSeconds;
        if (((EffectEmitterState *)v)->remaining <= 0.0f) {
            objectRequestHierarchyDestruction(emitter);
        }
    }
    ((EffectEmitterState *)v)->cooldown -= gRuntimeFrameSeconds;
    if (((EffectEmitterState *)v)->cooldown < 0.0f) {
        ((EffectEmitterState *)v)->cooldown = 0.0f;
    }
}

/// Initialize the animated appearance of a particle cluster renderer.
/// \param renderer Cluster renderer to initialize; must not already own state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80019834 (0x80019834).
void particleClusterRendererInitialize(void *renderer) {
    u8 *sp1C;
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0xC, 0x59A);
    sp1C = allocation;
    animationIntegerTrackInitialize(allocation, gRibbonAndClusterFrameAnimation);
    ((RuntimeObjectPrefix *)renderer)->state.bytes = sp1C;
}

/// Release the resources owned by a particle cluster renderer.
/// \param renderer Cluster renderer with owned drawing resources and state; these become invalid after
/// release.
/// \returns Nothing.
///
/// \note Portability: the unprototyped deallocator call passes an extra argument ignored by the original MIPS
/// ABI.
/// \note Original decompilation identifier: \c func_80019884 (0x80019884).
void particleClusterRendererDestroy(u8 *renderer) {
    s32 sp1C;

    sp1C = *(s32 *)(renderer + 0x54);
    heapFreeLocal(&gGameHeap, *(s32 *)(renderer + 0x30), renderer);
    heapFreeLocal(&gGameHeap, sp1C);
}

/// Advance a particle cluster's animated appearance and keep it aligned with the view.
/// \param renderer Cluster renderer to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800198C8 (0x800198C8).
void particleClusterRendererUpdate(u8 *renderer) {
    void *t;
    s32 sp18[1];

    t = ((RuntimeObjectPrefix *)renderer)->state.pointer;
    animationIntegerTrackEvaluate(t, sp18, gRuntimeFrameSeconds);
    objectAdjustPitchAlongCameraView(renderer);
}

/// Prepare a particle cluster renderer for drawing.
/// \param renderer Cluster renderer to draw.
/// \returns Address of the cluster's display list.
///
/// \note Requires initialized effect resources, valid active matrix buffers where used, and an active
/// display-list cursor with sufficient capacity. Drawing resources remain owned by the effect and must stay
/// live until rendering finishes. The effect texture bank must be loaded.
/// \note Original decompilation identifier: \c func_80019900 (0x80019900).
s32 particleClusterRendererPrepareDraw(u8 *renderer) {
    u8 *obj;
    Gfx *g;
    s32 idx;

    obj = *(u8 **)(renderer + 0x54);
    g = gRenderDisplayListCursor++;
    g->words.w0 = 0xDB06001C;
    idx = (*(u16 *)(*(u8 **)(renderer + 0x50) + 0x36) & 0x8000) ? 1 : 0;
    g->words.w1 = (u32)osVirtualToPhysical(((void **)(*(u8 **)(renderer + 0x50) + 0x3C))[idx]);
    textureLoadForDraw(&gRenderDisplayListCursor, 5, *(s16 *)(obj + 0x8));
    return *(s32 *)(renderer + 0x30);
}

/// Animate the orientation of a particle cluster element.
/// \param element Cluster element to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800199A0 (0x800199A0).
void particleClusterElementUpdate(u8 *element) {
    s32 i;

    for (i = 1; i < 3; i++) {
        *(f32 *)(element + 0xC + i * 4) +=
            ((f64)randomNext() * 2.3283064365386963e-10 * 30.0 + 60.0) * gRuntimeFrameSeconds;
        while (*(f32 *)(element + 0xC + i * 4) < 0.0f) {
            *(f32 *)(element + 0xC + i * 4) += 360.0f;
        }
        while (*(f32 *)(element + 0xC + i * 4) >= 360.0f) {
            *(f32 *)(element + 0xC + i * 4) -= 360.0f;
        }
    }
    objectAdjustPitchAlongCameraView(element);
}

/// Object template for the particle-cluster root and lifetime state.
/// \note Original identifier: \c D_80078030 (0x80078030).
extern u8 gParticleClusterObjectTemplate[];
/// Object template for individual oriented particle-cluster elements.
/// \note Original identifier: \c D_80078068 (0x80078068).
extern u8 gParticleClusterElementTemplate[];
/// Object template for the particle-cluster renderer.
/// \note Original identifier: \c D_800780A0 (0x800780A0).
extern u8 gParticleClusterRendererTemplate[];
s32 particleClusterCreateDisplayList(s32);

/// Create an animated particle cluster at a source object.
/// \param count Positive number of particles; drawing and matrix allocation sizes must fit in 32 bits.
/// \param lifetime Cluster lifetime in seconds.
/// \param radialScale Scale controlling the particles' radial extent.
/// \param particleScale Scale controlling the particles' thickness.
/// \param source Live source object supplying the cluster's position and target handle.
/// \returns Registered cluster object, managed by the object system.
/// \note Original decompilation identifier: \c func_80019B08 (0x80019B08).
u8 *particleClusterCreate(s32 count, f32 lifetime, f32 radialScale, f32 particleScale, u8 *source) {
    u8 *obj;
    u8 *v;
    u8 *prev;
    u8 *e;
    s32 i;

    obj = objectCreateFromTemplate(gParticleClusterObjectTemplate);
    v = *(u8 **)(obj + 0x54);
    prev = NULL;
    *(f32 *)(obj + 0x18) = *(f32 *)(source + 0x44);
    *(f32 *)(obj + 0x1C) = *(f32 *)(source + 0x48);
    *(f32 *)(obj + 0x20) = *(f32 *)(source + 0x4C);
    *(f32 *)(obj + 0x44) = *(f32 *)(source + 0x44);
    *(f32 *)(obj + 0x48) = *(f32 *)(source + 0x48);
    *(f32 *)(obj + 0x4C) = *(f32 *)(source + 0x4C);
    *(s32 *)(v + 4) = *(s32 *)(source + 0x38);
    *(f32 *)(v + 0) = lifetime;
    for (i = 0; i < count; i++) {
        e = objectCreateFromTemplate(gParticleClusterElementTemplate);
        *(f32 *)(e + 0xC) = (s32)randomBelow(360);
        *(f32 *)(e + 0x10) = (s32)randomBelow(360);
        *(f32 *)(e + 0x14) = (s32)randomBelow(360);
        *(f32 *)(e + 0) = radialScale;
        *(f32 *)(e + 8) = particleScale;
        if (prev != NULL) {
            *(u8 **)(prev + 0x28) = e;
        } else {
            *(u8 **)(obj + 0x2C) = e;
        }
        prev = e;
    }
    e = objectCreateFromTemplate(gParticleClusterRendererTemplate);
    *(s32 *)(e + 0x30) = particleClusterCreateDisplayList(count);
    *(u8 **)(prev + 0x28) = e;
    objectMakeHierarchyRoot(obj);
    objectAllocateHierarchyMatrixBuffers(obj);
    objectRegisterActiveHierarchy(obj);
    return obj;
}

/// Hierarchy template for falling particles.
/// \note Original identifier: \c D_80077CE0 (0x80077CE0).
extern u8 gFallingParticleObjectTemplate[];
/// Model display-list addresses indexed by falling-particle kind.
/// \note Original identifier: \c D_800780D8 (0x800780D8).
extern s32 gFallingParticleModels[];
/// Integer scale animation selected when falling particles are created.
/// \note Original identifier: \c D_8006B4F8 (0x8006B4F8).
extern s32 gFallingParticleInitialScaleAnimation;

/// Create a falling particle with the selected appearance and motion style.
/// \param kind Valid index in the falling-particle appearance table.
/// \param motionStyle Motion style: zero or one for camera-facing motion, two for tumbling.
/// \param position Readable three-component initial world position, copied during creation.
/// \param velocity Readable three-component initial world velocity, copied during creation.
/// \param lifetime Positive time in seconds before the particle begins disappearing.
/// \returns Registered particle object, managed by the object system.
/// \note Original decompilation identifier: \c func_80019C80 (0x80019C80).
u8 *fallingParticleCreate(s32 kind, s32 motionStyle, f32 *position, f32 *velocity, f32 lifetime) {
    u8 *obj;
    u8 *v;
    f32 scale;

    obj = objectCreateHierarchyFromTemplate(gFallingParticleObjectTemplate);
    v = *(u8 **)(obj + 0x54);
    *(f32 *)(obj + 0x18) = position[0];
    *(f32 *)(obj + 0x1C) = position[1];
    *(f32 *)(obj + 0x20) = position[2];
    *(f32 *)(obj + 0x44) = position[0];
    *(f32 *)(obj + 0x48) = position[1];
    *(f32 *)(obj + 0x4C) = position[2];
    *(f32 *)(obj + 0xC) = (s32)randomBelow(360);
    *(f32 *)(obj + 0x14) = 0.0f;
    *(f32 *)(obj + 0x10) = 0.0f;
    *(s32 *)(obj + 0x30) = gFallingParticleModels[kind];
    *(f32 *)(v + 0) = lifetime;
    *(s32 *)(v + 4) = motionStyle;
    *(f32 *)(v + 8) = velocity[0];
    *(f32 *)(v + 0xC) = velocity[1];
    *(f32 *)(v + 0x10) = velocity[2];
    animationIntegerTrackInitialize(v + 0x14, gFallingParticleInitialScaleAnimation);
    scale = 0.029999999329447746f;
    *(f32 *)(v + 0x20) = scale;
    *(f32 *)(obj + 0) = scale;
    *(f32 *)(obj + 4) = *(f32 *)(v + 0x20);
    *(f32 *)(obj + 8) = *(f32 *)(v + 0x20);
    objectRegisterActiveHierarchy(obj);
    return obj;
}

/// Initialize the state of a falling particle.
/// \param particle Particle object to initialize; must not already own state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80019DA0 (0x80019DA0).
void fallingParticleInitialize(void *particle) {
    u8 *sp1C;
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x24, 0x675);
    sp1C = allocation;
    _bzero((void *)allocation, 0x24);
    ((RuntimeObjectPrefix *)particle)->state.bytes = sp1C;
}

f32 mathCosDegrees(f32);
/// Integer scale animation selected when falling-particle lifetime expires.
/// \note Original identifier: \c D_8006B51C (0x8006B51C).
extern s32 gFallingParticleDisappearanceScaleAnimation;
typedef struct {
    f32 lifetime;
    s32 mode;
    f32 velocity[3];
    u8 animation[12];
    f32 scale;
} ParticleState;

/// Advance a falling particle's motion and appearance, removing it when it finishes disappearing.
/// \param particle Particle with initialized lifetime, velocity, and appearance animation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80019DEC (0x80019DEC).
void fallingParticleUpdate(u8 *particle) {
    f32 scale;
    s32 sample;
    ParticleState *state;

    state = *(ParticleState **)(particle + 0x54);
    if (state->lifetime > 0.0f) {
        state->lifetime = (f32)(state->lifetime - gRuntimeFrameSeconds);
        if (state->lifetime <= 0.0f) {
            state->lifetime = 0.0f;
            animationIntegerTrackInitialize(state->animation, gFallingParticleDisappearanceScaleAnimation);
        }
    }
    animationIntegerTrackEvaluate(state->animation, &sample, gRuntimeFrameSeconds);
    if ((sample == 0) && (state->lifetime <= 0.0f)) {
        objectRequestHierarchyDestruction(particle);
        return;
    }
    scale = (f32)sample * state->scale * 0.1f;
    ((RuntimeObjectPrefix *)particle)->scale[0] = scale;
    ((RuntimeObjectPrefix *)particle)->scale[1] = scale;
    ((RuntimeObjectPrefix *)particle)->scale[2] = scale;
    switch (state->mode) {
    case 0:
        objectOrientAlongCameraView(particle);
        ((RuntimeObjectPrefix *)particle)->rotation[0] += ((mathSinDegrees(state->lifetime * 80.0f) + 1.0f) * 180.0f);
        if (((RuntimeObjectPrefix *)particle)->rotation[0] < 0.0f) {
            do {
                ((RuntimeObjectPrefix *)particle)->rotation[0] += 360.0f;
            } while (((RuntimeObjectPrefix *)particle)->rotation[0] < 0.0f);
        }
        if (((RuntimeObjectPrefix *)particle)->rotation[0] >= 360.0f) {
            do {
                ((RuntimeObjectPrefix *)particle)->rotation[0] =
                    (f32)(((RuntimeObjectPrefix *)particle)->rotation[0] - 360.0f);
            } while (((RuntimeObjectPrefix *)particle)->rotation[0] >= 360.0f);
        }
        break;
    case 1:
        objectOrientAlongCameraView(particle);
        ((RuntimeObjectPrefix *)particle)->rotation[0] += ((mathCosDegrees(state->lifetime * 200.0f) + 1.0f) * 180.0f);
        if (((RuntimeObjectPrefix *)particle)->rotation[0] < 0.0f) {
            do {
                ((RuntimeObjectPrefix *)particle)->rotation[0] += 360.0f;
            } while (((RuntimeObjectPrefix *)particle)->rotation[0] < 0.0f);
        }
        if (((RuntimeObjectPrefix *)particle)->rotation[0] >= 360.0f) {
            do {
                ((RuntimeObjectPrefix *)particle)->rotation[0] =
                    (f32)(((RuntimeObjectPrefix *)particle)->rotation[0] - 360.0f);
            } while (((RuntimeObjectPrefix *)particle)->rotation[0] >= 360.0f);
        }
        ((RuntimeObjectPrefix *)particle)->position[0] +=
            (mathCosDegrees(state->lifetime * 200.0f) * gRuntimeFrameSeconds);
        ((RuntimeObjectPrefix *)particle)->position[2] +=
            (2.0f * mathCosDegrees(state->lifetime * 310.0f) * gRuntimeFrameSeconds);
        break;
    case 2:
        ((RuntimeObjectPrefix *)particle)->rotation[0] =
            (f32)((mathSinDegrees(state->lifetime * 40.0f) + 1.0f) * 180.0f);
        ((RuntimeObjectPrefix *)particle)->rotation[1] = (mathSinDegrees(state->lifetime * 300.0f) + 1.0f) * 180.0f;
        ((RuntimeObjectPrefix *)particle)->rotation[2] = 0.0f;
        break;
    }
    state->velocity[1] = (f32)(state->velocity[1] - (3.266666651f * gRuntimeFrameSeconds * 0.1f * 30.0f));
    if (state->velocity[1] < -10.0f) {
        state->velocity[1] = -10.0f;
    } else if (state->velocity[1] > 10.0f) {
        state->velocity[1] = 10.0f;
    }
    ((RuntimeObjectPrefix *)particle)->position[0] += (state->velocity[0] * gRuntimeFrameSeconds);
    ((RuntimeObjectPrefix *)particle)->position[1] += (state->velocity[1] * gRuntimeFrameSeconds);
    ((RuntimeObjectPrefix *)particle)->position[2] += (state->velocity[2] * gRuntimeFrameSeconds);
}

/// Initialize the state of a converging particle effect.
/// \param effect Effect object to initialize; must not already own state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001A228 (0x8001A228).
void convergingParticleEffectInitialize(void *effect) {
    u8 *sp1C;
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x2C, 0x6BB);
    sp1C = allocation;
    _bzero((void *)allocation, 0x2C);
    ((RuntimeObjectPrefix *)effect)->state.bytes = sp1C;
}

/// Release the resources owned by a converging particle effect.
/// \param effect Effect object with owned drawing resources and state; these become invalid after release.
/// \returns Nothing.
///
/// \note Portability: the unprototyped deallocator call passes an extra argument ignored by the original MIPS
/// ABI.
/// \note Original decompilation identifier: \c func_8001A274 (0x8001A274).
void convergingParticleEffectDestroy(u8 *effect) {
    u8 *obj;

    obj = *(u8 **)(effect + 0x54);
    heapFreeLocal(&gGameHeap, *(u8 **)obj, obj);
    heapFreeLocal(&gGameHeap, obj);
}

void vector3FromPitchYawDegrees(f32, f32, f32 *);
f32 vector3Normalize(u8 *);
f32 mathMapRangeClamped(f32, f32, f32, f32, f32);

/// Advance a converging particle effect around its target until its lifetime expires or the target
/// disappears.
/// \param effect Effect with state and hierarchy configured by its creation function.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001A2B4 (0x8001A2B4).
void convergingParticleEffectUpdate(u8 *effect) {
    u8 *state;
    u8 *target;
    s32 i;
    f32 fraction;
    f32 radius;
    f32 step;
    f32 center[3];
    s32 tmp;
    f32 delta[3];
    f32 age;
    f32 scale;

    state = *(u8 **)(effect + 0x54);
    *(f32 *)(state + 0x20) -= gRuntimeFrameSeconds;
    target = objectResolveAvailableHandle(*(s32 *)(state + 0x1C));
    if (*(f32 *)(state + 0x20) < 0.0f || target == NULL) {
        objectRequestHierarchyDestruction(effect);
        return;
    }
    animationIntegerTrackEvaluate(state + 0x10, &tmp, gRuntimeFrameSeconds);
    animationIntegerTrackEvaluate(state + 4, &tmp, gRuntimeFrameSeconds);
    fraction = *(f32 *)(state + 0x20) / *(f32 *)(state + 0x24);
    step = (1.0f / (f32) * (s32 *)(state + 0x28)) * 0.5f;
    vector3FromPitchYawDegrees((s32)randomBelow(10), *(f32 *)(state + 0x20) * 200.0f, center);
    center[0] = center[0] * 0.1f + *(f32 *)(target + 0x44);
    center[1] = center[1] * 0.1f + *(f32 *)(target + 0x48);
    center[2] = center[2] * 0.1f + *(f32 *)(target + 0x4C);
    for (i = 0, state = effect; state != NULL; state = *(u8 **)(state + 0x28)) {
        age = i * step + fraction;
        scale = 0.15f * age;
        *(f32 *)state = scale;
        *(f32 *)(state + 4) = scale;
        *(f32 *)(state + 8) = scale;
        radius = mathMapRangeClamped(0.6f, 0.15f, 0.8f, 1.0f, age) * 10.0f;
        delta[0] = *(f32 *)(state + 0x44) - center[0];
        delta[1] = *(f32 *)(state + 0x48) - center[1];
        delta[2] = *(f32 *)(state + 0x4C) - center[2];
        vector3Normalize((u8 *)delta);
        delta[0] *= radius;
        delta[1] *= radius;
        delta[2] *= radius;
        *(f32 *)(state + 0x18) = *(f32 *)(target + 0x44) + delta[0];
        *(f32 *)(state + 0x1C) = *(f32 *)(target + 0x48) + delta[1];
        *(f32 *)(state + 0x20) = *(f32 *)(target + 0x4C) + delta[2];
        *(f32 *)(state + 0x44) = *(f32 *)(state + 0x18);
        *(f32 *)(state + 0x48) = *(f32 *)(state + 0x1C);
        *(f32 *)(state + 0x4C) = *(f32 *)(state + 0x20);
        objectOrientAlongCameraView(state);
        i++;
    }
}

/// Prepare a converging particle effect for rendering.
/// \param effect Effect object to render.
/// \returns Address of the effect's display list.
///
/// \note Requires initialized effect resources, valid active matrix buffers where used, and an active
/// display-list cursor with sufficient capacity. Drawing resources remain owned by the effect and must stay
/// live until rendering finishes. The effect texture bank must be loaded.
/// \note Original decompilation identifier: \c func_8001A55C (0x8001A55C).
s32 convergingParticleEffectPrepareDraw(u8 *effect) {
    u8 *obj;
    Gfx *g;
    s32 idx;

    obj = *(u8 **)(effect + 0x54);
    g = gRenderDisplayListCursor++;
    g->words.w0 = 0xDB06001C;
    idx = (*(u16 *)(effect + 0x36) & 0x8000) ? 1 : 0;
    g->words.w1 = (u32)osVirtualToPhysical(((void **)(effect + 0x3C))[idx]);
    textureLoadForDraw(&gRenderDisplayListCursor, 5, *(s16 *)(obj + 0xC));
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 0xFF, 0xFF, 0x90, obj[0x19]);
    return *(s32 *)obj;
}

/// Object template for converging particle effects.
/// \note Original identifier: \c D_80077D50 (0x80077D50).
extern u8 gConvergingParticleObjectTemplate[];
/// Shared integer animation programs selecting texture frames for actors, effects and interface elements.
/// \note Original identifier: \c D_8006B090 (0x8006B090).
extern s32 gTextureFrameAnimationPrograms[];
/// Integer appearance animation used when messenger state is initialized.
/// \note Original identifier: \c D_8006B4E0 (0x8006B4E0).
extern s32 gMessengerFlightAnimation;
typedef struct {
    s32 animation;
    Gfx *displayList;
} LinkedEffectStyle;
/// Texture-animation selections and display lists indexed by converging-particle style.
/// \note Original identifier: \c D_800780F4 (0x800780F4).
extern LinkedEffectStyle gConvergingParticleStyles[];
typedef struct {
    Gfx *displayList;
    u8 textureAnimation[12];
    u8 alphaAnimation[12];
    s32 target;
    f32 remaining, lifetime;
    s32 count;
} LinkedEffectState;
/// Create styled particles that converge around a live target.
/// \param targetId Handle of the target the effect surrounds.
/// \param lifetime Positive effect lifetime in seconds.
/// \param count Positive number of visual elements; drawing and matrix allocation sizes must fit in 32 bits.
/// \param styleId Valid index in the converging-effect style table.
/// \returns Registered effect managed by the object system, or null if the target is unavailable.
///
/// \note Portability: paired texture setup commands preserve the original IDO scheduling.
/// \note Original decompilation identifier: \c func_8001A624 (0x8001A624).
u8 *convergingParticleEffectCreate(s32 targetId, f32 lifetime, s32 count, s32 styleId) {
    u8 *target;
    u8 *first;
    LinkedEffectState *state;
    u8 *tail;
    s32 randomA;
    s32 randomB;
    f32 direction[3];
    u8 *object;
    s32 i;
    Gfx *commands;
    target = objectResolveAvailableHandle(targetId);
    if (target == NULL)
        return NULL;
    first = objectCreateFromTemplate(gConvergingParticleObjectTemplate);
    state = *(LinkedEffectState **)(first + 0x54);
    state->displayList = (Gfx *)heapAllocLocal(&gGameHeap, count * 4 * sizeof(Gfx) + 0x80, 0x74E);
    state->target = targetId;
    state->remaining = lifetime;
    state->lifetime = lifetime;
    state->count = count;
    animationIntegerTrackInitialize(state->textureAnimation,
                                    gTextureFrameAnimationPrograms[gConvergingParticleStyles[styleId].animation]);
    animationIntegerTrackInitialize(state->alphaAnimation, gMessengerFlightAnimation);
    randomA = randomBelow(60);
    randomB = randomBelow(60);
    vector3FromPitchYawDegrees((f32)(randomB + randomA) - 80.0f, (s32)randomBelow(360), direction);
    direction[0] *= 10.0f;
    direction[1] *= 10.0f;
    direction[2] *= 10.0f;
    *(f32 *)(first + 0x18) = *(f32 *)(target + 0x44) + direction[0];
    *(f32 *)(first + 0x1C) = *(f32 *)(target + 0x48) + direction[1];
    *(f32 *)(first + 0x20) = *(f32 *)(target + 0x4C) + direction[2];
    *(f32 *)(first + 8) = *(f32 *)(first + 4) = *(f32 *)first = 0.15f;
    *(f32 *)(first + 0x44) = *(f32 *)(first + 0x18);
    *(f32 *)(first + 0x48) = *(f32 *)(first + 0x1C);
    *(f32 *)(first + 0x4C) = *(f32 *)(first + 0x20);
    tail = first;
    for (i = 0; i < count - 1; i++) {
        object = objectCreateDefault();
        *(u8 **)(tail + 0x28) = object;
        tail = object;
        randomA = randomBelow(60);
        randomB = randomBelow(60);
        vector3FromPitchYawDegrees((f32)(randomB + randomA) - 80.0f, (s32)randomBelow(360), direction);
        direction[0] *= 10.0f;
        direction[1] *= 10.0f;
        direction[2] *= 10.0f;
        *(f32 *)(object + 0x18) = *(f32 *)(target + 0x44) + direction[0];
        *(f32 *)(object + 0x1C) = *(f32 *)(target + 0x48) + direction[1];
        *(f32 *)(object + 0x20) = *(f32 *)(target + 0x4C) + direction[2];
        *(f32 *)(object + 8) = *(f32 *)(object + 4) = *(f32 *)object = 0.15f;
        *(f32 *)(object + 0x44) = *(f32 *)(object + 0x18);
        *(f32 *)(object + 0x48) = *(f32 *)(object + 0x1C);
        *(f32 *)(object + 0x4C) = *(f32 *)(object + 0x20);
    }
    objectMakeHierarchyRoot(first);
    objectAllocateHierarchyMatrixBuffers(first);
    objectRegisterActiveHierarchy(first);
    commands = state->displayList;
    gDPPipeSync(commands++);
    gSPGeometryMode(commands++, -1, 0x200005);
    gDPSetRenderMode(commands++, 0x0C184A50, 0);
    gDPSetCombine(commands++, 0x3417FF, 0x4FFE7E38);
    gDPSetTextureLOD(commands++, G_TL_TILE);
    gDPSetTextureDetail(commands++, G_TD_CLAMP);

    // clang-format off
    gSPTexture(commands++, 0x7FFF, 0, 0, 0, G_ON); gMoveWd(commands++, G_MW_FOG, 0x10, 0x7FFF);
    // clang-format on
    for (i = 0; i < count; i++) {
        gSPMatrix(commands++, (Mtx *)(0x07000000 + (i << 6)), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPDisplayList(commands++, gConvergingParticleStyles[styleId].displayList);
    }
    gSPTexture(commands++, 0x7FFF, 0, 0, 0, G_OFF);
    gMoveWd(commands++, G_MW_FOG, 0x10, 0x7FFF);
    gSPEndDisplayList(commands++);
    return first;
}

/// Initialize the state of a flickering effect.
/// \param effect Effect object to initialize; must not already own state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001AB6C (0x8001AB6C).
void flickeringEffectInitialize(void *effect) {
    u8 *sp1C;
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0xC, 0x7A2);
    sp1C = allocation;
    _bzero((void *)allocation, 0xC);
    ((RuntimeObjectPrefix *)effect)->state.bytes = sp1C;
}

/// Release the resources owned by a flickering effect.
/// \param effect Effect object with owned drawing resources and state; these become invalid after release.
/// \returns Nothing.
///
/// \note Portability: the unprototyped deallocator call passes an extra argument ignored by the original MIPS
/// ABI.
/// \note Original decompilation identifier: \c func_8001ABB8 (0x8001ABB8).
void flickeringEffectDestroy(u8 *effect) {
    u8 *obj;

    obj = *(u8 **)(effect + 0x54);
    heapFreeLocal(&gGameHeap, *(u8 **)obj, obj);
    heapFreeLocal(&gGameHeap, obj);
}

/// Update a flickering effect as it follows its target, ending it when its lifetime or target ends.
/// \param effect Effect with state and hierarchy configured by its creation function.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001ABF8 (0x8001ABF8).
void flickeringEffectUpdate(u8 *effect) {
    u8 *v;
    u8 *t;
    u8 *p;
    f64 lo;
    f64 hi;

    v = *(u8 **)(effect + 0x54);
    *(f32 *)(v + 8) -= gRuntimeFrameSeconds;
    t = objectResolveAvailableHandle(*(s32 *)(v + 4));
    if (*(f32 *)(v + 8) < 0.0f || t == NULL) {
        objectRequestHierarchyDestruction(effect);
        return;
    }
    *(f32 *)(effect + 0x18) = *(f32 *)(t + 0x44);
    *(f32 *)(effect + 0x1C) = *(f32 *)(t + 0x48);
    *(f32 *)(effect + 0x20) = *(f32 *)(t + 0x4C);
    *(f32 *)(effect + 0x44) = *(f32 *)(effect + 0x18);
    *(f32 *)(effect + 0x48) = *(f32 *)(effect + 0x1C);
    *(f32 *)(effect + 0x4C) = *(f32 *)(effect + 0x20);
    objectOrientAlongCameraView(effect);
    p = *(u8 **)(effect + 0x2C);
    if (p != NULL) {
        hi = 0.30000001192092896;
        lo = 0.4000000059604645;
        do {
            if ((f64)randomNext() * 2.3283064365386963e-10 < lo) {
                *(f32 *)(p + 0xC) = (s32)randomBelow(360);
            }
            if ((f64)randomNext() * 2.3283064365386963e-10 < hi) {
                *(f32 *)(p + 0x1C) = ((f64)randomNext() * 2.3283064365386963e-10 - 0.5) * 5.0;
                *(f32 *)(p + 0x20) = ((f64)randomNext() * 2.3283064365386963e-10 - 0.5) * 5.0;
            }
            p = *(u8 **)(p + 0x28);
        } while (p != NULL);
    }
}

/// Prepare a flickering effect for rendering.
/// \param effect Effect object to render.
/// \returns Address of the effect's display list.
///
/// \note Requires initialized effect resources, valid active matrix buffers where used, and an active
/// display-list cursor with sufficient capacity. Drawing resources remain owned by the effect and must stay
/// live until rendering finishes.
/// \note Original decompilation identifier: \c func_8001AE30 (0x8001AE30).
s32 flickeringEffectPrepareDraw(u8 *effect) {
    u8 *obj;
    Gfx *g;
    s32 idx;

    obj = *(u8 **)(effect + 0x54);
    g = gRenderDisplayListCursor++;
    g->words.w0 = 0xDB06001C;
    idx = (*(u16 *)(effect + 0x36) & 0x8000) ? 1 : 0;
    g->words.w1 = (u32)osVirtualToPhysical(((void **)(effect + 0x3C))[idx]);
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 0x80, 0xFF, 0xFF, 0xFF);
    return *(s32 *)obj;
}

typedef struct EffectAEDC {
    Gfx *displayList;
    s32 targetId;
    f32 duration;
} EffectAEDC;
/// Object template for flickering particle effects.
/// \note Original identifier: \c D_80077D88 (0x80077D88).
extern u8 gFlickeringEffectObjectTemplate[];
extern Gfx D_1006978[];
/// Four model display lists cycled across flickering-effect elements.
/// \note Original identifier: \c D_80078104 (0x80078104).
extern Gfx *gFlickeringEffectDisplayLists[];
/// Create a flickering effect that follows a live target.
/// \param targetId Handle of the target to follow.
/// \param duration Effect lifetime in seconds.
/// \param count Nonnegative number of visual elements; drawing and matrix allocation sizes must fit in 32
/// bits.
/// \returns Registered effect managed by the object system, or null if the target is unavailable.
/// \note Original decompilation identifier: \c func_8001AEDC (0x8001AEDC).
u8 *flickeringEffectCreate(s32 targetId, f32 duration, s32 count) {
    u8 *target;
    u8 *object;
    EffectAEDC *state;
    u8 *previous;
    u8 *child;
    Gfx *dl;
    s32 i;
    target = objectResolveAvailableHandle(targetId);
    if (target == NULL)
        return NULL;
    object = objectCreateFromTemplate(gFlickeringEffectObjectTemplate);
    state = *(EffectAEDC **)(object + 0x54);
    state->displayList = (Gfx *)heapAllocLocal(&gGameHeap, (count * 4 + 16) * sizeof(Gfx), 0x809);
    state->targetId = targetId;
    state->duration = duration;
    *(f32 *)(object + 0x18) = *(f32 *)(target + 0x44);
    *(f32 *)(object + 0x1C) = *(f32 *)(target + 0x48);
    *(f32 *)(object + 0x20) = *(f32 *)(target + 0x4C);
    *(f32 *)(object + 8) = *(f32 *)(object + 4) = *(f32 *)object = 0.2f;
    *(f32 *)(object + 0x44) = *(f32 *)(object + 0x18);
    *(f32 *)(object + 0x48) = *(f32 *)(object + 0x1C);
    *(f32 *)(object + 0x4C) = *(f32 *)(object + 0x20);
    previous = object;
    for (i = 0; i < count; i++) {
        child = objectCreateDefault();
        *(u8 **)(previous + 0x28) = child;
        previous = child;
        *(f32 *)(child + 8) = *(f32 *)(child + 4) = *(f32 *)child = 1.0f;
        *(f32 *)(child + 0xC) = i * 137.5f;
    }
    *(u8 **)(object + 0x2C) = *(u8 **)(object + 0x28);
    *(u8 **)(object + 0x28) = NULL;
    objectMakeHierarchyRoot(object);
    objectAllocateHierarchyMatrixBuffers(object);
    objectRegisterActiveHierarchy(object);
    dl = state->displayList;
    gDPPipeSync(dl++);
    gSPGeometryMode(dl++, -1, 0x200005);
    gDPSetRenderMode(dl++, 0x0C184A50, 0);
    gDPSetCombine(dl++, 0x3417FF, 0x4FFE7E38);
    gDPSetTextureLOD(dl++, G_TL_TILE);
    gDPSetTextureDetail(dl++, G_TD_CLAMP);
    gDPSetTextureLUT(dl++, G_TT_NONE);
    gSPTexture(dl++, 0x7FFF, 0, 0, 0, G_ON);
    gMoveWd(dl++, G_MW_FOG, 0x10, 0x7FFF);
    for (i = 0; i < count; i++) {
        gSPMatrix(dl++, (Mtx *)(0x07000040 + (i << 6)), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPDisplayList(dl++, gFlickeringEffectDisplayLists[i % 4]);
        gSPDisplayList(dl++, D_1006978);
    }
    gSPTexture(dl++, 0x7FFF, 0, 0, 0, G_OFF);
    gMoveWd(dl++, G_MW_FOG, 0x10, 0x7FFF);
    gSPEndDisplayList(dl++);
    return object;
}

/// Initialize the state of an animated radial effect.
/// \param effect Effect object to initialize; must not already own state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001B3A0 (0x8001B3A0).
void animatedRadialEffectInitialize(void *effect) {
    u8 *sp1C;
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x24, 0x84F);
    sp1C = allocation;
    _bzero((void *)allocation, 0x24);
    ((RuntimeObjectPrefix *)effect)->state.bytes = sp1C;
}

/// Release the resources owned by an animated radial effect.
/// \param effect Effect object with owned drawing resources and state; these become invalid after release.
/// \returns Nothing.
///
/// \note Portability: the unprototyped deallocator call passes an extra argument ignored by the original MIPS
/// ABI.
/// \note Original decompilation identifier: \c func_8001B3EC (0x8001B3EC).
void animatedRadialEffectDestroy(u8 *effect) {
    u8 *obj;

    obj = *(u8 **)(effect + 0x54);
    heapFreeLocal(&gGameHeap, *(u8 **)obj, obj);
    heapFreeLocal(&gGameHeap, obj);
}


/// Advance an animated radial effect as it follows its target, ending it when its lifetime or target ends.
/// \param effect Effect with state and hierarchy configured by its creation function.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001B42C (0x8001B42C).
void animatedRadialEffectUpdate(u8 *effect) {
    u8 *v;
    u8 *cursor;
    u8 *t;
    s32 tmp;

    v = *(u8 **)(effect + 0x54);
    *(f32 *)(v + 0x20) -= gRuntimeFrameSeconds;
    t = objectResolveAvailableHandle(*(s32 *)(v + 0x1C));
    if (*(f32 *)(v + 0x20) < 0.0f || t == NULL) {
        objectRequestHierarchyDestruction(effect);
        return;
    }
    animationIntegerTrackEvaluate(v + 0x10, &tmp, gRuntimeFrameSeconds);
    animationIntegerTrackEvaluate(v + 4, &tmp, gRuntimeFrameSeconds);
    *(f32 *)(effect + 0x18) = *(f32 *)(t + 0x44);
    *(f32 *)(effect + 0x1C) = *(f32 *)(t + 0x48) + 2.0f;
    *(f32 *)(effect + 0x20) = *(f32 *)(t + 0x4C);
    *(f32 *)(effect + 0x44) = *(f32 *)(effect + 0x18);
    *(f32 *)(effect + 0x48) = *(f32 *)(effect + 0x1C);
    *(f32 *)(effect + 0x4C) = *(f32 *)(effect + 0x20);
    objectOrientAlongCameraView(effect);
    for (cursor = *(u8 **)(effect + 0x2C); cursor != NULL; cursor = *(u8 **)(cursor + 0x28)) {
        *(f32 *)(cursor + 4) *= 1.0099999904632568f;
    }
}

/// Prepare an animated radial effect for rendering.
/// \param effect Effect object to render.
/// \returns Address of the effect's display list.
///
/// \note Requires initialized effect resources, valid active matrix buffers where used, and an active
/// display-list cursor with sufficient capacity. Drawing resources remain owned by the effect and must stay
/// live until rendering finishes. The effect texture bank must be loaded.
/// \note Original decompilation identifier: \c func_8001B550 (0x8001B550).
s32 animatedRadialEffectPrepareDraw(u8 *effect) {
    u8 *obj;
    Gfx *g;
    s32 idx;

    obj = *(u8 **)(effect + 0x54);
    g = gRenderDisplayListCursor++;
    g->words.w0 = 0xDB06001C;
    idx = (*(u16 *)(effect + 0x36) & 0x8000) ? 1 : 0;
    g->words.w1 = (u32)osVirtualToPhysical(((void **)(effect + 0x3C))[idx]);
    textureLoadForDraw(&gRenderDisplayListCursor, 5, *(s16 *)(obj + 0xC));
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 0xFF, 0xFF, 0x90, obj[0x19]);
    return *(s32 *)obj;
}

typedef struct EffectB618 {
    Gfx *displayList;
    u8 frameAnimation[12];
    u8 alphaAnimation[12];
    s32 targetId;
    f32 duration;
} EffectB618;
/// Integer texture-frame animation used by animated radial effects.
/// \note Original identifier: \c D_8006B3A4 (0x8006B3A4).
extern s32 gAnimatedRadialEffectFrameAnimation;
/// Integer appearance animation used when messenger state is initialized.
/// \note Original identifier: \c D_8006B4E0 (0x8006B4E0).
extern s32 gMessengerFlightAnimation;
/// Object template for animated radial particle effects.
/// \note Original identifier: \c D_80077DC0 (0x80077DC0).
extern u8 gAnimatedRadialEffectObjectTemplate[];
extern Gfx D_1004888[];
/// Create an animated radial effect that follows a live target.
/// \param targetId Handle of the target to follow.
/// \param duration Effect lifetime in seconds.
/// \param count Nonnegative number of visual elements; drawing and matrix allocation sizes must fit in 32
/// bits.
/// \returns Registered effect managed by the object system, or null if the target is unavailable.
/// \note Original decompilation identifier: \c func_8001B618 (0x8001B618).
u8 *animatedRadialEffectCreate(s32 targetId, f32 duration, s32 count) {
    u8 *object;
    u8 *previous;
    EffectB618 *state;
    u8 *child;
    Gfx *dl;
    u8 *target;
    s32 i;
    target = objectResolveAvailableHandle(targetId);
    if (target == NULL)
        return NULL;
    object = objectCreateFromTemplate(gAnimatedRadialEffectObjectTemplate);
    state = *(EffectB618 **)(object + 0x54);
    state->displayList = (Gfx *)heapAllocLocal(&gGameHeap, (count * 4 + 16) * sizeof(Gfx), 0x8B2);
    state->targetId = targetId;
    state->duration = duration;
    animationIntegerTrackInitialize(state->frameAnimation, gAnimatedRadialEffectFrameAnimation);
    animationIntegerTrackInitialize(state->alphaAnimation, gMessengerFlightAnimation);
    *(f32 *)(object + 0x18) = *(f32 *)(target + 0x44);
    *(f32 *)(object + 0x1C) = *(f32 *)(target + 0x48);
    *(f32 *)(object + 0x20) = *(f32 *)(target + 0x4C);
    *(f32 *)(object + 8) = *(f32 *)(object + 4) = *(f32 *)object = 0.06f;
    *(f32 *)(object + 0x44) = *(f32 *)(object + 0x18);
    *(f32 *)(object + 0x48) = *(f32 *)(object + 0x1C);
    *(f32 *)(object + 0x4C) = *(f32 *)(object + 0x20);
    previous = object;
    for (i = 0; i < count; i++) {
        child = objectCreateDefault();
        *(u8 **)(previous + 0x28) = child;
        previous = child;
        *(f32 *)(child + 0xC) = i * (360.0f / count);
    }
    *(u8 **)(object + 0x2C) = *(u8 **)(object + 0x28);
    *(u8 **)(object + 0x28) = NULL;
    objectMakeHierarchyRoot(object);
    objectAllocateHierarchyMatrixBuffers(object);
    objectRegisterActiveHierarchy(object);
    dl = state->displayList;
    gDPPipeSync(dl++);
    gSPGeometryMode(dl++, -1, 0x200005);
    gDPSetRenderMode(dl++, 0x0C184A50, 0);
    gDPSetCombine(dl++, 0x3417FF, 0x4FFE7E38);
    gDPSetTextureLOD(dl++, G_TL_TILE);
    gDPSetTextureDetail(dl++, G_TD_CLAMP);
    gDPSetTextureLUT(dl++, G_TT_NONE);
    gSPTexture(dl++, 0x7FFF, 0, 0, 0, G_ON);
    gMoveWd(dl++, G_MW_FOG, 0x10, 0x7FFF);
    for (i = 0; i < count; i++) {
        gSPMatrix(dl++, (Mtx *)(0x07000040 + (i << 6)), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPDisplayList(dl++, D_1004888);
    }
    gSPTexture(dl++, 0x7FFF, 0, 0, 0, G_OFF);
    gMoveWd(dl++, G_MW_FOG, 0x10, 0x7FFF);
    gSPEndDisplayList(dl++);
    return object;
}

/// Create an expanding orange visual effect that follows a target while it exists.
/// \param targetHandle Object handle identifying the target to follow.
/// \param lifetime Effect lifetime in seconds.
/// \returns Registered effect managed by the object system; an unavailable target does not prevent creation.
/// \note Original decompilation identifier: \c func_8001B9EC (0x8001B9EC).
u8 *expandingOrangeEffectCreate(s32 targetHandle, f32 lifetime) {
    u8 *sp1C;
    u8 *sp18;

    sp1C = objectCreateHierarchyFromTemplate(gExpandingOrangeEffectTemplate);
    sp18 = *(u8 **)(sp1C + 0x54);
    *(f32 *)(sp18 + 0x10) = lifetime;
    *(s32 *)sp18 = targetHandle;
    animationIntegerTrackInitialize(sp18 + 4, gExpandingOrangeEffectFrameAnimation);
    *(f32 *)(sp18 + 0x14) = 0.5f;
    *(f32 *)(sp1C + 8) = 0.5f;
    *(f32 *)(sp1C + 4) = 0.5f;
    *(f32 *)sp1C = 0.5f;
    objectRegisterActiveHierarchy(sp1C);
    return sp1C;
}

/// Initialize the state of an expanding orange effect.
/// \param effect Effect object to initialize; must not already own state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001BA6C (0x8001BA6C).
void expandingOrangeEffectInitialize(void *effect) {
    u8 *sp1C;
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x18, 0x923);
    sp1C = allocation;
    _bzero((void *)allocation, 0x18);
    ((RuntimeObjectPrefix *)effect)->state.bytes = sp1C;
}

/// Advance an expanding orange effect and follow its target until its lifetime expires.
/// \param effect Effect with state and hierarchy configured by its creation function.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001BAB8 (0x8001BAB8).
void expandingOrangeEffectUpdate(u8 *effect) {
    u8 *v;
    u8 *t;
    s32 tmp;

    v = *(u8 **)(effect + 0x54);
    *(f32 *)(v + 0x10) -= gRuntimeFrameSeconds;
    t = objectResolveAvailableHandle(*(s32 *)v);
    if (*(f32 *)(v + 0x10) < 0.0f) {
        objectRequestHierarchyDestruction(effect);
        return;
    }
    animationIntegerTrackEvaluate(v + 4, &tmp, gRuntimeFrameSeconds);
    *(f32 *)(v + 0x14) += gRuntimeFrameSeconds * 3.0f;
    if (*(f32 *)(v + 0x14) > 3.5f) {
        *(f32 *)(v + 0x14) = 3.5f;
    }
    if (t != NULL) {
        *(f32 *)(effect + 0x18) = *(f32 *)(t + 0x44);
        *(f32 *)(effect + 0x1C) = *(f32 *)(t + 0x48);
        *(f32 *)(effect + 0x20) = *(f32 *)(t + 0x4C);
        *(f32 *)(effect + 0x44) = *(f32 *)(effect + 0x18);
        *(f32 *)(effect + 0x48) = *(f32 *)(effect + 0x1C);
        *(f32 *)(effect + 0x4C) = *(f32 *)(effect + 0x20);
    }
    *(f32 *)(effect + 0) = *(f32 *)(v + 0x14);
    *(f32 *)(effect + 4) = *(f32 *)(v + 0x14);
    *(f32 *)(effect + 8) = *(f32 *)(v + 0x14);
    objectOrientAlongCameraView(effect);
}


/// Prepare an expanding orange effect for drawing with its current fading appearance.
/// \param effect Effect object to draw.
/// \returns Address of the effect's display list.
///
/// \note Requires initialized effect resources, valid active matrix buffers where used, and an active
/// display-list cursor with sufficient capacity. Drawing resources remain owned by the effect and must stay
/// live until rendering finishes. The effect texture bank must be loaded.
/// \note Original decompilation identifier: \c func_8001BBF0 (0x8001BBF0).
s32 expandingOrangeEffectPrepareDraw(u8 *effect) {
    u8 *obj;
    u8 alpha;

    obj = *(u8 **)(effect + 0x54);
    alpha = mathMapRangeClamped(0.5f, 255.0f, 3.5f, 0.0f, *(f32 *)(obj + 0x14));
    textureLoadForDraw(&gRenderDisplayListCursor, 5, *(s16 *)(obj + 0xC));
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 0xFF, 0x96, 0, alpha);
    return *(s32 *)(effect + 0x30);
}

/// Initialize an emitter of expanding orange effects.
/// \param emitter Emitter object to initialize; must not already own state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001BD04 (0x8001BD04).
void orangePulseEmitterInitialize(void *emitter) {
    u8 *sp1C;
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0xC, 0x95E);
    sp1C = allocation;
    _bzero((void *)allocation, 0xC);
    ((RuntimeObjectPrefix *)emitter)->state.bytes = sp1C;
}


/// Emit expanding orange effects at a live target until the emitter's lifetime expires or its target
/// disappears.
/// \param emitter Emitter with target and lifetime configured by its creation function.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001BD50 (0x8001BD50).
void orangePulseEmitterUpdate(u8 *emitter) {
    u8 *v = *(u8 **)(emitter + 0x54);

    *(f32 *)(v + 4) -= gRuntimeFrameSeconds;
    if (*(f32 *)(v + 4) < 0.0f) {
        objectRequestHierarchyDestruction(emitter);
        return;
    }
    *(f32 *)(v + 8) -= gRuntimeFrameSeconds;
    if (*(f32 *)(v + 8) < 0.0f) {
        *(f32 *)(v + 8) = 0.30000001192092896f;
        if (objectResolveAvailableHandle(*(s32 *)v) != 0) {
            expandingOrangeEffectCreate(*(s32 *)v, 1.0f);
            return;
        }
        *(f32 *)(v + 4) = 0.0f;
    }
}

/// Create an emitter that repeatedly produces expanding orange effects at a target.
/// \param targetHandle Object handle identifying the target.
/// \param lifetime Emitter lifetime in seconds.
/// \returns Registered emitter managed by the object system; an unavailable target does not prevent creation.
/// \note Original decompilation identifier: \c func_8001BE0C (0x8001BE0C).
u8 *orangePulseEmitterCreate(s32 targetHandle, f32 lifetime) {
    u8 *sp1C;
    u8 *emitterState;

    sp1C = objectCreateFromTemplate(gOrangePulseEmitterObjectTemplate);
    emitterState = *(u8 **)(sp1C + 0x54);
    *(f32 *)(emitterState + 4) = lifetime;
    *(s32 *)emitterState = targetHandle;
    objectMakeHierarchyRoot(sp1C);
    objectRegisterActiveHierarchy(sp1C);
    return sp1C;
}
