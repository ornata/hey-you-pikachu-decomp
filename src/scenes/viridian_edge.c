#include "common.h"

/// Provide a Viridian Forest Edge scene callback that requires no action.
/// \returns Nothing.
void viridianEdgeSceneNoOp34270(void) {
}
/// Provide a second Viridian Forest Edge scene callback that requires no action.
/// \returns Nothing.
void viridianEdgeSceneNoOp34278(void) {
}
extern s32 *D_8015BF30_scene_viridian_edge[8];

/// Look up the Viridian Forest Edge spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 viridianEdgeGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 20.0);
    if (iy < 0 || iy >= 8 || (ix = (s32)((f64)x / 20.0), ix < 0) || ix >= 8 || (iz = (s32)((f64)z / 20.0), iz < 0) ||
        iz >= 8) {
        return 0;
    }
    layer = D_8015BF30_scene_viridian_edge[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 8 + ix];
}
