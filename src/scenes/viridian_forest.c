#include "common.h"
u8 *toolboxTakeObject(s32, s32);
void itemDestroy(u8 *);

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

extern s32 *D_8014DB20_scene_viridian_forest[32];
/// Look up the forest spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 viridianForestGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 5.0);
    if (iy < 0 || iy >= 32 || (ix = (s32)((f64)x / 10.0), ix < 0) || ix >= 32 || (iz = (s32)((f64)z / 10.0), iz < 0) ||
        iz >= 32) {
        return 0;
    }
    layer = D_8014DB20_scene_viridian_forest[iy];
    if (layer == 0)
        return 0;
    return layer[iz * 32 + ix];
}
