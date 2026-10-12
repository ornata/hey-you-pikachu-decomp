#include "action_query.h"
#include "actor_rotation.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "direction_animation.h"
#include "actor_part_config.h"
#include "matrix_types.h"
#include "actor_parts.h"
#include "object_audio.h"
#include "actor_target.h"
#include "integer_animation.h"
#include "object_types.h"
#include "actor_script.h"
#include "heap_types.h"
#include "word_map.h"

/* Diagnostic format strings whose callers were compiled out. */
static const char sUnknownHoldPosFmt[] = "Unknown holdPos:%d\n";
static const char sJointTableFmt[] = "haraType:%d jointTotal:%d jointTableSize:%d\n";
static const char sOutOfMazeFmt[] = "ERROR: model %p(%s) is out of maze\n";

/// Owned texture animation tracks and double-buffered display lists for one component.
typedef struct AnimatedTextureState {
    s32 count;             ///< Number of initialized texture tracks, from zero through three.
    s32 textureIds[3];     ///< Texture selected for each animation track.
    f32 playbackRates[3];  ///< Per-track animation time multipliers.
    ValueTrack tracks[3];  ///< Integer frame-selection animation state.
    Gfx *displayLists[2];  ///< Two views into one owned display-list allocation.
    s32 commandsPerBuffer; ///< Reserved command count for each drawing buffer.
} AnimatedTextureState;

typedef char AnimatedTextureStateSizeCheck[(sizeof(AnimatedTextureState) == 0x4C) ? 1 : -1];
typedef char
    AnimatedTextureStateListsOffsetCheck[(((u32) & (((AnimatedTextureState *)0)->displayLists)) == 0x40) ? 1 : -1];

/* Lit model state: two display-list buffers, two light sets, the active set, env colour. */
typedef struct LitModel {
    /* 0x00 */ u8 pad0[0x54];
    /* 0x54 */ Gfx *dl[2];
    /* 0x5C */ u8 pad5C[4];
    /* 0x60 */ Lights2 lights[2];
    /* 0xB0 */ u8 padB0[0x22];
    /* 0xD2 */ u16 light;
    /* 0xD4 */ u8 padD4[0xC];
    /* 0xE0 */ u8 env[4];
} LitModel;

void heapFreeLocal();
/// Per-actor-kind part-table slot counts, including the root slot.
/// \note Original identifier: \c D_800B687C (0x800B687C).
extern s32 gActorPartCounts[];
/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;
void *heapAllocLocal();
void _bzero(void *, u32);
void matrix34MakeRotationXYZDegrees(f32 *, s32, s32, f32);
void matrix33Multiply(u8 *, f32 *, f32 *);
void matrix33GetFoldedEulerDegrees(f32 *, u8 *, u8 *, u8 *);
/// Current affine world transform shared by hierarchy traversal and actor motion consumers.
/// \note Original identifier: \c D_800AD9E0 (0x800AD9E0).
extern Matrix34 gObjectTraversalTransform;
s32 actorFindPartIndex(s32, s32);
void actorUpdateAnimatedTextureDrawLists(u8 *, s32);
void actorPopulatePartTable(u8 *, u8 *);
void actorInitializeChildMotionState(void *);
/// Framebuffer and command-buffer index for the current frame.
/// \note Original identifier: \c D_800F298C (0x800F298C).
extern s32 gGraphicsFramebufferIndex;

s32 audioCountPlayback(s32);
s32 audioStopPlayback(s32, f64);

/// Play a spatial sound for an actor and track its playback.
/// \param actor Actor with initialized model state emitting the sound.
/// \param kind Actor sound kind to play.
/// \returns Playback handle, or zero if playback could not start.
/// \note Original decompilation identifier: \c func_80014B60 (0x80014B60).
s32 actorPlayTrackedSound(u8 *actor, s32 kind) {
    u8 *v;
    s32 h;
    s32 i;
    s32 k;

    v = *(u8 **)(actor + 0x54);
    h = objectPlaySpatialSound(actor, kind, 20.0f, 200.0f);
    if (h != 0) {
        for (i = 0, k = *(s32 *)(v + 0x188); i < 4; i++) {
            if (audioCountPlayback(((s32 *)(v + 0x178))[k]) == 0) {
                break;
            }
            k++;
            k %= 4;
        }
        ((s32 *)(v + 0x178))[k] = h;
        *(s32 *)(v + 0x188) = (k + 1) % 4;
    }
    if (*(u16 *)(actor + 0x34) == 0x25 && kind >= 30 && kind < 125) {
        if (audioCountPlayback(*(s32 *)(v + 0x18C)) != 0) {
            audioStopPlayback(*(s32 *)(v + 0x18C), 0.0);
        }
        *(s32 *)(v + 0x18C) = h;
    }
    return h;
}

/* Camera record: 0xE0 bytes per camera, of which only the active view matrix is read here. */
typedef struct CameraVoiceView {
    /* 0x00 */ u8 pad0[0x24];
    /* 0x24 */ Mtx *matrix;
    /* 0x28 */ u8 pad28[0xB8];
} CameraVoiceView;

/// Camera storage used for view transforms, viewports and color overlays.
/// \note Original identifier: \c D_800ABCF0 (0x800ABCF0).
extern u8 gRenderCameras[];
/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern u32 gRenderCameraIndex;
/// Identifier of the currently active runtime program.
/// \note Original identifier: \c D_800AE4F0 (0x800AE4F0).
extern s32 gRuntimeCurrentProgram;
void audioGetPositionVolumeAndPan(f32 *, Mtx, s32 *, s32 *, f32, f32);
void audioSetSoundEffectPan(s32, s32);
void audioSetSoundEffectVolume(s32, s32);

/// Update tracked sounds for the listener view and forget completed playback.
/// \param position World-space sound position containing three floating-point coordinates.
/// \param voices Four tracked playback handles, updated to clear completed sounds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014C7C (0x80014C7C).
void actorUpdateTrackedSoundSpatialization(u8 *position, s32 *voices) {
    s32 i;
    s32 volume;
    s32 pan;

    for (i = 0; i < 4; i++, voices++) {
        if (audioCountPlayback(*voices) != 0) {
            if (gRuntimeCurrentProgram == 0) {
                if (((CameraVoiceView *)gRenderCameras)[gRenderCameraIndex].matrix == NULL) {
                    continue;
                }
                audioGetPositionVolumeAndPan((f32 *)position,
                                             *((CameraVoiceView *)gRenderCameras)[gRenderCameraIndex].matrix, &volume,
                                             &pan, 20.0f, 100.0f);
                audioSetSoundEffectPan(*voices, pan);
                audioSetSoundEffectVolume(*voices, volume);
            }
        } else {
            *voices = 0;
        }
    }
}

void collisionGridUnregisterObject(u8 *, u8 *); /* extern */
void scriptVmDestroy(s32);        /* extern */
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

/// Release actor model resources and remove its collision registration.
/// \param actor Actor with initialized model state; its state pointer becomes invalid after release.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014DE8 (0x80014DE8).
void actorReleaseModelResources(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    heapFreeLocal(&gGameHeap, (u8 *)((ActorScriptState *)actorState)->modelDrawLists[0]);
    collisionGridUnregisterObject(actor, actorState + 0x210);
    scriptVmDestroy(((ActorScriptState *)actorState)->vmAddress);
    heapFreeLocal(&gGameHeap, ((ActorScriptState *)actorState)->partTable);
    heapFreeLocal(&gGameHeap, actorState);
}

/// Release an actor child motion state and its optional facing state.
/// \param actor Child object with initialized motion state; its state pointer becomes invalid after release.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014E54 (0x80014E54).
void actorReleaseChildMotionState(u8 *actor) {
    u8 *obj;
    u8 *sub;

    obj = *(u8 **)(actor + 0x54);
    sub = *(u8 **)(obj + 0x7C);
    if (sub != NULL) {
        heapFreeLocal(&gGameHeap, sub);
    }
    heapFreeLocal(&gGameHeap, obj);
}

