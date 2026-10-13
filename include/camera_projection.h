/// \file
/// Defines camera projection outputs and pixel-rectangle viewport configuration.

#ifndef CAMERA_PROJECTION_H
#define CAMERA_PROJECTION_H

#include <ultra64.h>

/// Prepare a camera's viewport, projection, view and world-to-view transforms.
/// \param index Valid camera slot with initialized pose and loaded area configuration.
/// \param viewport Writable aligned viewport output, retained through submitted rendering tasks.
/// \param buffer Writable aligned storage for two Mtx records: combined projection and view.
/// The camera borrows the matrices until replaced; rendering tasks retain them through completion.
/// \returns Perspective normalization value for the prepared projection.
/// \note Original address: 0x80001844.
s32 cameraBuildViewProjection(s32 index, Vp *viewport, Mtx *buffer);

/// Configure an inclusive pixel viewport and report its corrected projection aspect.
/// \param viewport Writable viewport; resulting scale and translation must fit signed 16-bit fields.
/// \param x0 Left pixel coordinate.
/// \param y0 Top pixel coordinate.
/// \param x1 Inclusive right coordinate, at least x0.
/// \param y1 Inclusive bottom coordinate, at least y0.
/// \param aspectDenominator Nonzero aspect-correction denominator.
/// \param aspectNumerator Aspect-correction numerator.
/// \param aspectScale Finite additional projection aspect scale.
/// \returns Corrected projection aspect ratio.
/// \note Original decompilation identifier: \c func_8003C9F0 (0x8003C9F0).
f32 viewportSetPixelRectangle(Vp *viewport, s32 x0, s32 y0, s32 x1, s32 y1, s32 aspectDenominator, s32 aspectNumerator,
                              f32 aspectScale);

#endif
