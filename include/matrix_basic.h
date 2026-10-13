/// \file
/// Declares affine and linear matrix operations and graphics-format conversion.

#ifndef MATRIX_BASIC_H
#define MATRIX_BASIC_H

#include "matrix_types.h"

/// Set an affine matrix to the identity transform.
/// \param m Output affine matrix.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003A058 (0x8003A058).
void matrix34Identity(Matrix34 *m);

/// Transpose a 3x3 matrix in place.
/// \param p 3x3 matrix to transpose.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003A158 (0x8003A158).
void matrix33Transpose(f32 *p);

/// Transform a three-dimensional direction vector.
/// \param matrix Input 3x3 matrix.
/// \param vector Input three-component vector.
/// \param output Output three-component vector; may alias vector.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800395E0 (0x800395E0).
void matrix33TransformVector(f32 *matrix, f32 *vector, f32 *output);

/// Convert the affine components of a fixed-point matrix to floating point.
/// \param m Input fixed-point matrix.
/// \param out Output floating-point affine matrix.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80039D24 (0x80039D24).
void matrixFixedTo34(Mtx m, f32 *out);

/// Convert an affine transform to the graphics matrix representation.
/// \param matrix Input affine transform with finite components representable as signed 16.16 values.
/// \param output Writable graphics matrix that does not overlap the input.
/// \returns Nothing.
/// \note Portability: Conversion follows the target floating-point rounding mode.
/// \note Original address: 0x80042120.
void matrix34ToFixedMtx(Matrix34 *matrix, Mtx *output);

/// Compose affine transforms in left-then-right order.
/// \param left First input transform.
/// \param right Transform applied after left.
/// \param output Receives the composition; may equal either input, with no other overlap permitted.
/// \returns Nothing.
/// \note Original address: 0x80048F70.
void matrix34Concatenate(Matrix34 *left, Matrix34 *right, Matrix34 *output);

#endif
