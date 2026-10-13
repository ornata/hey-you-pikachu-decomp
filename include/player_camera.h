/// \file
/// Declares player view updates, recentering, positioning and camera transitions.

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

/// Refresh the primary camera from a player pose when camera updates are permitted.
/// \param player Borrowed initialized player record supplying world pose, eye height and view angles.
/// \returns Nothing; leaves the camera unchanged when the player's camera-update permission is disabled.
/// \note Original decompilation identifier: \c func_80111B68 (0x80111B68).
void playerUpdateViewCamera(u8 *player);

/// Advance recentering toward the default walking view orientation.
/// \param player Borrowed initialized player record receiving pitch and horizontal view-offset changes.
/// \returns Nothing; uses the current runtime frame duration.
/// \note Original decompilation identifier: \c func_80112138 (0x80112138).
void playerRecenterWalkingView(u8 *player);

/// Move the primary player and refresh its view camera when updates are permitted.
/// \param position Readable world X, Y and Z coordinates, copied during the call.
/// \returns Nothing; resets retained downward velocity.
/// \note Original decompilation identifier: \c func_80112480 (0x80112480).
void playerSetWorldPosition(const f32 *position);

#endif
