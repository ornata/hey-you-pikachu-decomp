#include "script_vm_execution.h"
#include "matrix_orientation.h"
#include "actor_script_time.h"
#include "script_vm.h"
#include "actor_meters.h"
#include "actor_part_shake.h"
#include "actor_appearance.h"
#include "actor_part_texture.h"
#include "collision_registration.h"
#include "action_query.h"
#include "heap_free.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "actor_parts.h"
#include "integer_animation.h"
#include "object_lookup.h"
#include "actor_script.h"
#include "heap_alloc.h"
#include "object_types.h"
/// Owned animated-texture state for the tinted model part.
/// Command storage belongs to this allocation; channel tracks borrow animation programs.
typedef struct ActorPartTextureState {
    f32 playbackRates[3]; ///< Per-channel texture animation time scales.
    u8 unknown00C[8];
    ValueTrack tracks[3]; ///< Three integer texture-frame tracks.
    u8 unknown038[0x18];
    Gfx commandBuffers[2][80]; ///< Drawing commands indexed by framebuffer.
    u8 tint[3];                ///< Red, green and blue primitive-color components.
    u8 unknown553;
    s32 collisionKey; ///< Collision-grid registration key, or zero when unregistered.
} ActorPartTextureState;
typedef char ActorPartTextureStateSizeCheck[(sizeof(ActorPartTextureState) == 0x558) ? 1 : -1];
typedef char ActorPartTextureTracksOffsetCheck[(((u32) & ((ActorPartTextureState *)0)->tracks) == 0x14) ? 1 : -1];
typedef char
    ActorPartTextureCommandsOffsetCheck[(((u32) & ((ActorPartTextureState *)0)->commandBuffers) == 0x50) ? 1 : -1];
typedef char ActorPartTextureTintOffsetCheck[(((u32) & ((ActorPartTextureState *)0)->tint) == 0x550) ? 1 : -1];
typedef char
    ActorPartTextureCollisionOffsetCheck[(((u32) & ((ActorPartTextureState *)0)->collisionKey) == 0x554) ? 1 : -1];

/// Owned rotation-follower state borrowing another part from the same actor.
typedef struct ActorPartLinkedRotationState {
    u16 linkedPartId;   ///< Identifier resolved within the owning actor.
    u16 axis;           ///< Rotation component from zero through two.
    u8 *linkedPart;     ///< Borrowed resolved part, or null before the first update.
    f32 referenceAngle; ///< Linked-part angle captured when its reference is resolved.
    f32 responseFactor; ///< Angular response factor for deviation from the reference angle.
} ActorPartLinkedRotationState;
typedef char ActorPartLinkedRotationSizeCheck[(sizeof(ActorPartLinkedRotationState) == 0x10) ? 1 : -1];
typedef char
    ActorPartLinkedRotationReferenceOffsetCheck[(((u32) & ((ActorPartLinkedRotationState *)0)->linkedPart) == 4) ? 1
                                                                                                                 : -1];
typedef char ActorPartLinkedRotationFactorOffsetCheck
    [(((u32) & ((ActorPartLinkedRotationState *)0)->responseFactor) == 0xC) ? 1 : -1];

void objectDebugReportNoOp(u8 *);
/// Model display-list addresses indexed by appearance slot zero variant.
/// \note Original identifier: \c D_800760C4 (0x800760C4).
extern s32 gActorAppearanceSlot0Models[];
/// Model display-list addresses indexed by appearance slot one variant.
/// \note Original identifier: \c D_800760AC (0x800760AC).
extern s32 gActorAppearanceSlot1Models[];
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern u8 *gPikachuActor;
/// Model-part identifiers controlled by each actor appearance slot.
/// \note Original identifier: \c D_800760E8 (0x800760E8).
extern s32 gActorAppearancePartIds[];
void actorComposePartOrientation(void *, s32, s32 *, s32);
void matrix34Identity(void *);
void matrix34Concatenate(void *, void *, void *);
/// Signed part-identifier sequence used to track model-part relative roll.
/// \note Original identifier: \c D_800A2770 (0x800A2770).
extern u8 gActorRelativeRollPartSequence[];
f32 mathMapRangeClamped(f32, f32, f32, f32, f32);
f32 mathAngleDistanceDegrees(f32, f32);
f32 mathLerpAngleDegrees(f32, f32, f32);
/// Shared integer animation programs selecting texture frames for actors, effects and interface elements.
/// \note Original identifier: \c D_8006B090 (0x8006B090).
extern s32 gTextureFrameAnimationPrograms[];
void actorPartBuildAnimatedTextureCommands(u8 *, s32);
/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;
/// Framebuffer and command-buffer index for the current frame.
/// \note Original identifier: \c D_800F298C (0x800F298C).
extern s32 gGraphicsFramebufferIndex;
extern u8 D_1049F80[];

/// Invoke the inactive actor and linked-object diagnostic reports for this build.
/// \param actor Actor whose linked object is included in the diagnostic request.
/// \param unused Unused callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013660 (0x80013660).
void actorDebugReportLinkedObjectNoOp(u8 *actor, s32 unused) {
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    unused = ((ActorScriptState *)actorState)->heldItemHandle;
    if (unused != 0) {
        objectDebugReportNoOp(objectFindLiveByHandle(unused));
    }
    objectDebugReportNoOp(actor);
}

void actorInitializeModelState(u8 *);

/// Initialize actor model resources, parts, and default gameplay model settings.
/// \param actor Actor whose model is initialized.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800136A4 (0x800136A4).
void actorInitializeGameplayModel(u8 *actor) {
    u8 *actorState;

    actorInitializeModelState(actor);
    actorAllocatePartTable(actor);
    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ((ActorScriptState *)actorState)->primaryPoseReference.captureReference = 1;
    ((ActorScriptState *)actorState)->secondaryPoseReference.captureReference = 1;
    ((ActorScriptState *)actorState)->actionResource = 100.0f;
    ((ActorScriptState *)actorState)->secondaryMeter = 0.0f;
}

