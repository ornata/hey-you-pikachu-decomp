/// \file
/// Declares constrained nearest-item selection over the live item registry.

#ifndef ITEM_SEARCH_H
#define ITEM_SEARCH_H

#include "ultra64.h"

/// Find the best nearby item satisfying the requested kind, hold-state, and viewing constraints.
/// \param origin Readable query world position in X, Y, Z order; copied during this call.
/// \param maxDistance Nonnegative maximum approximate world-space distance from the origin.
/// \param pitch Viewing pitch in degrees.
/// \param yaw Viewing yaw in degrees.
/// \param cone Maximum absolute viewing yaw offset in degrees; values of at least 180 allow every direction.
/// \param minKind Inclusive lower item kind; 165 selects the default lower bound of one.
/// \param maxKind Inclusive upper item kind; 165 selects the default upper bound of 164.
/// \param filter Required item hold state from zero through five, or six to accept any hold state.
/// \param weightEnabled Nonzero includes search weighting in candidate selection and resets the selected
/// item's search weight.
/// \returns Borrowed best eligible live item, or null when none qualifies; valid only while the item remains
/// live.
///
/// \note Requires an initialized registry and finite query values. Uses shared query state and must not be
/// reentered during traversal. The selected-state assignment preserves the original IDO register allocation.
/// \note Original decompilation identifier: \c func_80113130 (0x80113130).
u8 *itemFindNearest(f32 *origin, f32 maxDistance, f32 pitch, f32 yaw, f32 cone, s32 minKind, s32 maxKind, s32 filter,
                    s32 weightEnabled);

/// Find an eligible item of the requested kind near an actor's facing direction, honoring its interaction cooldown.
/// \param actor Actor supplying the reference position, facing, and cooldown.
/// \param kind Item kind to search for.
/// \param distance Inclusive maximum approximate search distance.
/// \returns The qualifying item kind, or zero during the cooldown or when no eligible item is found.
/// \note Original decompilation identifier: \c func_80117210 (0x80117210).
s16 actorFindInteractableItemKind(u8 *actor, s32 kind, f32 distance);

#endif