s32 actorAssetGetCachedStorage(s32);

/// Prepare the model drawing setup for its textures, lighting, and environment color.
/// \param actor Actor whose model drawing setup is prepared.
/// \param model Model state with initialized lights, an active light index of zero or one, and drawing buffers.
/// \param buffer Drawing buffer to prepare, zero or one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014EA0 (0x80014EA0).
void actorPrepareModelLightingDrawList(u8 *actor, LitModel *model, s32 buffer) {
    Gfx *g;

    g = model->dl[buffer];
    gSPSegment(g++, 0x0A, osVirtualToPhysical((void *)actorAssetGetCachedStorage(*(u16 *)(actor + 0x34))));
    gSPSetLights2(g++, model->lights[model->light]);
    gDPPipeSync(g++);
    gDPSetEnvColor(g++, model->env[0], model->env[1], model->env[2], model->env[3]);
    gSPEndDisplayList(g++);
}

void actorPrepareModelLightingDrawList(u8 *, LitModel *, s32);
void matrix34Identity(u8 *); /* extern */

/// Allocate and initialize actor model state and its drawing resources.
/// \param actor Actor with a cached model asset receiving newly owned model state; existing state is not released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014FE4 (0x80014FE4).
void actorInitializeModelState(u8 *actor) {
    s32 var_s0;
    u8 *allocation;
    u8 *drawListStorage;

    allocation = heapAllocLocal(&gGameHeap, 0x408, 0xC3);
    _bzero(allocation, 0x408);
    ((ActorScriptState *)allocation)->activeRequestId = 0;
    ((ActorScriptState *)allocation)->nextRequestId = 1;
    ((ActorScriptState *)allocation)->playbackMode = 0;
    ((ActorScriptState *)allocation)->pendingAction = -1;
    ((ActorScriptState *)allocation)->messengerContactMode = 2;
    ((ActorScriptState *)allocation)->motionDuration = 0.0f;
    ((ActorScriptState *)allocation)->motionRemaining = 0.0f;
    ((ActorScriptState *)allocation)->movementScale = 1.0f;
    matrix34Identity(allocation + 0x18);
    _bzero(allocation + 0x23C, 0x78);
    _bzero(allocation + 0x2B4, 0x3C);
    ((ActorScriptState *)allocation)->airborne = 0;
    ((ActorScriptState *)allocation)->nextSoundSlot = 0;
    _bzero(allocation + 0x178, 0x10);
    ((ActorScriptState *)allocation)->trackedVoice = 0;
    ((ActorScriptState *)allocation)->actionScriptId = -1;
    ((ActorScriptState *)allocation)->searchItemKind = 0x43;
    ((ActorScriptState *)allocation)->searchItemRange = 0.0f;
    directionAnimationReset((u8 *)&((ActorScriptState *)allocation)->directionAnimation);
    drawListStorage = heapAllocLocal(&gGameHeap, 0x80, 0xE8);
    ((ActorScriptState *)allocation)->modelDrawLists[0] = (Gfx *)drawListStorage;
    ((ActorScriptState *)allocation)->modelDrawLists[1] = (Gfx *)(drawListStorage + 0x40);
    var_s0 = 0;
    do {
        actorPrepareModelLightingDrawList(actor, (LitModel *)allocation, var_s0);
        var_s0 += 1;
    } while (var_s0 != 2);
    ((RuntimeObjectPrefix *)actor)->state.bytes = allocation;
}

/// Allocate initial motion state for an actor child object.
/// \param actor Child object receiving newly owned motion state; existing state is not released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015110 (0x80015110).
void actorInitializeChildMotionState(void *actor) {
    s16 *sp1C;
    s16 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x80, 0xFC);
    sp1C = allocation;
    _bzero(allocation, 0x80);
    *sp1C = 0;
    ((RuntimeObjectPrefix *)actor)->state.pointer = sp1C;
}

/// Initialize a child object for blended facing toward its parent target.
/// \param actor Child object whose current orientation initializes target-facing behavior.
/// \returns Nothing.
/// \note Portability: The allocator call retains an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_80015160 (0x80015160).
void actorInitializeTargetFacingChildState(u8 *actor) {
    u8 *obj;
    f32 *vec;

    actorInitializeChildMotionState(actor);
    obj = *(u8 **)(actor + 0x54);
    vec = (f32 *)heapAllocLocal(&gGameHeap, 0x1C, 0x10C, actor);
    vec[0] = *(f32 *)(actor + 0xC);
    vec[1] = *(f32 *)(actor + 0x10);
    vec[2] = *(f32 *)(actor + 0x14);
    vec[3] = *(f32 *)(actor + 0xC);
    vec[4] = *(f32 *)(actor + 0x10);
    vec[5] = *(f32 *)(actor + 0x14);
    vec[6] = 0.0f;
    *(f32 **)(obj + 0x7C) = vec;
}

