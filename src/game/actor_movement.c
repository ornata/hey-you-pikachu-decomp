#include "math_scalar.h"
#include "pikachu_actor.h"
#include "vector_metrics.h"
#include "math_angles.h"
#include "random.h"
#include "terrain_height.h"
#include "actor_action_callbacks.h"
#include "action_continuation.h"
#include "action_request.h"
#include "actor_rotation.h"
#include "object_movement.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "vector_direction.h"
#include "runtime_clock.h"
#include "area_maze.h"
#include "actor_parts.h"
#include "actor_target.h"
#include "actor_script.h"
#include "object_types.h"
extern float fabsf(float);
#pragma intrinsic(fabsf)
s32 actorPrepareApproachCompletion(u8 *, f32);
s32 actorAdvanceWithScaleOscillation(f32 *, f32);
s32 actorAdvanceWithGroundClearance(s32, f32);
s32 actorTurnAwayFromPlayer(s32, f32);
s32 actorBackstepTowardTarget(s32, f32);
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
/// Pending vertical movement applied by the airborne action until landing.
/// \note Original identifier: \c D_800A25D0 (0x800A25D0).
extern f32 gActorAirborneVerticalStep;
f32 vector3GetYawDegrees(f32 *);

/// Complete an actor action immediately.
/// \param actor Unused actor parameter retained by the action callback interface.
/// \returns Always one, indicating completion.
/// \note Original decompilation identifier: \c func_8000F7B0 (0x8000F7B0).
s32 actorActionCompleteImmediately(u8 *actor) {
    return 1;
}

/// Upper action-time bound of the timed ground spin.
/// \note Original identifier: \c D_800A25B4 (0x800A25B4).
extern f32 gActorGroundSpinEndTime;
/// Lower action-time bound of the timed ground spin.
/// \note Original identifier: \c D_800A25B8 (0x800A25B8).
extern f32 gActorGroundSpinStartTime;
/// Upper action-time bound of the ground spin vertical lift.
/// \note Original identifier: \c D_800A25BC (0x800A25BC).
extern f32 gActorGroundSpinLiftEndTime;
/// Lower action-time bound of the ground spin vertical lift.
/// \note Original identifier: \c D_800A25C0 (0x800A25C0).
extern f32 gActorGroundSpinLiftStartTime;
/// Yaw rotation rate in degrees per second during the ground spin.
/// \note Original identifier: \c D_800A25C4 (0x800A25C4).
extern f32 gActorGroundSpinYawRate;

/// Apply the timed spinning behavior with its brief vertical lift.
/// \param actor Actor executing the spinning behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_8000F7BC (0x8000F7BC).
s32 actorUpdateTimedGroundSpin(u8 *actor) {
    u8 *obj;
    f32 a;

    obj = *(u8 **)(actor + 0x54);
    a = *(f32 *)(obj + 0x138);
    if ((a < gActorGroundSpinEndTime) && (gActorGroundSpinStartTime < a)) {
        if ((a < gActorGroundSpinLiftEndTime) && (gActorGroundSpinLiftStartTime < a)) {
            *(f32 *)(obj + 0x1DC) = 10.0f * gRuntimeFrameSeconds;
        }
        *(s32 *)(obj + 0x1BC) = 0;
        actorAddRotationAxisDegrees((u8 *)actor, gActorGroundSpinYawRate * gRuntimeFrameSeconds, 1);
    }
    return 0;
}

/// Prepare approach completion using the fast approach-speed preset.
/// \param actor Actor whose approach completion is prepared.
/// \returns The approach-completion result; one selects a continuation immediately, zero keeps the action active.
/// \note Original decompilation identifier: \c func_8000F880 (0x8000F880).
s32 actorPrepareFastApproachCompletion(u8 *actor) {
    return actorPrepareApproachCompletion(actor, 6.9542f);
}

/// Prepare approach completion using the fastest approach-speed preset.
/// \param actor Actor whose approach completion is prepared.
/// \returns The approach-completion result; one selects a continuation immediately, zero keeps the action active.
/// \note Original decompilation identifier: \c func_8000F8A4 (0x8000F8A4).
s32 actorPrepareFastestApproachCompletion(u8 *actor) {
    return actorPrepareApproachCompletion(actor, 8.6197f);
}

/// Prepare approach completion using the slow approach-speed preset.
/// \param actor Actor whose approach completion is prepared.
/// \returns The approach-completion result; one selects a continuation immediately, zero keeps the action active.
/// \note Original decompilation identifier: \c func_8000F8C8 (0x8000F8C8).
s32 actorPrepareSlowApproachCompletion(u8 *actor) {
    return actorPrepareApproachCompletion(actor, 1.4111f);
}

/// Prepare approach completion using the medium approach-speed preset.
/// \param actor Actor whose approach completion is prepared.
/// \returns The approach-completion result; one selects a continuation immediately, zero keeps the action active.
/// \note Original decompilation identifier: \c func_8000F8EC (0x8000F8EC).
s32 actorPrepareMediumApproachCompletion(u8 *actor) {
    return actorPrepareApproachCompletion(actor, 3.4162f);
}

/// Prepare approach completion using the slowest approach-speed preset.
/// \param actor Actor whose approach completion is prepared.
/// \returns The approach-completion result; one selects a continuation immediately, zero keeps the action active.
/// \note Original decompilation identifier: \c func_8000F910 (0x8000F910).
s32 actorPrepareSlowestApproachCompletion(u8 *actor) {
    return actorPrepareApproachCompletion(actor, 1.27f);
}

extern double sqrt(double);
#pragma intrinsic(sqrt)
/// Approach-speed preset selecting the special completion continuation.
/// \note Original identifier: \c D_800A25C8 (0x800A25C8).
extern f32 gActorSpecialApproachSpeed;

/// Prepare an actor approach to finish as its target comes within reach.
/// \param object Actor with initialized action state and an existing reach-reference part; looping continuation chains must terminate.
/// \param speed Positive approach-speed control used to assess remaining travel.
/// \returns One when the special approach continuation is selected; otherwise zero, including when loop exit is requested.
/// \note Original decompilation identifier: \c func_8000F934 (0x8000F934).
s32 actorPrepareApproachCompletion(u8 *object, f32 speed) {
    u8 *state;
    f32 distance;
    u8 *target;
    f32 reference[3];
    s32 roll;

    state = *(u8 **)(object + 0x54);
    target = actorFindPart((u8 *)object, 0x3E);
    reference[0] = *(f32 *)(target + 0x44);
    reference[1] = *(f32 *)(target + 0x48);
    reference[2] = *(f32 *)(target + 0x4C);
    *(f32 *)(state + 0x1F8) = 1.0f;
    distance = -(f64) * (f32 *)(state + 0x1A4) +
               sqrt((f64)((*(f32 *)(state + 0x1EC) - reference[0]) * (*(f32 *)(state + 0x1EC) - reference[0]) +
                          (*(f32 *)(state + 0x1F4) - reference[2]) * (*(f32 *)(state + 0x1F4) - reference[2])));
    if (distance < 0.0f) {
        distance = 0.0f;
    }
    roll = randomBelow(0x3C);
    *(f32 *)(state + 0x1A0) = roll - 15;
    if ((distance / speed) < 1.5f) {
        if (speed == gActorSpecialApproachSpeed) {
            actionSetContinuation((u8 *)object, 0xC6);
            return 1;
        }
        actorRefreshTrackingTargetPosition((u8 *)object);
        actorTurnTowardTarget((u8 *)object, 300.0f);
        actionSkipLoopingContinuations((u8 *)object);
        actionRequestLoopExit((u8 *)object);
        *(f32 *)(state + 0x1F8) = (distance / speed);
    }
    return 0;
}

