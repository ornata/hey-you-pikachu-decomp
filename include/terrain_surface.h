/// \file
/// Declares current-area terrain material queries and borrowed node-cache contracts.

#ifndef TERRAIN_SURFACE_H
#define TERRAIN_SURFACE_H

#include "ultra64.h"

/// Query terrain material and surface attributes at a world position.
/// \param x World X coordinate.
/// \param y World Y coordinate used to select the terrain region.
/// \param z World Z coordinate.
/// \param cachedNode Optional writable terrain-node cache; initialize to null or a live node in the current
/// area. Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns The terrain material and attribute flags; 0x77 when no terrain is found, 0x170 for a maze block,
/// 0x8007 when a triangle surface cannot be resolved, or 0x4000 for terrain without supported attributes.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
/// \note Original decompilation identifier: \c func_8010BD64 (0x8010BD64).
s32 terrainGetSurfaceAttributes(f32 x, f32 y, f32 z, u8 **cachedNode);

#endif
