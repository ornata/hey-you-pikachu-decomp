/// \file
/// Declares clamped scalar range mapping and integer-power evaluation.

#ifndef MATH_SCALAR_H
#define MATH_SCALAR_H

#include "ultra64.h"

/// Map a value between ranges while clamping to the input endpoints.
/// \param inputStart Lower input endpoint.
/// \param outputStart Output at the lower endpoint.
/// \param inputEnd Upper input endpoint, greater than inputStart.
/// \param outputEnd Output at the upper endpoint.
/// \param value Input value to map.
/// \returns Mapped value, clamped to the corresponding endpoint outside the input range.
/// \note Original decompilation identifier: \c func_8003AA14 (0x8003AA14).
f32 mathMapRangeClamped(f32 inputStart, f32 outputStart, f32 inputEnd, f32 outputEnd, f32 value);

/// Raise a floating-point value to an integer power.
/// \param base Base value; must be nonzero for a negative exponent.
/// \param exponent Integer exponent, excluding the minimum signed integer.
/// \returns The base raised to the requested power.
/// \note Original decompilation identifier: \c func_8000DD40 (0x8000DD40).
f32 mathPowInteger(f32 base, s32 exponent);

#endif