/// Track and turn toward the target while applying the actor movement scale.
/// \pre The actor must be the current object update with initialized tracking and action state.
/// \param actor Actor executing the movement behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_8000FA84 (0x8000FA84).
s32 actorTrackTargetAndScaleMovement(u8 *actor) {
    u8 *obj;
    f32 v[3];

    obj = *(u8 **)(actor + 0x54);
    actorRefreshTrackingTargetPosition((u8 *)actor);
    actorTurnTowardTarget((u8 *)actor, 300.0f);
    if (*(f32 *)(obj + 0x1F8) != 1.0f) {
        objectGetPendingLocalMovement(v);
        v[0] = *(f32 *)(obj + 0x1F8) * v[0];
        v[1] = *(f32 *)(obj + 0x1F8) * v[1];
        v[2] = *(f32 *)(obj + 0x1F8) * v[2];
        objectSetPendingLocalMovement(v);
    }
    return 0;
}

/// Apply the actor movement scale to its pending local movement.
/// \pre The actor must be the current object update with initialized action state.
/// \param actor Actor executing the movement behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_8000FB24 (0x8000FB24).
s32 actorScalePendingMovement(u8 *actor) {
    u8 *sp24;
    f32 sp[3];

    sp24 = *(u8 **)(actor + 0x54);
    objectGetPendingLocalMovement(sp);
    sp[0] *= *(f32 *)(sp24 + 0x1F8);
    sp[1] *= *(f32 *)(sp24 + 0x1F8);
    sp[2] *= *(f32 *)(sp24 + 0x1F8);
    objectSetPendingLocalMovement(sp);
    return 0;
}

/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

/// Face the tracked target while stepping backward with slight lateral variation.
/// \pre The actor must be the current object update; current area size settings must be initialized with a nonzero reference scale.
/// \param actor Actor executing the movement behavior.
/// \param speed Backward local movement speed in units per second, before actor-size adjustment.
/// \returns One when movement constraint flags are active; otherwise zero.
/// \note Original decompilation identifier: \c func_8000FB8C (0x8000FB8C).
s32 actorBackstepTowardTarget(s32 actor, f32 speed) {
    u8 *obj;
    f32 amp;
    f32 v[3];

    obj = *(u8 **)(actor + 0x54);
    amp = gRuntimeFrameSeconds * speed;
    actorRefreshTrackingTargetPosition((u8 *)actor);
    actorTurnTowardTarget((u8 *)actor, 300.0f);
    amp *= 1.0 + (((*(f32 *)actor / *(f32 *)(gCurrentAreaConfiguration + 0xC)) - 1.0f) * 0.5f);
    v[0] = (f32)((s32)randomBelow(10) - 4);
    v[1] = 0.0f;
    v[2] = -amp;
    objectSetPendingLocalMovement(v);
    if (*(u16 *)(obj + 0x1EA) != 0) {
        return 1;
    }
    return 0;
}

/// Apply the slow backward-step behavior while facing the tracked target.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_8000FC78 (0x8000FC78).
s32 actorBackstepSlowly(u8 *actor) {
    return actorBackstepTowardTarget((s32)actor, 50.0f);
}

/// Apply the fast backward-step behavior while facing the tracked target.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_8000FC98 (0x8000FC98).
s32 actorBackstepQuickly(u8 *actor) {
    return actorBackstepTowardTarget((s32)actor, 400.0f);
}


/// Choose a random continuation once the actor is facing its target.
/// \param actor Actor whose facing and continuation are inspected and updated.
/// \returns One when a continuation is selected; otherwise zero.
/// \note Original decompilation identifier: \c func_8000FCB8 (0x8000FCB8).
s32 actorChooseAlignedTargetContinuation(u8 *actor) {
    u8 *obj;
    f32 v[3];

    obj = *(u8 **)(actor + 0x54);
    v[0] = *(f32 *)(obj + 0x1EC) - *(f32 *)(actor + 0x44);
    v[1] = 0.0f;
    v[2] = *(f32 *)(obj + 0x1F4) - *(f32 *)(actor + 0x4C);
    if (fabsf(mathWrapSignedDegrees(vector3GetYawDegrees(v) - *(f32 *)(obj + 0x10))) < 10.0f) {
        actionSetContinuation((u8 *)actor, (0.5 < (f64)randomNext() * 2.3283064365386963e-10) ? 0xBF : 0xC0);
        return 1;
    }
    return 0;
}


/// Turn toward the tracked target and request a loop exit when aligned.
/// \param actor Actor executing the turning behavior.
/// \returns Always zero; the loop-exit request controls action completion.
/// \note Original decompilation identifier: \c func_8000FDB0 (0x8000FDB0).
s32 actorTurnToTrackedTargetAndExitLoop(u8 *actor) {
    actorRefreshTrackingTargetPosition((u8 *)actor);
    if (actorTurnTowardTarget((u8 *)actor, 100.0f) != 0) {
        actionRequestLoopExit((u8 *)actor);
    }
    return 0;
}

/// Pivot toward the target and request a loop exit when aligned.
/// \param actor Actor executing the turning behavior.
/// \returns Always zero; the loop-exit request controls action completion.
/// \note Original decompilation identifier: \c func_8000FDF0 (0x8000FDF0).
s32 actorPivotToTargetAndExitLoop(u8 *actor) {
    if (actorPivotTowardTarget((u8 *)actor, 100.0f) != 0) {
        actionRequestLoopExit((u8 *)actor);
    }
    return 0;
}

/// Provide an actor action hook that requires no work.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000FE28 (0x8000FE28).
void actorActionNoOp(void) {
}

/// Select the turn continuation when the target lies to the actor left.
/// \param actor Actor whose target direction determines its continuation.
/// \returns One when the left-target continuation is selected; otherwise zero.
/// \note Original decompilation identifier: \c func_8000FE30 (0x8000FE30).
s32 actorSelectLeftTargetContinuation(u8 *actor) {
    u8 *obj;
    f32 v[3];

    obj = *(u8 **)(actor + 0x54);
    v[0] = *(f32 *)(obj + 0x1EC) - *(f32 *)(actor + 0x44);
    v[1] = 0.0f;
    v[2] = *(f32 *)(obj + 0x1F4) - *(f32 *)(actor + 0x4C);
    if (mathWrapSignedDegrees(vector3GetYawDegrees(v) - *(f32 *)(obj + 0x10)) < 0.0f) {
        actionSetContinuation((u8 *)actor, 0x22);
        return 1;
    }
    return 0;
}

/// Choose the primary follow-up action for an aligned target or a target to the actor left.
/// \param actor Actor whose target direction determines the follow-up action.
/// \returns One when a follow-up action is selected; otherwise zero.
/// \note Original decompilation identifier: \c func_8000FEC4 (0x8000FEC4).
s32 actorSelectAlignedOrLeftTargetContinuation(u8 *actor) {
    u8 *obj;
    f32 v[3];
    f32 d;

    obj = *(u8 **)(actor + 0x54);
    v[0] = *(f32 *)(obj + 0x1EC) - *(f32 *)(actor + 0x44);
    v[1] = 0.0f;
    v[2] = *(f32 *)(obj + 0x1F4) - *(f32 *)(actor + 0x4C);
    d = mathWrapSignedDegrees(vector3GetYawDegrees(v) - *(f32 *)(obj + 0x10));
    if (fabsf(d) < 20.0f) {
        actionSetContinuation((u8 *)actor, 0xBE);
        return 1;
    }
    if (d < 0.0f) {
        actionSetContinuation((u8 *)actor, 0xC4);
        return 1;
    }
    return 0;
}

