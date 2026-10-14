#include "render_state.h"
#include "game_heap.h"
#include "vector3_types.h"
#include "heap_free.h"
#include "word_map_lookup.h"
#include "matrix_basic.h"
#include "texture_setup.h"
#include "pikachu_actor.h"
#include "camera_storage.h"
#include "vector_metrics.h"
#include "math_angles.h"
#include "terrain_movement.h"
#include "terrain_height.h"
#include "terrain_surface.h"
#include "audio_sound_effect.h"
#include "audio_playback_stop.h"
#include "audio_playback_query.h"
#include "texture_animation_programs.h"
#include "script_vm_lifecycle.h"
#include "actor_assets.h"
#include "actor_behavior.h"
#include "matrix_point.h"
#include "matrix_orientation.h"
#include "motion_playback.h"
#include "action_script_playback.h"
#include "collision_registration.h"
#include "camera_types.h"
#include "audio_spatial.h"
#include "actor_sound.h"
#include "actor_color.h"
#include "color_transition.h"
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

#define ANIMATED_TEXTURE_OFFSET_CHECK(field, offset) \
    typedef char AnimatedTexture_##field##OffsetCheck[(((u32) & ((AnimatedTextureState *)0)->field) == (offset)) ? 1 : -1]
ANIMATED_TEXTURE_OFFSET_CHECK(textureIds, 0x4);
ANIMATED_TEXTURE_OFFSET_CHECK(playbackRates, 0x10);
ANIMATED_TEXTURE_OFFSET_CHECK(tracks, 0x1C);
ANIMATED_TEXTURE_OFFSET_CHECK(commandsPerBuffer, 0x48);
#undef ANIMATED_TEXTURE_OFFSET_CHECK

typedef char AnimatedTextureStateSizeCheck[(sizeof(AnimatedTextureState) == 0x4C) ? 1 : -1];
typedef char
    AnimatedTextureStateListsOffsetCheck[(((u32) & (((AnimatedTextureState *)0)->displayLists)) == 0x40) ? 1 : -1];

/// Per-actor-kind part-table slot counts, including the root slot.
/// \note Original identifier: \c D_800B687C (0x800B687C).
extern s32 gActorPartCounts[];
/// Allocate owned storage from a game heap.
/// \returns The allocation, or null when no suitable block is available.
/// \note Parameters are the initialized heap, byte count and allocation label described in heap_alloc.h.
/// This matching view also accepts ignored trailing actor addresses. Adopting the canonical prototype and
/// removing those arguments changes actorAllocatePartTable code; retain this view until call setup matches.
/// \note Original decompilation identifier: \c func_800380F4 (0x800380F4).
void *heapAllocLocal();
void _bzero(void *, u32);
/// Current affine world transform shared by hierarchy traversal and actor motion consumers.
/// \note Original identifier: \c D_800AD9E0 (0x800AD9E0).
extern Matrix34 gObjectTraversalTransform;
s32 actorFindPartIndex(s32, s32);
void actorUpdateAnimatedTextureDrawLists(u8 *, s32);
void actorInitializeChildMotionState(void *);


/// Play a spatial sound for an actor and track its playback.
/// \param actor Actor with initialized model state emitting the sound.
/// \param kind Actor sound kind to play.
/// \returns Playback handle, or zero if playback could not start.
/// \note Original decompilation identifier: \c func_80014B60 (0x80014B60).
s32 actorPlayTrackedSound(u8 *actor, s32 kind) {
    ActorScriptState *state;
    s32 handle;
    s32 attempt;
    s32 slot;

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    handle = objectPlaySpatialSound(actor, kind, 20.0f, 200.0f);
    if (handle != 0) {
        for (attempt = 0, slot = state->nextSoundSlot; attempt < 4; attempt++) {
            if (audioCountPlayback(state->soundHandles[slot]) == 0) {
                break;
            }
            slot++;
            slot %= 4;
        }
        state->soundHandles[slot] = handle;
        state->nextSoundSlot = (slot + 1) % 4;
    }
    if (((RuntimeObjectPrefix *)actor)->kind == 0x25 && kind >= 30 && kind < 125) {
        if (audioCountPlayback(state->trackedVoice) != 0) {
            audioStopPlayback(state->trackedVoice, 0.0);
        }
        state->trackedVoice = handle;
    }
    return handle;
}

/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern u32 gRenderCameraIndex;
/// Identifier of the currently active runtime program.
/// \note Original identifier: \c D_800AE4F0 (0x800AE4F0).
extern s32 gRuntimeCurrentProgram;

/// Update tracked sounds for the listener view and forget completed playback.
/// \param position World-space sound position containing three floating-point coordinates.
/// \param voices Four tracked playback handles, updated to clear completed sounds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014C7C (0x80014C7C).
void actorUpdateTrackedSoundSpatialization(f32 *position, s32 *voices) {
    s32 i;
    s32 volume;
    s32 pan;

    for (i = 0; i < 4; i++, voices++) {
        if (audioCountPlayback(*voices) != 0) {
            if (gRuntimeCurrentProgram == 0) {
                if (gRenderCameras[gRenderCameraIndex].matrices == NULL) {
                    continue;
                }
                audioGetPositionVolumeAndPan(position, *gRenderCameras[gRenderCameraIndex].matrices, &volume, &pan,
                                             20.0f, 100.0f);
                audioSetSoundEffectPan(*voices, pan);
                audioSetSoundEffectVolume(*voices, volume);
            }
        } else {
            *voices = 0;
        }
    }
}


