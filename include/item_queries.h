/// \file
/// Declares item-registry capacity, classification and effective-motion queries.

#ifndef ITEM_QUERIES_H
#define ITEM_QUERIES_H

#include "ultra64.h"

/// Read the remaining capacity for live items.
/// \returns Sixty minus the live item count; negative if callers have exceeded the nominal capacity.
/// \note Original decompilation identifier: \c func_80114FC0 (0x80114FC0).
s32 itemGetAvailableSlots(void);

/// Check whether an item kind belongs to a configured category.
/// \param kind Item kind identifier to test.
/// \param category Category identifier from zero through eighteen.
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

#endif
