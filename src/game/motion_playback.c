#include "math_angles.h"
#include "actor_behavior.h"
#include "matrix_orientation.h"
#include "animation_tracks.h"
#include "motion_playback.h"
#include "actor_action_cue.h"
#include "action_continuation.h"
#include "action_request.h"
#include "actor_appearance.h"
#include "actor_part_texture.h"
#include "action_query.h"
#include "actor_rotation.h"
#include "object_movement.h"
/* harvested: matched m2c drafts; pending style pass */
/* Action and motion state machine. The unit starts at motionExtractHeadingFromMatrix: its
 * .rodata opens with the debug-trace strings and ends with the float
 * literals IDO emits after every string of the file. */
#include "common.h"
#include "actor_parts.h"
#include "motion_cache.h"
#include "actor_script.h"
#include "object_types.h"
#include "word_map.h"
#include "word_map_lookup.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on


s32 actionRequestMotionData(s32);
/// Action records containing playback flags, motion identifiers and transition settings.
/// \note Original identifier: \c D_8006B590 (0x8006B590).
extern ActionDefinition gActionDefinitions[];
void motionExtractHeadingFromMatrix(f32 *, f32 *);
void motionRequestCurrentData(u8 *);
void actionResolveNextTransition(u8 *);
/// Separate view of fallback action identifiers in the action definitions, with a 0x2C-byte stride.
/// \note Original identifier: \c D_8006B5A4 (0x8006B5A4).
extern u8 gActionFallbackEntries[];
void actionConfigurePoseTransition(ActorScriptState *, s32, f32);
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Separate the horizontal heading from a motion orientation matrix.
/// \param matrix Twelve-component affine matrix updated to remove its horizontal heading; its first basis vector must have a nonzero horizontal projection.
/// \param heading Receives the extracted heading in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000DF70 (0x8000DF70).
void motionExtractHeadingFromMatrix(f32 *matrix, f32 *heading) {
    f32 x;
    f32 z;
    f32 len;

    x = matrix[0];
    z = matrix[2];
    len = sqrtf((x * x) + (z * z));
    if ((len < 0.001f) && (-0.001f < len)) {
        *heading = 0.0f;
    }
    x /= len;
    z /= len;
    *heading = -mathUnitDirectionDegrees(x, z);
    matrix34RotateWorldYDegrees(-*heading, matrix);
}

/// Apply a motion root orientation and its heading change to an object.
/// \param angles Three root rotation angles in degrees whose orientation has a nonzero horizontal heading projection.
/// \param object Object with initialized motion state receiving the root orientation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E00C (0x8000E00C).
void motionApplyRootOrientation(f32 *angles, u8 *object) {
    ActorScriptState *state;
    f32 y;

    state = ((RuntimeObjectPrefix *)object)->state.actor;
    matrix34MakeRotationXYZDegrees((f32 *)&state->rootMotionTransform, angles[0], angles[1], angles[2]);
    motionExtractHeadingFromMatrix((f32 *)&state->rootMotionTransform, &y);
    actorAddRotationAxisDegrees((u8 *)object, y - state->rootMotionHeading, 1);
    state->rootMotionHeading = y;
}

/// Request cached motion data associated with an action.
/// \param actionId Action identifier to inspect.
/// \returns A borrowed archive-relative base supplied by the motion cache, or zero for an invalid action or unsupported action type; data must be awaited before use and retained through cache use requests.
/// \note Original decompilation identifier: \c func_8000E08C (0x8000E08C).
s32 actionRequestMotionData(s32 actionId) {
    ActionDefinition *var_v0;
    if ((actionId < 0) || (actionId >= 0x14A)) {
        var_v0 = NULL;
    } else {
        var_v0 = &gActionDefinitions[actionId];
    }
    if ((var_v0 == NULL) || ((var_v0->attributes & 7) != 1)) {
        return 0;
    }
    {
        s32 sp18;
        motionCacheRead(&gMotionRomCache, var_v0->motionId, &sp18);
        return sp18;
    }
}

/// Request motion data for the object current action when its action type uses the motion cache.
/// \param object Object whose current motion is requested.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E110 (0x8000E110).
void motionRequestCurrentData(u8 *object) {
    ActorScriptState *motionState;
    s32 sp20;

    motionState = ((RuntimeObjectPrefix *)object)->state.actor;
    if (((motionState->actionAttributes & 7) == 1) || ((motionState->actionAttributes & 7) == 3)) {
        TRACE("_requestCurMotion:%d\n", motionState->motionId);
        motionCacheRead(&gMotionRomCache, motionState->motionId, &sp20);
    }
}

/// Reset motion playback for an object and its animated children.
/// \param object Object whose motion playback is reset.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E15C (0x8000E15C).
void motionResetPlayback(u8 *object) {
    ActorScriptState *motionState;
    motionState = ((RuntimeObjectPrefix *)object)->state.actor;
    motionState->loopCount = 0;
    motionState->motionDuration = 0.0f;
    motionState->motionRemaining = 0.0f;
    motionSetPlaybackMode(object, MOTION_PLAYBACK_DISABLED);
}

/// Set the motion playback mode for an object and its animated children.
/// \param object Object whose motion playback mode is updated.
/// \param mode Playback mode: 0 disables playback, 1 selects tracks, 2 selects timed playback, and 3 selects pose blending.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E190 (0x8000E190).
void motionSetPlaybackMode(u8 *object, s32 mode) {
    ActorScriptState *state = ((RuntimeObjectPrefix *)object)->state.actor;
    s32 i;
    u8 *e;
    u16 t;

    state->playbackMode = mode;
    for (i = 1; i < state->partCount; i++) {
        e = state->partTable[i];
        if (e == NULL) {
            continue;
        }
        t = ((RuntimeObjectPrefix *)e)->kind;
        if (t >= 0x26 && t < 0x64) {
            ((ChildMotionState *)((RuntimeObjectPrefix *)e)->state.pointer)->playbackMode = mode;
        }
    }
}