#include "actor_model.h"
#include "area_services.h"
#include "object_update.h"
#include "spatial_math.h"
#include "vector_operations.h"
/// Release actor model resources and remove its collision registration.
/// \param actor Actor with initialized model state; its state pointer becomes invalid after release.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014DE8 (0x80014DE8).
void actorReleaseModelResources(u8 *actor) {
    ActorScriptState *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.actor;
    heapFreeLocal(&gGameHeap, (u8 *)actorState->modelDrawLists[0]);
    collisionGridUnregisterObject(actor, &actorState->collisionRegistration);
    scriptVmDestroy((ScriptVm *)actorState->vmAddress);
    heapFreeLocal(&gGameHeap, actorState->partTable);
    heapFreeLocal(&gGameHeap, actorState);
}

/// Release an actor child motion state and its optional facing state.
/// \param actor Child object with initialized motion state; its state pointer becomes invalid after release.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014E54 (0x80014E54).
void actorReleaseChildMotionState(u8 *actor) {
    ChildMotionState *state;
    u8 *facingController;

    state = ((RuntimeObjectPrefix *)actor)->state.pointer;
    facingController = state->facingController.storage;
    if (facingController != NULL) {
        heapFreeLocal(&gGameHeap, facingController);
    }
    heapFreeLocal(&gGameHeap, state);
}


/// Prepare the model drawing setup for its textures, lighting, and environment color.
/// \param actor Actor whose model drawing setup is prepared.
/// \param model Model state with initialized lights, an active light index of zero or one, and drawing buffers.
/// \param buffer Drawing buffer to prepare, zero or one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014EA0 (0x80014EA0).
void actorPrepareModelLightingDrawList(u8 *actor, ActorScriptState *model, s32 buffer) {
    Gfx *g;

    g = model->modelDrawLists[buffer];
    gSPSegment(g++, 0x0A, osVirtualToPhysical(actorAssetGetCachedStorage(((RuntimeObjectPrefix *)actor)->kind)));
    gSPSetLights2(g++, (*(Lights2 *)&model->directionAnimation.poses[model->directionAnimation.poseIndex]));
    gDPPipeSync(g++);
    gDPSetEnvColor(g++, model->environmentColor[0], model->environmentColor[1], model->environmentColor[2],
                   model->environmentColor[3]);
    gSPEndDisplayList(g++);
}


/// Allocate and initialize actor model state and its drawing resources.
/// \param actor Actor with a cached model asset receiving newly owned model state; existing state is not released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014FE4 (0x80014FE4).
void actorInitializeModelState(u8 *actor) {
    s32 buffer;
    ActorScriptState *state;
    u8 *drawListStorage;

    state = heapAllocLocal(&gGameHeap, sizeof(ActorScriptState), 0xC3);
    _bzero(state, sizeof(ActorScriptState));
    state->activeRequestId = 0;
    state->nextRequestId = 1;
    state->playbackMode = 0;
    state->pendingAction = -1;
    state->messengerContactMode = 2;
    state->motionDuration = 0.0f;
    state->motionRemaining = 0.0f;
    state->movementScale = 1.0f;
    matrix34Identity((Matrix34 *)(u8 *)&state->rootMotionTransform);
    _bzero(&state->rootPlayback, sizeof(ActorRootPlaybackWorkspace));
    _bzero(&state->rootMovementTrack, sizeof(AnimationDeltaTrack3));
    state->airborne = 0;
    state->nextSoundSlot = 0;
    _bzero(state->soundHandles, 0x10);
    state->trackedVoice = 0;
    state->actionScript.index = -1;
    state->searchItemKind = 0x43;
    state->searchItemRange = 0.0f;
    directionAnimationReset(&state->directionAnimation);
    drawListStorage = heapAllocLocal(&gGameHeap, 0x80, 0xE8);
    state->modelDrawLists[0] = (Gfx *)drawListStorage;
    state->modelDrawLists[1] = (Gfx *)(drawListStorage + 0x40);
    buffer = 0;
    do {
        actorPrepareModelLightingDrawList(actor, state, buffer);
        buffer += 1;
    } while (buffer != 2);
    ((RuntimeObjectPrefix *)actor)->state.actor = state;
}

/// Allocate initial motion state for an actor child object.
/// \param actor Child object receiving newly owned motion state; existing state is not released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015110 (0x80015110).
void actorInitializeChildMotionState(void *actor) {
    ChildMotionState *sp1C;
    ChildMotionState *allocation;

    allocation = heapAllocLocal(&gGameHeap, sizeof(ChildMotionState), 0xFC);
    sp1C = allocation;
    _bzero(allocation, sizeof(ChildMotionState));
    sp1C->playbackMode = MOTION_PLAYBACK_DISABLED;
    ((RuntimeObjectPrefix *)actor)->state.pointer = sp1C;
}

/// Target-facing history and blend weight owned by one child motion state.
typedef struct TargetFacingState {
    f32 motionAngles[3]; ///< Last animated orientation before target-facing adjustment.
    f32 previousAngles[3]; ///< Orientation produced by the previous child update.
    f32 weight; ///< Current influence of the target-facing adjustment.
} TargetFacingState;

typedef char TargetFacingStateSizeCheck[(sizeof(TargetFacingState) == 0x1C) ? 1 : -1];
typedef char TargetFacingStateWeightOffsetCheck[((u32)&((TargetFacingState *)0)->weight == 0x18) ? 1 : -1];

/// Camera-facing orientation and blend weight owned by one child motion state.
typedef struct CameraFacingState {
    f32 angles[3]; ///< Last animated orientation before camera-facing adjustment.
    f32 weight; ///< Current influence of the camera-facing adjustment.
} CameraFacingState;

typedef char CameraFacingStateSizeCheck[(sizeof(CameraFacingState) == 0x10) ? 1 : -1];
typedef char CameraFacingWeightOffsetCheck[((u32) & ((CameraFacingState *)0)->weight == 0xC) ? 1 : -1];

