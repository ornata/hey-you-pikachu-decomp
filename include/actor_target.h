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

#endif