/// Initialize a child object for blended facing toward the camera.
/// \param actor Child object whose current orientation initializes camera-facing behavior.
/// \returns Nothing.
/// \note Portability: The allocator call retains an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_800151E4 (0x800151E4).
void actorInitializeCameraFacingChildState(u8 *actor) {
    u8 *obj;
    f32 *vec;

    actorInitializeChildMotionState(actor);
    obj = *(u8 **)(actor + 0x54);
    vec = (f32 *)heapAllocLocal(&gGameHeap, 0x10, 0x11C, actor);
    vec[0] = *(f32 *)(actor + 0xC);
    vec[1] = *(f32 *)(actor + 0x10);
    vec[2] = *(f32 *)(actor + 0x14);
    vec[3] = 0.0f;
    *(f32 **)(obj + 0x7C) = vec;
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
f32 mathUnitDirectionDegrees(f32, f32);
f32 mathSinDegrees(f32);
f32 mathCosDegrees(f32);
f32 terrainGetWorldHeight(f32, f32, f32, u8 **);
f32 mathWrapSignedDegrees(f32);
f32 mathApproachAngleDegrees(f32, f32, f32, f32, f32);

/// Smoothly tilt the actor to follow the ground slope ahead.
/// \param object Actor with initialized model state whose pitch follows the terrain.
/// \returns Nothing.
/// \note Portability: The empty surface check preserves the IDO live ranges required by the ROM.
/// \note Original decompilation identifier: \c func_80015250 (0x80015250).
void actorUpdateGroundSlopePitch(u8 *object) {
    u8 *state;
    f32 height;
    f32 nextX;
    f32 nextHeight;
    f32 heightDelta;
    f32 length;
    f32 pitch;
    f32 headingX;
    f32 headingZ;
    u8 *surface;

    state = *(u8 **)(object + 0x54);
    surface = NULL;
    headingX = mathSinDegrees(*(f32 *)(state + 0x10));
    headingZ = mathCosDegrees(*(f32 *)(state + 0x10));
    height = terrainGetWorldHeight(*(f32 *)(object + 0x44), *(f32 *)(object + 0x48), *(f32 *)(object + 0x4C), &surface);
    nextX = *(f32 *)(object + 0x44) + headingX;
    nextHeight = terrainGetWorldHeight(nextX, *(f32 *)(object + 0x48), *(f32 *)(object + 0x4C) + headingZ, &surface);
    heightDelta = height - nextHeight;
    if (surface == NULL) {
    }
    length = sqrtf(heightDelta * heightDelta + 1.0f);
    pitch = mathWrapSignedDegrees(mathUnitDirectionDegrees(1.0f / length, heightDelta / length));
    if (pitch < -30.0f) {
        pitch = -30.0f;
    } else if (pitch > 30.0f) {
        pitch = 30.0f;
    }
    actorAddRotationAxisDegrees(
        object,
        mathApproachAngleDegrees(*(f32 *)(state + 0xC), pitch, 1.0f, 90.0f, 180.0f * gRuntimeFrameSeconds) -
            *(f32 *)(state + 0xC),
        0);
}

typedef struct Vec3f {
    f32 x, y, z;
} Vec3f;
void terrainGetDownhillHeightDifferences(f32, f32, s32, f32, f32, f32 *, f32 *, s32);
f32 vector3Normalize(u8 *);

/// Add downhill drift to actor movement according to the nearby terrain slope.
/// \param actor Actor whose position determines the terrain slope.
/// \param displacement Three-component movement vector receiving additional horizontal drift.
/// \param surfaceCache Address of the cached terrain surface pointer used and updated by the slope query.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015394 (0x80015394).
void actorAddDownhillMovement(u8 *actor, f32 *displacement, s32 surfaceCache) {
    Vec3f v;
    f32 f;

    f = 0.2f;
    v.y = f;
    terrainGetDownhillHeightDifferences(*(f32 *)(actor + 0x18), *(f32 *)(actor + 0x1C), *(s32 *)(actor + 0x20), 50.0f, v.y, &v.x, &v.z, surfaceCache);
    vector3Normalize((u8 *)&v);
    f = 10.0f * gRuntimeFrameSeconds;
    displacement[0] += v.x * f;
    displacement[2] += v.z * f;
}

/// Apply the actor's prepared model drawing setup.
/// \param actor Actor whose current model drawing setup is applied.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015438 (0x80015438).
void actorDrawModelLightingSetup(u8 *actor) {
    gSPDisplayList(gRenderDisplayListCursor++,
                   ((u32 *)(*(u8 **)(actor + 0x54) + 0x54))[(*(u16 *)(actor + 0x36) & 0x8000) ? 1 : 0]);
}

void actorUpdateObjectRotationFromState(u8 *);
s32 actionScriptUpdate(u8 *);
void actionUpdatePlayback(u8 *);
void actorUpdateTrackedSoundSpatialization(u8 *, s32 *);
void actorUpdateGroundSlopePitch(u8 *);
void objectUpdateHierarchyInteractionVisibility(u8 *);
s32 collisionGridRegisterObject(u8 *);
void colorAdvanceTransition(u8 *, u8 *, u8 *);

/// Update actor behavior, animation, spatial state, and model drawing setup.
/// \param actor Actor with initialized model, action, hierarchy, and collision state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015488 (0x80015488).
void actorUpdateModel(u8 *actor) {
    u8 *obj;

    obj = *(u8 **)(actor + 0x54);
    collisionGridUnregisterObject(actor, obj + 0x210);
    actionScriptUpdate(actor);
    actionUpdatePlayback(actor);
    actorUpdateTrackedSoundSpatialization(actor + 0x44, (s32 *)(obj + 0x178));
    if (!(actionGetAttributes(actor) & 0x20000000) && !(*(u16 *)(obj + 0x1E8) & 8)) {
        actorUpdateGroundSlopePitch(actor);
    }
    actorUpdateObjectRotationFromState(actor);
    actorRefreshTrackingTargetPosition(actor);
    if ((gRuntimeCurrentProgram == 0) && !(*(u16 *)(obj + 0x1E8) & 0x20)) {
        objectUpdateHierarchyInteractionVisibility(actor);
    }
    *(s32 *)(obj + 0x210) = collisionGridRegisterObject(actor);
    directionAnimationUpdate((u8 *)&((ActorScriptState *)obj)->directionAnimation);
    colorAdvanceTransition(obj + 0xE0, obj + 0xE4, obj + 0xE8);
    actorPrepareModelLightingDrawList(actor, (LitModel *)obj, ((*(u16 *)(actor + 0x36) & 0x8000) ? 1 : 0) ^ 1);
}

/// Request a timed transition to a new actor model environment color.
/// \param actor Actor whose model environment color will change.
/// \param r Target red component from zero to 255.
/// \param g Target green component from zero to 255.
/// \param b Target blue component from zero to 255.
/// \param a Target alpha component from zero to 255.
/// \param duration Nonnegative transition duration in seconds; zero applies the target on the next update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015588 (0x80015588).
void actorTransitionModelEnvironmentColor(u8 *actor, u8 r, u8 g, u8 b, u8 a, f32 duration) {
    u8 *v;

    v = *(u8 **)(actor + 0x54);
    v[0xE4] = r;
    v[0xE5] = g;
    v[0xE6] = b;
    v[0xE7] = a;
    *(f32 *)(v + 0xE8) = duration;
}

/// Set the actor model environment color immediately.
/// \param actor Actor whose model environment color is changed.
/// \param r Red component from zero to 255.
/// \param g Green component from zero to 255.
/// \param b Blue component from zero to 255.
/// \param a Alpha component from zero to 255.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800155B8 (0x800155B8).
void actorSetModelEnvironmentColor(u8 *actor, u8 r, u8 g, u8 b, u8 a) {
    u8 *v;

    v = *(u8 **)(actor + 0x54);
    v[0xE0] = r;
    v[0xE1] = g;
    v[0xE2] = b;
    v[0xE3] = a;
}

void motionUpdateChildPlayback(u8 *);

/// Advance motion playback for an actor child object.
/// \param object Child object whose motion playback is updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800155E0 (0x800155E0).
void actorUpdateChildMotion(u8 *object) {
    motionUpdateChildPlayback(object);
}

/// Provide the child-object update callback for advancing motion playback.
/// \param object Child object whose motion playback is updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015600 (0x80015600).
void actorUpdateChildMotionCallback(u8 *object) {
    actorUpdateChildMotion(object);
}

typedef struct CamRec {
    f32 x;
    f32 y;
    f32 z;
    u8 pad[0xD4];
} CamRec;

/// Resolve the parent tracking position, including camera targets and the stored-position fallback.
/// \param child Child object with a valid parent matching parentState.
/// \param parentState Parent actor state containing tracking settings and the fallback position.
/// \param position Output array receiving three world-space coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015620 (0x80015620).
void actorGetParentTrackingPosition(u8 *child, u8 *parentState, f32 *position) {
    u8 *t;

    if (*(s32 *)(parentState + 0x19C) == 1) {
        t = actorResolveStoredTarget(*(u8 **)(child + 0x50));
        if (t != NULL) {
            if (*(u16 *)(t + 0x34) == 0x7F) {
                position[0] = ((CamRec *)gRenderCameras)[gRenderCameraIndex].x;
                position[1] = ((CamRec *)gRenderCameras)[gRenderCameraIndex].y;
                position[2] = ((CamRec *)gRenderCameras)[gRenderCameraIndex].z;
            } else {
                position[0] = *(f32 *)(t + 0x44);
                position[1] = *(f32 *)(t + 0x48);
                position[2] = *(f32 *)(t + 0x4C);
            }
            return;
        }
    }
    position[0] = *(f32 *)(parentState + 0x1EC);
    position[1] = *(f32 *)(parentState + 0x1F0);
    position[2] = *(f32 *)(parentState + 0x1F4);
}


void vector3FromZAxisPitchYawDegrees(f32, f32, f32 *);
void matrix33Transpose(f32 *);
void vector3GetEulerDegrees(f32 *, f32 *, f32 *, f32 *);
void matrix34TransformPointVector(f32 *, f32 *, f32 *);

/// Update child animation and blend its orientation toward the parent target when tracking is enabled.
/// \param object Child object with initialized target-facing state and a valid parent.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015704 (0x80015704).
void actorUpdateTargetFacingChild(u8 *object) {
    u8 *state;
    f32 rollOffset = 180.0f;
    f32 fullTurn = 360.0f;
    u8 *parent;
    u8 *parentState;
    f32 delta[3];
    f32 initial[3];
    Matrix34 matrix;
    f32 roll, pitch, yaw;
    f32 position[3];
    f32 previous[3];
    f32 *weight;
    u8 *follow;
    f32 length;
    s32 i;

    state = *(u8 **)(object + 0x54);
    parent = *(u8 **)(object + 0x50);
    follow = *(u8 **)(state + 0x7C);
    parentState = *(u8 **)(parent + 0x54);
    motionUpdateChildPlayback(object);
    weight = (f32 *)(follow + 0x18);
    if ((actionGetAttributes(parent) & 0x400000) && !(*(u16 *)(parentState + 0x1E8) & 1)) {
        *weight += 2.0f * gRuntimeFrameSeconds;
        if (*weight > 2.0f) {
            *weight = 2.0f;
        }
    } else {
        *weight -= 5.0f * gRuntimeFrameSeconds;
        if (*weight < 0.0f) {
            *weight = 0.0f;
        }
    }
    if (*(u16 *)state == 1 || *(u16 *)state == 3) {
        *(f32 *)(follow + 0) = *(f32 *)(object + 0xC);
        *(f32 *)(follow + 4) = *(f32 *)(object + 0x10);
        *(f32 *)(follow + 8) = *(f32 *)(object + 0x14);
    } else {
    }
    if (*weight > 0.0f) {
        actorGetParentTrackingPosition(object, parentState, position);
        delta[0] = position[0] - *(f32 *)(object + 0x44);
        delta[1] = position[1] - *(f32 *)(object + 0x48);
        delta[2] = position[2] - *(f32 *)(object + 0x4C);
        do {
            matrix = gObjectTraversalTransform;
        } while (0);
        matrix33Transpose(&matrix.m[0][0]);
        matrix.m[3][0] = 0.0f;
        matrix.m[3][1] = 0.0f;
        matrix.m[3][2] = 0.0f;
        matrix34TransformPointVector(&matrix.m[0][0], delta, delta);
        length = vector3Normalize((u8 *)delta) * 50.0f;
        if (length < *weight) {
            *weight = length;
        }
        vector3FromZAxisPitchYawDegrees(*(f32 *)(follow + 4), *(f32 *)(follow + 8), initial);
        vector3FromZAxisPitchYawDegrees(*(f32 *)(follow + 16), *(f32 *)(follow + 20), previous);
        for (i = 0; i < 3; i++) {
            delta[i] = (delta[i] * *weight + initial[i]) + 2.0f * (previous[i] * *weight);
        }
        vector3Normalize((u8 *)delta);
        if (delta[0] > -0.5f) {
            delta[0] = -0.5f;
        }
        if (delta[1] < -0.4f) {
            delta[1] = -0.4f;
        } else if (delta[1] > 0.4f) {
            delta[1] = 0.4f;
        }
        vector3GetEulerDegrees(delta, &roll, &pitch, &yaw);
        roll += rollOffset;
        while (roll < 0.0f) {
            roll += fullTurn;
        }
        while (roll >= fullTurn) {
            roll -= fullTurn;
        }
        *(f32 *)(object + 0xC) = roll;
        *(f32 *)(object + 0x10) = pitch;
        *(f32 *)(object + 0x14) = yaw;
    }
    *(f32 *)(follow + 12) = *(f32 *)(object + 0xC);
    *(f32 *)(follow + 16) = *(f32 *)(object + 0x10);
    *(f32 *)(follow + 20) = *(f32 *)(object + 0x14);
}

/// Fixed direction transform used when blending child orientation toward the camera.
/// \note Original identifier: \c D_80076C80 (0x80076C80).
extern u8 gActorCameraFacingDirectionTransform[];
/* Facing state at state+0x7C: the stored angles and the blend weight. */
typedef struct FacingOrientation {
    f32 angles[3];
    f32 weight;
} FacingOrientation;

/// Update child animation and blend its orientation toward the active camera when tracking is enabled.
/// \param object Child object with initialized camera-facing state and a valid parent.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015AE8 (0x80015AE8).
void actorUpdateCameraFacingChild(u8 *object) {
    u16 *state;
    FacingOrientation *orientation;
    f32 maxWeight = 2.0f;
    Vec3f direction;
    Vec3f original;
    Matrix34 matrix;
    f32 pitch;
    f32 yaw;
    f32 roll;
    f32 x;
    f32 *weight;

    state = *(u16 **)(object + 0x54);
    orientation = *(FacingOrientation **)((u8 *)state + 0x7C);
    motionUpdateChildPlayback(object);
    weight = &orientation->weight;
    if ((actionGetAttributes(*(u8 **)(object + 0x50)) & 0x800000) &&
        !(*(u16 *)(*(u8 **)(*(u8 **)(object + 0x50) + 0x54) + 0x1E8) & 2)) {
        *weight += 2.0f * gRuntimeFrameSeconds;
        if (*weight > maxWeight) {
            *weight = maxWeight;
        }
    } else {
        *weight -= 5.0f * gRuntimeFrameSeconds;
        if (*weight < 0.0f) {
            *weight = 0.0f;
        }
    }
    if (*state == 1 || *state == 3) {
        orientation->angles[0] = *(f32 *)(object + 0xC);
        orientation->angles[1] = *(f32 *)(object + 0x10);
        orientation->angles[2] = *(f32 *)(object + 0x14);
    }
    if (*weight > 0.0f) {
        direction.x = ((CamRec *)gRenderCameras)[gRenderCameraIndex].x - *(f32 *)(object + 0x44);
        direction.y = ((CamRec *)gRenderCameras)[gRenderCameraIndex].y - *(f32 *)(object + 0x48);
        direction.z = ((CamRec *)gRenderCameras)[gRenderCameraIndex].z - *(f32 *)(object + 0x4C);
        matrix34TransformPointVector((f32 *)gActorCameraFacingDirectionTransform, &direction.x, &direction.x);
        do {
            matrix = gObjectTraversalTransform;
        } while (0);
        matrix33Transpose(&matrix.m[0][0]);
        matrix.m[3][0] = 0.0f;
        matrix.m[3][1] = 0.0f;
        matrix.m[3][2] = 0.0f;
        matrix34TransformPointVector(&matrix.m[0][0], &direction.x, &direction.x);
        vector3Normalize((u8 *)&direction);
        direction.x *= *weight;
        direction.y *= *weight;
        direction.z *= *weight;
        vector3FromZAxisPitchYawDegrees(orientation->angles[1], orientation->angles[2], &original.x);
        direction.x += original.x;
        direction.y += original.y;
        direction.z += original.z;
        vector3Normalize((u8 *)&direction);
        x = -0.99f;
        if (direction.x < -0.99f) {
            direction.x = x;
        } else {
            x = -0.8f;
            if (direction.x > -0.8f) {
                direction.x = x;
            }
        }
        x = 0.2f;
        if (direction.y < 0.2f) {
            direction.y = x;
        } else {
            x = 0.3f;
            if (direction.y > 0.3f) {
                direction.y = x;
            }
        }
        vector3GetEulerDegrees(&direction.x, &pitch, &yaw, &roll);
        *(f32 *)(object + 0xC) = -180.0f;
        *(f32 *)(object + 0x10) = yaw;
        *(f32 *)(object + 0x14) = roll;
    }
}

/// Bind the referenced object transform for actor drawing.
/// \param actor Actor with a valid referenced object transform.
/// \returns Object drawing continuation link.
/// \note Original decompilation identifier: \c func_80015E34 (0x80015E34).
s32 actorBindReferencedObjectMatrixForDraw(u8 *actor) {
    gSPSegment(gRenderDisplayListCursor++, 7,
               osVirtualToPhysical(((void **)(*(u8 **)(actor + 0x50) +
                                              0x3C))[(*(u16 *)(*(u8 **)(actor + 0x50) + 0x36) & 0x8000) ? 1 : 0]));
    return *(s32 *)(actor + 0x30);
}

/// Update object rotation from the actor orientation state and reference transform.
/// \param actor Actor with initialized orientation state and a current reference transform.
/// \returns Nothing.
/// \note Portability/UB: The local rotation-helper declaration passes float bit patterns as integer arguments, relying on the original ABI.
/// \note Original decompilation identifier: \c func_80015EB8 (0x80015EB8).
void actorUpdateObjectRotationFromState(u8 *actor) {
    u8 *obj;
    f32 sp1C[12];
    obj = *(u8 **)(actor + 0x54);
    matrix34MakeRotationXYZDegrees(sp1C, *(s32 *)(obj + 0xC), *(s32 *)(obj + 0x10), 0.0f);
    matrix33Multiply(obj + 0x18, sp1C, sp1C);
    matrix33GetFoldedEulerDegrees(sp1C, actor + 0xC, actor + 0x10, actor + 0x14);
}

/// Set one actor orientation angle in degrees.
/// \param actor Actor whose orientation is updated.
/// \param angle Finite orientation angle in degrees.
/// \param axis Orientation component: zero for pitch, one for yaw, or two for roll.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015F1C (0x80015F1C).
void actorSetRotationAxisDegrees(u8 *actor, f32 angle, s32 axis) {
    u8 *obj;
    s32 off;

    obj = ((RuntimeObjectPrefix *)actor)->state.bytes;
    off = axis * 4;
    if (angle < 0.0f) {
        do {
            angle += 360.0f;
        } while (angle < 0.0f);
    }
    if (angle >= 360.0f) {
        do {
            angle -= 360.0f;
        } while (angle >= 360.0f);
    }
    *(f32 *)(obj + off + 0xC) = angle;
}

/// Rotate the actor by an angular displacement on one orientation axis.
/// \param actor Actor whose orientation is updated.
/// \param angle Finite angular displacement in degrees.
/// \param axis Orientation component: zero for pitch, one for yaw, or two for roll.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015F94 (0x80015F94).
void actorAddRotationAxisDegrees(u8 *actor, f32 angle, s32 axis) {
    u8 *obj;
    f32 *p;

    obj = ((RuntimeObjectPrefix *)actor)->state.bytes;
    p = &((f32 *)obj)[axis];
    p[3] = p[3] + angle;
    while (p[3] < 0.0f) {
        p[3] += 360.0f;
    }
    while (p[3] >= 360.0f) {
        p[3] -= 360.0f;
    }
}


/// Rotate the actor on one axis when the current action permits turning.
/// \param actor Actor whose orientation is updated.
/// \param angle Finite angular displacement in degrees.
/// \param axis Orientation component: zero for pitch, one for yaw, or two for roll.
/// \returns Nothing.
/// \pre Updated angles must remain finite and allow full-turn adjustments to change their floating-point values.
/// \note Original decompilation identifier: \c func_80016030 (0x80016030).
void actorAddActionRotationAxisDegrees(u8 *actor, f32 angle, s32 axis) {
    u8 *obj;
    f32 *p;

    if (actionGetAttributes(actor) & ACTION_ATTRIBUTE_ALLOW_TURNING) {
        obj = ((RuntimeObjectPrefix *)actor)->state.bytes;
        p = &((f32 *)obj)[axis];
        p[3] = p[3] + angle;
        while (p[3] < 0.0f) {
            p[3] += 360.0f;
        }
        while (p[3] >= 360.0f) {
            p[3] -= 360.0f;
        }
    }
}

/// Adjust actor pitch and yaw when the current action permits turning.
/// \param actor Actor whose orientation is updated.
/// \param angles Two finite pitch and yaw displacements in degrees, in that order.
/// \returns Nothing.
/// \pre Updated angles must remain finite and allow full-turn adjustments to change their floating-point values.
/// \note Original decompilation identifier: \c func_800160FC (0x800160FC).
void actorAddActionPitchAndYawDegrees(u8 *actor, f32 *angles) {
    u8 *v;

    if (actionGetAttributes(actor) & ACTION_ATTRIBUTE_ALLOW_TURNING) {
        v = ((RuntimeObjectPrefix *)actor)->state.bytes;
        ((ActorScriptState *)v)->orientation[0] += angles[0];
        while (((ActorScriptState *)v)->orientation[0] < 0.0f) {
            ((ActorScriptState *)v)->orientation[0] += 360.0f;
        }
        while (((ActorScriptState *)v)->orientation[0] >= 360.0f) {
            ((ActorScriptState *)v)->orientation[0] -= 360.0f;
        }
        ((ActorScriptState *)v)->orientation[1] += angles[1];
        while (((ActorScriptState *)v)->orientation[1] < 0.0f) {
            ((ActorScriptState *)v)->orientation[1] += 360.0f;
        }
        while (((ActorScriptState *)v)->orientation[1] >= 360.0f) {
            ((ActorScriptState *)v)->orientation[1] -= 360.0f;
        }
    }
}

/// Synchronize the actor orientation state with its object rotation.
/// \param actor Actor whose orientation state is synchronized.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016228 (0x80016228).
void actorCopyObjectRotationToState(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ((ActorScriptState *)actorState)->orientation[0] = (f32)((RuntimeObjectPrefix *)actor)->rotation[0];
    ((ActorScriptState *)actorState)->orientation[1] = (f32)((RuntimeObjectPrefix *)actor)->rotation[1];
    ((ActorScriptState *)actorState)->orientation[2] = (f32)((RuntimeObjectPrefix *)actor)->rotation[2];
}

/// Owned lookup mapping packed actor-kind and part identifiers to part-table slots.
/// \note Original identifier: \c D_800B6918 (0x800B6918).
extern WordMap gActorPartIndexMap;

/// Register the named parts belonging to an actor type.
/// \param bank Nonnegative actor type within the part-count table and the 15-bit actor-type range.
/// \param ids Part identifiers in [0, 65535], terminated by minus one.
/// \returns Pointer to the terminating entry in the part identifier list.
/// \note Original decompilation identifier: \c func_80016248 (0x80016248).
s32 *actorRegisterPartIds(s32 bank, s32 *ids) {
    s32 n = 1;
    s32 snd;

    while (*ids != -1) {
        snd = *ids + (bank << 16);
        wordMapInsert(&gActorPartIndexMap, n, (u32)snd);
        n++;
        ids++;
    }
    gActorPartCounts[bank] = n;
    return ids;
}

/// Packed actor-kind and terminated part-identifier lists used to initialize the part registry.
/// \note Original identifier: \c D_800760F0 (0x800760F0).
extern s32 gActorPartRegistrationData;

s32 *actorRegisterPartIds(s32, s32 *);

/// Initialize the actor part lookup from the configured actor types.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800162EC (0x800162EC).
void actorInitializePartIdMap(void) {
    s32 *p;

    wordMapInitialize(&gActorPartIndexMap, 0x5CF);
    p = &gActorPartRegistrationData;
    while (*p != -2) {
        p = actorRegisterPartIds(*p, p + 1) + 1;
    }
}

/// Interior reset span of per-kind part counters; retains the original separate symbol and extent.
/// \note Original identifier: \c D_800B6880 (0x800B6880).
extern u8 gActorPartCountResetSpan[0x94];

/// Initialize actor part registration and per-type part counts.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016354 (0x80016354).
void actorInitializePartRegistry(void) {
    _bzero(&gActorPartCountResetSpan, 0x94);
    actorInitializePartIdMap();
}


/// Release the actor part identifier lookup.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016384 (0x80016384).
void actorDestroyPartIdMap(void) {
    wordMapDestroy(&gActorPartIndexMap);
}

s32 wordMapLookup(WordMap *, u32, s32, s32);

/// Find the index of a named part for an actor type.
/// \param actorType Nonnegative actor type in the 15-bit actor-type range.
/// \param partId Identifier of the requested part.
/// \returns Part index, including zero for the actor root, or minus one if no part exists.
/// \note Original decompilation identifier: \c func_800163A8 (0x800163A8).
s32 actorFindPartIndex(s32 actorType, s32 partId) {
    s32 partIndex;

    if (partId == actorType) {
        return 0;
    }
    partIndex = wordMapLookup(&gActorPartIndexMap, (actorType << 0x10) + partId, partId, actorType);
    if (partIndex != 0) {
        return partIndex;
    }
    return -1;
}

/// Find an actor part by its identifier.
/// \param actor Actor whose parts are searched.
/// \param partId Identifier of the requested part.
/// \returns Borrowed part pointer, or null if it does not exist.
/// \note Original decompilation identifier: \c func_800163FC (0x800163FC).
u8 *actorFindPart(u8 *actor, s32 partId) {
    u8 *sp1C;
    s32 partIndex;

    sp1C = *(u8 **)(actor + 0x54);
    partIndex = actorFindPartIndex(*(u16 *)(actor + 0x34), partId);
    if (partIndex < 0) {
        return 0;
    }
    return *(u8 **)(*(u8 **)(sp1C + 0x4C) + partIndex * 4);
}

/// Get the object handle of a named actor part.
/// \param actor Actor whose part is requested.
/// \param partId Identifier of the requested part.
/// \returns Part object handle, or zero if the part does not exist.
/// \note Original decompilation identifier: \c func_80016448 (0x80016448).
s32 actorGetPartObjectHandle(u8 *actor, s32 partId) {
    u8 *partObject;

    partObject = actorFindPart(actor, partId);
    if (partObject == NULL) {
        return 0;
    }
    return ((RuntimeObjectPrefix *)partObject)->handle;
}

/// Get the default reference part configured for the actor type.
/// \param actor Actor whose default reference part is requested.
/// \returns Borrowed default reference part, or null if it does not exist.
/// \note Original decompilation identifier: \c func_8001647C (0x8001647C).
u8 *actorGetDefaultPart(u8 *actor) {
    return actorFindPart(actor, actorConfigGetDefaultPartId(*(u16 *)(actor + 0x34)));
}

/// Get the world position of a named part, using the default part when the requested part is absent.
/// \param actor Actor containing the requested part and a valid default part.
/// \param partId Identifier of the preferred part.
/// \param position Output array receiving three world-space coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800164AC (0x800164AC).
void actorGetPartWorldPosition(u8 *actor, s32 partId, f32 *position) {
    u8 *var_v0;
    var_v0 = actorFindPart(actor, partId);
    if (var_v0 == NULL) {
        var_v0 = actorGetDefaultPart(actor);
    }
    position[0] = *(f32 *)(var_v0 + 0x44);
    position[1] = *(f32 *)(var_v0 + 0x48);
    position[2] = *(f32 *)(var_v0 + 0x4C);
}


/// Get the world-space attachment position selected by the actor attachment mode.
/// \param actor Actor whose attachment mode and parts determine the position.
/// \param heightOffset Vertical offset applied by the raised attachment mode.
/// \param position Output array receiving three world-space coordinates; unchanged for an unsupported attachment mode.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800164FC (0x800164FC).
void actorGetConfiguredAttachmentPosition(u8 *actor, f32 heightOffset, f32 *position) {
    u8 *state;
    f32 first[3];
    f32 second[3];

    state = *(u8 **)(actor + 0x54);
    switch (*(u32 *)(state + 0x194)) {
    case 0:
        actorGetPartWorldPosition(actor, actorConfigGetFirstAttachmentPartId(((RuntimeObjectPrefix *)actor)->kind), first);
        actorGetPartWorldPosition(actor, actorConfigGetSecondAttachmentPartId(((RuntimeObjectPrefix *)actor)->kind), second);
        ((f32 *)position)[0] = (first[0] + second[0]) * 0.5f;
        ((f32 *)position)[1] = (f32)((first[1] + second[1]) * 0.5f);
        ((f32 *)position)[2] = (f32)((first[2] + second[2]) * 0.5f);
        return;
    case 1:
        actorGetPartWorldPosition(actor, actorConfigGetFirstAttachmentPartId(((RuntimeObjectPrefix *)actor)->kind), position);
        return;
    case 2:
        actorGetPartWorldPosition(actor, actorConfigGetSecondAttachmentPartId(((RuntimeObjectPrefix *)actor)->kind), position);
        return;
    case 3:
        actorGetPartWorldPosition(actor, 0x76, position);
        ((f32 *)position)[1] = (f32)(((f32 *)position)[1] + heightOffset);
        return;
    case 4:
        actorGetPartWorldPosition(actor, 0x77, position);
        return;
    case 5:
        actorGetPartWorldPosition(actor, 0x42, position);
        /* fallthrough */
    default:
        return;
    }
}

/// Allocate empty state for an actor animated texture component.
/// \param actor Component receiving newly owned animated texture state; existing state is not released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016658 (0x80016658).
void actorAllocateAnimatedTextureState(void *actor) {
    AnimatedTextureState *state;
    AnimatedTextureState *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x4C, 0x463);
    state = allocation;
    _bzero(allocation, 0x4C);
    ((RuntimeObjectPrefix *)actor)->state.pointer = state;
}