/// Playback duration indexed by motion identifier.
/// \note Original identifier: \c D_800773E0 (0x800773E0).
extern f32 gMotionDurations[];
/// Motion archive offsets indexed by motion identifier, including the next-entry end offset.
/// \note Original identifier: \c D_80077778 (0x80077778).
extern s32 gMotionArchiveOffsets[];
/// Archive-relative offsets of per-motion object track tables.
/// \note Original identifier: \c D_80077048 (0x80077048).
extern s32 gMotionTrackTableOffsets[];
/// Archive entry selecting one object's motion channels.
/// Entries end with a nonpositive object kind; offsets are relative to the cached archive base.
typedef struct MotionPartTrackEntry {
    s32 objectKind; ///< Positive object kind; kinds below 0x26 select root motion, others identify child parts.
    s32 curvesOffset; ///< Archive-relative offset of three rotation descriptors, followed by three movement descriptors for roots.
} MotionPartTrackEntry;

typedef char MotionPartTrackEntrySizeCheck[(sizeof(MotionPartTrackEntry) == 8) ? 1 : -1];

/// Start the current motion for an object and its animated children.
/// \param object Object with initialized motion and part state and a valid selected motion; the shared motion cache must be initialized.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E208 (0x8000E208).
void motionStartTrackPlayback(u8 *object) {
    ActorScriptState *state;
    s32 base;
    MotionPartTrackEntry *entry;
    ChildMotionState *childState;
    f32 rotation[3];
    u8 *child;
    f32 position[3];
    f32 duration;

    state = ((RuntimeObjectPrefix *)object)->state.actor;
    TRACE("motionNum:%d\n", state->motionId);
    TRACE("  %s\n", state->actionName);
    motionSetPlaybackMode(object, MOTION_PLAYBACK_DISABLED);
    state->movementScale = state->actionMovementScale;
    duration = gMotionDurations[state->motionId];
    state->motionDuration = duration;
    state->motionRemaining = duration;
    base = motionCacheReadBlocking(&gMotionRomCache, state->motionId);
    entry = (MotionPartTrackEntry *)(gMotionTrackTableOffsets[state->motionId] + base);
    state->defaultPoseOffset = *(f32 *)(base + gMotionArchiveOffsets[state->motionId]) * *(f32 *)(object + 4);
    state->verticalPoseOffset = state->defaultPoseOffset;
    while (entry->objectKind > 0) {
        if (entry->objectKind > 0 && entry->objectKind < 0x26) {
            state->playbackMode = MOTION_PLAYBACK_TRACKS;
            {
                u8 *data = (u8 *)base;
                data += entry->curvesOffset;
                animationTrack3Initialize(&state->rootPlayback.orientationTrack, (AnimationCurveDescriptor *)data,
                                          base);
            }
            {
                u8 *data = (u8 *)base;
                data += entry->curvesOffset;
                data += 0x18;
                animationDeltaTrack3Initialize(&state->rootMovementTrack, (AnimationCurveDescriptor *)data, base);
            }
            rotation[0] = 0.0f;
            rotation[1] = 0.0f;
            rotation[2] = 0.0f;
            _bzero(state->rootMovementTrack.previousValues, sizeof(state->rootMovementTrack.previousValues));
            animationDeltaTrack3Evaluate(&state->rootMovementTrack, rotation, NULL, 0.0f);
            animationTrack3Evaluate(&state->rootPlayback.orientationTrack, position, NULL, 0.0f);
            matrix34MakeRotationXYZDegrees((f32 *)&state->rootMotionTransform, position[0], position[1], position[2]);
            motionExtractHeadingFromMatrix((f32 *)&state->rootMotionTransform, &state->rootMotionHeading);
        } else {
            child = actorFindPart(object, entry->objectKind);
            if (child != NULL) {
                childState = ((RuntimeObjectPrefix *)child)->state.pointer;
                childState->playbackMode = MOTION_PLAYBACK_TRACKS;
                {
                    u8 *data = (u8 *)base;
                    data += entry->curvesOffset;
                    animationTrack3Initialize(&childState->playback.rotationTrack, (AnimationCurveDescriptor *)data,
                                              base);
                }
            }
        }
        entry++;
    }
    actionRequestMotionData(state->continuationAction);
}

/// Evaluate and apply the root orientation and movement of a motion.
/// \param object Object receiving the root motion.
/// \param state Actor-owned initialized root tracks and writable movement state.
/// \param time Nonnegative time step in seconds by which to advance the root tracks.
/// \returns Nothing.
/// \pre Object must be the current hierarchy-update object and state must belong to it;
/// borrowed motion keys must remain valid throughout playback.
/// \note Original decompilation identifier: \c func_8000E408 (0x8000E408).
void motionEvaluateRootTracks(u8 *object, struct ActorScriptState *state, f32 time) {
    f32 position[3];
    animationTrack3Evaluate(&state->rootPlayback.orientationTrack, position, NULL, time);
    animationDeltaTrack3Evaluate(&state->rootMovementTrack, state->poseDisplacement, state->localMotion, time);
    motionApplyRootOrientation(position, object);
    objectAddPendingLocalMovement(state->poseDisplacement);
}

