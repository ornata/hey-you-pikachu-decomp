/// \file
/// Declares camera-facing orientation updates for runtime object drawing.

#ifndef OBJECT_BILLBOARD_H
#define OBJECT_BILLBOARD_H

#include "ultra64.h"

/// Turn an object horizontally toward the active camera.
/// \param object Live object whose yaw and orientation-update flags may change.
/// \returns Nothing.
/// \pre The active camera's eye position is initialized.
/// \note Original decompilation identifier: \c func_80008830 (0x80008830).
void objectFaceCameraHorizontally(u8 *object);

/// Orient an object toward the active camera for drawing.
/// \param object Live object whose orientation and orientation-update flags may change.
/// \returns Nothing.
/// \pre The active camera's eye position is initialized.
/// \note Original decompilation identifier: \c func_800088BC (0x800088BC).
void objectUpdateCameraFacingForDraw(u8 *object);

#endif
