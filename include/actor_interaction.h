/// \file
/// Declares actor interactions with held items, target objects and appearance choices.

#ifndef ACTOR_INTERACTION_H
#define ACTOR_INTERACTION_H

#include <ultra64.h>

/// Attempt to make an actor hold the requested item, releasing its previous held item on success.
/// \param actor Actor that will hold the item.
/// \param itemId Identifier resolved to the item to hold.
/// \param forceRelease Nonzero to force release from the item current holder.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118FAC (0x80118FAC).
void actorTryHoldItem(u8 *actor, s32 itemId, s32 forceRelease);

/// Select both actor appearance variants and whether they persist across action changes.
/// \param actor Actor whose appearance is changed.
/// \param slot0Variant Model variant for appearance slot zero.
/// \param slot1Variant Model variant for appearance slot one.
/// \param preserve Nonzero to preserve the selected appearance across action changes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119210 (0x80119210).
void actorSetAppearanceVariants(u8 *actor, s32 slot0Variant, s32 slot1Variant, s32 preserve);

/// Select preset text for a scrolling board hierarchy.
/// \param handle Live object handle of the board hierarchy.
/// \param preset Text preset index, zero or one.
/// \param style Text style passed to the board.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119508 (0x80119508).
void scrollBoardSelectPresetText(s32 handle, s32 preset, s32 style);

/// Query the player heading relative to the direction toward an object.
/// \param object Object whose direction from the player is evaluated.
/// \returns Player heading minus the direction toward the object, wrapped to signed degrees.
/// \note Original decompilation identifier: \c func_80119338 (0x80119338).
f32 objectGetPlayerFacingError(u8 *object);

/// Query an object heading relative to the direction toward a resolved target.
/// \param object Object whose facing is evaluated.
/// \param targetId Identifier resolved to the target object.
/// \returns Object heading minus the target direction in degrees from 0 inclusive to 360 exclusive, or -1 when unresolved.
/// \note Original decompilation identifier: \c func_8011941C (0x8011941C).
f32 objectGetFacingErrorToTarget(u8 *object, s32 targetId);

/// Query an object heading relative to the direction toward the player.
/// \param object Object whose facing is evaluated.
/// \returns Object heading minus the direction toward the player, wrapped to signed degrees.
/// \note Original decompilation identifier: \c func_801193AC (0x801193AC).
f32 objectGetFacingErrorToPlayer(u8 *object);

/// Set the activation state of an object that supports being switched on and off.
/// \param on Zero to switch off, or nonzero to switch on.
/// \param id Identifier resolved to the object.
/// \returns The supplied on value when supported, or zero when unresolved or unsupported.
/// \note Original decompilation identifier: \c func_80119134 (0x80119134).
s32 objectSetOnState(s32 on, s32 id);

/// Query the remaining impact count of a resolved item.
/// \param id Identifier resolved to the item.
/// \returns The item impact count, or 0xFFFF0000 when the identifier does not resolve to an item.
/// \note Original decompilation identifier: \c func_8011908C (0x8011908C).
s32 itemGetImpactCount(s32 id);

/// Set the remaining impact count of a resolved item.
/// \param id Identifier resolved to the item.
/// \param count Impact count to assign.
/// \returns One when the item was updated, otherwise zero.
/// \note Original decompilation identifier: \c func_801190DC (0x801190DC).
s32 itemSetImpactCount(s32 id, s32 count);

#endif
