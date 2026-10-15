/// \file
/// Defines camera-space viewing regions and world-position visibility queries.

#ifndef CAMERA_VISIBILITY_H
#define CAMERA_VISIBILITY_H

#include <ultra64.h>

/// Viewing region with forward depth measured along negative camera-space Z.
typedef struct CameraViewRegion {
    f32 minimumDepth; ///< Inclusive nearest forward depth.
    f32 maximumDepth; ///< Inclusive farthest forward depth.
    f32 scaleX;       ///< Horizontal coordinate scale for the lateral limit.
    f32 scaleY;       ///< Vertical coordinate scale for the lateral limit.
    f32 margin;       ///< Additional lateral allowance in camera-space units.
} CameraViewRegion;
typedef char CameraViewRegionSizeCheck[(sizeof(CameraViewRegion) == 0x14) ? 1 : -1];
typedef char CameraViewRegionMarginOffsetCheck[((u32) & ((CameraViewRegion *)0)->margin == 0x10) ? 1 : -1];

/// Test a camera-space position against a viewing region.
/// \param region Region passed by value with inclusive depth limits.
/// \param point Borrowed three-component camera-space position in X, Y, Z order.
/// \returns One when depth is accepted and the point is strictly inside the lateral limits; zero otherwise.
/// \note Original decompilation identifier: \c func_800024C8 (0x800024C8).
s32 cameraTestViewPoint(CameraViewRegion region, f32 *point);

/// Transform a world position and test the current area's camera view limits.
/// \param margin Allowance beyond the camera view boundaries.
/// \param position Borrowed three-component world position in X, Y, Z order.
/// \param out Receives the camera-space position even when visibility is rejected.
/// \returns One when visible, or zero otherwise.
/// \pre The current area and active camera are initialized.
/// \note Original decompilation identifier: \c func_8010CFF8 (0x8010CFF8).
s32 areaTestCameraVisibility(f32 margin, f32 *position, f32 *out);

#endif
