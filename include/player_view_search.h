/// \file
/// Declares live-item, actor and object searches relative to the player viewing direction.

#ifndef PLAYER_VIEW_SEARCH_H
#define PLAYER_VIEW_SEARCH_H

#include "ultra64.h"

/// Find an item in the player viewing cone using the requested filters.
/// \param minKind Inclusive minimum item kind; 165 allows the first item kind.
/// \param maxKind Inclusive maximum item kind; 165 allows the last item kind.
/// \param distance Inclusive maximum approximate distance from the search origin.
/// \param cone Maximum absolute horizontal angle in degrees; 180 or greater permits all directions.
/// \param filter Required item state, or six to accept any state.
/// \param weighted Nonzero to prefer items with greater selection priority and reset the selected item's priority.
/// \returns The best qualifying item handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_801184C0 (0x801184C0).
s32 itemFindHandleFromPlayerView(s32 minKind, s32 maxKind, f32 distance, f32 cone, s32 filter, s32 weighted);

/// Find the nearest actor in the player viewing cone, honoring script actor-kind aliases.
/// \param object Object excluded from the search, or null.
/// \param minKind Inclusive minimum kind; 38 selects the first creature kind.
/// \param maxKind Inclusive maximum kind; 38 selects Pikachu.
/// \param distance Exclusive maximum approximate distance from the search origin.
/// \param cone Maximum horizontal angle in degrees; 360 or greater permits all directions.
/// \returns The nearest qualifying actor handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_80118544 (0x80118544).
s32 actorFindNearestFromPlayerView(u8 *object, s32 minKind, s32 maxKind, f32 distance, f32 cone);

/// Find the nearest object in the player viewing cone.
/// \param self Object excluded from the search, or null.
/// \param minKind Inclusive minimum kind.
/// \param maxKind Inclusive maximum kind.
/// \param distance Exclusive maximum approximate distance from the search origin.
/// \param cone Maximum horizontal angle in degrees; 360 or greater permits all directions.
/// \returns The nearest qualifying object handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_801185C4 (0x801185C4).
s32 objectFindNearestFromPlayerView(u8 *self, s32 minKind, s32 maxKind, f32 distance, f32 cone);

#endif
