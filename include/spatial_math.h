/// \file
/// Declares angular interpolation and vector and matrix transformations.

#ifndef SPATIAL_MATH_H
#define SPATIAL_MATH_H

#include <ultra64.h>

/// Get the game-convention Euler orientation of a direction.
/// \param vector Input three-component direction.
/// \param roll Receives roll in degrees; zero for a near-zero vector.
/// \param pitch Receives pitch in degrees; zero for a near-zero vector.
/// \param yaw Receives yaw in degrees; zero for a near-zero vector.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003BD28 (0x8003BD28).
void vector3GetEulerDegrees(f32 *vector, f32 *roll, f32 *pitch, f32 *yaw);

/// Measure the shortest separation between two degree angles.
/// \param from First finite angle in degrees.
/// \param to Second finite angle in degrees.
/// \returns Angular distance from 0 through 180 degrees.
/// \note Original decompilation identifier: \c func_8003B648 (0x8003B648).
f32 mathAngleDistanceDegrees(f32 from, f32 to);

/// Interpolate between two angles along the shortest arc.
/// \param from Starting finite angle in degrees.
/// \param to Target finite angle in degrees.
/// \param t Interpolation fraction; 0 retains from and 1 reaches to.
/// \returns Interpolated angle in [0, 360) degrees.
/// \note Original decompilation identifier: \c func_8003C174 (0x8003C174).
f32 mathLerpAngleDegrees(f32 from, f32 to, f32 t);

/// Apply scale factors along the local axes.
/// \param matrix Matrix whose linear transform is to be updated.
/// \param xScale Scale along the local X axis.
/// \param yScale Scale along the local Y axis.
/// \param zScale Scale along the local Z axis.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039904 (0x80039904).
void matrix33ScaleLocal(f32 *matrix, f32 xScale, f32 yScale, f32 zScale);

/// Replace a linear transform with its nonnegative axis scales.
/// \param m 3x3 matrix to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003A094 (0x8003A094).
void matrix33KeepScaleOnly(f32 *m);

/// Get the game-convention orientation angles of a direction relative to the Z axis.
/// \param vector Input three-component direction.
/// \param yaw Receives the first orientation angle in degrees; zero for a near-zero vector.
/// \param pitch Receives the second orientation angle in degrees; zero for a near-zero vector.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003BBA8 (0x8003BBA8).
void vector3GetZAxisAnglesDegrees(f32 *vector, f32 *yaw, f32 *pitch);

/// Transform a three-dimensional point.
/// \param mf Input affine matrix.
/// \param x Input point X coordinate.
/// \param y Input point Y coordinate.
/// \param z Input point Z coordinate.
/// \param ox Receives the transformed X coordinate.
/// \param oy Receives the transformed Y coordinate.
/// \param oz Receives the transformed Z coordinate.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003953C (0x8003953C).
void matrix34TransformPoint(f32 mf[4][3], f32 x, f32 y, f32 z, f32 *ox, f32 *oy, f32 *oz);

/// Approximate the length of a planar vector.
/// \param a First vector component.
/// \param b Second vector component.
/// \returns Approximate vector length.
/// \note Original decompilation identifier: \c func_8003A6F0 (0x8003A6F0).
f32 mathHypotApprox(f32 a, f32 b);

/// Determine the sign of the cosine of a degree angle.
/// \param angle Finite angle in degrees.
/// \returns -1 for negative cosine, 0 at a quarter or three-quarter turn, or 1 otherwise.
/// \note Original decompilation identifier: \c func_8003B578 (0x8003B578).
s32 mathCosSignDegrees(f32 angle);

#endif
