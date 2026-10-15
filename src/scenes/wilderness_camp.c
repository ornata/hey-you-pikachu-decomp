/// Geometry of the wilderness camp spatial lookup grid.
enum { WILDERNESS_CAMP_GRID_LAYERS = 16, WILDERNESS_CAMP_GRID_SIDE = 16 };
/// Height of one lookup layer in world-coordinate units.
#define WILDERNESS_CAMP_GRID_LAYER_HEIGHT 20.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define WILDERNESS_CAMP_GRID_CELL_WIDTH 20.0

#include "common.h"
#include "actor_create.h"
#include "scene_types.h"

#include "scene_configuration.h"
extern u8 D_8013A878_scene_wilderness_camp[];
extern s32 *D_8013CE38_scene_wilderness_camp[WILDERNESS_CAMP_GRID_LAYERS];

/// Spawn the Wilderness Camp resident creature with its scene behavior.
/// \returns Nothing.
void wildernessCampSpawnResident(void) {
    actorCreateForKind(0x11, 125.0F, 10.0F, 122.27F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_8013A878_scene_wilderness_camp, 0);
}

/// Provide the Wilderness Camp scene callback that requires no action.
/// \returns Nothing.
void wildernessCampSceneNoOp(void) {
}

/// Look up the Wilderness Camp spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x801342D4 in the wilderness camp overlay.
s32 wildernessCampGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / WILDERNESS_CAMP_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= WILDERNESS_CAMP_GRID_LAYERS || (ix = (s32)((f64)x / WILDERNESS_CAMP_GRID_CELL_WIDTH), ix < 0) ||
        ix >= WILDERNESS_CAMP_GRID_SIDE || (iz = (s32)((f64)z / WILDERNESS_CAMP_GRID_CELL_WIDTH), iz < 0) ||
        iz >= WILDERNESS_CAMP_GRID_SIDE) {
        return 0;
    }
    layer = D_8013CE38_scene_wilderness_camp[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * WILDERNESS_CAMP_GRID_SIDE) + ix];
}