/// Initialize a child object for blended facing toward its parent target.
/// \param actor Child object whose current orientation initializes target-facing behavior.
/// \returns Nothing.
/// \note Portability: The allocator call retains an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_80015160 (0x80015160).
void actorInitializeTargetFacingChildState(u8 *actor) {
    ChildMotionState *state;
    TargetFacingState *vec;

    actorInitializeChildMotionState(actor);
    state = ((RuntimeObjectPrefix *)actor)->state.pointer;
    vec = (TargetFacingState *)heapAllocLocal(&gGameHeap, sizeof(TargetFacingState), 0x10C, actor);
    vec->motionAngles[0] = ((RuntimeObjectPrefix *)actor)->rotation[0];
    vec->motionAngles[1] = ((RuntimeObjectPrefix *)actor)->rotation[1];
    vec->motionAngles[2] = ((RuntimeObjectPrefix *)actor)->rotation[2];
    vec->previousAngles[0] = ((RuntimeObjectPrefix *)actor)->rotation[0];
    vec->previousAngles[1] = ((RuntimeObjectPrefix *)actor)->rotation[1];
    vec->previousAngles[2] = ((RuntimeObjectPrefix *)actor)->rotation[2];
    vec->weight = 0.0f;
    state->facingController.target = vec;
}

/// Initialize a child object for blended facing toward the camera.
/// \param actor Child object whose current orientation initializes camera-facing behavior.
/// \returns Nothing.
/// \note Portability: The allocator call retains an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_800151E4 (0x800151E4).
void actorInitializeCameraFacingChildState(u8 *actor) {
    ChildMotionState *state;
    CameraFacingState *vec;

    actorInitializeChildMotionState(actor);
    state = ((RuntimeObjectPrefix *)actor)->state.pointer;
    vec = (CameraFacingState *)heapAllocLocal(&gGameHeap, sizeof(CameraFacingState), 0x11C, actor);
    vec->angles[0] = ((RuntimeObjectPrefix *)actor)->rotation[0];
    vec->angles[1] = ((RuntimeObjectPrefix *)actor)->rotation[1];
    vec->angles[2] = ((RuntimeObjectPrefix *)actor)->rotation[2];
    vec->weight = 0.0f;
    state->facingController.camera = vec;
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Smoothly tilt the actor to follow the ground slope ahead.
/// \param object Actor with initialized model state whose pitch follows the terrain.
/// \returns Nothing.
/// \note Portability: The empty surface check preserves the IDO live ranges required by the ROM.
/// \note Original decompilation identifier: \c func_80015250 (0x80015250).
void actorUpdateGroundSlopePitch(u8 *object) {
    ActorScriptState *state;
    f32 height;
    f32 nextX;
    f32 nextHeight;
    f32 heightDelta;
    f32 length;
    f32 pitch;
    f32 headingX;
    f32 headingZ;
    u8 *surface;

    state = ((RuntimeObjectPrefix *)object)->state.actor;
    surface = NULL;
    headingX = mathSinDegrees(state->orientation[1]);
    headingZ = mathCosDegrees(state->orientation[1]);
    height = terrainGetWorldHeight(((RuntimeObjectPrefix *)object)->worldPosition[0], ((RuntimeObjectPrefix *)object)->worldPosition[1], ((RuntimeObjectPrefix *)object)->worldPosition[2], &surface);
    nextX = ((RuntimeObjectPrefix *)object)->worldPosition[0] + headingX;
    nextHeight = terrainGetWorldHeight(nextX, ((RuntimeObjectPrefix *)object)->worldPosition[1], ((RuntimeObjectPrefix *)object)->worldPosition[2] + headingZ, &surface);
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
        mathApproachAngleDegrees(state->orientation[0], pitch, 1.0f, 90.0f, 180.0f * gRuntimeFrameSeconds) -
            state->orientation[0],
        0);
}


/// Add downhill drift to actor movement according to the nearby terrain slope.
/// \param actor Actor whose position determines the terrain slope.
/// \param displacement Three-component movement vector receiving additional horizontal drift.
/// \param surfaceCache Address of the cached terrain surface pointer used and updated by the slope query.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015394 (0x80015394).
void actorAddDownhillMovement(u8 *actor, f32 *displacement, u8 **surfaceCache) {
    Vec3f v;
    f32 f;

    f = 0.2f;
    v.y = f;
    terrainGetDownhillHeightDifferences(
        ((RuntimeObjectPrefix *)actor)->position[0], ((RuntimeObjectPrefix *)actor)->position[1],
        ((RuntimeObjectPrefix *)actor)->position[2], 50.0f, v.y, &v.x, &v.z, surfaceCache);
    vector3Normalize((f32 *)&v);
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
                   ((RuntimeObjectPrefix *)actor)
                       ->state.actor->modelDrawLists[(((RuntimeObjectPrefix *)actor)->flags & 0x8000) ? 1 : 0]);
}

void actorUpdateTrackedSoundSpatialization(f32 *, s32 *);
void actorUpdateGroundSlopePitch(u8 *);


