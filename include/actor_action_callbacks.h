/// \file
/// Declares per-frame actor action behaviors and their motion-completion callback contract.

#ifndef ACTOR_ACTION_CALLBACKS_H
#define ACTOR_ACTION_CALLBACKS_H

#include "ultra64.h"

/// Advance an actor's current action behavior for one frame.
/// \param actor Actor with initialized action and behavior state.
/// \returns One to end motion immediately, zero to allow normal timing and looping, or another nonzero value to suppress looping when motion ends.
typedef s32 (*ActorActionUpdateCallback)(u8 *actor);

/// Complete an actor action immediately.
/// \param actor Unused actor parameter retained by the action callback interface.
/// \returns Always one, indicating completion.
/// \note Original decompilation identifier: \c func_8000F7B0 (0x8000F7B0).
s32 actorActionCompleteImmediately(u8 *actor);

/// Apply the standard forward approach behavior with minimum ground clearance.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_800112FC (0x800112FC).
s32 actorAdvanceAboveGround(u8 *actor);

/// Apply the faster forward approach behavior with minimum ground clearance.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_8001131C (0x8001131C).
s32 actorAdvanceAboveGroundQuickly(u8 *actor);

/// Apply faster forward movement with slight lateral variation.
/// \pre The actor must be the current object update; current area size settings must have a nonzero reference scale.
/// \param actor Actor executing the movement behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80010530 (0x80010530).
s32 actorAdvanceQuicklyWithLateralVariation(u8 *actor);

/// Track and advance toward the target until close enough or movement is constrained.
/// \param actor Actor executing the movement behavior.
/// \returns One when the target proximity threshold is reached or movement constraint flags are active; otherwise zero.
/// \note Original decompilation identifier: \c func_800103B4 (0x800103B4).
s32 actorAdvanceToTarget(u8 *actor);

/// Track and face the target while applying forward movement.
/// \param actor Actor executing the movement behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80011A04 (0x80011A04).
s32 actorAdvanceWhileTrackingTarget(u8 *actor);

/// Apply the fast forward movement behavior with rhythmic body deformation.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_80011934 (0x80011934).
s32 actorAdvanceWithFastScaleOscillation(u8 *actor);

/// Apply forward movement with slight lateral variation.
/// \pre The actor must be the current object update; current area size settings must have a nonzero reference scale.
/// \param actor Actor executing the movement behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80010480 (0x80010480).
s32 actorAdvanceWithLateralVariation(u8 *actor);

/// Apply the medium-speed forward movement behavior with rhythmic body deformation.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_80011878 (0x80011878).
s32 actorAdvanceWithMediumScaleOscillation(u8 *actor);

/// Apply the slow forward movement behavior with rhythmic body deformation.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_800117B4 (0x800117B4).
s32 actorAdvanceWithSlowScaleOscillation(u8 *actor);

/// Move toward the target height while rotating horizontally.
/// \pre The actor must be the current object update with a nonzero vertical scale and initialized target position.
/// \param actor Actor executing the behavior.
/// \returns One when the height error is within tolerance; otherwise zero.
/// \note Original decompilation identifier: \c func_80010940 (0x80010940).
s32 actorApproachTargetHeightWhileSpinning(u8 *actor);

/// Approach the tracked target with horizontal turning and vertical movement.
/// \param object Actor with initialized tracking and action state and nonzero vertical and depth scales.
/// \returns One when the target is within the actor arrival radius; otherwise zero.
///
/// \note NONPORTABLE: IDO schedules the vertical sign and speed by source line.
/// Maze bounds retain byte views because direct members change generated code.
/// \note Original decompilation identifier: \c func_80010BA0 (0x80010BA0).
s32 actorApproachTargetInThreeDimensions(u8 *object);

