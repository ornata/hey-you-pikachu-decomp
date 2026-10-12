/// \file
/// Defines the recovered camera view, projection-storage and viewport layout.

#ifndef CAMERA_TYPES_H
#define CAMERA_TYPES_H

#include "ultra64.h"

/// Affine camera transform stored as twelve floating-point components.
typedef struct {
    f32 values[12]; ///< Linear basis followed by translation.
} CameraMatrix;

/// Camera record shared by rendering and positional-audio consumers.
/// Unresolved storage remains explicit; this view does not establish resource ownership.
typedef struct {
    f32 eye[3];          ///< World-space camera position.
    f32 target[3];       ///< World-space point toward which the camera looks.
    f32 up[3];           ///< Camera up direction.
    Mtx *matrices;       ///< Borrowed frame buffer containing projection and view matrices; null before preparation.
    CameraMatrix view;   ///< Unscaled world-to-view affine transform.
    CameraMatrix scaled; ///< World-to-view transform scaled for view-region queries.
    f32 distance;        ///< Area-dependent distance used for view-region scaling.
    u8 unknown08C[0x1C];
    s32 viewport[4]; ///< Pixel rectangle used to construct the viewport.
    u8 unknown0B8[0x28];
} MatrixCamera;

typedef char CameraMatrixSizeCheck[(sizeof(CameraMatrix) == 0x30) ? 1 : -1];
typedef char MatrixCameraSizeCheck[(sizeof(MatrixCamera) == 0xE0) ? 1 : -1];
typedef char MatrixCameraMatricesOffsetCheck[(((u32) & (((MatrixCamera *)0)->matrices)) == 0x24) ? 1 : -1];
typedef char MatrixCameraViewOffsetCheck[(((u32) & (((MatrixCamera *)0)->view)) == 0x28) ? 1 : -1];
typedef char MatrixCameraViewportOffsetCheck[(((u32) & (((MatrixCamera *)0)->viewport)) == 0xA8) ? 1 : -1];

#endif
