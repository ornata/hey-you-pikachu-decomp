/// \file
/// Declares world-space camera eye and target configuration.

#ifndef CAMERA_POSE_H
#define CAMERA_POSE_H

#include "ultra64.h"

/// Aim a camera at a world-space point with world-up orientation.
/// \param cameraIndex Valid camera slot to configure.
/// \param eye Three readable world-space eye coordinates, copied during the call.
/// \param target Three readable world-space target coordinates, copied during the call.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80000AD4 (0x80000AD4).
void cameraSetEyeAndTarget(s32 cameraIndex, f32 *eye, f32 *target);

/// Set a camera's world-space position and viewing angles.
/// \param index Valid camera slot to configure.
/// \param x World-space eye X coordinate.
/// \param y World-space eye height.
/// \param z World-space eye Z coordinate.
/// \param pitch Viewing pitch in degrees; positive angles look downward.
/// \param yaw Viewing yaw in degrees from positive Z toward positive X.
/// \param roll Up-vector angle in degrees from positive Y toward positive X.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800009C4 (0x800009C4).
void cameraSetPositionAndAngles(s32 index, f32 x, f32 y, f32 z, f32 pitch, f32 yaw, f32 roll);

/// Convert a position from the active camera's view space to world space.
/// \param point Three camera-space coordinates, replaced with world-space coordinates.
/// \returns Nothing.
/// \pre The active camera has prepared transforms.
/// \note Original decompilation identifier: \c func_8000258C (0x8000258C).
void cameraViewPointToWorld(f32 *point);

#endif
