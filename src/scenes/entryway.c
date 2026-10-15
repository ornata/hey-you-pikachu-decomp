/// Geometry of the entryway spatial lookup grid.
enum { ENTRYWAY_GRID_LAYERS = 16, ENTRYWAY_GRID_SIDE = 16 };
/// Height of one lookup layer in world-coordinate units.
#define ENTRYWAY_GRID_LAYER_HEIGHT 21.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define ENTRYWAY_GRID_CELL_WIDTH 10.0

#include "common.h"
#include "actor_create.h"
#include "scene_types.h"

#include "toolbox.h"
#include "scene_configuration.h"
extern u8 D_80139E38_scene_entryway[];

/// Initialize entryway scene state and spawn its resident creature.
/// \returns Nothing.
void entrywayInitializeObjects(void) {
    toolboxClearCollection();
    actorCreateForKind(1, 82.26F, 3.0F, 104.37F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_80139E38_scene_entryway, 0);
}

extern s32 *D_8013C888_scene_entryway[ENTRYWAY_GRID_LAYERS];
/// Look up the entryway spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x801342D8 in the entryway overlay.
s32 entrywayGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;

    iy = (s32)((f64)y / ENTRYWAY_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= ENTRYWAY_GRID_LAYERS || (ix = (s32)((f64)x / ENTRYWAY_GRID_CELL_WIDTH), ix < 0) ||
        ix >= ENTRYWAY_GRID_SIDE || (iz = (s32)((f64)z / ENTRYWAY_GRID_CELL_WIDTH), iz < 0) ||
        iz >= ENTRYWAY_GRID_SIDE) {
        return 0;
    }
    layer = D_8013C888_scene_entryway[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * ENTRYWAY_GRID_SIDE) + ix];
}
