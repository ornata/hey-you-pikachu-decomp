/// \file
/// Declares three-component vector length and in-place direction normalization.

#ifndef VECTOR_METRICS_H
#define VECTOR_METRICS_H

#include "ultra64.h"

/// Compute the length of a three-dimensional vector.
/// \param vector Input three-component vector.
/// \returns Vector length.
/// \note Original decompilation identifier: \c func_8003A5D0 (0x8003A5D0).
f32 vector3Length(f32 *vector);

/// Approximate the length of a three-dimensional vector.
/// \param vector Input three-component vector.
/// \returns Approximate vector length.
/// \note Original decompilation identifier: \c func_8003A5FC (0x8003A5FC).
f32 vector3LengthApprox(f32 *vector);

/// Normalize a direction, clearing vectors too small to normalize.
/// \param vector Three-component vector to update.
/// \returns Original length, or zero when the vector length is at most 0.001.
/// \note Original decompilation identifier: \c func_8003A7C0 (0x8003A7C0).
f32 vector3Normalize(f32 *vector);

#endif