/// Update actor behavior, animation, spatial state, and model drawing setup.
/// \param actor Actor with initialized model, action, hierarchy, and collision state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015488 (0x80015488).
void actorUpdateModel(u8 *actor) {
    ActorScriptState *state;

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    collisionGridUnregisterObject(actor, &state->collisionRegistration);
    actionScriptUpdate(actor);
    actionUpdatePlayback(actor);
    actorUpdateTrackedSoundSpatialization(((RuntimeObjectPrefix *)actor)->worldPosition, state->soundHandles);
    if (!(actionGetAttributes(actor) & 0x20000000) && !(state->behaviorFlags & ACTOR_INHIBIT_TERRAIN_AND_GRAVITY)) {
        actorUpdateGroundSlopePitch(actor);
    }
    actorUpdateObjectRotationFromState(actor);
    actorRefreshTrackingTargetPosition(actor);
    if ((gRuntimeCurrentProgram == 0) && !(state->behaviorFlags & ACTOR_INHIBIT_INTERACTION_VISIBILITY)) {
        objectUpdateHierarchyInteractionVisibility(actor);
    }
    state->collisionRegistration = collisionGridRegisterObject(actor);
    directionAnimationUpdate(&state->directionAnimation);
    colorAdvanceTransition(state->environmentColor, state->targetEnvironmentColor,
                           &state->environmentColorTimeRemaining);
    actorPrepareModelLightingDrawList(actor, state, ((((RuntimeObjectPrefix *)actor)->flags & 0x8000) ? 1 : 0) ^ 1);
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
    ActorScriptState *state;

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    state->targetEnvironmentColor[0] = r;
    state->targetEnvironmentColor[1] = g;
    state->targetEnvironmentColor[2] = b;
    state->targetEnvironmentColor[3] = a;
    state->environmentColorTimeRemaining = duration;
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
    ActorScriptState *state;

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    state->environmentColor[0] = r;
    state->environmentColor[1] = g;
    state->environmentColor[2] = b;
    state->environmentColor[3] = a;
}


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


/// Resolve the parent tracking position, including camera targets and the stored-position fallback.
/// \param child Child object whose hierarchy root is the actor described by parentState.
/// \param parentState Parent actor state containing tracking settings and the fallback position.
/// \param position Output array receiving three world-space coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015620 (0x80015620).
void actorGetParentTrackingPosition(u8 *child, ActorScriptState *parentState, f32 *position) {
    u8 *targetObject;

    if (parentState->targetMode == ACTOR_TARGET_FOLLOW) {
        targetObject = actorResolveStoredTarget(((RuntimeObjectPrefix *)child)->root.bytes);
        if (targetObject != NULL) {
            if (((RuntimeObjectPrefix *)targetObject)->kind == 0x7F) {
                position[0] = gRenderCameras[gRenderCameraIndex].eye[0];
                position[1] = gRenderCameras[gRenderCameraIndex].eye[1];
                position[2] = gRenderCameras[gRenderCameraIndex].eye[2];
            } else {
                position[0] = ((RuntimeObjectPrefix *)targetObject)->worldPosition[0];
                position[1] = ((RuntimeObjectPrefix *)targetObject)->worldPosition[1];
                position[2] = ((RuntimeObjectPrefix *)targetObject)->worldPosition[2];
            }
            return;
        }
    }
    position[0] = parentState->targetPosition[0];
    position[1] = parentState->targetPosition[1];
    position[2] = parentState->targetPosition[2];
}