/// Evaluate a child rotation track and end playback when the track finishes.
/// \param obj Child object receiving rotation angles.
/// \param track Initialized child motion playback state to advance.
/// \param step Nonnegative time step in seconds by which to advance the rotation track.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E470 (0x8000E470).
void motionEvaluateChildRotation(u8 *obj, ChildMotionState *track, f32 step) {
    s32 r;

    r = animationTrack3Evaluate(&track->playback.rotationTrack, ((RuntimeObjectPrefix *)obj)->rotation, NULL, step);
    while (((RuntimeObjectPrefix *)obj)->rotation[0] < 0.0f) {
        ((RuntimeObjectPrefix *)obj)->rotation[0] += 360.0f;
    }
    while (((RuntimeObjectPrefix *)obj)->rotation[0] >= 360.0f) {
        ((RuntimeObjectPrefix *)obj)->rotation[0] -= 360.0f;
    }
    while (((RuntimeObjectPrefix *)obj)->rotation[1] < 0.0f) {
        ((RuntimeObjectPrefix *)obj)->rotation[1] += 360.0f;
    }
    while (((RuntimeObjectPrefix *)obj)->rotation[1] >= 360.0f) {
        ((RuntimeObjectPrefix *)obj)->rotation[1] -= 360.0f;
    }
    while (((RuntimeObjectPrefix *)obj)->rotation[2] < 0.0f) {
        ((RuntimeObjectPrefix *)obj)->rotation[2] += 360.0f;
    }
    while (((RuntimeObjectPrefix *)obj)->rotation[2] >= 360.0f) {
        ((RuntimeObjectPrefix *)obj)->rotation[2] -= 360.0f;
    }
    if (r == 1) {
        ((ChildMotionState *)track)->playbackMode = MOTION_PLAYBACK_DISABLED;
    }
}

/// Start a transition from the current pose to the initial pose of the selected motion.
/// \param object Object with initialized motion and part state, a valid selected motion, and a positive transition duration in seconds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E5EC (0x8000E5EC).
void motionStartPoseTransition(u8 *object) {
    f32 duration;
    AnimationTrack3 track;
    ActorScriptState *state;
    s32 base;
    f32 rotation[3];
    MotionPartTrackEntry *entry;
    f32 angles[3];
    Matrix34 *from;
    Matrix34 *current;
    u8 *child;
    f32 childRotation[3];
    ChildMotionState *childState;
    f32 blendWeight = 1.0f;
    s32 rootKindLimit = 0x26;
    s32 rotationMode = MOTION_PLAYBACK_POSE_BLEND;
    f32 transformTime = 0.0f;

    state = ((RuntimeObjectPrefix *)object)->state.actor;
    motionCacheRead(&gMotionRomCache, state->motionId, &base);
    entry = (MotionPartTrackEntry *)(gMotionTrackTableOffsets[state->motionId] + base);
    duration = state->actionMovementScale;
    state->motionDuration = duration;
    state->motionRemaining = duration;
    state->movementScale = blendWeight;
    motionSetPlaybackMode(object, MOTION_PLAYBACK_DISABLED);
    motionCacheReadBlocking(&gMotionRomCache, state->motionId);
    state->defaultPoseOffset = *(f32 *)(base + gMotionArchiveOffsets[state->motionId]) * *(f32 *)(object + 4);
    while (entry->objectKind > 0) {
        if (entry->objectKind > 0 && entry->objectKind < rootKindLimit) {
            state->playbackMode = rotationMode;
            from = state->rootPlayback.posePair;
            current = (Matrix34 *)&state->rootMotionTransform;
            animationTrack3Initialize(&track, (AnimationCurveDescriptor *)((u8 *)base + entry->curvesOffset), base);
            animationTrack3Evaluate(&track, rotation, 0, transformTime);
            *from = *current;
            current = from + 1;
            matrix34MakeRotationXYZDegrees((f32 *)current, rotation[0], rotation[1], rotation[2]);
            motionExtractHeadingFromMatrix((f32 *)current, angles);
            state->rootMotionHeading = 0.0f;
        } else {
            child = actorFindPart(object, entry->objectKind);
            if (child != NULL) {
                childState = ((RuntimeObjectPrefix *)child)->state.pointer;
                childState->playbackMode = rotationMode;
                animationTrack3Initialize(&track, (AnimationCurveDescriptor *)((u8 *)base + entry->curvesOffset), base);
                animationTrack3Evaluate(&track, childRotation, 0, transformTime);
                from = childState->playback.posePair;
                matrix34MakeRotationXYZDegrees((f32 *)from, ((RuntimeObjectPrefix *)child)->rotation[0],
                                               ((RuntimeObjectPrefix *)child)->rotation[1],
                                               ((RuntimeObjectPrefix *)child)->rotation[2]);
                matrix34MakeRotationXYZDegrees((f32 *)(from + 1), childRotation[0], childRotation[1], childRotation[2]);
            }
        }
        entry++;
    }
    TRACE("motionNum:%d\n", state->motionId);
    TRACE("  %s\n", state->actionName);
    TRACE("bias:%p\n", base);
    TRACE("fcvNode:%p\n", entry);
}

/// Blend a stored pair of orientations for a motion transition.
/// \param from Two consecutive twelve-component orientation matrices.
/// \param out Receives the blended twelve-component orientation matrix.
/// \param t Weight of the first pose, clamped from zero through one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E81C (0x8000E81C).
void motionBlendPosePair(u8 *from, f32 *out, f32 t) {
    if (t < 0.0f) {
        t = 0.0f;
    } else if (t > 1.0f) {
        t = 1.0f;
    }
    matrix34BlendOrientation(1.0f - t, (f32 *)from, (f32 *)(from + 0x30), out);
}

