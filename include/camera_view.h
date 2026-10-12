/// \file
/// Declares view transforms used by object and item directional searches.

#ifndef CAMERA_VIEW_H
#define CAMERA_VIEW_H

#include "ultra64.h"

/// Construct a view transform from a position and Euler orientation.
/// \param position Three-component viewing position.
/// \param angles Three Euler rotation angles in degrees, in the engine rotation order.
/// \param matrix Address of the twelve-component destination affine matrix.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000D6D8 (0x8000D6D8).
void matrix34MakeViewFromEulerDegrees(f32 *position, f32 *angles, f32 *matrix);

#endif