/// Approach the tracked target at medium speed with rhythmic body deformation until arrival or a movement constraint.
/// \pre The actor must be the current object update with a nonzero base scale and initialized tracking state.
/// \param actor Actor executing the approach behavior.
/// \returns One when the target is in range or movement is constrained; otherwise zero.
///
/// \note Original decompilation identifier: \c func_80011898 (0x80011898).
s32 actorApproachTargetWithMediumScaleOscillation(u8 *actor);

/// Approach the tracked target slowly with rhythmic body deformation until arrival or a movement constraint.
/// \pre The actor must be the current object update with a nonzero base scale and initialized tracking state.
/// \param actor Actor executing the approach behavior.
/// \returns One when the target is in range or movement is constrained; otherwise zero.
///
/// \note Original decompilation identifier: \c func_800117D4 (0x800117D4).
s32 actorApproachTargetWithSlowScaleOscillation(u8 *actor);

/// Approach the tracked target horizontally while keeping the destination inside scene bounds.
/// \param object Actor with initialized tracking and action state and nonzero depth scale; current scene bounds must be initialized.
/// \returns One when the bounded destination is within the actor arrival radius; otherwise zero.
///
/// \note NONPORTABLE: IDO schedules these vector stores by source line.
/// \note Original decompilation identifier: \c func_80010F2C (0x80010F2C).
s32 actorApproachTargetWithinSceneBounds(u8 *object);

/// Apply the fast backward-step behavior while facing the tracked target.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_8000FC98 (0x8000FC98).
s32 actorBackstepQuickly(u8 *actor);

/// Apply the slow backward-step behavior while facing the tracked target.
/// \param actor Actor executing the movement behavior.
/// \returns The movement result: one ends the current motion, zero leaves it active.
/// \note Original decompilation identifier: \c func_8000FC78 (0x8000FC78).
s32 actorBackstepSlowly(u8 *actor);

/// Finish the current action on landing or choose its constrained-movement continuation.
/// \param actor Actor whose airborne state and movement constraints are inspected.
/// \returns One when grounded or movement constraints select a continuation; otherwise zero.
/// \note Original decompilation identifier: \c func_80010234 (0x80010234).
s32 actorFinishOnLandingOrMovementConstraint(u8 *actor);

/// Maintain a fixed ground clearance while facing the tracked target.
/// \param actor Actor whose height and facing are updated.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_800113E8 (0x800113E8).
s32 actorFollowGroundAndFaceTarget(u8 *actor);

/// Pivot toward the target and request a loop exit when aligned.
/// \param actor Actor executing the turning behavior.
/// \returns Always zero; the loop-exit request controls action completion.
/// \note Original decompilation identifier: \c func_8000FDF0 (0x8000FDF0).
s32 actorPivotToTargetAndExitLoop(u8 *actor);

/// Apply the actor movement scale to its pending local movement.
/// \pre The actor must be the current object update with initialized action state.
/// \param actor Actor executing the movement behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_8000FB24 (0x8000FB24).
s32 actorScalePendingMovement(u8 *actor);

/// Apply a horizontal spin during an actor action.
/// \param actor Actor executing the behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_800108E0 (0x800108E0).
s32 actorSpinYaw(u8 *actor);

/// Track and turn toward the target while applying the actor movement scale.
/// \pre The actor must be the current object update with initialized tracking and action state.
/// \param actor Actor executing the movement behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_8000FA84 (0x8000FA84).
s32 actorTrackTargetAndScaleMovement(u8 *actor);

/// Track and face the target until it is within the actor approach range.
/// \param actor Actor executing the behavior.
/// \returns One when the target is within the approach range; otherwise zero.
/// \note Original decompilation identifier: \c func_80010B10 (0x80010B10).
s32 actorTrackTargetUntilInRange(u8 *actor);

/// Advance the actor while tumbling and rising.
/// \param actor Actor executing the behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80010628 (0x80010628).
s32 actorTumbleForward(u8 *actor);