/// Release an actor animated texture state and its drawing resources.
/// \param actor Component with initialized animated textures; its state pointer becomes invalid after release.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800166A4 (0x800166A4).
void actorReleaseAnimatedTextureState(u8 *actor) {
    AnimatedTextureState *obj;

    obj = *(AnimatedTextureState **)(actor + 0x54);
    heapFreeLocal(&gGameHeap, obj->displayLists[0], obj);
    heapFreeLocal(&gGameHeap, obj);
}

Gfx *textureLoadForDraw(Gfx **, s32, s32);

/// Advance animated textures and prepare their drawing commands.
/// \param actor Component with zero through three initialized texture tracks and allocated drawing buffers.
/// \param buffer Drawing buffer to prepare, zero or one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800166E4 (0x800166E4).
void actorUpdateAnimatedTextureDrawLists(u8 *actor, s32 buffer) {
    AnimatedTextureState *v;
    Gfx *dl;
    s32 i;
    Gfx *gfx;
    ValueTrack *track;
    s32 frame;
    Gfx *base;

    v = *(AnimatedTextureState **)(actor + 0x54);
    base = v->displayLists[buffer];
    dl = base;
    gfx = base + v->count;
    for (i = 0; i < v->count; i++) {
        gSPBranchList(dl++, gfx);
        track = &v->tracks[i];
        frame = track->value;
        animationIntegerTrackEvaluate(track, &frame, v->playbackRates[i] * gRuntimeFrameSeconds);
        textureLoadForDraw(&gfx, v->textureIds[i], frame);
        gSPEndDisplayList(gfx++);
    }
}