/// Track and turn toward the target at a speed selected for the current actor state.
/// \param actor Actor executing the turning behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_8000FF8C (0x8000FF8C).
s32 actorUpdateAdaptiveTargetTurn(u8 *actor) {
    void *sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.pointer;
    actorRefreshTrackingTargetPosition((u8 *)actor);
    if (((ActorScriptState *)sp1C)->airborne != 0) {
        actorTurnTowardTarget((u8 *)actor, 720.0f);
    } else {
        actorTurnTowardTarget((u8 *)actor, 240.0f);
    }
    return 0;
}

/// Choose the alternate follow-up action for an aligned target or a target to the actor left.
/// \param actor Actor whose target direction determines the follow-up action.
/// \returns One when a follow-up action is selected; otherwise zero.
/// \note Original decompilation identifier: \c func_8000FFE4 (0x8000FFE4).
s32 actorSelectAlternateAlignedOrLeftTargetContinuation(u8 *actor) {
    u8 *obj;
    f32 v[3];
    f32 d;

    obj = *(u8 **)(actor + 0x54);
    v[0] = *(f32 *)(obj + 0x1EC) - *(f32 *)(actor + 0x44);
    v[1] = 0.0f;
    v[2] = *(f32 *)(obj + 0x1F4) - *(f32 *)(actor + 0x4C);
    d = mathWrapSignedDegrees(vector3GetYawDegrees(v) - *(f32 *)(obj + 0x10));
    if (fabsf(d) < 20.0f) {
        actionSetContinuation((u8 *)actor, 0xC1);
        return 1;
    }
    if (d < 0.0f) {
        actionSetContinuation((u8 *)actor, 0xC5);
        return 1;
    }
    return 0;
}

/// Pivot toward the target at a speed selected for the current actor state.
/// \param actor Actor executing the turning behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_800100AC (0x800100AC).
s32 actorUpdateAdaptiveTargetPivot(u8 *actor) {
    u8 *actorState = *(u8 **)(actor + 0x54);
    if (*(s32 *)(actorState + 0x1BC) != 0) {
        actorPivotTowardTarget((u8 *)actor, 720.0f);
    } else {
        actorPivotTowardTarget((u8 *)actor, 240.0f);
    }
    return 0;
}

extern double fabs(double);
#pragma intrinsic(fabs)
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern f32 gPlayerState[];

/// Turn an actor away from the player position.
/// \param actor Actor whose horizontal facing is updated.
/// \param turnSpeed Nonnegative angular turn speed in degrees per second.
/// \returns One when the facing is within one degree of the desired heading; otherwise zero.
/// \note Original decompilation identifier: \c func_800100F0 (0x800100F0).
s32 actorTurnAwayFromPlayer(s32 actor, f32 turnSpeed) {
    u8 *obj;
    f32 v[3];
    f32 a;
    s32 pad[2]; /* unused, sizes the frame */

    obj = *(u8 **)(actor + 0x54);
    v[0] = *(f32 *)(actor + 0x44) - gPlayerState[0];
    v[1] = 0.0f;
    v[2] = *(f32 *)(actor + 0x4C) - gPlayerState[2];
    a = vector3GetYawDegrees(v);
    if (fabs((f64)(a - *(f32 *)(obj + 0x10))) < 1.0) {
        return 1;
    }
    actorAddRotationAxisDegrees(
        (u8 *)actor,
        mathApproachAngleDegrees(*(f32 *)(obj + 0x10), a, 1.0f, 90.0f, turnSpeed * gRuntimeFrameSeconds) -
            *(f32 *)(obj + 0x10),
        1);
    if (fabsf(a - *(f32 *)(obj + 0x10)) < 1.0f) {
        return 1;
    }
    return 0;
}

/// Update the action behavior that turns an actor away from the player position.
/// \param actor Actor executing the turn-away behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80010210 (0x80010210).
s32 actorUpdateTurnAwayFromPlayer(u8 *actor) {
    actorTurnAwayFromPlayer((s32)actor, 720.0f);
    return 0;
}

/// Finish the current action on landing or choose its constrained-movement continuation.
/// \param actor Actor whose airborne state and movement constraints are inspected.
/// \returns One when grounded or movement constraints select a continuation; otherwise zero.
/// \note Original decompilation identifier: \c func_80010234 (0x80010234).
s32 actorFinishOnLandingOrMovementConstraint(u8 *actor) {
    u8 *actorState = *(u8 **)(actor + 0x54);
    if (*(u16 *)(actorState + 0x1EA) != 0) {
        actionSetContinuation((u8 *)actor, 0x3D);
        return 1;
    }
    if (*(s32 *)(actorState + 0x1BC) == 0) {
        return 1;
    }
    return 0;
}

/// Wait for the actor to reach the ground.
/// \param actor Actor whose airborne state is inspected.
/// \returns One when grounded; otherwise zero.
/// \note Original decompilation identifier: \c func_80010284 (0x80010284).
s32 actorWaitUntilGrounded(u8 *actor) {
    u8 *actorState = *(u8 **)(actor + 0x54);
    if (*(s32 *)(actorState + 0x1BC) == 0) {
        return 1;
    }
    return 0;
}

/// Choose whether to finish an action based on its repetition count.
/// \param actor Actor whose action repetition count is inspected.
/// \returns One when the action should finish; otherwise zero.
/// \note Original decompilation identifier: \c func_800102A8 (0x800102A8).
s32 actorShouldEndRepeatedAction(u8 *actor) {
    u8 *actorState = *(u8 **)(actor + 0x54);
    if ((s32)randomBelow(*(s16 *)(actorState + 0x11C)) >= 4) {
        return 1;
    }
    return 0;
}


/// Turn slowly toward the tracked target until aligned.
/// \param actor Actor executing the turning behavior.
/// \returns One when aligned with the constrained target heading; otherwise zero.
/// \note Original decompilation identifier: \c func_800102DC (0x800102DC).
s32 actorTurnToTrackedTargetSlowly(u8 *actor) {
    actorRefreshTrackingTargetPosition((u8 *)actor);
    if (actorTurnTowardTarget((u8 *)actor, 120.0f) != 0) {
        return 1;
    }
    return 0;
}


/// Turn quickly toward the tracked target until aligned.
/// \param actor Actor executing the turning behavior.
/// \returns One when aligned with the constrained target heading; otherwise zero.
/// \note Original decompilation identifier: \c func_80010314 (0x80010314).
s32 actorTurnToTrackedTargetQuickly(u8 *actor) {
    actorRefreshTrackingTargetPosition((u8 *)actor);
    if (actorTurnTowardTarget((u8 *)actor, 1440.0f) != 0) {
        return 1;
    }
    return 0;
}