/// Start timed motion playback using the current action duration.
/// \param object Object whose motion timer is started.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E88C (0x8000E88C).
void motionStartTimedPlayback(u8 *object) {
    ActorScriptState *motionState;
    f32 duration;

    motionState = ((RuntimeObjectPrefix *)object)->state.actor;
    motionSetPlaybackMode(object, MOTION_PLAYBACK_DISABLED);
    duration = motionState->actionMovementScale;
    TRACE("interTime:%f\n", duration);
    motionState->playbackMode = MOTION_PLAYBACK_TIMED;
    motionState->movementScale = 1.0f;
    motionState->motionDuration = duration;
    motionState->motionRemaining = duration;
}

/// Invoke the current action initialization callback when present.
/// \param object Object whose current action supplies the callback.
/// \returns The initialization callback result, or zero when no callback is present.
/// \note Original decompilation identifier: \c func_8000E8D8 (0x8000E8D8).
s32 actionInvokeInitializer(u8 *object) {
    ActorScriptState *state;
    ActorActionInitializer fn;
    s32 result;

    state = ((RuntimeObjectPrefix *)object)->state.actor;
    fn = state->actionInitializer;
    if (fn != NULL) {
        TRACE("initFunc:%p\n", fn);
        result = fn(object);
        TRACE("initResult:%d\n", result);
        return result;
    }
    return 0;
}

void motionStartTrackPlayback(u8 *);
void motionStartTimedPlayback(u8 *);
void motionStartPoseTransition(u8 *);

/// Start playback and associated animation cues for the current action.
/// \param object Object whose current action is started.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000E914 (0x8000E914).
void actionStartPlayback(u8 *object) {
    ActorScriptState *state;
    s32 mode;
    ActorActionCueCommand *anim;

    state = ((RuntimeObjectPrefix *)object)->state.actor;
    mode = state->actionAttributes & 7;
    TRACE("_startAction: actionType:%d\n", mode);
    state->actionStartPending = 0;
    state->secondaryCueClock = 0.0f;
    state->secondaryCueCommand = actorSelectActionScript(object, state->secondaryCueProgram, 0);
    if (state->loopCount == 0) {
        state->loopCount = 1;
        anim = actorSelectActionScript(object, state->primaryCueProgram, 1);
        if (anim != 0 && (anim != state->lastPrimaryCueProgram ||
                          (1.2f < state->primaryCueClock && !(state->actionAttributes & 8)))) {
            TRACE("lastAfcKey:%p\n    afcKey:%p\n", state->lastPrimaryCueProgram, anim);
            if (((RuntimeObjectPrefix *)object)->kind == 0x25) {
                if (!(state->behaviorFlags & ACTOR_BEHAVIOR_PRESERVE_APPEARANCE)) {
                    actorAppearanceReset(object);
                }
                actorResetPartTint(object);
            }
            state->primaryCueCommand = anim;
            state->lastPrimaryCueProgram = anim;
            state->primaryCueClock = 0.0f;
        }
    }
    switch (mode) {
    case MOTION_PLAYBACK_TRACKS:
        motionStartTrackPlayback(object);
        break;
    case MOTION_PLAYBACK_TIMED:
        motionStartTimedPlayback(object);
        break;
    case MOTION_PLAYBACK_POSE_BLEND:
        motionStartPoseTransition(object);
        break;
    }
}


/// Apply an action record when it is compatible with the current motion state.
/// \param state Actor action state receiving the record.
/// \param record Action record whose referenced names, scripts, and callbacks remain valid while selected.
/// \returns One when the record is applied; zero when its motion state is incompatible.
/// \note Original decompilation identifier: \c func_8000EA88 (0x8000EA88).
s32 actionTryApplyCompatibleRecord(ActorScriptState *state, ActionDefinition *record) {
    if ((record->attributes & 7) != 1 ||
        record->initialPoseId == ((u8 *)state)[(u32) & ((ActorScriptState *)0)->currentPoseId]) {
        *(ActionDefinition *)&state->actionId = *record;
        return 1;
    }
    TRACE("error: _setAction(%p): unknown action_type(%d)\n", state, record->attributes & 7);
    return 0;
}

/// Configure a pose transition leading to a selected action.
/// \param state Actor action state to configure.
/// \param nextAction Valid destination action identifier from zero through 329.
/// \param duration Positive pose transition duration in seconds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000EAF8 (0x8000EAF8).
void actionConfigurePoseTransition(ActorScriptState *state, s32 nextAction, f32 duration) {
    ActionDefinition *var_v0;

    if ((nextAction < 0) || (nextAction >= 0x14A)) {
        var_v0 = NULL;
    } else {
        var_v0 = &gActionDefinitions[nextAction];
    }
    state->actionId = -2 - nextAction;
    state->actionName = "\xCA\xE4\xB4\xD6"; /* EUC-JP "interpolation" */
    TRACE("_setInterAct:  next-motion:%s  next-action:%s\n", *(char **)(var_v0 + 4), state->actionName);
    state->actionGravityScale = var_v0->gravityScale;
    state->actionAttributes = (var_v0->attributes & 0x20100000) | 0x04010003;
    state->motionId = var_v0->motionId;
    state->previousPoseId = state->currentPoseId;
    state->currentPoseId = var_v0->initialPoseId;
    state->continuationAction = nextAction;
    state->actionInitializer = NULL;
    state->actionCallback = NULL;
    state->actionMovementScale = duration;
    state->primaryCueProgram = 0;
    state->secondaryCueProgram = 0;
}