/// Update and bind the actor animated textures for drawing.
/// \param actor Animated texture component being drawn.
/// \returns Object drawing continuation link.
/// \note Original decompilation identifier: \c func_80016808 (0x80016808).
s32 actorBindAnimatedTexturesForDraw(u8 *actor) {
    AnimatedTextureState *obj;

    obj = *(AnimatedTextureState **)(actor + 0x54);
    actorUpdateAnimatedTextureDrawLists(actor, gGraphicsFramebufferIndex);
    gSPSegment(gRenderDisplayListCursor++, 6, osVirtualToPhysical(obj->displayLists[gGraphicsFramebufferIndex]));
    return *(s32 *)(actor + 0x30);
}

/// Shared integer animation programs selecting texture frames for actors, effects and interface elements.
/// \note Original identifier: \c D_8006B090 (0x8006B090).
extern s32 gTextureFrameAnimationPrograms[];

/// Configure an actor animated texture component from texture and animation definitions.
/// \param actor Actor with part 0x78 and allocated texture state; existing drawing buffers are not released.
/// \param parts Up to three valid texture/animation identifier pairs, terminated by a negative texture identifier; not retained.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016884 (0x80016884).
void actorInitializeAnimatedTextures(u8 *actor, s32 *parts) {
    u8 *obj;
    AnimatedTextureState *v;
    s32 frame;

    obj = actorFindPart(actor, 0x78);
    v = *(AnimatedTextureState **)(obj + 0x54);
    v->count = 0;
    for (; (frame = parts[0]) >= 0; parts += 2) {
        v->textureIds[v->count] = frame;
        v->playbackRates[v->count] = 1.0f;
        animationIntegerTrackInitialize(&v->tracks[v->count], (ValueKey *)gTextureFrameAnimationPrograms[parts[1]]);
        v->count++;
    }
    v->commandsPerBuffer = v->count * 20;
    v->displayLists[0] = heapAllocLocal(&gGameHeap, v->commandsPerBuffer * sizeof(Gfx) * 2, 0x4B8);
    v->displayLists[1] = v->displayLists[0] + v->commandsPerBuffer;
    actorUpdateAnimatedTextureDrawLists(obj, 0);
    actorUpdateAnimatedTextureDrawLists(obj, 1);
}

