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

#endif
