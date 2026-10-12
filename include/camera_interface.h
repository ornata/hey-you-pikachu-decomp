/// \file
/// Declares the camera-relative transform used to place interface models in world space.

#ifndef CAMERA_INTERFACE_H
#define CAMERA_INTERFACE_H

#include "ultra64.h"

/// Build the camera-relative world transform used to position interface models.
/// \param output Writable twelve-component affine transform receiving the active camera position and current player viewing orientation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80000800 (0x80000800).
void cameraBuildInterfaceTransform(f32 *output);

#endif