/// Load an indexed action record and configure its continuation.
/// \param state Actor action state receiving the record.
/// \param actionId Action record identifier.
/// \param nextAction Continuation action identifier stored for subsequent playback.
/// \returns One when the record is loaded; zero when the action identifier is invalid.
/// \note Original decompilation identifier: \c func_8000EBA0 (0x8000EBA0).
s32 actionLoadRecordByIndex(ActorScriptState *state, s32 actionId, s32 nextAction) {
    ActionDefinition *a;

    if (actionId < 0 || actionId >= 330) {
        a = NULL;
    } else {
        a = &gActionDefinitions[actionId];
    }
    if (a == NULL) {
        return 0;
    }
    *(ActionDefinition *)&state->actionId = *a;
    state->continuationAction = nextAction;
    if ((state->actionAttributes & 7) == 1 && ((state->actionAttributes & 0x20000) || state->primaryCueProgram == 0)) {
        state->primaryCueProgram = 0;
        state->secondaryCueProgram = 0;
        state->actionMovementScale = 2.0f;
    }
    state->actionAttributes &= 0xF9FFFFF7;
    return 1;
}

/// Prepare playback context for a newly selected action.
/// \param state Actor action state to prepare.
/// \param sequence Borrowed default sequence address used when the action supplies no sequence; it must remain valid throughout playback.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000EC7C (0x8000EC7C).
void actionPrepareSelectedContext(ActorScriptState *state, ActorActionCueCommand *sequence) {
    state->loopExitRequested = 0;
    state->pendingAction = -1;
    state->activeRequestId = state->pendingRequestId;
    state->pendingRequestId = 0;
    state->selectedActionKind = state->pendingActionKind;
    state->pendingActionKind = 0;
    if (state->primaryCueProgram == 0) {
        state->primaryCueProgram = sequence;
    }
}

typedef struct GlyphPair {
    u8 hi;
    u8 lo;
    s16 id;
} GlyphPair;
/// Explicit source/destination pose pairs and the actions joining them.
/// \note Original identifier: \c D_8006EE48 (0x8006EE48).
extern GlyphPair gActionExplicitPoseTransitions[];
/// Owned lookup map from source/destination pose pairs to compatible transitions.
/// \note Original identifier: \c D_800B6830 (0x800B6830).
extern WordMap gActionPoseTransitionMap;

/// Fallback command sequence used when an action has no configured sequence.
/// \note Original identifier: \c D_80091D18 (0x80091D18).
extern ActorActionCueCommand gActionDefaultCommandSequence[];
/// Select and initialize the next compatible action or pose transition.
/// \param object Actor with initialized action state and transition map; initializer callbacks must eventually accept an action, and fallback action identifiers must be valid.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000ECB4 (0x8000ECB4).
void actionResolveNextTransition(u8 *object) {
    u8 *state;
    ActorActionCueCommand *sequence;
    f32 blendTime;
    s32 nextIndex;
    s32 prevIndex;
    ActionDefinition *previous;
    ActionDefinition *next;
    s32 transition;
    s32 animation;

    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    if ((u16)((ActorScriptState *)state)->playbackMode == MOTION_PLAYBACK_DISABLED) {
        ((ActorScriptState *)state)->continuationAction = -1;
    }
    ((ActorScriptState *)state)->loopCount = 0;
    ((ActorScriptState *)state)->actionStartPending = 1;
    blendTime = 0.2f;
    for (;;) {
        prevIndex = ((ActorScriptState *)state)->continuationAction;
        if (prevIndex < 0 || prevIndex >= 0x14A) {
            previous = NULL;
        } else {
            previous = &gActionDefinitions[prevIndex];
        }
        nextIndex = ((ActorScriptState *)state)->pendingAction;
        if (nextIndex < 0 || nextIndex >= 0x14A) {
            next = NULL;
        } else {
            next = &gActionDefinitions[nextIndex];
        }
        if (previous == NULL && next == NULL) {
            actionStopPlayback(object);
            return;
        }
        if (next == NULL || (previous != NULL && (previous->attributes & 0x04000000))) {
            *(ActionDefinition *)&((ActorScriptState *)state)->actionId = *previous;
        } else {
            sequence = next->primaryCueProgram;
            if (sequence == NULL) {
                sequence = gActionDefaultCommandSequence;
            }
            if (actionTryApplyCompatibleRecord((ActorScriptState *)state, next)) {
                actionPrepareSelectedContext((ActorScriptState *)state, sequence);
            } else if ((next->attributes & 0x02000000) && next->continuationAction != -1) {
                nextIndex = next->continuationAction;
                if (nextIndex < 0 || nextIndex >= 0x14A) {
                    next = NULL;
                } else {
                    next = &gActionDefinitions[nextIndex];
                }
                if (actionTryApplyCompatibleRecord((ActorScriptState *)state, next)) {
                    actionPrepareSelectedContext((ActorScriptState *)state, sequence);
                } else {
                    goto find_transition;
                }
            } else {
                sequence = NULL;
            find_transition:
                transition = wordMapLookup(&gActionPoseTransitionMap, next->initialPoseId + (state[0xFF] << 8));
                animation = transition - 0x100;
                if (transition == 0xFE) {
                    actionConfigurePoseTransition((ActorScriptState *)state, nextIndex, blendTime);
                    actionPrepareSelectedContext((ActorScriptState *)state, sequence);
                } else if (animation == -3) {
                    *(ActionDefinition *)&((ActorScriptState *)state)->actionId = *next;
                    actionPrepareSelectedContext((ActorScriptState *)state, sequence);
                } else if (actionLoadRecordByIndex((ActorScriptState *)state, animation, nextIndex)) {
                    actionPrepareSelectedContext((ActorScriptState *)state, sequence);
                } else if (previous == NULL) {
                    actionConfigurePoseTransition((ActorScriptState *)state, nextIndex, blendTime);
                    actionPrepareSelectedContext((ActorScriptState *)state, sequence);
                } else {
                    *(ActionDefinition *)&((ActorScriptState *)state)->actionId = *previous;
                }
            }
        }
        if (actionInvokeInitializer(object) == 0) {
            motionRequestCurrentData(object);
            return;
        }
    }
}