void actorReleaseModelResources(u8 *);

/// Release actor model resources.
/// \param object Actor whose model resources are released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800136F4 (0x800136F4).
void actorReleaseGameplayModel(u8 *object) {
    actorReleaseModelResources(object);
}

void _bzero(void *, s32);                        /* extern */
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

/// Initialize a model part to track orientation changes from a reference pose.
/// \param object Model part receiving owned orientation-tracking state; previous state must already be released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013714 (0x80013714).
void actorPartInitializeOrientationCache(u8 *object) {
    ActorOrientationCache *cache;
    ActorOrientationCache *allocation;

    allocation = heapAllocLocal(&gGameHeap, sizeof(ActorOrientationCache), 0xAA);
    cache = allocation;
    _bzero(allocation, sizeof(ActorOrientationCache));
    cache->captureReference = 1;
    ((RuntimeObjectPrefix *)object)->state.pointer = cache;
}

/// Initialize a model part to track two orientation changes from reference poses.
/// \param object Model part receiving owned paired tracking state; previous state must already be released.
/// \returns 1.
/// \note Original decompilation identifier: \c func_80013768 (0x80013768).
s32 actorPartInitializePairedOrientationCaches(u8 *object) {
    ActorOrientationCache *allocation = heapAllocLocal(&gGameHeap, 2 * sizeof(ActorOrientationCache), 0xB9);
    _bzero(allocation, 2 * sizeof(ActorOrientationCache));
    allocation[0].captureReference = 1;
    allocation[1].captureReference = 1;
    ((RuntimeObjectPrefix *)object)->state.pointer = allocation;
    return 1;
}

/// Linked part, rotation axis and response factor definitions for supported part kinds.
/// \note Original identifier: \c D_800A2750 (0x800A2750).
extern u8 gActorPartLinkedRotationDefinitions[];
/// Initialize a model part to follow rotation changes of its designated linked part.
/// \param object Part of a kind with a linked-rotation definition, receiving owned state; previous state must already be released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800137C0 (0x800137C0).
void actorPartInitializeLinkedRotation(u8 *object) {
    u8 *state;
    u8 *record;
    state = (u8 *)heapAllocLocal(&gGameHeap, 0x10, 0xD6);
    _bzero(state, 0x10);
    record = gActorPartLinkedRotationDefinitions + (((RuntimeObjectPrefix *)object)->kind * 8) - 0x348;
    ((ActorPartLinkedRotationState *)state)->linkedPartId = *(u16 *)record;
    ((ActorPartLinkedRotationState *)state)->axis = *(u16 *)(record + 2);
    ((ActorPartLinkedRotationState *)state)->responseFactor = *(f32 *)(record + 4);
    ((ActorPartLinkedRotationState *)state)->linkedPart = 0;
    ((ActorPartLinkedRotationState *)state)->referenceAngle = 0.0f;
    ((RuntimeObjectPrefix *)object)->state.bytes = state;
}

/// Default integer animation program for the first two actor texture channels.
/// \note Original identifier: \c D_8006B0B8 (0x8006B0B8).
extern s32 gActorPartDefaultTextureAnimation;
/// Default integer animation program for the third actor texture channel.
/// \note Original identifier: \c D_8006B180 (0x8006B180).
extern s32 gActorPartDefaultThirdTextureAnimation;
/// Initialize the model part animated textures, tint, and collision size.
/// \param object Model part receiving owned texture state; previous state must already be released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013848 (0x80013848).
void actorPartInitializeAnimatedTextures(u8 *object) {
    u8 *state;
    u8 *buffer;
    s32 i;
    state = (u8 *)heapAllocLocal(&gGameHeap, 0x558, 0xED);
    _bzero(state, 0x558);
    ((ActorPartTextureState *)state)->tint[0] = 0xBE;
    ((ActorPartTextureState *)state)->tint[1] = 0x28;
    ((ActorPartTextureState *)state)->tint[2] = 0x14;
    ((ActorPartTextureState *)state)->playbackRates[0] = 1.0f;
    ((ActorPartTextureState *)state)->playbackRates[1] = 1.0f;
    ((ActorPartTextureState *)state)->playbackRates[2] = 1.0f;
    animationIntegerTrackInitialize(&((ActorPartTextureState *)state)->tracks[0], (ValueKey *)(gActorPartDefaultTextureAnimation));
    animationIntegerTrackInitialize(&((ActorPartTextureState *)state)->tracks[1], (ValueKey *)(gActorPartDefaultTextureAnimation));
    animationIntegerTrackInitialize(&((ActorPartTextureState *)state)->tracks[2], (ValueKey *)(gActorPartDefaultThirdTextureAnimation));
    /* Initialize both command buffers directly; macro temporaries change the original frame. */
    for (i = 0, buffer = state; i != 2; i++, buffer += 0x280) {
        ((Gfx *)(buffer + 0x50))->words.w0 = (u32)G_ENDDL << 24;
        ((Gfx *)(buffer + 0x50))->words.w1 = 0;
        ((Gfx *)(buffer + 0x58))->words.w0 = (u32)G_ENDDL << 24;
        ((Gfx *)(buffer + 0x58))->words.w1 = 0;
        ((Gfx *)(buffer + 0x60))->words.w0 = (u32)G_ENDDL << 24;
        ((Gfx *)(buffer + 0x60))->words.w1 = 0;
        ((Gfx *)(buffer + 0x68))->words.w0 = (u32)G_RDPPIPESYNC << 24;
        ((Gfx *)(buffer + 0x68))->words.w1 = 0;
        ((Gfx *)(buffer + 0x70))->words.w0 = 0xFC30FEAC;
        ((Gfx *)(buffer + 0x70))->words.w1 = 0x40FE7838;
        ((Gfx *)(buffer + 0x78))->words.w0 = (u32)G_ENDDL << 24;
        ((Gfx *)(buffer + 0x78))->words.w1 = 0;
    }
    ((RuntimeObjectPrefix *)object)->state.bytes = state;
    *(f32 *)(object + 0x24) *= 1.5f;
}