/// Immediately face the actor toward its stored target position.
/// \param object Actor whose horizontal facing is updated.
/// \returns Always one, indicating completion.
/// \note Original decompilation identifier: \c func_8001034C (0x8001034C).
s32 actorFaceTargetPosition(u8 *object) {
    f32 *position;
    struct {
        f32 x, y, z;
    } direction;
    f32 *target;
    u8 *state;
    f32 angle;
    state = *(u8 **)(object + 0x54);
    position = (f32 *)(object + 0x44);
    target = (f32 *)(state + 0x1EC);
    direction.x = target[0] - position[0];
    direction.y = 0.0f;
    direction.z = target[2] - position[2];
    angle = vector3GetYawDegrees((f32 *)&direction);
    actorSetRotationAxisDegrees(object, angle, 1);
    return 1;
}

/// Track and advance toward the target until close enough or movement is constrained.
/// \param actor Actor executing the movement behavior.
/// \returns One when the target proximity threshold is reached or movement constraint flags are active; otherwise zero.
/// \note Original decompilation identifier: \c func_800103B4 (0x800103B4).
s32 actorAdvanceToTarget(u8 *actor) {
    u8 *obj;
    f32 out[3];
    f32 dir[3];

    obj = *(u8 **)(actor + 0x54);
    actorRefreshTrackingTargetPosition((u8 *)actor);
    actorTurnTowardTarget((u8 *)actor, 300.0f);
    dir[0] = *(f32 *)(obj + 0x1EC) - *(f32 *)(actor + 0x44);
    dir[1] = 0.0f;
    dir[2] = *(f32 *)(obj + 0x1F4) - *(f32 *)(actor + 0x4C);
    if ((vector3Length(dir) < *(f32 *)(obj + 0x1A4) + *(f32 *)(obj + 0x204)) || (*(u16 *)(obj + 0x1EA) != 0)) {
        return 1;
    }
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = gRuntimeFrameSeconds * 1000.0f;
    objectAddPendingLocalMovement(out);
    return 0;
}

/// Apply forward movement with slight lateral variation.
/// \pre The actor must be the current object update; current area size settings must have a nonzero reference scale.
/// \param actor Actor executing the movement behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80010480 (0x80010480).
s32 actorAdvanceWithLateralVariation(u8 *actor) {
    s32 pad; /* unused, sizes the frame */
    f32 v[3];
    f32 amp;

    amp = gRuntimeFrameSeconds * 400.0f;
    amp *= 1.0 + (((*((f32 *)actor) / *(f32 *)(gCurrentAreaConfiguration + 0xC)) - 1.0f) * 0.5f);
    v[0] = (f32)((s32)randomBelow(10) - 4);
    v[1] = 0.0f;
    v[2] = amp;
    objectAddPendingLocalMovement(v);
    return 0;
}

/// Apply faster forward movement with slight lateral variation.
/// \pre The actor must be the current object update; current area size settings must have a nonzero reference scale.
/// \param actor Actor executing the movement behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80010530 (0x80010530).
s32 actorAdvanceQuicklyWithLateralVariation(u8 *actor) {
    s32 pad; /* unused, sizes the frame */
    f32 v[3];
    f32 amp;

    amp = gRuntimeFrameSeconds * 600.0f;
    amp *= 1.0 + (((*((f32 *)actor) / *(f32 *)(gCurrentAreaConfiguration + 0xC)) - 1.0f) * 0.5f);
    v[0] = (f32)((s32)randomBelow(10) - 4);
    v[1] = 0.0f;
    v[2] = amp;
    objectAddPendingLocalMovement(v);
    return 0;
}

/// Apply a small random horizontal turn during an action.
/// \param actor Actor receiving the facing adjustment.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_800105E0 (0x800105E0).
s32 actorApplyRandomYawJitter(u8 *actor) {
    actorAddRotationAxisDegrees((u8 *)actor, (f32)((s32)randomBelow(0xA) - 5), 1);
    return 0;
}

/// Advance the actor while tumbling and rising.
/// \param actor Actor executing the behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80010628 (0x80010628).
s32 actorTumbleForward(u8 *actor) {
    u8 *obj;
    f32 v[3];
    f32 amp;

    amp = gRuntimeFrameSeconds * 4000.0f;
    obj = *(u8 **)(actor + 0x54);
    actorAddRotationAxisDegrees((u8 *)actor, 45.0f, 0);
    if ((90.0f < *(f32 *)(obj + 0xC)) && (*(f32 *)(obj + 0xC) < 270.0f)) {
        amp = -amp;
    }
    *(f32 *)(actor + 0x1C) = *(f32 *)(actor + 0x1C) + 1.5f;
    v[1] = 0.0f;
    v[0] = 0.0f;
    v[2] = amp;
    objectAddPendingLocalMovement(v);
    return 0;
}

/// Local forward movement rate for the tumbling approach.
/// \note Original identifier: \c D_800A25CC (0x800A25CC).
extern f32 gActorTumbleForwardSpeed;
/// Approach the tracked target while tumbling and rising.
/// \param actor Actor executing the behavior.
/// \returns One when the target is within the approach range; otherwise zero.
/// \note Original decompilation identifier: \c func_800106E4 (0x800106E4).
s32 actorTumbleTowardTarget(u8 *actor) {
    u8 *state = *(u8 **)(actor + 0x54);
    f32 motion[3];
    f32 direction[3];
    f32 amplitude;

    amplitude = gRuntimeFrameSeconds * gActorTumbleForwardSpeed;
    actorRefreshTrackingTargetPosition(actor);
    actorTurnTowardTarget((u8 *)actor, 300.0f);
    actorAddRotationAxisDegrees(actor, (f32)((s32)randomBelow(10) - 5), 1);
    actorAddRotationAxisDegrees(actor, 45.0f, 0);
    if (90.0f < *(f32 *)(state + 0xC) && *(f32 *)(state + 0xC) < 270.0f) {
        amplitude = -amplitude;
    }
    *(f32 *)(actor + 0x1C) += 1.5f;
    motion[1] = 0.0f;
    motion[0] = 0.0f;
    motion[2] = amplitude;
    objectAddPendingLocalMovement(motion);
    direction[0] = *(f32 *)(state + 0x1EC) - *(f32 *)(actor + 0x44);
    direction[1] = 0.0f;
    direction[2] = *(f32 *)(state + 0x1F4) - *(f32 *)(actor + 0x4C);
    amplitude = vector3Length(direction);
    if (amplitude < *(f32 *)(state + 0x1A4) + *(f32 *)(state + 0x204)) {
        return 1;
    }
    return 0;
}

/// Raise the actor local position at the faster vertical step rate.
/// \param actor Actor executing the behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80010838 (0x80010838).
s32 actorRaiseLocalHeightQuickly(u8 *actor) {
    ((RuntimeObjectPrefix *)actor)->position[1] = (f32)(((RuntimeObjectPrefix *)actor)->position[1] + 2.0f);
    return 0;
}

/// Apply the action vertical movement while airborne and finish on landing.
/// \param actor Actor whose airborne state controls the pending vertical movement.
/// \returns One when grounded; otherwise zero.
/// \note Original decompilation identifier: \c func_80010854 (0x80010854).
s32 actorUpdateAirborneVerticalMovement(u8 *actor) {
    u8 *obj;
    f32 sp18[3];

    obj = *(u8 **)(actor + 0x54);
    if (*(s32 *)(obj + 0x1BC) != 0) {
        sp18[0] = 0.0f;
        sp18[1] = gActorAirborneVerticalStep;
        sp18[2] = 0.0f;
        objectAddPendingLocalMovement(sp18);
        return 0;
    }
    return 1;
}

