/// Geometry of the ochre entrance spatial lookup grid.
enum { OCHRE_ENTRANCE_GRID_LAYERS = 16, OCHRE_ENTRANCE_GRID_SIDE = 16 };
/// Height of one lookup layer in world-coordinate units.
#define OCHRE_ENTRANCE_GRID_LAYER_HEIGHT 20.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define OCHRE_ENTRANCE_GRID_CELL_WIDTH 20.0

#include "random.h"
#include "common.h"
#include "actor_create.h"

#include "togepi.h"
#include "scene_configuration.h"
#include "scene_types.h"
extern u8 D_8013D380_scene_ochre_entrance[], D_8013DD80_scene_ochre_entrance[];
extern f32 D_8013E8A0_scene_ochre_entrance;

/// Populate the forest entrance with residents appropriate to saved progress.
/// \returns Nothing.
void ochreEntranceInitializeResidents(void) {
    switch (progressGetValue(0x56)) {
    case 0:
        break;
    default:
        if (progressGetValue(0x16) != 0 && randomBelow(3) == 0) {
            togepiCreate(D_8013E8A0_scene_ochre_entrance, 18.25F, 149.5F, 0.0F, 90.0F, D_8013DD80_scene_ochre_entrance,
                         3);
        } else {
            actorCreateForKind(0xC, 192.5F, 18.25F, 149.5F, 0.0F, 90.0F, gCurrentAreaConfiguration->objectScale * 1.5F,
                               D_8013DD80_scene_ochre_entrance, 3);
        }
    case 2:
        actorCreateForKind(3, 187.5F, 18.25F, 164.0F, 0.0F, 100.0F, gCurrentAreaConfiguration->objectScale,
                           D_8013DD80_scene_ochre_entrance, 2);
    case 1:
        actorCreateForKind(4, 181.5F, 18.25F, 164.5F, 0.0F, 100.0F, gCurrentAreaConfiguration->objectScale,
                           D_8013DD80_scene_ochre_entrance, 1);
        break;
    }
    actorCreateForKind(1, 168.0F, 18.0F, 144.5F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_8013D380_scene_ochre_entrance, 0);
}

extern s32 *D_80174674_scene_ochre_entrance[OCHRE_ENTRANCE_GRID_LAYERS];
/// Look up the forest-entrance spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x80134458 in the ochre entrance overlay.
s32 ochreEntranceGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;

    iy = (s32)((f64)y / OCHRE_ENTRANCE_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= OCHRE_ENTRANCE_GRID_LAYERS || (ix = (s32)((f64)x / OCHRE_ENTRANCE_GRID_CELL_WIDTH), ix < 0) ||
        ix >= OCHRE_ENTRANCE_GRID_SIDE || (iz = (s32)((f64)z / OCHRE_ENTRANCE_GRID_CELL_WIDTH), iz < 0) ||
        iz >= OCHRE_ENTRANCE_GRID_SIDE) {
        return 0;
    }
    layer = D_80174674_scene_ochre_entrance[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * OCHRE_ENTRANCE_GRID_SIDE) + ix];
}
