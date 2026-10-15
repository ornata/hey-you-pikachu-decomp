#include "actor_kind.h"
/// Geometry of the playground spatial lookup grid.
enum { PLAYGROUND_GRID_LAYERS = 8, PLAYGROUND_GRID_SIDE = 8 };
/// Height of one lookup layer in world-coordinate units.
#define PLAYGROUND_GRID_LAYER_HEIGHT 20.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define PLAYGROUND_GRID_CELL_WIDTH 40.0

#include "landmark.h"
#include "cave.h"
#include "common.h"
#include "actor_create.h"
#include "scene_types.h"
#include "scene_configuration.h"
extern u8 D_8013EE54_scene_playground[];
extern f32 D_801344B0_scene_playground[];
extern f32 D_801344BC_scene_playground[];
extern f32 D_801344D4_scene_playground[];
extern s32 *D_8015A04C_scene_playground[PLAYGROUND_GRID_LAYERS];
/// Spawn the playground resident creature with its scene behavior.
/// \returns Nothing.
void playgroundSpawnResident(void) {
    actorCreateForKind(ACTOR_KIND_MAGNEMITE, 320.0F, 10.0F, 225.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_8013EE54_scene_playground, 0);
}
/// Create the playground cave, its linked landmark, and the resident creature.
/// \returns Nothing.
void playgroundInitializeObjects(void) {
    u8 *object;
    object = caveCreate(D_801344D4_scene_playground, D_801344B0_scene_playground, D_801344BC_scene_playground, 0);
    caveSetLinkedLandmark(object, landmarkCreate(D_801344B0_scene_playground[6], D_801344B0_scene_playground[7],
                                                      D_801344B0_scene_playground[8], 0x90));
    playgroundSpawnResident();
}
/// Look up the playground spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x80134334 in the playground overlay.
s32 playgroundGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / PLAYGROUND_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= PLAYGROUND_GRID_LAYERS || (ix = (s32)((f64)x / PLAYGROUND_GRID_CELL_WIDTH), ix < 0) ||
        ix >= PLAYGROUND_GRID_SIDE || (iz = (s32)((f64)z / PLAYGROUND_GRID_CELL_WIDTH), iz < 0) ||
        iz >= PLAYGROUND_GRID_SIDE) {
        return 0;
    }
    layer = D_8015A04C_scene_playground[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * PLAYGROUND_GRID_SIDE) + ix];
}