/// Update child animation and blend its orientation toward the parent target when tracking is enabled.
/// \param object Child object with initialized target-facing state and a valid actor hierarchy root.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015704 (0x80015704).
void actorUpdateTargetFacingChild(u8 *object) {
    ChildMotionState *state;
    f32 rollOffset = 180.0f;
    f32 fullTurn = 360.0f;
    u8 *parent;
    ActorScriptState *parentState;
    f32 delta[3];
    f32 initial[3];
    Matrix34 matrix;
    f32 roll, pitch, yaw;
    f32 position[3];
    f32 previous[3];
    f32 *weight;
    TargetFacingState *follow;
    f32 length;
    s32 i;

    state = ((RuntimeObjectPrefix *)object)->state.pointer;
    parent = ((RuntimeObjectPrefix *)object)->root.bytes;
    follow = state->facingController.target;
    parentState = ((RuntimeObjectPrefix *)parent)->state.actor;
    motionUpdateChildPlayback(object);
    weight = &follow->weight;
    if ((actionGetAttributes(parent) & 0x400000) && !(parentState->behaviorFlags & ACTOR_INHIBIT_TARGET_FACING)) {
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
    if ((u16)state->playbackMode == MOTION_PLAYBACK_TRACKS ||
        (u16)state->playbackMode == MOTION_PLAYBACK_POSE_BLEND) {
        follow->motionAngles[0] = ((RuntimeObjectPrefix *)object)->rotation[0];
        follow->motionAngles[1] = ((RuntimeObjectPrefix *)object)->rotation[1];
        follow->motionAngles[2] = ((RuntimeObjectPrefix *)object)->rotation[2];
    } else {
    }
    if (*weight > 0.0f) {
        actorGetParentTrackingPosition(object, parentState, position);
        delta[0] = position[0] - ((RuntimeObjectPrefix *)object)->worldPosition[0];
        delta[1] = position[1] - ((RuntimeObjectPrefix *)object)->worldPosition[1];
        delta[2] = position[2] - ((RuntimeObjectPrefix *)object)->worldPosition[2];
        do {
            matrix = gObjectTraversalTransform;
        } while (0);
        matrix33Transpose(&matrix.m[0][0]);
        matrix.m[3][0] = 0.0f;
        matrix.m[3][1] = 0.0f;
        matrix.m[3][2] = 0.0f;
        matrix34TransformPointVector(&matrix.m[0][0], delta, delta);
        length = vector3Normalize((f32 *)delta) * 50.0f;
        if (length < *weight) {
            *weight = length;
        }
        vector3FromZAxisPitchYawDegrees(follow->motionAngles[1], follow->motionAngles[2], initial);
        vector3FromZAxisPitchYawDegrees(follow->previousAngles[1], follow->previousAngles[2], previous);
        for (i = 0; i < 3; i++) {
            delta[i] = (delta[i] * *weight + initial[i]) + 2.0f * (previous[i] * *weight);
        }
        vector3Normalize((f32 *)delta);
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
        ((RuntimeObjectPrefix *)object)->rotation[0] = roll;
        ((RuntimeObjectPrefix *)object)->rotation[1] = pitch;
        ((RuntimeObjectPrefix *)object)->rotation[2] = yaw;
    }
    follow->previousAngles[0] = ((RuntimeObjectPrefix *)object)->rotation[0];
    follow->previousAngles[1] = ((RuntimeObjectPrefix *)object)->rotation[1];
    follow->previousAngles[2] = ((RuntimeObjectPrefix *)object)->rotation[2];
}

/// Fixed direction transform used when blending child orientation toward the camera.
/// \note Original identifier: \c D_80076C80 (0x80076C80).
extern u8 gActorCameraFacingDirectionTransform[];


/// Update child animation and blend its orientation toward the active camera when tracking is enabled.
/// \param object Child object with initialized camera-facing state and a valid actor hierarchy root.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015AE8 (0x80015AE8).
void actorUpdateCameraFacingChild(u8 *object) {
    ChildMotionState *state;
    CameraFacingState *orientation;
    f32 maxWeight = 2.0f;
    Vec3f direction;
    Vec3f original;
    Matrix34 matrix;
    f32 pitch;
    f32 yaw;
    f32 roll;
    f32 x;
    f32 *weight;

    state = ((RuntimeObjectPrefix *)object)->state.pointer;
    orientation = state->facingController.camera;
    motionUpdateChildPlayback(object);
    weight = &orientation->weight;
    if ((actionGetAttributes(((RuntimeObjectPrefix *)object)->root.bytes) & 0x800000) &&
        !(((RuntimeObjectPrefix *)object)->root.object->state.actor->behaviorFlags & ACTOR_INHIBIT_CAMERA_FACING)) {
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
    if ((u16)state->playbackMode == MOTION_PLAYBACK_TRACKS ||
        (u16)state->playbackMode == MOTION_PLAYBACK_POSE_BLEND) {
        orientation->angles[0] = ((RuntimeObjectPrefix *)object)->rotation[0];
        orientation->angles[1] = ((RuntimeObjectPrefix *)object)->rotation[1];
        orientation->angles[2] = ((RuntimeObjectPrefix *)object)->rotation[2];
    }
    if (*weight > 0.0f) {
        direction.x = gRenderCameras[gRenderCameraIndex].eye[0] - ((RuntimeObjectPrefix *)object)->worldPosition[0];
        direction.y = gRenderCameras[gRenderCameraIndex].eye[1] - ((RuntimeObjectPrefix *)object)->worldPosition[1];
        direction.z = gRenderCameras[gRenderCameraIndex].eye[2] - ((RuntimeObjectPrefix *)object)->worldPosition[2];
        matrix34TransformPointVector((f32 *)gActorCameraFacingDirectionTransform, &direction.x, &direction.x);
        do {
            matrix = gObjectTraversalTransform;
        } while (0);
        matrix33Transpose(&matrix.m[0][0]);
        matrix.m[3][0] = 0.0f;
        matrix.m[3][1] = 0.0f;
        matrix.m[3][2] = 0.0f;
        matrix34TransformPointVector(&matrix.m[0][0], &direction.x, &direction.x);
        vector3Normalize((f32 *)&direction);
        direction.x *= *weight;
        direction.y *= *weight;
        direction.z *= *weight;
        vector3FromZAxisPitchYawDegrees(orientation->angles[1], orientation->angles[2], &original.x);
        direction.x += original.x;
        direction.y += original.y;
        direction.z += original.z;
        vector3Normalize((f32 *)&direction);
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
        ((RuntimeObjectPrefix *)object)->rotation[0] = -180.0f;
        ((RuntimeObjectPrefix *)object)->rotation[1] = yaw;
        ((RuntimeObjectPrefix *)object)->rotation[2] = roll;
    }
}

/// Bind the referenced object transform for actor drawing.
/// \param actor Actor with a valid referenced object transform.
/// \returns Object drawing continuation link.
/// \note Original decompilation identifier: \c func_80015E34 (0x80015E34).
s32 actorBindReferencedObjectMatrixForDraw(u8 *actor) {
    gSPSegment(gRenderDisplayListCursor++, 7,
               osVirtualToPhysical(((RuntimeObjectPrefix *)actor)->root.object->matrixBuffers[(((RuntimeObjectPrefix *)actor)->root.object->flags & 0x8000) ? 1 : 0]));
    return (s32)((RuntimeObjectPrefix *)actor)->displayList;
}

/// Update object rotation from the actor orientation state and reference transform.
/// \param actor Actor with initialized orientation state and a current reference transform.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015EB8 (0x80015EB8).
void actorUpdateObjectRotationFromState(u8 *actor) {
    ActorScriptState *state;
    f32 rotationMatrix[12];
    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    matrix34MakeRotationXYZDegrees(rotationMatrix, state->orientation[0], state->orientation[1], 0.0f);
    matrix33Multiply(&state->rootMotionTransform.m[0][0], rotationMatrix, rotationMatrix);
    matrix33GetFoldedEulerDegrees(rotationMatrix, &((RuntimeObjectPrefix *)actor)->rotation[0], &((RuntimeObjectPrefix *)actor)->rotation[1], &((RuntimeObjectPrefix *)actor)->rotation[2]);
}

/// Set one actor orientation angle in degrees.
/// \param actor Actor whose orientation is updated.
/// \param angle Finite orientation angle in degrees.
/// \param axis Orientation component: zero for pitch, one for yaw, or two for roll.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015F1C (0x80015F1C).
void actorSetRotationAxisDegrees(u8 *actor, f32 angle, s32 axis) {
    ActorScriptState *state;

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
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
    state->orientation[axis] = angle;
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
    ActorScriptState *state;

    if (actionGetAttributes(actor) & ACTION_ATTRIBUTE_ALLOW_TURNING) {
        state = ((RuntimeObjectPrefix *)actor)->state.actor;
        state->orientation[0] += angles[0];
        while (state->orientation[0] < 0.0f) {
            state->orientation[0] += 360.0f;
        }
        while (state->orientation[0] >= 360.0f) {
            state->orientation[0] -= 360.0f;
        }
        state->orientation[1] += angles[1];
        while (state->orientation[1] < 0.0f) {
            state->orientation[1] += 360.0f;
        }
        while (state->orientation[1] >= 360.0f) {
            state->orientation[1] -= 360.0f;
        }
    }
}

/// Synchronize the actor orientation state with its object rotation.
/// \param actor Actor whose orientation state is synchronized.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016228 (0x80016228).
void actorCopyObjectRotationToState(u8 *actor) {
    ActorScriptState *state;

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    state->orientation[0] = (f32)((RuntimeObjectPrefix *)actor)->rotation[0];
    state->orientation[1] = (f32)((RuntimeObjectPrefix *)actor)->rotation[1];
    state->orientation[2] = (f32)((RuntimeObjectPrefix *)actor)->rotation[2];
}

/// Owned lookup mapping packed actor-kind and part identifiers to part-table slots.
/// \note Original identifier: \c D_800B6918 (0x800B6918).
extern WordMap gActorPartIndexMap;

/// Register the named parts belonging to an actor type.
/// \param actorType Nonnegative actor type within the part-count table and the 15-bit actor-type range.
/// \param partIds Part identifiers in [0, 65535], terminated by minus one.
/// \returns Pointer to the terminating entry in the part identifier list.
/// \note Original decompilation identifier: \c func_80016248 (0x80016248).
s32 *actorRegisterPartIds(s32 actorType, s32 *partIds) {
    s32 partIndex = 1;
    s32 lookupKey;

    while (*partIds != -1) {
        lookupKey = *partIds + (actorType << 16);
        wordMapInsert(&gActorPartIndexMap, partIndex, (u32)lookupKey);
        partIndex++;
        partIds++;
    }
    gActorPartCounts[actorType] = partIndex;
    return partIds;
}

/// Packed actor-kind and terminated part-identifier lists used to initialize the part registry.
/// \note Original identifier: \c D_800760F0 (0x800760F0).
extern s32 gActorPartRegistrationData;


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
    partIndex = wordMapLookup(&gActorPartIndexMap, (actorType << 0x10) + partId);
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
    ActorScriptState *state;
    s32 partIndex;

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    partIndex = actorFindPartIndex(((RuntimeObjectPrefix *)actor)->kind, partId);
    if (partIndex < 0) {
        return 0;
    }
    return state->partTable[partIndex];
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
    return actorFindPart(actor, actorConfigGetDefaultPartId(((RuntimeObjectPrefix *)actor)->kind));
}

/// Get the world position of a named part, using the default part when the requested part is absent.
/// \param actor Actor containing the requested part and a valid default part.
/// \param partId Identifier of the preferred part.
/// \param position Output array receiving three world-space coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800164AC (0x800164AC).
void actorGetPartWorldPosition(u8 *actor, s32 partId, f32 *position) {
    u8 *part;
    part = actorFindPart(actor, partId);
    if (part == NULL) {
        part = actorGetDefaultPart(actor);
    }
    position[0] = ((RuntimeObjectPrefix *)part)->worldPosition[0];
    position[1] = ((RuntimeObjectPrefix *)part)->worldPosition[1];
    position[2] = ((RuntimeObjectPrefix *)part)->worldPosition[2];
}


/// Get the world-space attachment position selected by the actor attachment mode.
/// \param actor Actor whose attachment mode and parts determine the position.
/// \param heightOffset Vertical offset applied by the raised attachment mode.
/// \param position Output array receiving three world-space coordinates; unchanged for an unsupported attachment mode.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800164FC (0x800164FC).
void actorGetConfiguredAttachmentPosition(u8 *actor, f32 heightOffset, f32 *position) {
    ActorScriptState *state;
    f32 first[3];
    f32 second[3];

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    switch ((u32)state->attachmentMode) {
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

    allocation = heapAllocLocal(&gGameHeap, sizeof(AnimatedTextureState), 0x463);
    state = allocation;
    _bzero(allocation, sizeof(AnimatedTextureState));
    ((RuntimeObjectPrefix *)actor)->state.pointer = state;
}

/// Release an actor animated texture state and its drawing resources.
/// \param actor Component with initialized animated textures; its state pointer becomes invalid after release.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800166A4 (0x800166A4).
void actorReleaseAnimatedTextureState(u8 *actor) {
    AnimatedTextureState *obj;

    obj = ((RuntimeObjectPrefix *)actor)->state.pointer;
    heapFreeLocal(&gGameHeap, obj->displayLists[0]);
    heapFreeLocal(&gGameHeap, obj);
}


/// Advance animated textures and prepare their drawing commands.
/// \param actor Component with zero through three initialized texture tracks and allocated drawing buffers.
/// \param buffer Drawing buffer to prepare, zero or one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800166E4 (0x800166E4).
void actorUpdateAnimatedTextureDrawLists(u8 *actor, s32 buffer) {
    AnimatedTextureState *state;
    Gfx *dl;
    s32 i;
    Gfx *gfx;
    ValueTrack *track;
    s32 frame;
    Gfx *base;

    state = ((RuntimeObjectPrefix *)actor)->state.pointer;
    base = state->displayLists[buffer];
    dl = base;
    gfx = base + state->count;
    for (i = 0; i < state->count; i++) {
        gSPBranchList(dl++, gfx);
        track = &state->tracks[i];
        frame = track->value;
        animationIntegerTrackEvaluate(track, &frame, state->playbackRates[i] * gRuntimeFrameSeconds);
        textureLoadForDraw(&gfx, state->textureIds[i], frame);
        gSPEndDisplayList(gfx++);
    }
}

/// Update and bind the actor animated textures for drawing.
/// \param actor Animated texture component being drawn.
/// \returns Object drawing continuation link.
/// \note Original decompilation identifier: \c func_80016808 (0x80016808).
s32 actorBindAnimatedTexturesForDraw(u8 *actor) {
    AnimatedTextureState *obj;

    obj = ((RuntimeObjectPrefix *)actor)->state.pointer;
    actorUpdateAnimatedTextureDrawLists(actor, gGraphicsFramebufferIndex);
    gSPSegment(gRenderDisplayListCursor++, 6, osVirtualToPhysical(obj->displayLists[gGraphicsFramebufferIndex]));
    return (s32)((RuntimeObjectPrefix *)actor)->displayList;
}


/// Configure an actor animated texture component from texture and animation definitions.
/// \param actor Actor with part 0x78 and allocated texture state; existing drawing buffers are not released.
/// \param parts Up to three valid texture/animation identifier pairs, terminated by a negative texture identifier; not retained.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016884 (0x80016884).
void actorInitializeAnimatedTextures(u8 *actor, s32 *parts) {
    u8 *obj;
    AnimatedTextureState *state;
    s32 frame;

    obj = actorFindPart(actor, 0x78);
    state = ((RuntimeObjectPrefix *)obj)->state.pointer;
    state->count = 0;
    for (; (frame = parts[0]) >= 0; parts += 2) {
        state->textureIds[state->count] = frame;
        state->playbackRates[state->count] = 1.0f;
        animationIntegerTrackInitialize(&state->tracks[state->count], gTextureFrameAnimationPrograms[parts[1]]);
        state->count++;
    }
    state->commandsPerBuffer = state->count * 20;
    state->displayLists[0] = heapAllocLocal(&gGameHeap, state->commandsPerBuffer * sizeof(Gfx) * 2, 0x4B8);
    state->displayLists[1] = state->displayLists[0] + state->commandsPerBuffer;
    actorUpdateAnimatedTextureDrawLists(obj, 0);
    actorUpdateAnimatedTextureDrawLists(obj, 1);
}

/// Allocate an empty part table sized for the actor type.
/// \param actor Actor with initialized model state and registered type receiving a new part table; existing tables are not released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800169B8 (0x800169B8).
void actorAllocatePartTable(u8 *actor) {
    ActorScriptState *state;
    s32 kind;
    s32 n;
    s32 size;

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    kind = ((RuntimeObjectPrefix *)actor)->kind;
    n = gActorPartCounts[kind];
    state->partCount = n;
    size = n * sizeof(*state->partTable);
    state->partTable = heapAllocLocal(&gGameHeap, size, 0x4D2);
    _bzero(state->partTable, size);
}

/// Register matching objects from a hierarchy in an actor part table.
/// \param hierarchy First object in the hierarchy to register, or null.
/// \param actor Actor whose allocated part table borrows matching objects for their lifetimes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016A1C (0x80016A1C).
void actorPopulatePartTable(u8 *hierarchy, u8 *actor) {
    s32 partIndex;
    u8 *firstChild;
    u8 *object;

    object = hierarchy;
    if (hierarchy != NULL) {
        do {
            partIndex = actorFindPartIndex(((RuntimeObjectPrefix *)actor)->kind, ((RuntimeObjectPrefix *)object)->kind);
            if (partIndex >= 0) {
                ((RuntimeObjectPrefix *)actor)->state.actor->partTable[partIndex] = object;
            }
            firstChild = ((RuntimeObjectPrefix *)object)->child.bytes;
            if (firstChild != NULL) {
                actorPopulatePartTable(firstChild, actor);
            }
            object = ((RuntimeObjectPrefix *)object)->sibling.bytes;
        } while (object != NULL);
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
            ((RuntimeObjectPrefix *)e)->flags |= set;
            ((RuntimeObjectPrefix *)e)->flags &= ~clear;
        }
    }
}


/// Find the lowest world height among the actor configured reference parts.
/// \param actor Actor with a valid default reference part.
/// \returns Lowest reference-part world height, including the default reference part.
/// \note Original decompilation identifier: \c func_80016B1C (0x80016B1C).
f32 actorGetLowestReferencePartHeight(u8 *actor) {
    f32 min = ((RuntimeObjectPrefix *)actorGetDefaultPart(actor))->worldPosition[1];
    const s32 *ids = actorConfigGetGroundReferencePartIds(((RuntimeObjectPrefix *)actor)->kind);
    s32 i;
    u8 *o;

    for (i = 0; ids[i] > 0; i++) {
        o = actorFindPart(actor, ids[i]);
        if (o != NULL && ((RuntimeObjectPrefix *)o)->worldPosition[1] < min) {
            min = ((RuntimeObjectPrefix *)o)->worldPosition[1];
        }
    }
    return min;
}


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
    ActorScriptState *state;
    const s32 *p;
    u8 *e;
    f32 top;

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    top = ((RuntimeObjectPrefix *)reference)->worldPosition[1];
    for (p = actorConfigGetGroundReferencePartIds(((RuntimeObjectPrefix *)actor)->kind); *p > 0; p++) {
        e = actorFindPart(actor, *p);
        if (e != NULL && ((RuntimeObjectPrefix *)e)->worldPosition[1] < top) {
            top = ((RuntimeObjectPrefix *)e)->worldPosition[1];
        }
    }
    *groundHeight =
        terrainGetWorldHeightForQuery(collisionMode, ((RuntimeObjectPrefix *)reference)->worldPosition[0] + displacement[0], ((RuntimeObjectPrefix *)reference)->worldPosition[1], ((RuntimeObjectPrefix *)reference)->worldPosition[2] + displacement[2], surface);
    return top - *groundHeight - state->groundOffset;
}


