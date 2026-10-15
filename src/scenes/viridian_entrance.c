/// Geometry of the viridian entrance spatial lookup grid.
enum { VIRIDIAN_ENTRANCE_GRID_LAYERS = 32, VIRIDIAN_ENTRANCE_GRID_SIDE = 32 };
/// Height of one lookup layer in world-coordinate units.
#define VIRIDIAN_ENTRANCE_GRID_LAYER_HEIGHT 5.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define VIRIDIAN_ENTRANCE_GRID_CELL_WIDTH 10.0

#include "interaction_proxy.h"
#include "story_chapter.h"
#include "common.h"
#include "item_object.h"
#include "actor_create.h"
#include "area_maze.h"
#include "scene_types.h"
#include "toolbox.h"

extern u8 D_80144BC4_scene_viridian_entrance[];
extern u8 D_8014CF50_scene_viridian_entrance[];

/// Create the forest-entrance scenery object.
/// \returns Nothing.
void viridianEntranceCreateSceneryObject(void) {
    mazePlaceBlockObject(D_8014CF50_scene_viridian_entrance, D_80144BC4_scene_viridian_entrance);
}

extern u8 D_801346C0_scene_viridian_entrance[];
extern u8 D_80144F1C_scene_viridian_entrance[];
extern u8 D_80144FDC_scene_viridian_entrance[];

/// Create the forest-entrance prop.
/// \returns Nothing.
void viridianEntranceCreateSceneProp(void) {
    mazeReplaceBlock(D_801346C0_scene_viridian_entrance, D_80144F1C_scene_viridian_entrance,
                     (MazeGrid *)D_80144FDC_scene_viridian_entrance, 0);
}

extern u8 D_8014E9F4_scene_viridian_entrance[];

/// Create the forest-entrance access object.
/// \returns Nothing.
void viridianEntranceCreateAccessObject(void) {
    f32 position[3];

    position[0] = 160.0F;
    position[1] = 7.5F;
    position[2] = 262.0F;
    interactionProxyCreate(position, 1, 4, 0, (char *)D_8014E9F4_scene_viridian_entrance);
}

/// Create and register the entrance items required by the saved progress stage.
/// \returns Nothing.
void viridianEntranceCreateTrackedItems(void) {
    toolboxStoreObjectInRow((s32)itemCreate(0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1), 0);
    toolboxStoreObjectInRow((s32)itemCreate(0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 5), 0);
    toolboxStoreObjectInRow((s32)itemCreate(0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 6), 0);
    toolboxStoreObjectInRow((s32)itemCreate(0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 7), 0);
}

/// Populate the normal forest entrance with scenery, a prop, access, and progress-dependent items.
/// \returns Nothing.
void viridianEntranceInitializeNormalObjects(void) {
    viridianEntranceCreateSceneryObject();
    viridianEntranceCreateSceneProp();
    if (progressGetCurrentChapter() == 4) {
        viridianEntranceCreateTrackedItems();
    }
    viridianEntranceCreateAccessObject();
}

#include "scene_configuration.h"
extern u8 D_8014CF3C_scene_viridian_entrance[];

/// Populate the mode-two forest entrance with its prop and resident group.
/// \returns Nothing.
void viridianEntranceInitializeModeTwoObjects(void) {
    s32 i;

    viridianEntranceCreateSceneProp();
    for (i = 0; i < 6; i++) {
        actorCreateForKind(0x24, 180.0F, 1.5F, 215.0F, 0, 0.0F, gCurrentAreaConfiguration->objectScale,
                           D_8014CF3C_scene_viridian_entrance, i);
    }
}

/// Current area scenario index shared by actor scripts, scene population, cards and collection titles.
/// \note Original identifier: \c D_800F0B70 (0x800F0B70).
extern s32 gAreaScenarioIndex;

/// Populate the forest entrance for the current game mode.
/// \returns Nothing.
void viridianEntranceInitializeObjects(void) {
    if (gAreaScenarioIndex == 2) {
        viridianEntranceInitializeModeTwoObjects();
        return;
    }
    viridianEntranceInitializeNormalObjects();
}

extern s32 *D_8017467C_scene_viridian_entrance[VIRIDIAN_ENTRANCE_GRID_LAYERS];
/// Look up the forest-entrance spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x80134590 in the viridian entrance overlay.
s32 viridianEntranceGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / VIRIDIAN_ENTRANCE_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= VIRIDIAN_ENTRANCE_GRID_LAYERS ||
        (ix = (s32)((f64)x / VIRIDIAN_ENTRANCE_GRID_CELL_WIDTH), ix < 0) || ix >= VIRIDIAN_ENTRANCE_GRID_SIDE ||
        (iz = (s32)((f64)z / VIRIDIAN_ENTRANCE_GRID_CELL_WIDTH), iz < 0) || iz >= VIRIDIAN_ENTRANCE_GRID_SIDE) {
        return 0;
    }
    layer = D_8017467C_scene_viridian_entrance[iy];
    if (layer == 0) {
        return 0;
    }
    return layer[(iz * VIRIDIAN_ENTRANCE_GRID_SIDE) + ix];
}
