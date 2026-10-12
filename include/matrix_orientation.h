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

#endif
