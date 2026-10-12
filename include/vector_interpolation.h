/// \file
/// Declares smooth timed transitions between three-component positions.

#ifndef VECTOR_INTERPOLATION_H
#define VECTOR_INTERPOLATION_H

#include "ultra64.h"

/// Interpolate smoothly between two positions with zero endpoint velocities.
/// \param a Starting three-component position.
/// \param b Ending three-component position.
/// \param out Receives the interpolated position in storage separate from the endpoint.
/// \param len Positive interpolation duration, in the same units as t.
/// \param t Elapsed interpolation time; values beyond the duration reach the endpoint, while negative values extrapolate.
/// \returns Minus one when the result reaches the endpoint within tolerance; otherwise zero.
/// \note Original decompilation identifier: \c func_8000DBE4 (0x8000DBE4).
s32 vector3InterpolateSmoothTimed(f32 *a, f32 *b, f32 *out, f32 len, f32 t);

#endif