#include "motion_transitions.h"
/// Initialize the pose-transition lookup map from explicit transitions and action definitions.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F00C (0x8000F00C).
void actionInitializeTransitionMap(void) {
    s32 i;
    s32 key;
    ActionDefinition *e;
    GlyphPair *p;

    wordMapInitialize(&gActionPoseTransitionMap, 0x125);
    TRACE("apply action_joint_pose from actJointPose\n");
    for (p = gActionExplicitPoseTransitions; p->hi != '0'; p++) {
        key = (p->hi << 8) + p->lo;
        TRACE("firstPose:%d lastPose:%d act:%s\n", p->hi, p->lo, gActionDefinitions[p->id].name);
        if (wordMapLookup(&gActionPoseTransitionMap, key) == 0) {
            TRACE("firstPose:%d lastPose:%d act:%s apply:%d\n", p->hi, p->lo,
                  gActionDefinitions[p->id].name, p->id);
            wordMapInsert(&gActionPoseTransitionMap, p->id + 0x100, key);
        }
    }
    TRACE("apply action_joint_pose from action\n");
    for (i = 0, e = gActionDefinitions; i < 0x14A; i++, e++) {
        if (e->attributes & 0x40000) {
            key = e->finalPoseId;
            key = (e->initialPoseId << 8) + key;
            if (e->initialPoseId != e->finalPoseId && wordMapLookup(&gActionPoseTransitionMap, key) == 0) {
                TRACE("firstPose:%d lastPose:%d act:%s apply:%d\n", e->initialPoseId, e->finalPoseId, e->name, i);
                wordMapInsert(&gActionPoseTransitionMap, i + 0x100, key);
            }
        }
    }
    TRACE("total %d joint_actions applied\n", i);
}


/// Release the action pose-transition lookup map.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F128 (0x8000F128).
void actionDestroyTransitionMap(void) {
    wordMapDestroy(&gActionPoseTransitionMap);
}

/// Advance the current action, its motion, callbacks, and transitions.
/// \param object Actor with initialized action state, part state, and transition map; pose blending requires a positive total transition duration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F14C (0x8000F14C).
/// \note Two pose reads retain checked member-offset addressing to preserve the original IDO floating-point operand order.
void actionUpdatePlayback(u8 *object) {
    f32 remaining;
    f32 step;
    ActorActionUpdateCallback callback;
    s32 callbackResult;
    s32 result;
    u16 mode;
    ActorScriptState *state;

    state = ((RuntimeObjectPrefix *)object)->state.actor;
    state->poseDisplacement[0] = 0.0f;
    state->poseDisplacement[1] = 0.0f;
    state->poseDisplacement[2] = 0.0f;
    if (state->actionStartPending != 0) {
        actionStartPlayback(object);
    }
    if ((u16)state->playbackMode != MOTION_PLAYBACK_DISABLED) {
        actorUpdateActionScript(object, &state->primaryCueCommand, &state->primaryCueClock);
        actorUpdateActionScript(object, &state->secondaryCueCommand, &state->secondaryCueClock);
        step = state->movementScale * gRuntimeFrameSeconds;
        state->motionRemaining = (f32)(state->motionRemaining - step);
        remaining = state->motionRemaining;
        if (remaining <= 0.0f) {
            step += remaining;
        }
        mode = (u16)state->playbackMode;
        switch (mode) {
        case MOTION_PLAYBACK_TIMED:
            break;
        case MOTION_PLAYBACK_TRACKS:
            motionCacheReadBlocking(&gMotionRomCache, state->motionId);
            motionEvaluateRootTracks(object, (ActorScriptState *)state, step);
            break;
        case MOTION_PLAYBACK_POSE_BLEND:
            step = remaining / state->motionDuration;
            state->verticalPoseOffset =
                (f32)(((*(f32 *)((s8 *)state + (u32) & ((ActorScriptState *)0)->defaultPoseOffset)) * (1.0f - step)) +
                      ((*(f32 *)((s8 *)state + (u32) & ((ActorScriptState *)0)->verticalPoseOffset)) * step));
            TRACE("rate:%f  centerHeight:%f  default:%f\n", step, state->verticalPoseOffset, state->defaultPoseOffset);
            motionBlendPosePair((u8 *)state->rootPlayback.posePair, (f32 *)&state->rootMotionTransform, step);
            break;
        default:
            TRACE("error: actHara(%p): unknown action_type(%d)\n", object, mode);
            break;
        }
        callback = state->actionCallback;
        result = 0;
        if (callback != NULL) {
            callbackResult = callback(object);
            result = callbackResult;
            if (callbackResult == 1) {
                state->motionRemaining = 0.0f;
            }
            remaining = state->motionRemaining;
        } else {
            remaining = state->motionRemaining;
        }
        if (!(remaining > 0.0f)) {
            TRACE("IS_LOOP:%d  attr:%04x, stopFlag:%d\n", state->actionAttributes & 8, state->actionAttributes,
                  state->loopExitRequested);
            if ((state->actionAttributes & 8) && (state->loopExitRequested == 0) && (result == 0) &&
                (actionInvokeInitializer(object) == 0)) {
                state->loopCount = (s16)(state->loopCount + 1);
                state->actionStartPending = 1;
                return;
            }
            actionResolveNextTransition(object);
        }
    }
}

void motionEvaluateChildRotation(u8 *, ChildMotionState *, f32);
void motionBlendPosePair(u8 *, f32 *, f32);

