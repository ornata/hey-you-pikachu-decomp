/// \file
/// Declares world-height queries and borrowed current-area terrain-node caches.

#ifndef TERRAIN_HEIGHT_H
#define TERRAIN_HEIGHT_H

#include "ultra64.h"

/// Query terrain world height at a supplied position.
/// \param x World X coordinate.
/// \param y World Y coordinate used to select the terrain region.
/// \param z World Z coordinate.
/// \param previous Optional writable terrain-node cache; initialize to null or a live node in the current
/// area. Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns Terrain world height, or zero if no surface is found.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
/// \note Original decompilation identifier: \c func_8010BC40 (0x8010BC40).
f32 terrainGetWorldHeight(f32 x, f32 y, f32 z, u8 **previous);

/// Query terrain world height through the caller's terrain-query interface.
/// \param unused Unused query argument.
/// \param x World X coordinate.
/// \param y World Y coordinate used to select the terrain region.
/// \param z World Z coordinate.
/// \param previous Optional writable terrain-node cache; initialize to null or a live node in the current
/// area. Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns Terrain world height, or zero if no surface is found.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
/// \note Original decompilation identifier: \c func_8010BCC0 (0x8010BCC0).
f32 terrainGetWorldHeightForQuery(s32 unused, f32 x, f32 y, f32 z, u8 **previous);

#endif