/// Approach the tracked target while tumbling and rising.
/// \param actor Actor executing the behavior.
/// \returns One when the target is within the approach range; otherwise zero.
/// \note Original decompilation identifier: \c func_800106E4 (0x800106E4).
s32 actorTumbleTowardTarget(u8 *actor);

/// Turn toward the tracked target and request a loop exit when aligned.
/// \param actor Actor executing the turning behavior.
/// \returns Always zero; the loop-exit request controls action completion.
/// \note Original decompilation identifier: \c func_8000FDB0 (0x8000FDB0).
s32 actorTurnToTrackedTargetAndExitLoop(u8 *actor);

/// Turn quickly toward the tracked target until aligned.
/// \param actor Actor executing the turning behavior.
/// \returns One when aligned with the constrained target heading; otherwise zero.
/// \note Original decompilation identifier: \c func_80010314 (0x80010314).
s32 actorTurnToTrackedTargetQuickly(u8 *actor);

/// Turn slowly toward the tracked target until aligned.
/// \param actor Actor executing the turning behavior.
/// \returns One when aligned with the constrained target heading; otherwise zero.
/// \note Original decompilation identifier: \c func_800102DC (0x800102DC).
s32 actorTurnToTrackedTargetSlowly(u8 *actor);

/// Pivot toward the target at a speed selected for the current actor state.
/// \param actor Actor executing the turning behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_800100AC (0x800100AC).
s32 actorUpdateAdaptiveTargetPivot(u8 *actor);

/// Track and turn toward the target at a speed selected for the current actor state.
/// \param actor Actor executing the turning behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_8000FF8C (0x8000FF8C).
s32 actorUpdateAdaptiveTargetTurn(u8 *actor);

/// Apply the action vertical movement while airborne and finish on landing.
/// \param actor Actor whose airborne state controls the pending vertical movement.
/// \returns One when grounded; otherwise zero.
/// \note Original decompilation identifier: \c func_80010854 (0x80010854).
s32 actorUpdateAirborneVerticalMovement(u8 *actor);

/// Position and orient an attached chain and adjust its pose for the target height.
/// \param object Chain actor with initialized state and a valid chain ending at its designated end part; the primary actor attachment and its transform matrices must exist.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80011A94 (0x80011A94).
s32 actorUpdateAttachedChainPose(u8 *object);

/// Animate a diminishing sequence of vertical body-scale pulses and restore the base scale when complete.
/// \param object Actor whose vertical scale animation has been initialized with actorInitializeVerticalScalePulse.
/// \returns One when the pulse sequence has completed; otherwise zero.
/// \note Original decompilation identifier: \c func_80011528 (0x80011528).
s32 actorUpdateDampedVerticalScalePulses(u8 *object);

/// Apply the timed spinning behavior with its brief vertical lift.
/// \param actor Actor executing the spinning behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_8000F7BC (0x8000F7BC).
s32 actorUpdateTimedGroundSpin(u8 *actor);

/// Update the action behavior that turns an actor away from the player position.
/// \param actor Actor executing the turn-away behavior.
/// \returns Always zero, leaving action completion to its normal timing.
/// \note Original decompilation identifier: \c func_80010210 (0x80010210).
s32 actorUpdateTurnAwayFromPlayer(u8 *actor);

/// Animate one vertical body-scale pulse and restore the base scale when complete.
/// \param actor Actor whose vertical scale pulse has been initialized with actorInitializeVerticalScalePulse.
/// \returns One when the pulse has completed; otherwise zero.
/// \note Original decompilation identifier: \c func_80011470 (0x80011470).
s32 actorUpdateVerticalScalePulse(u8 *actor);

/// Wait for the actor to reach the ground.
/// \param actor Actor whose airborne state is inspected.
/// \returns One when grounded; otherwise zero.
/// \note Original decompilation identifier: \c func_80010284 (0x80010284).
s32 actorWaitUntilGrounded(u8 *actor);

#endif
