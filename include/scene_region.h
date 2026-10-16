/// \file
/// Declares rectangular scene regions and their sampled occupancy highlights.
#ifndef SCENE_REGION_H
#define SCENE_REGION_H
#include <ultra64.h>

/// Create and register a rectangular scene region.
/// \param position Three world-space center coordinates, copied during creation.
/// \param width Region half-extent along X.
/// \param depth Region half-extent along Z.
/// \param points Borrowed world-coordinate triples, valid for the region lifetime,
/// ending with a complete three-coordinate marker whose X is negative.
/// \returns Nothing; the object system owns the registered region.
void sceneRegionCreate(f32 *position, f32 width, f32 depth, f32 *points);

/// Read a scene region's stored player highlight.
/// \param handle Handle of an initialized scene region; other kinds are unsupported.
/// \returns Stored unsigned highlight byte, or zero when the handle is not live.
/// \note A normal region update enables highlighting when the player is strictly
/// inside its horizontal bounds and at or above its height, unless Pikachu also
/// qualifies. Reading the byte does not update occupancy or extend object lifetime.
u8 sceneRegionIsHighlightedByHandle(s32 handle);
#endif