/// Remove the model part collision registration and release its animated-texture resources.
/// \param object Actor model part with initialized animated-texture state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013968 (0x80013968).
void actorPartReleaseAnimatedTextures(u8 *object) {
    s32 textureStateAddress = *(s32 *)(object + 0x54);
    collisionGridUnregisterObject(object, &((ActorPartTextureState *)textureStateAddress)->collisionKey);
    heapFreeLocal(&gGameHeap, (void *)textureStateAddress);
}

/// Provide an inactive model-part initialization hook shared by two part kinds.
/// \param unused Unused callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800139A0 (0x800139A0).
void actorPartInitializeSharedNoOp(s32 unused) {
}

/// Provide an inactive initialization hook for the linked-item appearance part.
/// \param unused Unused callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800139A8 (0x800139A8).
void actorPartInitializeLinkedItemNoOp(s32 unused) {
}

/// Provide an inactive model-part initialization hook for this build.
/// \param unused Unused callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800139B0 (0x800139B0).
void actorPartInitializeNoOp(s32 unused) {
}

/// Update a model part rotation from its designated linked part.
/// \param object Part with initialized linked-rotation state whose designated linked part exists and remains valid.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800139B8 (0x800139B8).
void actorPartUpdateLinkedRotation(u8 *object) {
    f32 r;
    u8 *d;
    s32 k;
    u8 *o;
    u8 *p;
    f32 diff;
    s32 off;

    d = ((RuntimeObjectPrefix *)object)->state.bytes;
    p = ((ActorPartLinkedRotationState *)d)->linkedPart;
    k = ((ActorPartLinkedRotationState *)d)->axis;
    if (p != NULL) {
        o = p;
        off = k * 4;
    } else {
        p = actorFindPart(((RuntimeObjectPrefix *)object)->root.bytes, ((ActorPartLinkedRotationState *)d)->linkedPartId);
        o = p;
        off = k * 4;
        ((ActorPartLinkedRotationState *)d)->linkedPart = p;
        ((ActorPartLinkedRotationState *)d)->referenceAngle = *(f32 *)(p + off + 0xC);
    }
    diff = *(f32 *)(o + off + 0xC) - ((ActorPartLinkedRotationState *)d)->referenceAngle;
    r = mathLerpAngleDegrees(0.0f, diff, ((ActorPartLinkedRotationState *)d)->responseFactor);
    *(f32 *)(object + off + 0xC) = r;
}

/// Adjust model-part roll and scale to follow the actor primary pose.
/// \param object Part belonging to an actor with initialized gameplay pose state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013A58 (0x80013A58).
void actorPartFollowPrimaryRoll(u8 *object) {
    f32 partRoll;
    u8 *sub;
    u8 *data;

    sub = ((RuntimeObjectPrefix *)object)->root.bytes;
    data = ((RuntimeObjectPrefix *)sub)->state.bytes;
    ((RuntimeObjectPrefix *)object)->rotation[1] = 0;
    ((RuntimeObjectPrefix *)object)->rotation[0] = 0;
    partRoll = mathLerpAngleDegrees(0.0f, ((ActorScriptState *)data)->primaryPoseAngles[2], 0.45f);
    ((RuntimeObjectPrefix *)object)->rotation[2] = partRoll;
    ((RuntimeObjectPrefix *)object)->scale[1] = mathMapRangeClamped(0.0f, 1.0f, 30.0f, 1.1f, mathAngleDistanceDegrees(0.0f, partRoll));
}

/// Provide an inactive model-part update hook for this build.
/// \param unused Unused callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013AD8 (0x80013AD8).
void actorPartUpdateNoOp(s32 unused) {
}

/// Adjust model-part orientation and scale to follow the actor primary pose.
/// \param object Part belonging to an actor with initialized gameplay pose state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013AE0 (0x80013AE0).
void actorPartFollowPrimaryOrientation(u8 *object) {
    u8 *sub;
    u8 *data;
    f32 partRoll;

    sub = ((RuntimeObjectPrefix *)object)->root.bytes;
    data = ((RuntimeObjectPrefix *)sub)->state.bytes;
    ((RuntimeObjectPrefix *)object)->rotation[0] = mathLerpAngleDegrees(0.0f, ((ActorScriptState *)data)->primaryPoseAngles[0], 0.8f);
    ((RuntimeObjectPrefix *)object)->rotation[1] = mathLerpAngleDegrees(0.0f, ((ActorScriptState *)data)->primaryPoseAngles[1], 0.8f);
    partRoll = mathLerpAngleDegrees(0.0f, ((ActorScriptState *)data)->primaryPoseAngles[2], 0.8f);
    ((RuntimeObjectPrefix *)object)->rotation[2] = partRoll;
    ((RuntimeObjectPrefix *)object)->scale[1] = mathMapRangeClamped(0.0f, 1.0f, 55.0f, 1.35f, mathAngleDistanceDegrees(0.0f, partRoll));
}

