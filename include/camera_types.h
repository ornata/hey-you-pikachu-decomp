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
    u8 *backdropImage;   ///< Heap-allocated image storage for this view, or null before allocation.
    s32 backdropWidth;   ///< Backdrop image width in pixels.
    s32 backdropHeight;  ///< Backdrop image height in pixels.
    s32 backdropX;       ///< Horizontal image offset used for backdrop drawing.
    s32 backdropY;       ///< Vertical image offset used for backdrop drawing.
    f32 backdropScroll;  ///< Accumulated horizontal backdrop scroll in pixels.
    s32 useBackdrop;     ///< Nonzero draws the configured backdrop instead of clearing to black.
    s32 viewport[4];    ///< Pixel rectangle in left, top, right, bottom order for viewport, scissor and depth clearing.
    u8 overlayColor[4]; ///< Blended primary overlay RGBA used for drawing.
    u8 targetBaseOverlayColor[4];      ///< Requested base overlay RGBA.
    u8 baseOverlayColor[4];            ///< Current base overlay RGBA.
    f32 baseOverlayTimeRemaining;      ///< Seconds remaining in the base overlay transition.
    u8 targetBlendOverlayColor[4];     ///< Requested additional blend RGBA.
    u8 blendOverlayColor[4];           ///< Current additional blend RGBA; alpha controls its contribution.
    f32 blendOverlayTimeRemaining;     ///< Seconds remaining in the additional blend transition.
    u8 targetSecondaryOverlayColor[4]; ///< Requested independently drawn secondary RGBA.
    u8 secondaryOverlayColor[4];       ///< Current independently drawn secondary RGBA.
    f32 secondaryOverlayTimeRemaining; ///< Seconds remaining in the secondary overlay transition.
} MatrixCamera;

typedef char CameraMatrixSizeCheck[(sizeof(CameraMatrix) == 0x30) ? 1 : -1];
typedef char MatrixCameraSizeCheck[(sizeof(MatrixCamera) == 0xE0) ? 1 : -1];
typedef char MatrixCameraMatricesOffsetCheck[(((u32) & (((MatrixCamera *)0)->matrices)) == 0x24) ? 1 : -1];
typedef char MatrixCameraViewOffsetCheck[(((u32) & (((MatrixCamera *)0)->view)) == 0x28) ? 1 : -1];
typedef char MatrixCameraViewportOffsetCheck[(((u32) & (((MatrixCamera *)0)->viewport)) == 0xA8) ? 1 : -1];

typedef char MatrixCameraOverlayOffsetCheck[(((u32) & (((MatrixCamera *)0)->overlayColor)) == 0xB8) ? 1 : -1];
typedef char MatrixCameraBaseOverlayOffsetCheck[(((u32) & (((MatrixCamera *)0)->baseOverlayColor)) == 0xC0) ? 1 : -1];
typedef char MatrixCameraBlendOverlayOffsetCheck[(((u32) & (((MatrixCamera *)0)->blendOverlayColor)) == 0xCC) ? 1 : -1];
typedef char
    MatrixCameraSecondaryOverlayOffsetCheck[(((u32) & (((MatrixCamera *)0)->secondaryOverlayColor)) == 0xD8) ? 1 : -1];

typedef char MatrixCameraBackdropImageOffsetCheck[(((u32) & ((MatrixCamera *)0)->backdropImage) == 0x8C) ? 1 : -1];
typedef char MatrixCameraBackdropWidthOffsetCheck[(((u32) & ((MatrixCamera *)0)->backdropWidth) == 0x90) ? 1 : -1];
typedef char MatrixCameraBackdropScrollOffsetCheck[(((u32) & ((MatrixCamera *)0)->backdropScroll) == 0xA0) ? 1 : -1];

typedef char MatrixCameraBackdropOffsetCheck[(((u32) & (((MatrixCamera *)0)->useBackdrop)) == 0xA4) ? 1 : -1];

#endif
