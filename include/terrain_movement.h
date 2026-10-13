/// \file
/// Declares terrain-constrained displacement and downhill motion sampling.

#ifndef TERRAIN_MOVEMENT_H
#define TERRAIN_MOVEMENT_H

#include "ultra64.h"

/// Constrain a requested world displacement to movement permitted by terrain and the selected collision
/// policy.
/// \param queryKind Collision policy from zero through nine; policy five also requires the active primary
/// actor.
/// \param x Starting world X coordinate.
/// \param y Starting world Y coordinate.
/// \param z Starting world Z coordinate.
/// \param width Nonnegative horizontal clearance width.
/// \param depth Vertical span used for horizontal clearance queries.
/// \param height Permitted clearance height above the starting Y coordinate.
/// \param deltaX Writable requested X displacement, replaced by allowed movement.
/// \param deltaY Writable requested Y displacement, replaced by allowed movement.
/// \param deltaZ Writable requested Z displacement, replaced by allowed movement.
/// \param hitNode Optional writable terrain-node cache; initialize to null or a live node in the current
/// area. Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns Collision flags from the final tested movement step, or zero when no collision is reported.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
///
/// \note Displacement outputs must be distinct. Inputs and horizontal distance must be finite, with the
/// movement step count representable as a signed integer.
/// \note Original decompilation identifier: \c func_8010CAA4 (0x8010CAA4).
s32 terrainConstrainMovement(s32 queryKind, f32 x, f32 y, f32 z, f32 width, f32 depth, f32 height, f32 *deltaX,
                             f32 *deltaY, f32 *deltaZ, u8 **hitNode);

/// Measure terrain height changes toward positive X and Z for motion responses.
/// \param x World X coordinate.
/// \param y World Y coordinate used to select terrain.
/// \param z World Z coordinate.
/// \param maxSlope Nonnegative largest absolute height difference to accept; larger differences produce zero.
/// \param step Horizontal sampling distance along each positive axis.
/// \param slopeX Writable center height minus the positive-X sample height, or zero for negligible or
/// excessive change.
/// \param slopeZ Distinct writable output for center height minus the positive-Z sample height, or zero for
/// negligible or excessive change.
/// \param node Optional writable terrain-node cache; initialize to null or a live node in the current area.
/// Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns Nothing.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
/// \note Original decompilation identifier: \c func_8010CE98 (0x8010CE98).
void terrainGetDownhillHeightDifferences(f32 x, f32 y, f32 z, f32 maxSlope, f32 step, f32 *slopeX, f32 *slopeZ,
                                         u8 **node);

#endif
