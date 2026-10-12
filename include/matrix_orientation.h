/// \file
/// Declares affine orientation construction and blending for actor and model transforms.

#ifndef MATRIX_ORIENTATION_H
#define MATRIX_ORIENTATION_H

#include "ultra64.h"

/// Build an affine rotation applying X, then Y, then Z rotations.
/// \param m Output affine matrix.
/// \param x Rotation about the X axis in degrees.
/// \param y Rotation about the Y axis in degrees.
/// \param z Rotation about the Z axis in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039994 (0x80039994).
void matrix34MakeRotationXYZDegrees(f32 *m, f32 x, f32 y, f32 z);

/// Blend two orientations into an affine matrix with zero translation.
/// \param w Weight of the second orientation, normally from zero through one.
/// \param a First input orientation matrix.
/// \param b Second input orientation matrix.
/// \param out Receives a twelve-component matrix with unit nondegenerate basis vectors; near-zero blended vectors are cleared.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000D918 (0x8000D918).
void matrix34BlendOrientation(f32 w, f32 *a, f32 *b, f32 *out);

/// Compose two linear transforms as b times a.
/// \param a First input 3x3 matrix.
/// \param b Second input 3x3 matrix.
/// \param c Output 3x3 matrix; may alias either input.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80038FF0 (0x80038FF0).
void matrix33Multiply(f32 *a, f32 *b, f32 *c);

/// Extract an equivalent Euler orientation with roll near zero.
/// \param matrix Input rotation matrix.
/// \param roll Receives roll in degrees, folded near zero and represented as a positive angle.
/// \param pitch Receives the corresponding pitch in degrees.
/// \param yaw Receives the corresponding yaw in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003B764 (0x8003B764).
void matrix33GetFoldedEulerDegrees(f32 *matrix, f32 *roll, f32 *pitch, f32 *yaw);

/// Apply a local Euler rotation in X, then Y, then Z order.
/// \param matrixAddress Matrix whose linear transform is updated in place.
/// \param xDegrees Rotation about the X axis in degrees.
/// \param yDegrees Rotation about the Y axis in degrees.
/// \param zDegrees Rotation about the Z axis in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039814 (0x80039814).
void matrix33ApplyRotationXYZDegrees(f32 *matrixAddress, f32 xDegrees, f32 yDegrees, f32 zDegrees);

/// Apply a local Euler rotation in Z, then Y, then X order.
/// \param matrixAddress Matrix whose linear transform is updated in place.
/// \param xDegrees Rotation about the X axis in degrees.
/// \param yDegrees Rotation about the Y axis in degrees.
/// \param zDegrees Rotation about the Z axis in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003988C (0x8003988C).
void matrix33ApplyRotationZYXDegrees(f32 *matrixAddress, f32 xDegrees, f32 yDegrees, f32 zDegrees);

/// Rotate an affine transform about the world Y axis.
/// \param angle Rotation angle in degrees.
/// \param m Twelve-component affine matrix updated in place, including its translation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000D764 (0x8000D764).
void matrix34RotateWorldYDegrees(f32 angle, f32 *m);

/// Extract roll, pitch, and yaw from a rotation matrix.
/// \param matrix Input nine-component rotation matrix.
/// \param roll Receives roll in degrees.
/// \param pitch Receives pitch in degrees.
/// \param yaw Receives yaw in degrees.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000D2E4 (0x8000D2E4).
void matrix3GetEulerAnglesDegrees(f32 *matrix, f32 *roll, f32 *pitch, f32 *yaw);

#endif