/// Adjust model-part orientation and scale to follow the actor secondary pose.
/// \param object Part belonging to an actor with initialized gameplay pose state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013B90 (0x80013B90).
void actorPartFollowSecondaryOrientation(u8 *object) {
    u8 *parent = ((RuntimeObjectPrefix *)object)->root.bytes;
    u8 *state = ((RuntimeObjectPrefix *)parent)->state.bytes;
    f32 factor;
    f32 scale;
    ((RuntimeObjectPrefix *)object)->rotation[0] = mathLerpAngleDegrees(0.0f, ((ActorScriptState *)state)->secondaryPoseAngles[0], 0.35f);
    ((RuntimeObjectPrefix *)object)->rotation[1] = mathLerpAngleDegrees(0.0f, ((ActorScriptState *)state)->secondaryPoseAngles[1], 0.5f);
    ((RuntimeObjectPrefix *)object)->rotation[2] = mathLerpAngleDegrees(0.0f, ((ActorScriptState *)state)->secondaryPoseAngles[2], 0.5f);
    scale = mathMapRangeClamped(0.0f, 1.0f, 70.0f, 0.8f, mathAngleDistanceDegrees(0, ((RuntimeObjectPrefix *)object)->rotation[2]));
    factor = mathMapRangeClamped(0.0f, 1.0f, 55.0f, 0.8f, mathAngleDistanceDegrees(0, ((ActorScriptState *)state)->primaryPoseAngles[2]));
    ((RuntimeObjectPrefix *)object)->scale[0] = factor * scale;
    scale = mathMapRangeClamped(0.0f, 1.0f, 80.0f, 0.8f, mathAngleDistanceDegrees(0, ((RuntimeObjectPrefix *)object)->rotation[0]));
    factor = mathMapRangeClamped(0.0f, 1.0f, 55.0f, 0.8f, mathAngleDistanceDegrees(0, ((ActorScriptState *)state)->primaryPoseAngles[0]));
    ((RuntimeObjectPrefix *)object)->scale[2] = factor * scale;
}

/// Adjust model-part roll and scale from its tracked relative orientation.
/// \param object Part with initialized orientation-tracking state and a valid owning actor.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013CDC (0x80013CDC).
void actorPartFollowRelativeRoll(u8 *object) {
    ActorOrientationCache *state;
    s32 parent;
    f32 matrix[12];
    f32 x, y, z;
    parent = ((RuntimeObjectPrefix *)object)->root.address;
    state = (ActorOrientationCache *)((RuntimeObjectPrefix *)object)->state.pointer;
    actorEvaluateRelativePartOrientation(state, (u8 *)matrix, parent, (s32)gActorRelativeRollPartSequence);
    matrix33GetFoldedEulerDegrees(matrix, &x, &y, &z);
    ((RuntimeObjectPrefix *)object)->rotation[0] = 0;
    ((RuntimeObjectPrefix *)object)->rotation[1] = 0;
    ((RuntimeObjectPrefix *)object)->rotation[2] = mathLerpAngleDegrees(0.0f, z, 0.3f);
    *(f32 *)object = mathMapRangeClamped(0.0f, 1.0f, 90.0f, 1.25f, mathAngleDistanceDegrees(0.0f, z));
}

f32 mathWrapSignedDegrees(f32);
/// First signed part-identifier sequence used by paired orientation followers.
/// \note Original identifier: \c D_800A277C (0x800A277C).
extern u8 gActorBlendedOrientationFirstPartSequence[];
/// Second signed part-identifier sequence used by paired orientation followers.
/// \note Original identifier: \c D_800A2788 (0x800A2788).
extern u8 gActorBlendedOrientationSecondPartSequence[];
/// Adjust model-part orientation and scale using its paired tracked poses.
/// \param object Part with initialized paired orientation-tracking state and a valid owning actor.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013D7C (0x80013D7C).
void actorPartFollowBlendedOrientation(u8 *object) {
    s32 parent;
    ActorOrientationCache *state;
    f32 first[12];
    f32 second[12];
    f32 midpoint[12];
    f32 yawScale;
    f32 pitchScale;
    parent = ((RuntimeObjectPrefix *)object)->root.address;
    state = (ActorOrientationCache *)((RuntimeObjectPrefix *)object)->state.pointer;
    actorEvaluateRelativePartOrientation(state, (u8 *)first, parent, (s32)gActorBlendedOrientationFirstPartSequence);
    actorEvaluateRelativePartOrientation(state + 1, (u8 *)second, parent,
                                         (s32)gActorBlendedOrientationSecondPartSequence);
    matrix34BlendOrientation(0.5f, first, second, midpoint);
    matrix33GetFoldedEulerDegrees(midpoint, (f32 *)(object + 0xC), (f32 *)(object + 0x10), (f32 *)(object + 0x14));
    pitchScale = mathMapRangeClamped(0.0f, 1.0f, 45.0f, 0.78f, mathWrapSignedDegrees(((RuntimeObjectPrefix *)object)->rotation[0]));
    yawScale = mathMapRangeClamped(0.0f, 1.0f, 30.0f, 0.95f, mathAngleDistanceDegrees(0.0f, ((RuntimeObjectPrefix *)object)->rotation[1]));
    *(f32 *)(object + 8) = yawScale * pitchScale;
}

/// Set the color of the actor model part that supports tinting.
/// \param actor Actor containing an initialized tinted model part.
/// \param red Red color component, from 0 through 255.
/// \param green Green color component, from 0 through 255.
/// \param blue Blue color component, from 0 through 255.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013E6C (0x80013E6C).
void actorSetPartTint(u8 *actor, u8 red, u8 green, u8 blue) {
    u8 *obj;

    obj = *(u8 **)(actorFindPart(actor, 0x79) + 0x54);
    ((ActorPartTextureState *)obj)->tint[0] = red;
    ((ActorPartTextureState *)obj)->tint[1] = green;
    ((ActorPartTextureState *)obj)->tint[2] = blue;
}

