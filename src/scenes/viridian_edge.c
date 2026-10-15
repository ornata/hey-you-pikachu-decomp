/// Geometry of the viridian edge spatial lookup grid.
enum { VIRIDIAN_EDGE_GRID_LAYERS = 8, VIRIDIAN_EDGE_GRID_SIDE = 8 };
/// Height of one lookup layer in world-coordinate units.
#define VIRIDIAN_EDGE_GRID_LAYER_HEIGHT 20.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define VIRIDIAN_EDGE_GRID_CELL_WIDTH 20.0

#include "common.h"

/// Provide a Viridian Forest Edge scene callback that requires no action.
/// \returns Nothing.
void viridianEdgeSceneNoOp34270(void) {
}
/// Provide a second Viridian Forest Edge scene callback that requires no action.
/// \returns Nothing.
void viridianEdgeSceneNoOp34278(void) {
}
extern s32 *D_8015BF30_scene_viridian_edge[VIRIDIAN_EDGE_GRID_LAYERS];

/// Look up the Viridian Forest Edge spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x80134280 in the viridian edge overlay.
s32 viridianEdgeGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / VIRIDIAN_EDGE_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= VIRIDIAN_EDGE_GRID_LAYERS || (ix = (s32)((f64)x / VIRIDIAN_EDGE_GRID_CELL_WIDTH), ix < 0) ||
        ix >= VIRIDIAN_EDGE_GRID_SIDE || (iz = (s32)((f64)z / VIRIDIAN_EDGE_GRID_CELL_WIDTH), iz < 0) ||
        iz >= VIRIDIAN_EDGE_GRID_SIDE) {
        return 0;
    }
    layer = D_8015BF30_scene_viridian_edge[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * VIRIDIAN_EDGE_GRID_SIDE) + ix];
}
