/// \file
/// Declares vector geometry, heading conversion and history-aware random selection.

#ifndef VECTOR_OPERATIONS_H
#define VECTOR_OPERATIONS_H

#include <ultra64.h>

/// Construct a unit direction using Z-axis pitch and planar yaw.
/// \param pitch Pitch in degrees from the XY plane toward negative Z.
/// \param yaw Yaw in degrees from positive X toward positive Y.
/// \param direction Receives the three-component unit direction.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000CE20 (0x8000CE20).
void vector3FromZAxisPitchYawDegrees(f32 pitch, f32 yaw, f32 *direction);

/// Get the horizontal direction of a three-dimensional vector.
/// \param vector Input vector whose X and Z components determine the direction.
/// \returns Yaw in degrees from positive Z toward positive X, or zero for a near-zero horizontal vector.
/// \note Original decompilation identifier: \c func_8000CAA4 (0x8000CAA4).
f32 vector3GetYawDegrees(f32 *vector);

/// Get the horizontal direction of a point after an affine transformation.
/// \param matrix Input affine transformation matrix.
/// \param point Input three-component point.
/// \returns Yaw in degrees from positive Z toward positive X, or zero for a near-zero horizontal result.
/// \note Original decompilation identifier: \c func_8000DAB8 (0x8000DAB8).
f32 matrix34GetTransformedPointYawDegrees(f32 *matrix, f32 *point);

/// Normalize a planar vector using an approximate length.
/// \param x First component, updated in place.
/// \param y Second component, updated in place.
/// \returns The approximate original length, or zero for a near-zero vector whose components are cleared.
/// \note Original decompilation identifier: \c func_8000D5E0 (0x8000D5E0).
f32 vector2NormalizeComponentsApprox(f32 *x, f32 *y);

/// Get a direction's yaw and its pitch projected onto the X-Y plane.
/// \param direction Input three-component direction.
/// \param yaw Receives yaw in degrees from positive Z toward positive X.
/// \param pitch Receives signed projected pitch in degrees, positive downward; both outputs are zero for a zero-length direction.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000CD64 (0x8000CD64).
void vector3GetYawAndProjectedPitchDegrees(f32 *direction, f32 *yaw, f32 *pitch);

/// Rotate a planar vector in place.
/// \param angle Rotation in degrees from the first axis toward the second.
/// \param x First component, updated in place.
/// \param y Second component, updated in place.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000D660 (0x8000D660).
void vector2RotateComponentsDegrees(f32 angle, f32 *x, f32 *y);

/// Choose a random index while accounting for recent selections in small ranges.
/// \param n Positive exclusive upper bound; ranges through 32 retain selection history.
/// \returns A selected index from zero through n minus one.
/// \note Original decompilation identifier: \c func_8000DE58 (0x8000DE58).
s32 randomBelowWithHistory(s32 n);

#endif