/// Allocate an empty part table sized for the actor type.
/// \param actor Actor with initialized model state and registered type receiving a new part table; existing tables are not released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800169B8 (0x800169B8).
void actorAllocatePartTable(u8 *actor) {
    u8 *v;
    s32 kind;
    s32 n;
    s32 size;

    v = *(u8 **)(actor + 0x54);
    kind = *(u16 *)(actor + 0x34);
    n = gActorPartCounts[kind];
    *(s32 *)(v + 0x50) = n;
    size = n * 4;
    *(void **)(v + 0x4C) = heapAllocLocal(&gGameHeap, size, 0x4D2);
    _bzero(*(void **)(v + 0x4C), size);
}

/// Register matching objects from a hierarchy in an actor part table.
/// \param hierarchy First object in the hierarchy to register, or null.
/// \param actor Actor whose allocated part table borrows matching objects for their lifetimes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016A1C (0x80016A1C).
void actorPopulatePartTable(u8 *hierarchy, u8 *actor) {
    s32 partIndex;
    u8 *firstChild;
    u8 *var_s0;

    var_s0 = hierarchy;
    if (hierarchy != NULL) {
        do {
            partIndex = actorFindPartIndex(*(u16 *)(actor + 0x34), *(u16 *)(var_s0 + 0x34));
            if (partIndex >= 0) {
                *(u8 **)(*(u8 **)(*(u8 **)(actor + 0x54) + 0x4C) + partIndex * 4) = var_s0;
            }
            firstChild = *(u8 **)(var_s0 + 0x2C);
            if (firstChild != NULL) {
                actorPopulatePartTable(firstChild, actor);
            }
            var_s0 = *(u8 **)(var_s0 + 0x28);
        } while (var_s0 != NULL);
    }
}

