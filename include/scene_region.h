/// \file
/// Declares rectangular scene-region creation and player occupancy queries.
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

/// Query whether a region has its player highlight enabled.
/// \param handle Handle of an initialized scene region; other kinds are unsupported.
/// \returns Highlight flag, or zero when the handle is not live.
u8 sceneRegionIsHighlightedByHandle(s32 handle);
#endif
