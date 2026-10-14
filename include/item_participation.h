/// \file
/// Declares item release, launch and search/collision participation contracts.

#ifndef ITEM_PARTICIPATION_H
#define ITEM_PARTICIPATION_H

#include "ultra64.h"

/// Release an item from its current holder when its lifecycle permits release.
/// \param object Live initialized item with valid hold state and item-kind callbacks.
/// \param force Nonzero bypasses the holder's release restriction; suspended items and toolbox-stored items cannot be released.
/// \returns One if the item is free after release, or zero if release is refused.
///
/// \note Portability: the release callback receives the former holder address as a 32-bit integer, or zero
/// when no live holder is found.
/// \note Original decompilation identifier: \c func_80114044 (0x80114044).
s32 itemTryRelease(u8 *object, s32 force);

/// Launch a live item with collision-aware starting placement and the requested motion.
/// \param id Handle of a live item that permits release; requires the active primary actor.
/// \param origin Readable three-component world position from which starting placement is checked.
/// \param target Requested starting world position in X, Y, Z order; nearby obstacles may limit placement.
/// \param velocity Readable world-space launch velocity in X, Y, Z order, scaled by the item kind's launch
/// speed.
/// \param effect Nonzero requests the throw sound effect.
/// \returns Nothing.
///
/// \note Requires initialized terrain and valid item-kind resources. Launch vectors must remain readable
/// throughout the call and must not overlap item fields modified before they are consumed.
/// \note Original decompilation identifier: \c func_8011418C (0x8011418C).
void itemThrow(s32 id, f32 *origin, f32 *target, f32 *velocity, s32 effect);

/// Exclude a live item from item searches and world collision queries.
/// \param objectId Handle of a live item to exclude.
/// \returns Nothing.
///
/// \note Requires an initialized registry and collision system; the handle must resolve to an initialized
/// item.
/// \note Original decompilation identifier: \c func_80113FC8 (0x80113FC8).
void itemRemoveFromSearchAndCollision(s32 objectId);

/// Make a live item available to item searches.
/// \param objectId Handle of a live item to register.
/// \returns Nothing.
///
/// \note Requires an initialized registry and an item whose embedded search node is not already linked.
/// \note Original decompilation identifier: \c func_8011400C (0x8011400C).
void itemRegisterForSearch(s32 objectId);

/// Hide an item and suspend its participation in the world when its lifecycle permits it.
/// \param objectId Item handle; a missing item is ignored.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80114C00 (0x80114C00).
void itemHideAndSuspend(s32 objectId);

/// Restore an item to visible, unheld participation in the world.
/// \param objectId Item handle; a missing item is ignored.
/// \returns Nothing.
///
/// \note Use for an initialized suspended item whose former holder relationship has already been cleared.
/// \note Original decompilation identifier: \c func_80114C80 (0x80114C80).
void itemRestoreToWorld(s32 objectId);

#endif
