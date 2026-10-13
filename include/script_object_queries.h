/// \file
/// Declares player and object operations exposed to actor scripts.

#ifndef SCRIPT_OBJECT_QUERIES_H
#define SCRIPT_OBJECT_QUERIES_H

#include <ultra64.h>

/// Set the primary camera position and look-at target from script arguments.
/// \param vm Script VM containing position X, Y, Z followed by target X, Y, Z, with target Z on top of the stack.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118A1C (0x80118A1C).
void scriptVmSetPrimaryCameraView(s32 vm);

/// Find an item near an actor within its facing cone and the requested filters.
/// \param actor Actor supplying the search origin and facing.
/// \param distance Inclusive maximum approximate distance from the search origin.
/// \param cone Maximum absolute horizontal angle in degrees; 180 or greater permits all directions.
/// \param minKind Inclusive minimum item kind; 165 allows the first item kind.
/// \param maxKind Inclusive maximum item kind; 165 allows the last item kind.
/// \param filter Required item state, or six to accept any state.
/// \param weighted Nonzero to prefer items with greater selection priority and reset the selected item's priority.
/// \returns The best qualifying item handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_8011750C (0x8011750C).
s32 actorFindItemHandleInFacingCone(u8 *actor, f32 distance, f32 cone, s32 minKind, s32 maxKind, s32 filter,
                                    s32 weighted);

/// Get the bounding radius of an available object.
/// \param handle Handle of the object to query.
/// \returns The bounding radius, or zero when the handle is unavailable.
/// \note Original decompilation identifier: \c func_80117A64 (0x80117A64).
f32 objectGetBoundingRadiusByHandle(s32 handle);

/// Set the default mode for an actor's actions that inherit it.
/// \param actor Actor whose default action mode is changed.
/// \param mode Default action mode representable as a signed 16-bit value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80117A94 (0x80117A94).
void actorSetDefaultActionMode(u8 *actor, s32 mode);

/// Get the handle of the item held by the player.
/// \returns The stored held-item handle, or zero when no item is recorded.
/// \note Original decompilation identifier: \c func_80117AD0 (0x80117AD0).
s32 playerGetHeldItemHandle(void);

/// Consider an object for preferred interaction and test whether it lies in an interaction region.
/// \param object Object to evaluate as an interaction candidate.
/// \returns One when the object qualifies for either interaction region, or zero otherwise.
/// \note Original decompilation identifier: \c func_80117ADC (0x80117ADC).
s32 interactionConsiderObjectInRegion(u8 *object);

/// Select which player controls remain enabled.
/// \param mask Low sixteen input bits to retain; bits 64 and 128 also enable the horizontal and vertical analog axes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80117B14 (0x80117B14).
void playerSetInputKeepMask(s32 mask);

/// Get the player input blocked by the current control mask.
/// \returns Blocked button bits, including bits 64 and 128 for sufficiently strong blocked analog input.
/// \note Original decompilation identifier: \c func_80117B20 (0x80117B20).
u16 playerGetBlockedInput(void);

/// Request a player interaction mode targeting an actor.
/// \param actor Actor to use as the interaction target.
/// \param mode Requested mode: zero for off, one for Pikachu mode, or two for follow mode; player permissions can prevent the change.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80117B30 (0x80117B30).
void playerSetActorInteractionMode(u8 *actor, s32 mode);

/// Find an item near a world position using the requested kind and state filters.
/// \param position Three-component world-space search origin.
/// \param distance Inclusive maximum approximate distance from the search origin.
/// \param minKind Inclusive minimum item kind; 165 allows the first item kind.
/// \param maxKind Inclusive maximum item kind; 165 allows the last item kind.
/// \param filter Required item state, or six to accept any state.
/// \param weighted Nonzero to prefer items with greater selection priority and reset the selected item's priority.
/// \returns The best qualifying item handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_80117C74 (0x80117C74).
s32 itemFindHandleNearPosition(f32 *position, f32 distance, s32 minKind, s32 maxKind, s32 filter, s32 weighted);

/// Set an available object's uniform scale relative to its area scaling convention.
/// \param handle Handle of the object to resize; unavailable handles have no effect. Area-scaled objects require an initialized area with nonzero scale.
/// \param scale Requested uniform scale; the object must have a nonzero current scale.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80117DA8 (0x80117DA8).
void objectSetUniformScaleByHandle(s32 handle, f32 scale);

/// Get an available object's uniform scale relative to its area scaling convention.
/// \param handle Handle of the object to query.
/// \returns The uniform scale, or zero when the handle is unavailable.
/// \pre Area-scaled objects require an initialized area with nonzero scale.
/// \note Original decompilation identifier: \c func_80117E24 (0x80117E24).
f32 objectGetUniformScaleByHandle(s32 handle);

/// Get the active rotation speed of a voice-reactive spinner.
/// \param handle Handle of the spinner object to query.
/// \returns Rotation speed in degrees per second, or zero when the handle is unavailable, the object is not a spinner, or its speed is nonpositive.
/// \note Original decompilation identifier: \c func_80117E80 (0x80117E80).
f32 voiceReactiveSpinnerGetSpeedByHandle(s32 handle);

#endif
