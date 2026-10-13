/// \file
/// Declares degree-based angle approximation, normalization and turn controls.

#ifndef MATH_ANGLES_H
#define MATH_ANGLES_H

#include "ultra64.h"

/// Approximate the sine of a degree angle.
/// \param deg Angle in degrees whose rounded value in 2048 units per turn fits a signed 32-bit integer.
/// \returns Approximate sine of the angle.
/// \note Original decompilation identifier: \c func_8003C3DC (0x8003C3DC).
f32 mathSinDegrees(f32 deg);

/// Approximate the cosine of a degree angle.
/// \param angle Angle in degrees whose value plus 90 satisfies mathSinDegrees input limits.
/// \returns Approximate cosine of the angle.
/// \note Original decompilation identifier: \c func_8003C4D0 (0x8003C4D0).
f32 mathCosDegrees(f32 angle);

/// Approximate the direction angle of a planar unit vector.
/// \param x Unit vector X component.
/// \param y Unit vector Y component.
/// \returns Direction in degrees from the positive X axis, between 0 and 360.
/// \note Original decompilation identifier: \c func_8003B318 (0x8003B318).
f32 mathUnitDirectionDegrees(f32 x, f32 y);

/// Normalize an angle to a signed single-turn range.
/// \param angle Finite angle in degrees.
/// \returns Equivalent angle in (-180, 180].
/// \note Original decompilation identifier: \c func_8003B6D4 (0x8003B6D4).
f32 mathWrapSignedDegrees(f32 angle);

/// Turn toward a target using a distance-dependent step.
/// \param from Starting finite angle in degrees.
/// \param to Target finite angle in degrees.
/// \param lo Nonnegative snap distance and minimum turn step in degrees.
/// \param stepDistance Angular separation at which the maximum turn step is reached; greater than lo.
/// \param hi Nonnegative maximum turn step in degrees.
/// \returns Updated angle in degrees; may lie outside a single-turn range.
/// \note Original decompilation identifier: \c func_8003C25C (0x8003C25C).
f32 mathApproachAngleDegrees(f32 from, f32 to, f32 lo, f32 stepDistance, f32 hi);

#endif