/// Raise the actor local position at the slower vertical step rate.
/// \param actor Actor executing the behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_800108A8 (0x800108A8).
s32 actorRaiseLocalHeightSlowly(u8 *actor) {
    ((RuntimeObjectPrefix *)actor)->position[1] = (f32)(((RuntimeObjectPrefix *)actor)->position[1] + 0.5f);
    return 0;
}

/// Lower the actor local position at the slower vertical step rate.
/// \param actor Actor executing the behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_800108C4 (0x800108C4).
s32 actorLowerLocalHeightSlowly(u8 *actor) {
    ((RuntimeObjectPrefix *)actor)->position[1] = (f32)(((RuntimeObjectPrefix *)actor)->position[1] - 0.5f);
    return 0;
}

/// Apply a horizontal spin during an actor action.
/// \param actor Actor executing the behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_800108E0 (0x800108E0).
s32 actorSpinYaw(u8 *actor) {
    actorAddRotationAxisDegrees((u8 *)actor, 20.0f, 1);
    return 0;
}

/// Restore the actor pitch to its neutral orientation.
/// \param actor Actor executing the behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80010908 (0x80010908).
s32 actorResetPitch(u8 *actor) {
    u8 *obj;

    obj = *(u8 **)(actor + 0x54);
    actorAddRotationAxisDegrees((u8 *)actor, -*(f32 *)(obj + 0xC), 0);
    return 0;
}

/// Yaw rotation rate in degrees per second while approaching target height.
/// \note Original identifier: \c D_800A25D4 (0x800A25D4).
extern f32 gActorHeightApproachYawRate;

/// Move toward the target height while rotating horizontally.
/// \pre The actor must be the current object update with a nonzero vertical scale and initialized target position.
/// \param actor Actor executing the behavior.
/// \returns One when the height error is within tolerance; otherwise zero.
/// \note Original decompilation identifier: \c func_80010940 (0x80010940).
s32 actorApproachTargetHeightWhileSpinning(u8 *actor) {
    u8 *obj;
    f32 v[3];
    f32 d;
    f32 lim;
    s32 sgn;

    obj = *(u8 **)(actor + 0x54);
    d = (*(f32 *)(obj + 0x1F0) - *(f32 *)(actor + 0x48)) / *(f32 *)(actor + 4);
    if (fabsf(d) < 1.0f) {
        return 1;
    }
    lim = 300.0f * gRuntimeFrameSeconds;
    if (fabsf(d) < lim) {
        lim = fabsf(d);
    }
    v[0] = 0.0f;
    sgn = (0.0f < d) ? 1 : ((d < 0.0f) ? -1 : 0);
    v[2] = 0.0f;
    v[1] = (f32)sgn * lim;
    objectAddPendingLocalMovement(v);
    actorAddRotationAxisDegrees((u8 *)actor, gActorHeightApproachYawRate * gRuntimeFrameSeconds, 1);
    return 0;
}

/// Restore the actor to a level pitch.
/// \param actor Actor executing the behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80010A44 (0x80010A44).
s32 actorRestoreLevelPitch(u8 *actor) {
    u8 *obj;

    obj = *(u8 **)(actor + 0x54);
    actorAddRotationAxisDegrees((u8 *)actor, -*(f32 *)(obj + 0xC), 0);
    return 0;
}

/// Set the actor to its forward-tilted pose.
/// \param actor Actor executing the behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80010A7C (0x80010A7C).
s32 actorSetForwardTiltPose(u8 *actor) {
    u8 *actorState;
    u8 *sp24;

    actorState = *(u8 **)(actor + 0x54);
    sp24 = actorState;
    actorAddRotationAxisDegrees((u8 *)actor, -*(f32 *)(actorState + 0xC), 0);
    actorAddRotationAxisDegrees((u8 *)actor, -*(f32 *)(actorState + 0x10), 1);
    actorAddRotationAxisDegrees((u8 *)actor, -90.0f, 1);
    actorAddRotationAxisDegrees((u8 *)actor, 10.0f, 0);
    actorAddRotationAxisDegrees((u8 *)actor, 90.0f, 1);
    return 0;
}

/// Track and face the target until it is within the actor approach range.
/// \param actor Actor executing the behavior.
/// \returns One when the target is within the approach range; otherwise zero.
/// \note Original decompilation identifier: \c func_80010B10 (0x80010B10).
s32 actorTrackTargetUntilInRange(u8 *actor) {
    u8 *obj;
    f32 v[3];

    obj = *(u8 **)(actor + 0x54);
    actorRefreshTrackingTargetPosition((u8 *)actor);
    actorTurnTowardTarget((u8 *)actor, 300.0f);
    v[0] = *(f32 *)(obj + 0x1EC) - *(f32 *)(actor + 0x44);
    v[1] = 0.0f;
    v[2] = *(f32 *)(obj + 0x1F4) - *(f32 *)(actor + 0x4C);
    if (*(f32 *)(obj + 0x204) + *(f32 *)(obj + 0x1A4) > vector3Length(v)) {
        return 1;
    }
    return 0;
}

typedef struct {
    u8 unknown[0x1A4];
    f32 radius;
} MovementState;
#define MOVE_MAX(a, b) ((a) < (b) ? (b) : (a))
#define MOVE_MIN(a, b) ((a) < (b) ? (a) : (b))

/// Approach the tracked target with horizontal turning and vertical movement.
/// \param object Actor with initialized tracking and action state and nonzero vertical and depth scales.
/// \returns One when the target is within the actor arrival radius; otherwise zero.
///
/// \note NONPORTABLE: IDO schedules the vertical sign and speed by source line.
/// Maze bounds retain byte views because direct members change generated code.
/// \note Original decompilation identifier: \c func_80010BA0 (0x80010BA0).
s32 actorApproachTargetInThreeDimensions(u8 *object) {
    u8 *state;
    f32 movement[3];
    f32 direction[3];
    f32 vertical;
    f32 step;
    f32 distance;
    f32 heading;
    s32 sign;

    state = *(u8 **)(object + 0x54);
    actorRefreshTrackingTargetPosition(object);
    direction[0] = *(f32 *)(state + 0x1EC) - *(f32 *)(object + 0x44);
    direction[1] = *(f32 *)(state + 0x1F0) - *(f32 *)(object + 0x48);
    direction[2] = *(f32 *)(state + 0x1F4) - *(f32 *)(object + 0x4C);
    if (vector3Length(direction) < 1.0f + ((MovementState *)state)->radius) {
        return 1;
    }
    movement[0] = direction[0];
    movement[1] = 0.0f;
    movement[2] = direction[2];
    distance = vector3Length(movement);
    step = MOVE_MIN(100.0f, MOVE_MAX(10.0f, distance * 0.5f)) * gRuntimeFrameSeconds;
    if (distance <= step) {
        if (distance > 0.0f && actorTurnTowardTarget(object, 720.0f) == 0) {
            step = 0.0f;
        } else {
            step = distance;
        }
    } else {
        heading = vector3GetYawDegrees(direction);
        actorAddRotationAxisDegrees(
            object,
            mathApproachAngleDegrees(
                *(f32 *)(state + 0x10), heading, 5.0f, 180.0f,
                MOVE_MAX(300.0f * gRuntimeFrameSeconds, 179.0f - 100.0f * distance * gRuntimeFrameSeconds)) -
                *(f32 *)(state + 0x10),
            1);
    }

    // clang-format off
    sign = direction[1] > 0.0f ? 1 : (direction[1] < 0.0f ? -1 : 0); vertical = MOVE_MIN(100.0f, MOVE_MAX(10.0f, fabsf(direction[1]) * 0.5f)) * sign * gRuntimeFrameSeconds;
    // clang-format on
    if (fabsf(direction[1]) <= fabsf(vertical)) {
        vertical = direction[1];
    }
    movement[0] = 0.0f;
    movement[1] = vertical / *(f32 *)(object + 4);
    movement[2] = step / *(f32 *)(object + 8);
    objectAddPendingLocalMovement(movement);
    return 0;
}

