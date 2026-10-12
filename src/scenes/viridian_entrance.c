#include "common.h"
#include "actor_create.h"
#include "area_maze.h"
#include "scene_types.h"
u8 *interactionProxyCreate(u8 *, s32, s32, s32, s32);
u8 *itemCreate(f32, f32, f32, f32, f32, s32);
s32 toolboxStoreObjectInRow(s32, s32);

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

    position[0] = 160.0f;
    position[1] = 7.5f;
    position[2] = 262.0f;
    interactionProxyCreate((u8 *)position, 1, 4, 0, (s32)D_8014E9F4_scene_viridian_entrance);
}

/// Create and register the entrance items required by the saved progress stage.
/// \returns Nothing.
void viridianEntranceCreateTrackedItems(void) {
    toolboxStoreObjectInRow((s32)itemCreate(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1), 0);
    toolboxStoreObjectInRow((s32)itemCreate(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5), 0);
    toolboxStoreObjectInRow((s32)itemCreate(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 6), 0);
    toolboxStoreObjectInRow((s32)itemCreate(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 7), 0);
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

/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
extern u8 D_8014CF3C_scene_viridian_entrance[];

/// Populate the mode-two forest entrance with its prop and resident group.
/// \returns Nothing.
void viridianEntranceInitializeModeTwoObjects(void) {
    s32 i;

    viridianEntranceCreateSceneProp();
    for (i = 0; i < 6; i++) {
        actorCreateForKind(0x24, 180.0f, 1.5f, 215.0f, 0, 0.0f,
                           ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
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

extern s32 *D_8017467C_scene_viridian_entrance[32];
/// Look up the forest-entrance spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 viridianEntranceGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 5.0);
    if (iy < 0 || iy >= 32 || (ix = (s32)((f64)x / 10.0), ix < 0) || ix >= 32 || (iz = (s32)((f64)z / 10.0), iz < 0) ||
        iz >= 32) {
        return 0;
    }
    layer = D_8017467C_scene_viridian_entrance[iy];
    if (layer == 0)
        return 0;
    return layer[iz * 32 + ix];
}
