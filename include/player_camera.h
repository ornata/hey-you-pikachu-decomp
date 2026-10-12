/// \file
/// Declares preparation of the primary player camera transition.

#ifndef PLAYER_CAMERA_H
#define PLAYER_CAMERA_H

#include "ultra64.h"

/// Configure a smooth primary-camera transition from the current view for the camera-transition mode.
/// \param eye Readable destination camera eye position as world X, Y, Z coordinates; copied during this call.
/// \param target Readable destination look-at point as world X, Y, Z coordinates; copied during this call.
/// \param duration Positive finite requested transition duration in seconds.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_80112698 (0x80112698).
void playerPrepareCameraTransition(f32 *eye, f32 *target, f32 duration);

#endif