/// Approach the tracked target horizontally while keeping the destination inside scene bounds.
/// \param object Actor with initialized tracking and action state and nonzero depth scale; current scene bounds must be initialized.
/// \returns One when the bounded destination is within the actor arrival radius; otherwise zero.
///
/// \note NONPORTABLE: IDO schedules these vector stores by source line.
/// \note Original decompilation identifier: \c func_80010F2C (0x80010F2C).
s32 actorApproachTargetWithinSceneBounds(u8 *object) {
    u8 *state;
    f32 movement[3];
    f32 direction[3];
    f32 step;
    f32 distance;
    f32 heading;
    f32 turn;

    state = *(u8 **)(object + 0x54);
    actorRefreshTrackingTargetPosition(object);
    if (*(f32 *)(state + 0x1EC) < *(f32 *)((u8 *)gAreaMazeRoot + 8)) {
        *(f32 *)(state + 0x1EC) = *(f32 *)((u8 *)gAreaMazeRoot + 8);
    } else if (*(f32 *)(state + 0x1EC) > *(f32 *)((u8 *)gAreaMazeRoot + 0x14) + *(f32 *)((u8 *)gAreaMazeRoot + 8)) {
        *(f32 *)(state + 0x1EC) = *(f32 *)((u8 *)gAreaMazeRoot + 0x14) + *(f32 *)((u8 *)gAreaMazeRoot + 8);
    }
    *(f32 *)(state + 0x1F0) = *(f32 *)(object + 0x48);
    if (*(f32 *)(state + 0x1F4) < *(f32 *)((u8 *)gAreaMazeRoot + 0x10)) {
        *(f32 *)(state + 0x1F4) = *(f32 *)((u8 *)gAreaMazeRoot + 0x10);
    } else if (*(f32 *)(state + 0x1F4) > *(f32 *)((u8 *)gAreaMazeRoot + 0x1C) + *(f32 *)((u8 *)gAreaMazeRoot + 0x10)) {
        *(f32 *)(state + 0x1F4) = *(f32 *)((u8 *)gAreaMazeRoot + 0x1C) + *(f32 *)((u8 *)gAreaMazeRoot + 0x10);
    }
    direction[0] = *(f32 *)(state + 0x1EC) - *(f32 *)(object + 0x44);
    direction[1] = *(f32 *)(state + 0x1F0) - *(f32 *)(object + 0x48);
    direction[2] = *(f32 *)(state + 0x1F4) - *(f32 *)(object + 0x4C);
    if (vector3Length(direction) < 1.0f + ((MovementState *)state)->radius) {
        return 1;
    }

    // clang-format off
    movement[0] = direction[0]; movement[1] = 0.0f; movement[2] = direction[2];
    // clang-format on
    distance = vector3Length(movement);
    step = MOVE_MIN(10.0f, MOVE_MAX(10.0f, distance * 0.5f)) * gRuntimeFrameSeconds;
    if (distance <= step) {
        if (distance > 0.0f && actorTurnTowardTarget(object, 720.0f) == 0) {
            step = 0.0f;
        } else {
            step = distance;
        }
    } else {
        heading = vector3GetYawDegrees(direction);
        turn = gRuntimeFrameSeconds;
        turn = MOVE_MAX(300.0f * turn, 179.0f - 100.0f * distance * turn);
        actorAddRotationAxisDegrees(object,
                      mathApproachAngleDegrees(*(f32 *)(state + 0x10), heading, 5.0f, 180.0f, turn) - *(f32 *)(state + 0x10), 1);
    }
    movement[0] = 0;
    movement[1] = 0.0f;
    movement[2] = step / *(f32 *)(object + 8);
    objectAddPendingLocalMovement(movement);
    return 0;
}
#undef MOVE_MAX
#undef MOVE_MIN


/// Record ground clearance and check whether the tracked target is within approach range.
/// \param actor Actor whose ground clearance and target are inspected.
/// \returns One when the target is in range or movement constraint flags are active; otherwise zero.
/// \note Original decompilation identifier: \c func_80011224 (0x80011224).
s32 actorRecordGroundClearanceAndCheckTarget(u8 *actor) {
    u8 *obj;
    f32 v[3];
    f32 y;

    obj = *(u8 **)(actor + 0x54);
    actorRefreshTrackingTargetPosition((u8 *)actor);
    y = terrainGetWorldHeight(*(f32 *)(actor + 0x18), *(f32 *)(actor + 0x1C), *(f32 *)(actor + 0x20), 0);
    if (*(f32 *)(actor + 0x48) < y) {
        *(f32 *)(obj + 0x1F8) = 0.0f;
    } else {
        *(f32 *)(obj + 0x1F8) = *(f32 *)(actor + 0x48) - y;
    }
    v[0] = *(f32 *)(obj + 0x1EC) - *(f32 *)(actor + 0x44);
    v[1] = 0.0f;
    v[2] = *(f32 *)(obj + 0x1F4) - *(f32 *)(actor + 0x4C);
    if (*(f32 *)(obj + 0x204) + *(f32 *)(obj + 0x1A4) > vector3Length(v)) {
        return 1;
    }
    if (*(u16 *)(obj + 0x1EA) != 0) {
        return 1;
    }
    return 0;
}

/// Apply the standard forward approach behavior with minimum ground clearance.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_800112FC (0x800112FC).
s32 actorAdvanceAboveGround(u8 *actor) {
    return actorAdvanceWithGroundClearance((s32)actor, 500.0f);
}

/// Apply the faster forward approach behavior with minimum ground clearance.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_8001131C (0x8001131C).
s32 actorAdvanceAboveGroundQuickly(u8 *actor) {
    return actorAdvanceWithGroundClearance((s32)actor, 800.0f);
}


/// Track and advance toward the target while maintaining minimum ground clearance.
/// \param actor Actor executing the movement behavior.
/// \param speed Forward local movement speed in units per second.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_8001133C (0x8001133C).
s32 actorAdvanceWithGroundClearance(s32 actor, f32 speed) {
    f32 y;
    f32 v[3];
    s32 pad; /* unused, sizes the frame */

    actorRefreshTrackingTargetPosition((u8 *)actor);
    actorTurnTowardTarget((u8 *)actor, 300.0f);
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = gRuntimeFrameSeconds * speed;
    objectAddPendingLocalMovement(v);
    y = terrainGetWorldHeight(*(f32 *)(actor + 0x18), *(f32 *)(actor + 0x1C), *(f32 *)(actor + 0x20), 0);
    if (*(f32 *)(actor + 0x1C) - y < 10.0f) {
        *(f32 *)(actor + 0x18) = *(f32 *)(actor + 0x18);
        *(f32 *)(actor + 0x1C) = y + 10.0f;
        *(f32 *)(actor + 0x20) = *(f32 *)(actor + 0x20);
    }
    return 0;
}

