/// \file
/// Declares item-registry capacity, classification and effective-motion queries.

#ifndef ITEM_QUERIES_H
#define ITEM_QUERIES_H

#include "ultra64.h"
#include "item_category.h"

/// Read the remaining capacity for live items.
/// \returns Sixty minus the live item count; negative if callers have exceeded the nominal capacity.
/// \note Original decompilation identifier: \c func_80114FC0 (0x80114FC0).
s32 itemGetAvailableSlots(void);

/// Check whether an item kind belongs to a configured category.
/// \param kind Item kind identifier to test.
/// \param category Membership category index from zero through eighteen; established names use ItemCategoryId.
/// \note Voice noun-group values use a separate domain and are not accepted as category indices.
/// \returns One if the kind belongs to the category, or zero otherwise.
/// \note Original decompilation identifier: \c func_80114FD4 (0x80114FD4).
s32 itemKindIsInCategory(s32 kind, s32 category);

/// Look up an item kind class.
/// \param kind Item kind identifier from zero through 165.
/// \returns The configured item class identifier.
/// \note Original decompilation identifier: \c func_80115020 (0x80115020).
s8 itemGetClass(s32 kind);

/// Look up the progress-field selector controlling an item kind’s availability.
/// \param kind Item kind identifier from zero through 165.
/// \returns The configured availability progress-field selector.
/// \note Original decompilation identifier: \c func_80115044 (0x80115044).
s16 itemGetAvailabilityFlag(s32 kind);

/// Obtain an item's effective coefficient for gravity and friction.
/// \param item Initialized item whose current motion properties are queried.
/// \returns The effective motion coefficient for the item's current state.
/// \note Original decompilation identifier: \c func_80114B84 (0x80114B84).
f32 itemGetMotionCoefficient(u8 *item);

/// Check whether an item's current state permits pickup by the requested holder.
/// \param item Initialized item object to query.
/// \param byPlayer Nonzero checks player pickup rules; zero checks actor pickup rules.
/// \returns One if pickup is permitted, or zero otherwise.
/// \note Original decompilation identifier: \c func_801144EC (0x801144EC).
s32 itemCanBePickedUp(u8 *item, s32 byPlayer);

/// Check whether an item permits requesting Pikachu cooperation.
/// \param item Initialized item object to query.
/// \returns One if cooperation is permitted, or zero otherwise.
/// \note Original decompilation identifier: \c func_801145A0 (0x801145A0).
s32 itemAllowsCooperation(u8 *item);

/// Check whether an item has been marked as picked up by the player.
/// \param item Initialized item whose pickup history is queried.
/// \returns One when player pickup is recorded, or zero otherwise.
/// \note Original decompilation identifier: \c func_80118910 (0x80118910).
s32 itemHasPlayerPickupHistory(u8 *item);

#endif