/// Update scaled actor motion displacement with the applicable upward movement limit.
/// \param state Actor state whose pending motion displacement is updated.
/// \param status Movement status flags selecting the upward movement limit.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016C98 (0x80016C98).
void actorUpdateMotionDisplacement(ActorScriptState *state, s32 status) {
    f32 movementScale;
    f32 upwardLimit;

    matrix33TransformVector((f32 *)&gObjectTraversalTransform, state->localMotion, state->worldMotion);
    movementScale = state->movementScale;
    state->worldMotion[0] = (f32)(state->worldMotion[0] * movementScale);
    state->worldMotion[1] = (f32)(state->worldMotion[1] * movementScale);
    state->worldMotion[2] = (f32)(state->worldMotion[2] * movementScale);
    if (status & 0x200) {
        upwardLimit = 30.0f;
        if (state->worldMotion[1] > 30.0f) {
            goto block_4;
        }
    } else {
        upwardLimit = 5.0f;
        if (state->worldMotion[1] > 5.0f) {
        block_4:
            state->worldMotion[1] = upwardLimit;
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
    ActorScriptState *state;
    f32(*m)[3] = gObjectTraversalTransform.m;
    f32 *w;
    f32 dy;

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    w = state->poseDisplacement;
    dy = m[0][1] * w[0] + m[1][1] * w[1] + m[2][1] * w[2];
    state->verticalPoseOffset += dy;
    ((RuntimeObjectPrefix *)pose)->position[0] = ((RuntimeObjectPrefix *)reference)->worldPosition[0];
    ((RuntimeObjectPrefix *)pose)->position[1] = ((RuntimeObjectPrefix *)actor)->position[1] - state->verticalPoseOffset - state->groundOffset;
    ((RuntimeObjectPrefix *)pose)->position[2] = ((RuntimeObjectPrefix *)reference)->worldPosition[2];
    ((RuntimeObjectPrefix *)pose)->rotation[0] = state->orientation[0];
    ((RuntimeObjectPrefix *)pose)->rotation[1] = state->orientation[1];
    ((RuntimeObjectPrefix *)pose)->rotation[2] = state->orientation[2];
}

/// Adjust actor movement for terrain, gravity, and ground contact while updating its movement state.
/// \param object Actor with initialized action, model, ground-reference pose, and default part state.
/// \param displacement Three-component movement vector adjusted for terrain and gravity.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016DE0 (0x80016DE0).
void actorConstrainMovementToTerrain(u8 *object, f32 *displacement) {
    f32 radius;
    f32 clearance;
    s32 status;
    ActorScriptState *state;
    u8 *reference;
    f32 floorDelta;
    u8 *pose;
    s32 collisionMode;
    u8 *surface;
    s32 collisionFlags;
    f32 gravityDelta;
    f32 verticalCorrection;
    s32 supported;

    state = ((RuntimeObjectPrefix *)object)->state.actor;
    surface = 0;
    if (state->behaviorFlags & ACTOR_INHIBIT_HORIZONTAL_MOVEMENT) {
        ((f32 *)displacement)[2] = 0.0f;
        ((f32 *)displacement)[0] = 0.0f;
    }
    reference = actorGetDefaultPart(object);
    pose = state->groundReference;
    actorUpdateGroundReferencePose(object, reference, pose);
    status = actionGetAttributes(object);
    if (state->behaviorFlags & ACTOR_INHIBIT_TERRAIN_AND_GRAVITY) {
        status |= 0x20000000;
    }
    if (!(status & 0x20000000)) {
        if (object == ((u8 *)gPikachuActor)) {
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
            actorAddDownhillMovement(object, displacement, &surface);
            radius = state->clearanceHeight * 0.001f;
        } else {
            radius = state->clearanceHeight;
        }
        if (gRuntimeCurrentProgram == 0) {
            verticalCorrection = 0.0f;
            state->movementCollisionFlags = terrainConstrainMovement(
                collisionMode, ((RuntimeObjectPrefix *)reference)->worldPosition[0],
                ((RuntimeObjectPrefix *)reference)->worldPosition[1],
                ((RuntimeObjectPrefix *)reference)->worldPosition[2], state->collisionWidth,
                state->collisionDepth, radius, displacement, &verticalCorrection,
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
            gravityDelta =
                state->movementScale * (actionGetGravityScale(object) * 3.2666667f * gRuntimeFrameSeconds) * 30.0f;
            if (collisionFlags & 0x100) {
                gravityDelta *= 5.0f;
            }
            state->gravityVelocity -= gravityDelta;
            if ((status & 0x01000000) != 0) {
                ((f32 *)displacement)[0] = (f32)(state->worldMotion[0] * gRuntimeFrameSeconds);
                ((f32 *)displacement)[1] =
                    (f32)((state->gravityVelocity + state->worldMotion[1]) *
                          gRuntimeFrameSeconds);
                ((f32 *)displacement)[2] = (f32)(state->worldMotion[2] * gRuntimeFrameSeconds);
            } else {
                ((f32 *)displacement)[1] += state->gravityVelocity * gRuntimeFrameSeconds;
            }
        } else {
            actorUpdateMotionDisplacement(state, status);
            state->gravityVelocity = 0.0f;
        }
        if ((((f32 *)displacement)[1] + clearance) >= 0.05f) {
            state->airborne = 1;
            return;
        }
        state->airborne = 0;
        if ((((f32 *)displacement)[1] + clearance) < 0.0f) {
            ((f32 *)displacement)[1] = (f32)(0.03f - clearance);
            if ((state->gravityVelocity + state->worldMotion[1]) < 0.0f) {
                state->worldMotion[1] = 0.0f;
                state->gravityVelocity = 0.0f;
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
    ActorScriptState *state;

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    state->behaviorFlags |= flags;
}

/// Disable actor behavior inhibition flags.
/// \param actor Actor whose behavior inhibition flags are changed.
/// \param flags Behavior inhibition bits to change.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800171A8 (0x800171A8).
void actorClearBehaviorInhibitFlags(u8 *actor, u16 flags) {
    ActorScriptState *state;

    state = ((RuntimeObjectPrefix *)actor)->state.actor;
    state->behaviorFlags &= ~flags;
}