/// Maintain a fixed ground clearance while facing the tracked target.
/// \param actor Actor whose height and facing are updated.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_800113E8 (0x800113E8).
s32 actorFollowGroundAndFaceTarget(u8 *actor) {
    f32 y;
    actorRefreshTrackingTargetPosition((u8 *)actor);
    actorTurnTowardTarget((u8 *)actor, 300.0f);
    y = terrainGetWorldHeight(*(f32 *)(actor + 0x18), *(f32 *)(actor + 0x1C), *(f32 *)(actor + 0x20), 0);
    *(f32 *)(actor + 0x18) = *(f32 *)(actor + 0x18);
    *(f32 *)(actor + 0x1C) = y + 11.5f;
    *(f32 *)(actor + 0x20) = *(f32 *)(actor + 0x20);
    return 0;
}

/// Initialize a vertical body-scale pulse and restore the actor base height scale.
/// \param actor Actor whose pulse state and vertical scale are initialized.
/// \returns Always zero.
/// \note Original decompilation identifier: \c func_80011454 (0x80011454).
s32 actorInitializeVerticalScalePulse(u8 *actor) {
    u8 *obj;

    obj = *(u8 **)(actor + 0x54);
    *(f32 *)(obj + 0x1F8) = 0.0f;
    *(f32 *)(actor + 4) = *(f32 *)(actor + 0);
    return 0;
}

/// Relative body-scale amplitude of the single vertical pulse.
/// \note Original identifier: \c D_800A25D8 (0x800A25D8).
extern f32 gActorVerticalPulseAmplitude;

/// Animate one vertical body-scale pulse and restore the base scale when complete.
/// \param actor Actor whose vertical scale pulse has been initialized with actorInitializeVerticalScalePulse.
/// \returns One when the pulse has completed; otherwise zero.
/// \note Original decompilation identifier: \c func_80011470 (0x80011470).
s32 actorUpdateVerticalScalePulse(u8 *actor) {
    u8 *obj;
    f32 t;

    obj = *(u8 **)((u8 *)((f32 *)actor) + 0x54);
    t = *(f32 *)(obj + 0x1F8);
    if (360.0f <= t) {
        ((f32 *)actor)[1] = ((f32 *)actor)[0];
        *(f32 *)(obj + 0x1F8) = 0.0f;
        return 1;
    }
    ((f32 *)actor)[1] =
        (1.0f + gActorVerticalPulseAmplitude * (mathSinDegrees(*(f32 *)(obj + 0x1F8) + 210.0f) - -0.5f)) *
        ((f32 *)actor)[0];
    *(f32 *)(obj + 0x1F8) = *(f32 *)(obj + 0x1F8) + 40.0f;
    return 0;
}

/// Relative body-scale amplitude of the first damped pulse.
/// \note Original identifier: \c D_800A25DC (0x800A25DC).
extern f32 gActorDampedPulseInitialAmplitude;
/// Relative body-scale amplitude of the second damped pulse.
/// \note Original identifier: \c D_800A25E8 (0x800A25E8).
extern f32 gActorDampedPulseSecondAmplitude;
/// Phase threshold in degrees selecting the second damped pulse amplitude.
/// \note Original identifier: \c D_800A25E0 (0x800A25E0).
extern f64 gActorDampedPulseSecondPhase;
/// Animate a diminishing sequence of vertical body-scale pulses and restore the base scale when complete.
/// \param object Actor whose vertical scale animation has been initialized with actorInitializeVerticalScalePulse.
/// \returns One when the pulse sequence has completed; otherwise zero.
/// \note Original decompilation identifier: \c func_80011528 (0x80011528).
s32 actorUpdateDampedVerticalScalePulses(u8 *object) {
    u8 *state;
    f32 angle;
    f32 sampleAngle;
    f32 amplitude;
    f64 phase;
    state = *(u8 **)((u8 *)((f32 *)object) + 0x54);
    amplitude = gActorDampedPulseInitialAmplitude;
    if (*(f32 *)(state + 0x1F8) >= 1080.0f) {
        ((f32 *)object)[1] = ((f32 *)object)[0];
        *(f32 *)(state + 0x1F8) = 0.0f;
        return 1;
    }
    angle = *(f32 *)(state + 0x1F8);
    phase = angle;
    sampleAngle = angle;
    if (gActorDampedPulseSecondPhase <= phase)
        amplitude = gActorDampedPulseSecondAmplitude;
    if (719.0 < phase)
        amplitude = 0.02f;
    ((f32 *)object)[1] = (1.0f + amplitude * (mathSinDegrees(sampleAngle + 210.0f) - -0.5f)) * ((f32 *)object)[0];
    *(f32 *)(state + 0x1F8) = *(f32 *)(state + 0x1F8) + 40.0f;
    return 0;
}

/// Advance an actor with rhythmic body deformation and slight lateral variation.
/// \param object Actor currently being updated, with a nonzero base scale.
/// \param speed Forward movement speed and oscillation-rate control; resulting phase magnitudes must permit normalization by 360-degree steps.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_8001162C (0x8001162C).
s32 actorAdvanceWithScaleOscillation(f32 *object, f32 speed) {
    f32 *axes = object;
    f32 direction[3];
    f32 amplitude;
    f32 phase;
    f32 scale;
    f32 baseScale;
    amplitude = gRuntimeFrameSeconds * speed;
    phase = (f32)(u64)runtimeGetElapsedMilliseconds() * 360.0f * 0.001f * (speed * 0.01f);
    while (phase < 0.0f)
        phase += 360.0f;
    while (phase >= 360.0f)
        phase -= 360.0f;
    scale = mathSinDegrees(phase);
    axes[1] = (1.0 + 0.03 * (f64)scale) * (f64)axes[0];
    scale = mathSinDegrees(phase);
    baseScale = axes[0];
    scale = (f64)baseScale * (1.0 + (0.1 * (f64)scale));
    axes[2] = scale;
    amplitude *= scale / baseScale;
    direction[0] = (f32)((s32)randomBelow(10) - 4);
    direction[2] = amplitude;
    direction[1] = 0.0f;
    objectAddPendingLocalMovement(direction);
    return 0;
}

/// Apply the slow forward movement behavior with rhythmic body deformation.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_800117B4 (0x800117B4).
s32 actorAdvanceWithSlowScaleOscillation(u8 *actor) {
    return actorAdvanceWithScaleOscillation((f32 *)actor, 150.0f);
}

/// Approach the tracked target slowly with rhythmic body deformation until arrival or a movement constraint.
/// \pre The actor must be the current object update with a nonzero base scale and initialized tracking state.
/// \param actor Actor executing the approach behavior.
/// \returns One when the target is in range or movement is constrained; otherwise zero.
///
/// \note Original decompilation identifier: \c func_800117D4 (0x800117D4).
s32 actorApproachTargetWithSlowScaleOscillation(u8 *actor) {
    s32 pad[3]; /* unused, sizes the frame */
    u8 *obj;
    f32 v[3];

    obj = *(u8 **)(actor + 0x54);
    actorRefreshTrackingTargetPosition((u8 *)actor);
    actorTurnTowardTarget((u8 *)actor, 300.0f);
    v[0] = *(f32 *)(obj + 0x1EC) - *(f32 *)(actor + 0x44);
    v[1] = 0.0f;
    v[2] = *(f32 *)(obj + 0x1F4) - *(f32 *)(actor + 0x4C);
    if ((vector3Length(v) < *(f32 *)(obj + 0x1A4) + *(f32 *)(obj + 0x204)) || (*(u16 *)(obj + 0x1EA) != 0)) {
        return 1;
    }
    return actorAdvanceWithSlowScaleOscillation(actor);
}