/// Set and clear object flags on selected actor parts.
/// \param actor Actor containing the selected parts.
/// \param set Low sixteen flag bits to enable.
/// \param clear Low sixteen flag bits to disable, taking precedence over enabled bits.
/// \param ids Part identifiers terminated by a negative identifier.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016A94 (0x80016A94).
void actorUpdateNamedPartFlags(u8 *actor, s32 set, s32 clear, s32 *ids) {
    u8 *e;
    s32 *p;

    for (p = ids; *p >= 0; p++) {
        e = actorFindPart(actor, *p);
        if (e != NULL) {
            *(u16 *)(e + 0x36) |= set;
            *(u16 *)(e + 0x36) &= ~clear;
        }
    }
}


/// Find the lowest world height among the actor configured reference parts.
/// \param actor Actor with a valid default reference part.
/// \returns Lowest reference-part world height, including the default reference part.
/// \note Original decompilation identifier: \c func_80016B1C (0x80016B1C).
f32 actorGetLowestReferencePartHeight(u8 *actor) {
    f32 min = *(f32 *)(actorGetDefaultPart(actor) + 0x48);
    const s32 *ids = actorConfigGetGroundReferencePartIds(*(u16 *)(actor + 0x34));
    s32 i;
    u8 *o;

    for (i = 0; ids[i] > 0; i++) {
        o = actorFindPart(actor, ids[i]);
        if (o != NULL && *(f32 *)(o + 0x48) < min) {
            min = *(f32 *)(o + 0x48);
        }
    }
    return min;
}

f32 terrainGetWorldHeightForQuery(s32, f32, f32, f32, u8 **);

/// Measure actor ground clearance at a displaced reference position.
/// \param actor Actor whose reference parts and ground offset determine clearance.
/// \param collisionMode Collision-mode argument retained for the terrain query; currently unused by that query.
/// \param reference Object providing the reference world position.
/// \param displacement Three-component displacement whose horizontal components offset the ground query.
/// \param groundHeight Output receiving the ground world height, or zero when no surface is found.
/// \param surface Optional cached surface pointer, updated by the ground query.
/// \returns Signed height above the ground after the actor ground offset is applied.
/// \note Original decompilation identifier: \c func_80016BA8 (0x80016BA8).
f32 actorMeasureGroundClearance(u8 *actor, s32 collisionMode, u8 *reference, f32 *displacement, f32 *groundHeight, u8 **surface) {
    u8 *v;
    const s32 *p;
    u8 *e;
    f32 top;

    v = *(u8 **)(actor + 0x54);
    top = *(f32 *)(reference + 0x48);
    for (p = actorConfigGetGroundReferencePartIds(*(u16 *)(actor + 0x34)); *p > 0; p++) {
        e = actorFindPart(actor, *p);
        if (e != NULL && *(f32 *)(e + 0x48) < top) {
            top = *(f32 *)(e + 0x48);
        }
    }
    *groundHeight =
        terrainGetWorldHeightForQuery(collisionMode, *(f32 *)(reference + 0x44) + displacement[0], *(f32 *)(reference + 0x48), *(f32 *)(reference + 0x4C) + displacement[2], surface);
    return top - *groundHeight - *(f32 *)(v + 0x20C);
}

void matrix33TransformVector(f32 *, f32 *, f32 *, u8 *);

/// Update scaled actor motion displacement with the applicable upward movement limit.
/// \param state Actor state whose pending motion displacement is updated.
/// \param status Movement status flags selecting the upward movement limit.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016C98 (0x80016C98).
void actorUpdateMotionDisplacement(u8 *state, s32 status) {
    f32 movementScale;
    f32 var_f0;

    matrix33TransformVector((f32 *)&gObjectTraversalTransform, (f32 *)(state + 0x1CC), (f32 *)(state + 0x1D8), state);
    movementScale = ((ActorScriptState *)state)->movementScale;
    ((ActorScriptState *)state)->worldMotion[0] = (f32)(((ActorScriptState *)state)->worldMotion[0] * movementScale);
    ((ActorScriptState *)state)->worldMotion[1] = (f32)(((ActorScriptState *)state)->worldMotion[1] * movementScale);
    ((ActorScriptState *)state)->worldMotion[2] = (f32)(((ActorScriptState *)state)->worldMotion[2] * movementScale);
    if (status & 0x200) {
        var_f0 = 30.0f;
        if (((ActorScriptState *)state)->worldMotion[1] > 30.0f) {
            goto block_4;
        }
    } else {
        var_f0 = 5.0f;
        if (((ActorScriptState *)state)->worldMotion[1] > 5.0f) {
        block_4:
            ((ActorScriptState *)state)->worldMotion[1] = var_f0;
        }
    }
}

