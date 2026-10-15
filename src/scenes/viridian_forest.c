/// Geometry of the viridian forest spatial lookup grid.
enum { VIRIDIAN_FOREST_GRID_LAYERS = 32, VIRIDIAN_FOREST_GRID_SIDE = 32 };
/// Height of one lookup layer in world-coordinate units.
#define VIRIDIAN_FOREST_GRID_LAYER_HEIGHT 5.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define VIRIDIAN_FOREST_GRID_CELL_WIDTH 10.0

#include "item_object.h"
#include "common.h"
#include "toolbox.h"

/// Remove the removable items held in the tracked scene-item slots.
/// \returns Nothing.
void viridianForestRemoveTrackedItems(void) {
    u8 *object;
    s32 column;
    s32 row;

    row = 0;
    do {
        column = 0;
        do {
            object = toolboxTakeObject(row, column);
            if (object != NULL) {
                itemDestroy(object);
            }
            column++;
        } while (column != 10);
        row++;
    } while (row != 2);
}

/// Prepare the forest scene by removing its removable tracked items.
/// \returns Nothing.
void viridianForestInitializeObjects(void) {
    viridianForestRemoveTrackedItems();
}

extern s32 *D_8014DB20_scene_viridian_forest[VIRIDIAN_FOREST_GRID_LAYERS];
/// Look up the forest spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x80134308 in the viridian forest overlay.
s32 viridianForestGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / VIRIDIAN_FOREST_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= VIRIDIAN_FOREST_GRID_LAYERS || (ix = (s32)((f64)x / VIRIDIAN_FOREST_GRID_CELL_WIDTH), ix < 0) ||
        ix >= VIRIDIAN_FOREST_GRID_SIDE || (iz = (s32)((f64)z / VIRIDIAN_FOREST_GRID_CELL_WIDTH), iz < 0) ||
        iz >= VIRIDIAN_FOREST_GRID_SIDE) {
        return 0;
    }
    layer = D_8014DB20_scene_viridian_forest[iy];
    if (layer == 0) {
        return 0;
    }
    return layer[(iz * VIRIDIAN_FOREST_GRID_SIDE) + ix];
}