/// Restore the default color of the actor model part that supports tinting.
/// \param actor Actor containing an initialized tinted model part.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013EB4 (0x80013EB4).
void actorResetPartTint(u8 *actor) {
    actorSetPartTint(actor, 0xBE, 0x28, 0x14);
}

Gfx *textureLoadForDraw(Gfx **, s32, s32);
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
typedef struct {
    s32 value[3];
} TextureSegments;
/// Texture resource segment identifiers for the three animated actor texture channels.
/// \note Original identifier: \c D_800760DC (0x800760DC).
extern TextureSegments gActorPartTextureSegments;
/// Advance the model part texture animations and prepare their drawing commands.
/// \param object Actor model part with initialized animated-texture state.
/// \param buffer Drawing buffer index, either 0 or 1.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80013EDC (0x80013EDC).
void actorPartBuildAnimatedTextureCommands(u8 *object, s32 buffer) {
    s32 primitiveAlpha = 255;
    TextureSegments segments = gActorPartTextureSegments;
    u8 *state = ((RuntimeObjectPrefix *)object)->state.bytes;
    s32 finalSegment = 3;
    Gfx *branches;
    Gfx *commands;
    s32 value;
    s32 segment;
    u8 *animation = state;
    f32 *frame = (f32 *)(state + 0x14);
    f32 *speed = (f32 *)state;
    buffer *= 0x280;
    branches = (Gfx *)(state + buffer + 0x50);
    commands = (Gfx *)(state + buffer + 0x78);
    for (segment = 0; segment < 3; segment++) {
        value = *(s16 *)(animation + 0x1C);
        animationIntegerTrackEvaluate((ValueTrack *)(frame), &value, *speed * gRuntimeFrameSeconds);
        gSPBranchList(branches++, commands);
        gDPPipeSync(commands++);
        textureLoadForDraw(&commands, segments.value[segment], value);
        gSPEndDisplayList(commands++);
        animation += 0xC;
        frame += 3;
        speed++;
    }
    gSPBranchList(branches++, commands);
    gSPBranchList(branches++, commands);
    gDPPipeSync(commands++);
    gDPSetPrimColor(commands++, 0, 0, ((ActorPartTextureState *)state)->tint[0], ((ActorPartTextureState *)state)->tint[1], ((ActorPartTextureState *)state)->tint[2], primitiveAlpha);
    gDPSetCombine(commands++, 0x30FEAC, 0x40FE7838);
    textureLoadForDraw(&commands, finalSegment, 0);
    gSPEndDisplayList(commands++);
}

/// Refresh the model part registration for collision queries.
/// \param object Actor model part with initialized animated-texture state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800140F0 (0x800140F0).
void actorPartUpdateCollisionRegistration(u8 *object) {
    u8 *obj;

    obj = ((RuntimeObjectPrefix *)object)->state.bytes;
    collisionGridUnregisterObject(object, &((ActorPartTextureState *)obj)->collisionKey);
    ((ActorPartTextureState *)obj)->collisionKey = collisionGridRegisterObject(object);
}

/// Evaluate the combined orientation of a signed sequence of actor model parts.
/// \param matrix Output 3-by-4 orientation matrix; identity when no listed parts are available.
/// \param owner Address of the actor containing the listed model parts.
/// \param cursor Zero-terminated sequence of part identifiers, excluding the minimum signed integer; negative identifiers request inverse part rotations.
/// \param forward Nonzero to evaluate the sequence orientation, or zero to evaluate its inverse.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001412C (0x8001412C).
void actorComposePartOrientation(void *matrix, s32 owner, s32 *cursor, s32 forward) {
    u8 *node;
    s32 direction;
    s32 count;
    s32 first;
    s32 id;
    if (forward)
        direction = 1;
    else
        direction = -1;
    count = 0;
    while (*cursor != 0) {
        count++;
        cursor++;
    }
    if (direction > 0)
        cursor -= count;
    else
        cursor--;
    first = 1;
    for (; count != 0; count--) {
        id = *cursor;
        if (direction > 0)
            cursor++;
        else
            cursor--;
        node = actorFindPart((u8 *)owner, id >= 0 ? id : -id);
        if (node == NULL)
            continue;
        {
            if (first != 0) {
                first = 0;
                if (id * direction > 0)
                    matrix34MakeRotationXYZDegrees(matrix, ((RuntimeObjectPrefix *)node)->rotation[0], ((RuntimeObjectPrefix *)node)->rotation[1], ((RuntimeObjectPrefix *)node)->rotation[2]);
                else
                    matrix34MakeRotationZYXDegrees(matrix, -((RuntimeObjectPrefix *)node)->rotation[0], -((RuntimeObjectPrefix *)node)->rotation[1], -((RuntimeObjectPrefix *)node)->rotation[2]);
            } else if (id * direction > 0)
                matrix33ApplyRotationXYZDegrees(matrix, ((RuntimeObjectPrefix *)node)->rotation[0], ((RuntimeObjectPrefix *)node)->rotation[1], ((RuntimeObjectPrefix *)node)->rotation[2]);
            else
                matrix33ApplyRotationZYXDegrees(matrix, -((RuntimeObjectPrefix *)node)->rotation[0], -((RuntimeObjectPrefix *)node)->rotation[1], -((RuntimeObjectPrefix *)node)->rotation[2]);
        }
    }
    if (first)
        matrix34Identity(matrix);
}