/// Update the actor vertical pose offset and prepare its ground-reference pose.
/// \param actor Actor whose vertical pose offset is advanced.
/// \param reference Object providing the horizontal world position.
/// \param pose Output object receiving the ground-reference position and actor orientation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016D58 (0x80016D58).
void actorUpdateGroundReferencePose(u8 *actor, u8 *reference, u8 *pose) {
    u8 *v;
    f32(*m)[3] = gObjectTraversalTransform.m;
    f32 *w;
    f32 dy;

    v = *(u8 **)(actor + 0x54);
    w = (f32 *)(v + 0x1C0);
    dy = m[0][1] * w[0] + m[1][1] * w[1] + m[2][1] * w[2];
    *(f32 *)(v + 4) += dy;
    *(f32 *)(pose + 0x18) = *(f32 *)(reference + 0x44);
    *(f32 *)(pose + 0x1C) = *(f32 *)(actor + 0x1C) - *(f32 *)(v + 4) - *(f32 *)(v + 0x20C);
    *(f32 *)(pose + 0x20) = *(f32 *)(reference + 0x4C);
    *(f32 *)(pose + 0xC) = *(f32 *)(v + 0xC);
    *(f32 *)(pose + 0x10) = *(f32 *)(v + 0x10);
    *(f32 *)(pose + 0x14) = *(f32 *)(v + 0x14);
}

/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern u8 *gPikachuActor;
s32 areaIsPositionOutsideBounds(f32, f32, f32);
s32 terrainGetSurfaceAttributes(f32, f32, f32, u8 **);
s32 terrainConstrainMovement(s32, f32, f32, f32, f32, f32, f32, f32 *, f32 *, f32 *, u8 **);
/// Adjust actor movement for terrain, gravity, and ground contact while updating its movement state.
/// \param object Actor with initialized action, model, ground-reference pose, and default part state.
/// \param displacement Three-component movement vector adjusted for terrain and gravity.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016DE0 (0x80016DE0).
void actorConstrainMovementToTerrain(u8 *object, f32 *displacement) {
    f32 radius;
    f32 clearance;
    s32 status;
    u8 *state;
    u8 *reference;
    f32 floorDelta;
    u8 *pose;
    s32 collisionMode;
    u8 *surface;
    s32 collisionFlags;
    f32 gravityDelta;
    f32 verticalCorrection;
    s32 supported;

    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    surface = 0;
    if (((ActorScriptState *)state)->behaviorInhibitFlags & 4) {
        ((f32 *)displacement)[2] = 0.0f;
        ((f32 *)displacement)[0] = 0.0f;
    }
    reference = actorGetDefaultPart(object);
    pose = ((ActorScriptState *)state)->groundReference;
    actorUpdateGroundReferencePose(object, reference, pose);
    status = actionGetAttributes(object);
    if (((ActorScriptState *)state)->behaviorInhibitFlags & 8) {
        status |= 0x20000000;
    }
    if (!(status & 0x20000000)) {
        if (object == gPikachuActor) {
            collisionMode = 3;
        } else {
            collisionMode = 4;
        }
        if (areaIsPositionOutsideBounds(((RuntimeObjectPrefix *)object)->worldPosition[0], ((RuntimeObjectPrefix *)object)->worldPosition[1],
                          ((RuntimeObjectPrefix *)object)->worldPosition[2]) != 0) {
            objectDebugReportNoOp(object);
        }
        collisionFlags = terrainGetSurfaceAttributes(((RuntimeObjectPrefix *)object)->worldPosition[0], ((RuntimeObjectPrefix *)object)->worldPosition[1],
                                       ((RuntimeObjectPrefix *)object)->worldPosition[2], &surface);
        if (collisionFlags & 0x100) {
            actorAddDownhillMovement(object, displacement, (s32)&surface);
            radius = ((ActorScriptState *)state)->clearanceHeight * 0.001f;
        } else {
            radius = ((ActorScriptState *)state)->clearanceHeight;
        }
        if (gRuntimeCurrentProgram == 0) {
            verticalCorrection = 0.0f;
            ((ActorScriptState *)state)->movementCollisionFlags = terrainConstrainMovement(
                collisionMode, ((RuntimeObjectPrefix *)reference)->worldPosition[0],
                ((RuntimeObjectPrefix *)reference)->worldPosition[1],
                ((RuntimeObjectPrefix *)reference)->worldPosition[2], ((ActorScriptState *)state)->collisionWidth,
                ((ActorScriptState *)state)->collisionDepth, radius, displacement, &verticalCorrection,
                displacement + 2, &surface);
            clearance = actorMeasureGroundClearance(object, collisionMode, reference, displacement, &floorDelta, &surface);
            floorDelta = ((RuntimeObjectPrefix *)pose)->position[1] - floorDelta;
        } else {
            clearance = 0.0f;
            floorDelta = 0.0f;
        }
        if ((((status & 0x01000000) != 0) || (floorDelta > 0.0f)) && (clearance > 0.0f)) {
            supported = 1;
        } else {
            supported = 0;
        }
        if (supported != 0) {
            gravityDelta = ((ActorScriptState *)state)->movementScale *
                           (actionGetMovementScale(object) * 3.2666667f * gRuntimeFrameSeconds) * 30.0f;
            if (collisionFlags & 0x100) {
                gravityDelta *= 5.0f;
            }
            ((ActorScriptState *)state)->gravityVelocity -= gravityDelta;
            if ((status & 0x01000000) != 0) {
                ((f32 *)displacement)[0] = (f32)(((ActorScriptState *)state)->worldMotion[0] * gRuntimeFrameSeconds);
                ((f32 *)displacement)[1] =
                    (f32)((((ActorScriptState *)state)->gravityVelocity + ((ActorScriptState *)state)->worldMotion[1]) *
                          gRuntimeFrameSeconds);
                ((f32 *)displacement)[2] = (f32)(((ActorScriptState *)state)->worldMotion[2] * gRuntimeFrameSeconds);
            } else {
                ((f32 *)displacement)[1] += ((ActorScriptState *)state)->gravityVelocity * gRuntimeFrameSeconds;
            }
        } else {
            actorUpdateMotionDisplacement(state, status);
            ((ActorScriptState *)state)->gravityVelocity = 0.0f;
        }
        if ((((f32 *)displacement)[1] + clearance) >= 0.05f) {
            ((ActorScriptState *)state)->airborne = 1;
            return;
        }
        ((ActorScriptState *)state)->airborne = 0;
        if ((((f32 *)displacement)[1] + clearance) < 0.0f) {
            ((f32 *)displacement)[1] = (f32)(0.03f - clearance);
            if ((((ActorScriptState *)state)->gravityVelocity + ((ActorScriptState *)state)->worldMotion[1]) < 0.0f) {
                ((ActorScriptState *)state)->worldMotion[1] = 0.0f;
                ((ActorScriptState *)state)->gravityVelocity = 0.0f;
            }
        }
    }
}

/// Enable actor behavior inhibition flags.
/// \param actor Actor whose behavior inhibition flags are changed.
/// \param flags Behavior inhibition bits to change.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80017190 (0x80017190).
void actorSetBehaviorInhibitFlags(u8 *actor, u16 flags) {
    u8 *actorState;

    actorState = *(u8 **)(actor + 0x54);
    *(u16 *)(actorState + 0x1E8) |= flags;
}

/// Disable actor behavior inhibition flags.
/// \param actor Actor whose behavior inhibition flags are changed.
/// \param flags Behavior inhibition bits to change.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800171A8 (0x800171A8).
void actorClearBehaviorInhibitFlags(u8 *actor, u16 flags) {
    u8 *actorState;

    actorState = *(u8 **)(actor + 0x54);
    *(u16 *)(actorState + 0x1E8) &= ~flags;
}
