/// \file
/// Declares actor target lookup, tracking and facing operations.

#ifndef ACTOR_TARGET_H
#define ACTOR_TARGET_H

#include <ultra64.h>

/// Resolve the actor's retained target handle.
/// \param actor Actor with initialized script state.
/// \returns Borrowed live target, or null if the handle is unavailable; no ownership is transferred.
u8 *actorResolveStoredTarget(u8 *actor);

/// Resolve a target when tracking is enabled.
/// \param actor Actor with initialized script state.
/// \returns Borrowed live target, or null when tracking is disabled or resolution fails.
u8 *actorGetTrackingTarget(u8 *actor);

/// Select the actor's response to messenger contact.
/// \param actor Actor whose initialized script state receives the setting.
/// \param mode Contact mode interpreted by messenger interactions.
/// \returns Nothing.
void actorSetMessengerContactMode(u8 *actor, s32 mode);

/// Turn toward the retained destination within facing constraints.
/// \param actor Actor with initialized destination and orientation state, currently being updated.
/// \param speed Angular speed control in degrees per second.
/// \returns One when the constrained heading is within one degree; otherwise zero.
s32 actorTurnTowardTarget(u8 *actor, f32 speed);

/// Pivot toward the retained destination with compensating movement.
/// \param actor Actor with initialized destination and orientation state, currently being updated.
/// \param speed Angular speed control in degrees per second.
/// \returns One when the constrained heading is within one degree; otherwise zero.
s32 actorPivotTowardTarget(u8 *actor, f32 speed);

/// Refresh a pursuit or escape destination from the tracked target.
/// \param actor Actor with initialized script state; unavailable targets leave its destination unchanged.
/// \returns Nothing.
void actorRefreshTrackingTargetPosition(u8 *actor);

/// Measure the distance from the actor's local position to its stored target position.
/// \param actor Actor whose target distance is queried.
/// \returns The approximate three-dimensional distance to the target position.
/// \note Original decompilation identifier: \c func_801187E4 (0x801187E4).
f32 actorGetTargetDistance(u8 *actor);

/// Set an actor's target position to its fallback object's current position.
/// \param actor Actor with a live fallback object whose position supplies the destination.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118890 (0x80118890).
void actorSetTargetToFallbackObject(u8 *actor);

/// Set the actor's stored target position.
/// \param actor Actor whose target position is changed.
/// \param position Three world-space coordinates to copy; the pointer is not retained.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118644 (0x80118644).
void actorSetTargetPosition(u8 *actor, f32 *position);

/// Choose a target position away from the supplied threat position.
/// \param object Actor whose escape target is changed; its fallback target handle must be live when the positions coincide.
/// \param target Three-component world-space threat position; the pointer is not retained.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011866C (0x8011866C).
void actorChooseEscapePosition(u8 *object, f32 *target);

#endif