/// Advance child motion playback using its parent action timing.
/// \param object Animated child part with initialized playback state and a valid parent action state; pose blending requires a positive parent transition duration.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_8000F33C (0x8000F33C).
void motionUpdateChildPlayback(u8 *object) {
    ChildMotionState *state;
    ActorScriptState *parentState;
    f32 v[12];

    state = ((RuntimeObjectPrefix *)object)->state.pointer;
    if ((u16)state->playbackMode != MOTION_PLAYBACK_DISABLED) {
        parentState = ((RuntimeObjectPrefix *)object)->root.object->state.actor;
        switch ((u16)state->playbackMode) {
        case MOTION_PLAYBACK_TRACKS:
            motionEvaluateChildRotation(object, state, parentState->movementScale * gRuntimeFrameSeconds);
            break;
        case MOTION_PLAYBACK_TIMED:
            state->playbackMode = MOTION_PLAYBACK_DISABLED;
            break;
        case MOTION_PLAYBACK_POSE_BLEND:
            motionBlendPosePair((u8 *)state->playback.posePair, v,
                                parentState->motionRemaining / parentState->motionDuration);
            matrix3GetEulerAnglesDegrees(v, &((RuntimeObjectPrefix *)object)->rotation[0],
                                         &((RuntimeObjectPrefix *)object)->rotation[1],
                                         &((RuntimeObjectPrefix *)object)->rotation[2]);
            break;
        default:
            TRACE("error: actJoint(%p): unknown action_type(%d)\n", object, (u16)state->playbackMode);
            break;
        }
    }
}

/// Request an action, starting immediately when playback is idle or replacing the pending request until current playback ends.
/// \param object Actor receiving the action request.
/// \param id Valid action identifier from zero through 329.
/// \param kind Action kind supplied by the requesting script, representable as a signed 16-bit value.
/// \param out Receives the identifier assigned to this request.
/// \returns One when idle playback accepts the request immediately; zero when it is queued.
/// \note Original decompilation identifier: \c func_8000F404 (0x8000F404).
s32 actionRequestPlayback(u8 *object, s32 id, s32 kind, s32 *out) {
    u8 *v;

    v = ((RuntimeObjectPrefix *)object)->state.bytes;
    TRACE("\n\nactRequest: %s\n", actionGetName(id));
    if (actionRequestMotionData(id) == 0) {
        actionRequestMotionData(*(s16 *)(gActionFallbackEntries + id * 0x2C));
    }
    ((ActorScriptState *)v)->pendingAction = id;
    ((ActorScriptState *)v)->pendingActionKind = kind;
    *out = ((ActorScriptState *)v)->nextRequestId;
    ((ActorScriptState *)v)->pendingRequestId = *out;
    (((ActorScriptState *)v)->nextRequestId)++;
    if ((u16)((ActorScriptState *)v)->playbackMode == MOTION_PLAYBACK_DISABLED) {
        TRACE("play ID: %d\n", *out);
        actionResolveNextTransition(object);
        return 1;
    }
    TRACE("request to slot: ID: %d\n", *out);
    ((ActorScriptState *)v)->loopExitRequested = 1;
    return 0;
}

/// Request that the current action leave its loop when playback reaches its end.
/// \param object Actor whose action state is accessed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F4C4 (0x8000F4C4).
void actionRequestLoopExit(u8 *object) {
    u8 *actionState = ((RuntimeObjectPrefix *)object)->state.bytes;
    ((ActorScriptState *)actionState)->loopExitRequested = 1;
}

/// Stop action playback and clear pending action requests.
/// \param object Actor whose action state is accessed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F4D4 (0x8000F4D4).
void actionStopPlayback(u8 *object) {
    u8 *actionState;

    actionState = ((RuntimeObjectPrefix *)object)->state.bytes;
    ((ActorScriptState *)actionState)->actionStartPending = 0;
    ((ActorScriptState *)actionState)->loopExitRequested = 0;
    ((ActorScriptState *)actionState)->activeRequestId = 0;
    ((ActorScriptState *)actionState)->lastPrimaryCueProgram = NULL;
    ((ActorScriptState *)actionState)->primaryCueCommand = NULL;
    ((ActorScriptState *)actionState)->secondaryCueCommand = NULL;
    ((ActorScriptState *)actionState)->currentPoseId = 0;
    motionResetPlayback(object);
    actionCancelPendingRequest(object);
}

/// Cancel the pending action request.
/// \param object Actor whose action state is accessed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F51C (0x8000F51C).
void actionCancelPendingRequest(u8 *object) {
    u8 *objectState;

    objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    ((ActorScriptState *)objectState)->pendingAction = -1;
    ((ActorScriptState *)objectState)->pendingRequestId = 0;
    ((ActorScriptState *)objectState)->pendingActionKind = 0;
}

/// Replace current playback with a transition to a selected action pose.
/// \param object Actor receiving the pose transition.
/// \param actionId Valid destination action identifier from zero through 329.
/// \param duration Positive pose transition duration in seconds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F534 (0x8000F534).
void actionBeginPoseTransition(u8 *object, s32 actionId, f32 duration) {
    u8 *obj;

    obj = ((RuntimeObjectPrefix *)object)->state.bytes;
    actionStopPlayback(object);
    ((ActorScriptState *)obj)->loopCount = 0;
    ((ActorScriptState *)obj)->actionStartPending = 1;
    actionConfigurePoseTransition((ActorScriptState *)obj, actionId, duration);
    ((ActorScriptState *)obj)->continuationAction = -1;
    motionRequestCurrentData(object);
}