/// Evaluate model-part orientation relative to a saved reference pose.
/// \param cache Initialized writable orientation-tracking state; a set reset flag captures a new reference pose on this call.
/// \param matrix Output 3-by-4 relative orientation matrix; identity when capturing a new reference pose.
/// \param actor Address of the actor containing the listed model parts.
/// \param parts Address of a zero-terminated signed part-identifier sequence.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800142E0 (0x800142E0).
void actorEvaluateRelativePartOrientation(ActorOrientationCache *cache, u8 *matrix, s32 actor, s32 parts) {
    if (cache->captureReference != 0) {
        cache->captureReference = 0;
        actorComposePartOrientation(&cache->inverseReference, actor, (s32 *)parts, 0);
        matrix34Identity(matrix);
        return;
    }
    actorComposePartOrientation(matrix, actor, (s32 *)parts, 1);
    matrix34Concatenate(&cache->inverseReference, matrix, matrix);
}

f32 mathSinDegrees(f32);
/// Advance the temporary shake of the selected actor model part.
/// \param object Actor whose selected model part is shaking.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014360 (0x80014360).
void actorUpdatePartShake(u8 *object) {
    u8 *state = ((RuntimeObjectPrefix *)object)->state.bytes;
    u8 *child;
    u16 *childState;
    f32 rotation[3];
    if (((ActorScriptState *)state)->shakingPart != NULL) {
        child = ((ActorScriptState *)state)->shakingPart;
        childState = *(u16 **)(child + 0x54);
        ((ActorScriptState *)state)->partShakeTimeRemaining -= gRuntimeFrameSeconds;
        if (((ActorScriptState *)state)->partShakeTimeRemaining <= 0.0f) {
            ((ActorScriptState *)state)->shakingPart = NULL;
            if (*childState == 0) {
                ((RuntimeObjectPrefix *)child)->rotation[0] = ((ActorScriptState *)state)->partShakeReferenceAngles[0];
                ((RuntimeObjectPrefix *)child)->rotation[1] = ((ActorScriptState *)state)->partShakeReferenceAngles[1];
                ((RuntimeObjectPrefix *)child)->rotation[2] = ((ActorScriptState *)state)->partShakeReferenceAngles[2];
            }
        } else {
            rotation[0] = mathSinDegrees(((ActorScriptState *)state)->partShakeTimeRemaining * 1200.0f) * 3.0f;
            rotation[1] = mathSinDegrees(((ActorScriptState *)state)->partShakeTimeRemaining * 2400.0f) * 8.0f;
            rotation[2] = 0.0f;
            if (*childState == 0) {
                rotation[0] += ((ActorScriptState *)state)->partShakeReferenceAngles[0];
                rotation[1] += ((ActorScriptState *)state)->partShakeReferenceAngles[1];
                rotation[2] += ((ActorScriptState *)state)->partShakeReferenceAngles[2];
                ((RuntimeObjectPrefix *)child)->rotation[0] = rotation[0];
                ((RuntimeObjectPrefix *)child)->rotation[1] = rotation[1];
                ((RuntimeObjectPrefix *)child)->rotation[2] = rotation[2];
                return;
            }
            ((RuntimeObjectPrefix *)child)->rotation[0] += rotation[0];
            ((RuntimeObjectPrefix *)child)->rotation[1] += rotation[1];
            ((RuntimeObjectPrefix *)child)->rotation[2] += rotation[2];
        }
    }
}

u32 randomNext(void);
/// Upper duration bound for randomized actor model-part shaking, in seconds.
/// \note Original identifier: \c D_800A27B0 (0x800A27B0).
extern f64 gActorPartShakeMaximumSeconds;

/// Start or extend a temporary shake of an actor model part.
/// \param actor Address of the actor containing the model part.
/// \param part Identifier of an existing model part with initialized behavior state that remains valid throughout the shake.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800144D4 (0x800144D4).
void actorStartPartShake(u8 *actor, s32 part) {
    u8 *obj;
    u8 *t;

    obj = ((RuntimeObjectPrefix *)actor)->state.bytes;
    t = actorFindPart(actor, part);
    if (t != ((ActorScriptState *)obj)->shakingPart) {
        ((ActorScriptState *)obj)->partShakeReferenceAngles[0] = ((RuntimeObjectPrefix *)t)->rotation[0];
        ((ActorScriptState *)obj)->partShakeReferenceAngles[1] = ((RuntimeObjectPrefix *)t)->rotation[1];
        ((ActorScriptState *)obj)->partShakeReferenceAngles[2] = ((RuntimeObjectPrefix *)t)->rotation[2];
    }
    ((ActorScriptState *)obj)->shakingPart = t;
    ((ActorScriptState *)obj)->partShakeTimeRemaining =
        (f32)((((f64)randomNext() * 2.3283064365386963e-10 * 0.5) + 0.5) * gActorPartShakeMaximumSeconds);
}

