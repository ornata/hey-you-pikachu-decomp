/// \file
/// Declares persistent rendering camera storage shared by scene and audio consumers.

#ifndef CAMERA_STORAGE_H
#define CAMERA_STORAGE_H

#include "camera_types.h"

/// Persistent camera slots used for view transforms, backdrop state and overlays.
/// References are borrowed; consumers must select a camera slot configured by rendering initialization.
/// \note Original identifier: \c D_800ABCF0 (0x800ABCF0).
extern MatrixCamera gRenderCameras[];

#endif