/// Get the attributes of the currently playing action.
/// \param object Actor whose action state is accessed.
/// \returns The current action attributes, or 0x20100400 when playback is inactive.
/// \note Original decompilation identifier: \c func_8000F594 (0x8000F594).
s32 actionGetAttributes(u8 *object) {
    u8 *objectState;

    objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if ((u16)((ActorScriptState *)objectState)->playbackMode == MOTION_PLAYBACK_DISABLED) {
        return 0x20100400;
    }
    return ((ActorScriptState *)objectState)->actionAttributes;
}

/// Get the gravity multiplier associated with the currently playing action.
/// \param object Actor whose current action is inspected.
/// \returns The current action gravity multiplier, or one when playback is inactive.
/// \note Original decompilation identifier: \c func_8000F5BC (0x8000F5BC).
f32 actionGetGravityScale(u8 *object) {
    u8 *objectState;

    objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if ((u16)((ActorScriptState *)objectState)->playbackMode == MOTION_PLAYBACK_DISABLED) {
        return 1.0f;
    }
    return ((ActorScriptState *)objectState)->actionGravityScale;
}

/// Get the motion identifier of the currently playing action.
/// \param object Actor whose action state is accessed.
/// \returns The current motion identifier, or minus one when playback is inactive.
/// \note Original decompilation identifier: \c func_8000F5E8 (0x8000F5E8).
s16 actionGetCurrentMotionId(u8 *object) {
    u8 *objectState;

    objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if ((u16)((ActorScriptState *)objectState)->playbackMode == MOTION_PLAYBACK_DISABLED) {
        return -1;
    }
    return ((ActorScriptState *)objectState)->motionId;
}

/// Get a display name for an action or action sentinel.
/// \param actionId Action identifier or negative action sentinel.
/// \returns The stored action name, a sentinel name, or an invalid-action marker; ownership remains with the action system.
/// \note Original decompilation identifier: \c func_8000F60C (0x8000F60C).
char *actionGetName(s32 actionId) {
    ActionDefinition *var_v1;
    if ((actionId < 0) || (actionId >= 0x14A)) {
        var_v1 = NULL;
    } else {
        var_v1 = &gActionDefinitions[actionId];
    }
    if (var_v1 != NULL) {
        return var_v1->name;
    }
    switch (actionId) {
    case -3:
        return "ACT_NONE";
    case -2:
        return "ACT_INTER";
    case -1:
        return "ACT_END";
    default:
        return "\xCA\xD1\xA1\xA3"; /* EUC-JP "strange." */
    }
}

/// Get the request identifier associated with active action playback.
/// \param object Actor whose action state is accessed.
/// \returns The current request identifier, or zero when playback is inactive.
/// \note Original decompilation identifier: \c func_8000F69C (0x8000F69C).
s32 actionGetActiveRequestId(u8 *object) {
    u8 *objectState;

    objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    if ((u16)((ActorScriptState *)objectState)->playbackMode == MOTION_PLAYBACK_DISABLED) {
        return 0;
    }
    return ((ActorScriptState *)objectState)->activeRequestId;
}

/// Get the most recently selected action request identifier.
/// \param object Actor whose action state is accessed.
/// \returns The stored request identifier, regardless of playback activity.
/// \note Original decompilation identifier: \c func_8000F6C0 (0x8000F6C0).
s32 actionGetStoredRequestId(u8 *object) {
    u8 *obj;

    obj = ((RuntimeObjectPrefix *)object)->state.bytes;
    return ((ActorScriptState *)obj)->activeRequestId;
}

/// Set the continuation action for current playback.
/// \param object Actor whose continuation is changed.
/// \param actionId Continuation action identifier or termination sentinel, representable as a signed 16-bit value; identifiers outside 0 through 329 end continuation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F6CC (0x8000F6CC).
void actionSetContinuation(u8 *object, s32 actionId) {
    u8 *actionState;

    actionState = ((RuntimeObjectPrefix *)object)->state.bytes;
    TRACE("actSetNext:%s\n", actionGetName(actionId));
    if ((actionId < 0) || (actionId >= 0x14A)) {
        ((ActorScriptState *)actionState)->continuationAction = actionId;
        actionRequestLoopExit(object);
        return;
    }
    ((ActorScriptState *)actionState)->continuationAction = actionId;
    actionRequestMotionData(actionId);
}

/// Skip looping continuation actions to reach their nonlooping successor or termination sentinel.
/// \pre The continuation chain must not contain a cycle of looping actions.
/// \param object Actor whose action state is accessed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F714 (0x8000F714).
void actionSkipLoopingContinuations(u8 *object) {
    s16 continuationId;
    u8 *actionState;
    ActionDefinition *continuationRecord;
    actionState = ((RuntimeObjectPrefix *)object)->state.bytes;
loop_1:
    continuationId = ((ActorScriptState *)actionState)->continuationAction;
    if ((continuationId < 0) || (continuationId >= 0x14A)) {
        continuationRecord = NULL;
    } else {
        continuationRecord = &gActionDefinitions[continuationId];
    }
    if ((continuationRecord != NULL) && (continuationRecord->attributes & 8)) {
        ((ActorScriptState *)actionState)->continuationAction = continuationRecord->continuationAction;
        TRACE("next:%s\n", actionGetName(((ActorScriptState *)actionState)->continuationAction));
        goto loop_1;
    }
}

/// Validate that action records have consistent table identifiers in debug builds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F77C (0x8000F77C).
void actionValidateTableIndices(void) {
    s32 i;
    ActionDefinition *p;

    for (i = 0, p = gActionDefinitions; i < 0x14A; i++, p++) {
        if (p->id != i) {
            TRACE("Invalid action table.\n actIdx(%d) != %d\n", i, p->id);
            TRACE(" action:%s\n", p->name);
        }
    }
}