void actorUpdateModel(u8 *);
void actorUpdatePartShake(u8 *);
/// Signed part-identifier sequence defining the actor primary tracked pose.
/// \note Original identifier: \c D_800A2794 (0x800A2794).
extern u8 gActorPrimaryPosePartSequence[];
/// Signed part-identifier sequence defining the actor secondary tracked pose.
/// \note Original identifier: \c D_800A27A0 (0x800A27A0).
extern u8 gActorSecondaryPosePartSequence[];
/// Advance the actor gameplay pose, model effects, and action resource.
/// \param object Actor to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014590 (0x80014590).
void actorUpdateGameplayModel(u8 *object) {
    u8 *state = ((RuntimeObjectPrefix *)object)->state.bytes;
    if (object != gPikachuActor)
        actorUpdateScriptVm(object);
    actorUpdateModel(object);
    actorEvaluateRelativePartOrientation(&((ActorScriptState *)state)->primaryPoseReference, (u8 *)&((ActorScriptState *)state)->primaryPoseTransform, (s32)object,
                                         (s32)gActorPrimaryPosePartSequence);
    matrix33GetFoldedEulerDegrees((f32 *)&((ActorScriptState *)state)->primaryPoseTransform, &((ActorScriptState *)state)->primaryPoseAngles[0], &((ActorScriptState *)state)->primaryPoseAngles[1], &((ActorScriptState *)state)->primaryPoseAngles[2]);
    actorEvaluateRelativePartOrientation(&((ActorScriptState *)state)->secondaryPoseReference, (u8 *)&((ActorScriptState *)state)->secondaryPoseTransform, (s32)object,
                                         (s32)gActorSecondaryPosePartSequence);
    matrix33GetFoldedEulerDegrees((f32 *)&((ActorScriptState *)state)->secondaryPoseTransform, &((ActorScriptState *)state)->secondaryPoseAngles[0], &((ActorScriptState *)state)->secondaryPoseAngles[1], &((ActorScriptState *)state)->secondaryPoseAngles[2]);
    actorUpdatePartShake(object);
    switch (actionGetAttributes(object) & 0xC0000000) {
    case 0:
        actorAdjustActionMeter(object, gRuntimeFrameSeconds);
        return;
    case 0x40000000:
        actorAdjustActionMeter(object, gRuntimeFrameSeconds * -10.0f);
        return;
    case 0xC0000000:
        actorAdjustActionMeter(object, gRuntimeFrameSeconds * 10.0f);
    case 0x80000000:
        return;
    }
}

/// Prepare the model part animated textures for the current draw.
/// \param object Actor model part with initialized animated-texture state.
/// \returns Borrowed address of the model part display list.
/// \note Original decompilation identifier: \c func_800146DC (0x800146DC).
s32 actorPartPrepareAnimatedTextureDraw(u8 *object) {
    u8 *obj;

    obj = ((RuntimeObjectPrefix *)object)->state.bytes;
    actorPartBuildAnimatedTextureCommands(object, gGraphicsFramebufferIndex);
    gSPSegment(gRenderDisplayListCursor++, 6, osVirtualToPhysical(obj + 0x50 + gGraphicsFramebufferIndex * 0x280));
    return *(s32 *)(object + 0x30);
}

/// Find the actor model part controlled by an appearance slot.
/// \param actor Address of the actor whose appearance is queried or changed.
/// \param slot Appearance slot: 0 for part 0x36, or 1 for part 0x3D.
/// \returns A borrowed model part owned by the actor, or null when absent.
/// \note Original decompilation identifier: \c func_80014760 (0x80014760).
f32 *actorAppearanceGetPart(u8 *actor, s32 slot) {
    return (f32 *)actorFindPart(actor, gActorAppearancePartIds[slot]);
}

/// Set the uniform scale of an actor appearance part.
/// \param actor Address of the actor whose appearance is queried or changed.
/// \param slot Appearance slot: 0 for part 0x36, or 1 for part 0x3D.
/// \param scale Uniform scale applied to the selected part, which must exist.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001478C (0x8001478C).
void actorAppearanceSetScale(u8 *actor, s32 slot, f32 scale) {
    f32 *p;

    p = actorAppearanceGetPart(actor, slot);
    p[0] = scale;
    p[1] = scale;
    p[2] = scale;
}

/// Query the scale of an actor appearance part.
/// \param actor Address of the actor whose appearance is queried or changed.
/// \param slot Appearance slot: 0 for part 0x36, or 1 for part 0x3D.
/// \returns The selected part scale along its first axis; the part must exist.
/// \note Original decompilation identifier: \c func_800147C0 (0x800147C0).
f32 actorAppearanceGetScale(u8 *actor, s32 slot) {
    return *actorAppearanceGetPart(actor, slot);
}

/// Select the model variant for an actor appearance slot.
/// \param actor Address of the actor whose appearance is queried or changed.
/// \param slot Appearance slot: 0 for part 0x36, or 1 for part 0x3D.
/// \param variant Valid model-variant identifier for the selected slot, or zero to use its default model.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800147E4 (0x800147E4).
void actorAppearanceSetVariant(u8 *actor, s32 slot, s32 variant) {
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ((ActorScriptState *)actorState)->appearanceVariants[slot] = variant;
}

/// Query the model variant selected for an actor appearance slot.
/// \param actor Address of the actor whose appearance is queried or changed.
/// \param slot Appearance slot: 0 for part 0x36, or 1 for part 0x3D.
/// \returns The selected model-variant identifier, or zero for the default model.
/// \note Original decompilation identifier: \c func_800147F8 (0x800147F8).
s32 actorAppearanceGetVariant(u8 *actor, s32 slot) {
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    return ((ActorScriptState *)actorState)->appearanceVariants[slot];
}

/// Restore default model variants and unit scale for both actor appearance slots.
/// \param actor Address of the actor whose appearance is queried or changed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001480C (0x8001480C).
void actorAppearanceReset(u8 *actor) {
    s32 var_s0;

    var_s0 = 0;
    do {
        actorAppearanceSetScale(actor, var_s0, 1.0f);
        actorAppearanceSetVariant(actor, var_s0, 0);
        var_s0 += 1;
    } while (var_s0 != 2);
}

/// Select the model to draw for actor appearance slot 1.
/// \param object Appearance part belonging to the actor whose variant is selected.
/// \returns Borrowed address of the selected variant model, or the part default model.
/// \note Original decompilation identifier: \c func_80014880 (0x80014880).
s32 actorPartGetAppearanceSlot1Model(u8 *object) {
    u8 *actorState = ((RuntimeObjectPrefix *)object)->root.object->state.bytes;
    s32 variant = ((ActorScriptState *)actorState)->appearanceVariants[1];
    if (variant == 0) {
        return *(s32 *)(object + 0x30);
    }
    return gActorAppearanceSlot1Models[variant];
}

