/// \file
/// Declares affine transformation of three-component positions.

#ifndef MATRIX_POINT_H
#define MATRIX_POINT_H

#include "ultra64.h"

/// Transform a position into the coordinate space described by an affine matrix.
/// \param matrix Twelve-component affine transform, including translation.
/// \param point Three-component input position.
/// \param output Receives three transformed coordinates; may alias point.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FB90 (0x8003FB90).
void matrix34TransformPointVector(f32 *matrix, f32 *point, f32 *output);

#endif
