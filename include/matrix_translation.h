/// \file
/// Declares local-space translation of affine transforms.

#ifndef MATRIX_TRANSLATION_H
#define MATRIX_TRANSLATION_H

#include "ultra64.h"

/// Apply a translation in the local coordinate system.
/// \param mf Affine matrix to update.
/// \param x Local X displacement.
/// \param y Local Y displacement.
/// \param z Local Z displacement.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003973C (0x8003973C).
void matrix34TranslateLocal(f32 mf[4][3], f32 x, f32 y, f32 z);

#endif