/// Apply the medium-speed forward movement behavior with rhythmic body deformation.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_80011878 (0x80011878).
s32 actorAdvanceWithMediumScaleOscillation(u8 *actor) {
    return actorAdvanceWithScaleOscillation((f32 *)actor, 300.0f);
}

/// Approach the tracked target at medium speed with rhythmic body deformation until arrival or a movement constraint.
/// \pre The actor must be the current object update with a nonzero base scale and initialized tracking state.
/// \param actor Actor executing the approach behavior.
/// \returns One when the target is in range or movement is constrained; otherwise zero.
///
/// \note Original decompilation identifier: \c func_80011898 (0x80011898).
s32 actorApproachTargetWithMediumScaleOscillation(u8 *actor) {
    s32 pad[3]; /* unused, sizes the frame */
    u8 *obj;
    f32 v[3];

    obj = *(u8 **)(actor + 0x54);
    actorRefreshTrackingTargetPosition((u8 *)actor);
    actorTurnTowardTarget((u8 *)actor, 300.0f);
    v[0] = *(f32 *)(obj + 0x1EC) - *(f32 *)(actor + 0x44);
    v[1] = 0.0f;
    v[2] = *(f32 *)(obj + 0x1F4) - *(f32 *)(actor + 0x4C);
    if ((vector3Length(v) < *(f32 *)(obj + 0x204)) || (*(u16 *)(obj + 0x1EA) != 0)) {
        return 1;
    }
    return actorAdvanceWithMediumScaleOscillation(actor);
}

/// Apply the fast forward movement behavior with rhythmic body deformation.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_80011934 (0x80011934).
s32 actorAdvanceWithFastScaleOscillation(u8 *actor) {
    return actorAdvanceWithScaleOscillation((f32 *)actor, 900.0f);
}

/// Restore uniform actor scale after body deformation.
/// \param actor Actor whose vertical and depth scales are restored from its base scale.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80011954 (0x80011954).
s32 actorRestoreUniformScale(u8 *actor) {
    f32 uniformScale;

    uniformScale = ((RuntimeObjectPrefix *)actor)->scale[0];
    ((RuntimeObjectPrefix *)actor)->scale[1] = uniformScale;
    ((RuntimeObjectPrefix *)actor)->scale[2] = uniformScale;
    return 0;
}

/// Check whether the tracked target is within approach range or movement is constrained.
/// \param actor Actor whose target and movement constraints are inspected.
/// \returns One when the target is in range or movement constraint flags are active; otherwise zero.
/// \note Original decompilation identifier: \c func_80011968 (0x80011968).
s32 actorCheckTrackedTargetArrival(u8 *actor) {
    u8 *obj;
    f32 v[3];

    obj = *(u8 **)(actor + 0x54);
    actorRefreshTrackingTargetPosition((u8 *)actor);
    v[0] = *(f32 *)(obj + 0x1EC) - *(f32 *)(actor + 0x44);
    v[1] = 0.0f;
    v[2] = *(f32 *)(obj + 0x1F4) - *(f32 *)(actor + 0x4C);
    if (vector3Length(v) < *(f32 *)(obj + 0x1A4) + *(f32 *)(obj + 0x204)) {
        return 1;
    }
    if (*(u16 *)(obj + 0x1EA) != 0) {
        return 1;
    }
    return 0;
}

/// Track and face the target while applying forward movement.
/// \param actor Actor executing the movement behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80011A04 (0x80011A04).
s32 actorAdvanceWhileTrackingTarget(u8 *actor) {
    u8 *obj;
    f32 out[3];
    f32 dir[3];

    obj = *(u8 **)(actor + 0x54);
    actorRefreshTrackingTargetPosition((u8 *)actor);
    actorTurnTowardTarget((u8 *)actor, 300.0f);
    dir[0] = *(f32 *)(obj + 0x1EC) - *(f32 *)(actor + 0x44);
    dir[1] = 0.0f;
    dir[2] = *(f32 *)(obj + 0x1F4) - *(f32 *)(actor + 0x4C);
    out[1] = 0.0f;
    out[0] = 0.0f;
    out[2] = gRuntimeFrameSeconds * 500.0f;
    objectAddPendingLocalMovement(out);
    return 0;
}

void matrixFixedTo34(Mtx, f32 *);
void matrix33TransformVector(f32 *, f32 *, f32 *);

/// Position and orient an attached chain and adjust its pose for the target height.
/// \param object Chain actor with initialized state and a valid chain ending at its designated end part; the primary actor attachment and its transform matrices must exist.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80011A94 (0x80011A94).
s32 actorUpdateAttachedChainPose(u8 *object) {
    u8 *state;
    u8 *attachment;
    u8 *target;
    f32 matrix[12];
    f32 direction[3];
    f32 pitch;
    f32 yaw;
    u8 *end;
    u8 *part;
    u8 *next;
    u8 *partState;
    u8 *nextState;
    f32 distance;
    s32 buffer;

    state = *(u8 **)(object + 0x54);
    attachment = actorFindPart(((u8 *)gPikachuActor), 0x35);
    *(f32 *)(object + 0x18) = *(f32 *)(attachment + 0x44);
    *(f32 *)(object + 0x1C) = *(f32 *)(attachment + 0x48);
    *(f32 *)(object + 0x20) = *(f32 *)(attachment + 0x4C);
    if (*(u16 *)(attachment + 0x36) & 0x8000)
        buffer = 1;
    else
        buffer = 0;
    matrixFixedTo34(**(Mtx **)(attachment + 0x3C + ((buffer ^ 1) * 4)), matrix);
    direction[0] = 0.0f;
    direction[1] = 0.0f;
    direction[2] = -100.0f;
    matrix33TransformVector(matrix, direction, direction);
    vector3GetLookAnglesDegrees(direction, &pitch, &yaw);
    actorAddRotationAxisDegrees(object, pitch - *(f32 *)(state + 0xC), 0);
    actorAddRotationAxisDegrees(object, yaw - *(f32 *)(state + 0x10) + 90.0f, 1);
    target = actorResolveStoredTarget(object);
    if (target != NULL) {
        end = actorFindPart(object, 0x73);
        part = *(u8 **)(*(u8 **)(object + 0x2C) + 0x2C);
        while (part != end) {
            next = *(u8 **)(part + 0x2C);
            partState = *(u8 **)(part + 0x54);
            nextState = *(u8 **)(next + 0x54);
            *(f32 *)(partState + 4) = 0.0f;
            *(f32 *)(part + 0x14) = *(f32 *)(next + 0x14) + *(f32 *)partState - *(f32 *)nextState;
            part = next;
        }
        distance = fabsf(*(f32 *)(target + 0x48) - *(f32 *)(end + 0x48));
        if (*(f32 *)(state + 0x1A4) < distance) {
            *(f32 *)(end + 0x14) = mathMapRangeClamped(*(f32 *)(state + 0x1A4), 0, 20.0f, -90.0f, distance);
        }
    }
    return 0;
}
