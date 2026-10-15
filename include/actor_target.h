/// \file
/// Declares actor target lookup, tracking and facing operations.

#ifndef ACTOR_TARGET_H
#define ACTOR_TARGET_H

#include <ultra64.h>

/// Supported interpretations of an actor's retained target.
/// Stored and script-supplied modes retain their signed 32-bit representation.
typedef enum ActorTargetMode {
    ACTOR_TARGET_FIXED = 0,  ///< Keep the stored destination without following a target.
    ACTOR_TARGET_FOLLOW = 1, ///< Refresh the destination toward the retained object.
    ACTOR_TARGET_ESCAPE = 2  ///< Refresh the destination away from the retained object.
} ActorTargetMode;

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

/// Select the player's primary object as an actor's tracked movement target without refreshing its stored destination.
/// \param actor Actor whose tracked target and follow mode are changed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118A00 (0x80118A00).
void actorTrackPlayerPrimaryObject(u8 *actor);

/// Select the player's secondary object as an actor's tracked escape source, retaining destination and allowance.
/// \param actor Actor whose tracked target and escape mode are changed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801189E4 (0x801189E4).
void actorTrackEscapeFromPlayerSecondaryObject(u8 *actor);

/// Place the actor at its stored target position.
/// \param actor Actor to reposition.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118A94 (0x80118A94).
void actorMoveToTargetPosition(u8 *actor);

/// Measure the horizontal distance to an actor's stored target position.
/// \param actor Actor whose target distance is measured.
/// \returns The approximate horizontal distance to the target position.
/// \note Original decompilation identifier: \c func_80117434 (0x80117434).
f32 actorGetHorizontalTargetDistance(u8 *actor);

/// Get one world-space coordinate of an actor's stored target position.
/// \param actor Actor whose target position is queried.
/// \param axis Nonnegative coordinate axis: zero for X, one for Y, or two for Z.
/// \returns The coordinate, or -65536 when the axis is greater than two.
/// \note Original decompilation identifier: \c func_80117480 (0x80117480).
f32 actorGetTargetCoordinate(u8 *actor, s32 axis);

/// Track an available object as an actor's movement destination. Otherwise disable target tracking.
/// Unavailable targets retain the previous handle, destination, and allowance.
/// A zero category does not establish whether a destination was selected.
/// \param actor Actor whose destination is changed.
/// \param handle Handle of the target object.
/// \param distanceAllowance Distance allowance used when approaching the destination.
/// \returns The target script category, or zero when the target is unavailable or has no supported category.
/// \note Original decompilation identifier: \c func_80117584 (0x80117584).
s32 actorTrackObjectPosition(u8 *actor, s32 handle, f32 distanceAllowance);

/// Choose an available object's current position as an actor's destination and disable ongoing tracking.
/// \param actor Actor whose destination is changed.
/// \param handle Handle of the target object.
/// \param distanceAllowance Distance allowance used when approaching the destination.
/// \returns The target script category, or zero when the target is unavailable or has no supported category.
/// \note Original decompilation identifier: \c func_80117624 (0x80117624).
s32 actorSetDestinationFromObject(u8 *actor, s32 handle, f32 distanceAllowance);

/// Track an available object as the source of an actor's escape destination. Otherwise disable target tracking.
/// \param actor Actor whose destination is changed.
/// \param handle Handle of the target object.
/// \returns The target script category, or zero when the target is unavailable or has no supported category.
/// \note Original decompilation identifier: \c func_801176A8 (0x801176A8).
s32 actorTrackEscapeFromObject(u8 *actor, s32 handle);

/// Choose an escape destination away from an available object and disable ongoing target tracking.
/// \param actor Actor whose destination is changed.
/// \param handle Handle of the target object.
/// \returns The target script category, or zero when the target is unavailable or has no supported category.
/// \note Original decompilation identifier: \c func_80117744 (0x80117744).
s32 actorChooseEscapePositionFromHandle(u8 *actor, s32 handle);

/// Choose an escape destination away from the player and disable ongoing target tracking.
/// \param actor Actor whose escape destination is selected.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801177C8 (0x801177C8).
void actorChooseEscapePositionFromPlayer(u8 *actor);

/// Get the handle of an actor's available target.
/// \param object Actor whose target is requested.
/// \returns The target handle, or zero when no target is available.
/// \note Original decompilation identifier: \c func_801177FC (0x801177FC).
s32 actorGetAvailableTargetHandle(u8 *object);

/// Get an actor's target tracking mode.
/// \param actor Actor whose tracking mode is requested.
/// \returns The current target tracking mode.
/// \note Original decompilation identifier: \c func_80117828 (0x80117828).
s32 actorGetTargetTrackingMode(u8 *actor);

/// Set how an actor tracks its target.
/// \param actor Actor whose tracking mode is changed.
/// \param mode Target tracking mode: zero disables tracking, one follows the target, and two moves away from it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80117834 (0x80117834).
void actorSetTargetTrackingMode(u8 *actor, s32 mode);

#endif