/// Select the model to draw for actor appearance slot 0.
/// \param object Appearance part belonging to the actor whose variant is selected.
/// \returns Borrowed address of the selected variant model, or the part default model.
/// \note Original decompilation identifier: \c func_800148B0 (0x800148B0).
s32 actorPartGetAppearanceSlot0Model(u8 *object) {
    u8 *actorState = ((RuntimeObjectPrefix *)object)->root.object->state.bytes;
    s32 variant = ((ActorScriptState *)actorState)->appearanceVariants[0];
    if (variant == 0) {
        return *(s32 *)(object + 0x30);
    }
    return gActorAppearanceSlot0Models[variant];
}

/// Select a model-part appearance appropriate to the actor linked item.
/// \param object Model part belonging to the actor whose linked item is considered.
/// \returns A borrowed linked-item variant model when applicable, otherwise the part default model.
/// \note Original decompilation identifier: \c func_800148E0 (0x800148E0).
u8 *actorPartGetLinkedItemModel(u8 *object) {
    u8 *state;
    s32 targetId;
    u8 *target;
    state = ((RuntimeObjectPrefix *)object)->root.object->state.bytes;
    targetId = *(s32 *)(state + 0x190);
    if (targetId == 0 || *(s32 *)(state + 0x194) != 3) {
        return *(u8 **)(object + 0x30);
    }
    target = objectResolveAvailableHandle(targetId);
    if (target == NULL || *(s16 *)(*(u8 **)(target + 0x54) + 4) != 0x50) {
        return *(u8 **)(object + 0x30);
    }
    return D_1049F80;
}

/// Select a texture animation and its playback speed for the actor tinted model part.
/// \param actor Actor containing an initialized model part with animated textures.
/// \param speed Nonnegative playback speed multiplier; zero holds the current animation position.
/// \param channel Texture-animation channel, from 0 through 2.
/// \param animation Valid animation identifier for the selected channel.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_80014960 (0x80014960).
void actorSetPartTextureAnimation(u8 *actor, f32 speed, s32 channel, s32 animation) {
    int off;
    u8 *textureState;
    textureState = *(u8 **)(actorFindPart(actor, 0x79) + 0x54);
    ((ActorPartTextureState *)textureState)->playbackRates[channel] = speed;
    off = channel * 0xC;
    animationIntegerTrackInitialize((ValueTrack *)((textureState + off) + 0x14), (ValueKey *)(gTextureFrameAnimationPrograms[animation]));
}

/// Adjust the actor secondary gameplay meter within its allowed range.
/// \param object Object whose owning actor has the meter to adjust.
/// \param delta Signed change to apply to the meter.
/// \returns The updated meter value, from 0 through 100.
/// \note Original decompilation identifier: \c func_800149D0 (0x800149D0).
f32 actorAdjustSecondaryMeter(u8 *object, f32 delta) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)object)->root.object->state.bytes;
    ((ActorScriptState *)actorState)->secondaryMeter = ((ActorScriptState *)actorState)->secondaryMeter + delta;
    if (((ActorScriptState *)actorState)->secondaryMeter < 0.0f) {
        ((ActorScriptState *)actorState)->secondaryMeter = 0.0f;
    } else if (((ActorScriptState *)actorState)->secondaryMeter > 100.0f) {
        ((ActorScriptState *)actorState)->secondaryMeter = 100.0f;
    }
    return ((ActorScriptState *)actorState)->secondaryMeter;
}

extern float fabsf(float);
#pragma intrinsic(fabsf)
void hudExtendHorizontalMotionDisplay(f32);
/// Minimum change magnitude requesting the action-meter display when the meter changes.
/// \note Original identifier: \c D_800A27B8 (0x800A27B8).
extern f32 gActorActionMeterDisplayDeltaThreshold;
/// Low action-meter threshold requesting its display when the meter changes.
/// \note Original identifier: \c D_800A27BC (0x800A27BC).
extern f32 gActorActionMeterDisplayLowThreshold;

/// Adjust the actor action resource and request its HUD display when appropriate.
/// \param object Object whose owning actor has the action resource to adjust.
/// \param delta Signed change to apply to the action resource.
/// \returns The updated action resource, from 0 through 100.
/// \note Original decompilation identifier: \c func_80014A3C (0x80014A3C).
f32 actorAdjustActionMeter(u8 *object, f32 delta) {
    u8 *state;
    f32 meter;

    state = ((RuntimeObjectPrefix *)object)->root.object->state.bytes;
    meter = ((ActorScriptState *)state)->actionResource + delta;
    if (meter < 0.0f) {
        meter = 0.0f;
    } else if (meter > 100.0f) {
        meter = 100.0f;
    }
    if ((fabsf(delta) >= 1.0f) || ((meter != ((ActorScriptState *)state)->actionResource) &&
                                   ((gActorActionMeterDisplayDeltaThreshold <= fabsf(delta)) ||
                                    (meter <= gActorActionMeterDisplayLowThreshold) || (meter == 100.0f)))) {
        hudExtendHorizontalMotionDisplay(3.0f);
    }
    ((ActorScriptState *)state)->actionResource = meter;
    return meter;
}

/// Query the primary actor script elapsed time.
/// \pre The primary actor and its script context must be initialized.
/// \returns Total elapsed time in seconds accumulated by the primary actor script context.
/// \note Original decompilation identifier: \c func_80014B3C (0x80014B3C).
f32 actorGetPrimaryScriptElapsedTime(void) {
    u8 *actorState = ((RuntimeObjectPrefix *)gPikachuActor)->state.bytes;
    u8 *scriptContext = (u8 *)((ActorScriptState *)actorState)->vmAddress;
    return ((ScriptVm *)scriptContext)->totalElapsed;
}
