#include "actor_kind.h"
/// Geometry of the cobalt island spatial lookup grid.
enum { COBALT_ISLAND_GRID_LAYERS = 16, COBALT_ISLAND_GRID_SIDE = 16 };
/// Height of one lookup layer in world-coordinate units.
#define COBALT_ISLAND_GRID_LAYER_HEIGHT 15.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define COBALT_ISLAND_GRID_CELL_WIDTH 20.0

#include "random_scale.h"
#include "player_chase.h"
#include "random.h"
#include "ambient_audio.h"
#include "item_mode.h"
#include "landmark.h"
#include "object_types.h"
#include "common.h"
#include "item_object.h"
#include "actor_create.h"
#include "area_maze.h"

#include "scene_configuration.h"
#include "scene_types.h"

/// Provide an island-scene callback that requires no action.
/// \returns Nothing.
void cobaltIslandSceneNoOp80134270(void) {
}

/// Spawn a type-eight island item at the requested position.
/// \param position World-space position as three coordinates in X, Y, Z order.
/// \returns Nothing.
void cobaltIslandSpawnItemType8(f32 *position) {
    f32 x = position[0];
    f32 y = position[1];
    f32 z = position[2];
    itemSetMode(itemCreate(x, y, z, 0.0F, 0.0F, 8), ITEM_MODE_STATIONARY);
}

extern u8 D_8014D854_scene_cobalt_island[];
extern f64 D_80151E00_scene_cobalt_island;

/// Spawn a type-sixteen island resident with a random heading.
/// \param residentIndex Identifier assigned to the resident within the scene population.
/// \returns Nothing.
void cobaltIslandSpawnResidentType16(s32 residentIndex) {
    actorCreateForKind(ACTOR_KIND_TREASURE_CHEST, 0.0F, 0.0F, 0.0F, 0,
                       (f32)((f64)randomNext() * RANDOM_U32_UNIT_SCALE * D_80151E00_scene_cobalt_island),
                       gCurrentAreaConfiguration->objectScale, D_8014D854_scene_cobalt_island, residentIndex);
}

extern u8 D_8014DCE8_scene_cobalt_island[];

/// Spawn the fixed type-eleven resident for the island scene.
/// \returns Nothing.
void cobaltIslandSpawnResidentType11(void) {
    actorCreateForKind(ACTOR_KIND_LAPRAS, 160.0F, -1.0F, 400.0F, 0.0F, 180.0F, gCurrentAreaConfiguration->objectScale,
                       D_8014DCE8_scene_cobalt_island, 0);
}

extern u8 D_8014E00C_scene_cobalt_island[];
extern f32 D_80151E08_scene_cobalt_island;

/// Spawn the fixed type-eighteen resident for the island scene.
/// \returns Nothing.
void cobaltIslandSpawnResidentType18(void) {
    actorCreateForKind(ACTOR_KIND_SLOWPOKE, 301.0F, 6.66F, 61.0F, 0.0F, D_80151E08_scene_cobalt_island,
                       gCurrentAreaConfiguration->objectScale, D_8014E00C_scene_cobalt_island, 0);
}

extern u8 D_80134A1C_scene_cobalt_island[];
extern u8 D_80134A28_scene_cobalt_island[];
extern u8 D_8014E140_scene_cobalt_island[];
extern u8 D_8014EC40_scene_cobalt_island[];
extern u8 D_8014EC50_scene_cobalt_island[];
extern u8 D_8014F750_scene_cobalt_island[];
extern u8 D_8014F760_scene_cobalt_island[];
extern u8 D_80150260_scene_cobalt_island[];
extern u8 D_80150270_scene_cobalt_island[];
extern u8 D_80150D70_scene_cobalt_island[];

/// Create the island props appropriate to the current scene variant.
/// \returns Nothing.
void cobaltIslandCreateSceneProps(void) {
    if (playerGetChaseStage() < 2) {
        mazeReplaceBlock(D_80134A1C_scene_cobalt_island, D_8014E140_scene_cobalt_island,
                         (MazeGrid *)D_8014EC40_scene_cobalt_island, 0);
        mazeReplaceBlock(D_80134A28_scene_cobalt_island, D_8014EC50_scene_cobalt_island,
                         (MazeGrid *)D_8014F750_scene_cobalt_island, 0);
        return;
    }
    mazeReplaceBlock(D_80134A1C_scene_cobalt_island, D_8014F760_scene_cobalt_island,
                     (MazeGrid *)D_80150260_scene_cobalt_island, 0);
    mazeReplaceBlock(D_80134A28_scene_cobalt_island, D_80150270_scene_cobalt_island,
                     (MazeGrid *)D_80150D70_scene_cobalt_island, 0);
}

extern f32 D_80150D80_scene_cobalt_island[];

/// Create the island scene's ambient audio emitter.
/// \returns Nothing.
void cobaltIslandCreateAmbientAudio(void) {
    ambientAudioCreate(D_80150D80_scene_cobalt_island, 1, 4.0F, 4.0F, 0x2B, 0x2B, 1.0F, 3,
                       (Vec3f *)D_80150D80_scene_cobalt_island);
}

extern f32 D_80134A34_scene_cobalt_island[][3];
extern u8 D_8014E12C_scene_cobalt_island[];
typedef struct {
    s32 words[24];
} CobaltIslandSetup;
extern CobaltIslandSetup D_80150DA4_scene_cobalt_island;

/// Populate the island with items, residents, props, ambient audio, and a landmark.
/// \returns Nothing.
void cobaltIslandInitializeObjects(void) {
    s32 i;
    s32 stage;
    CobaltIslandSetup setup;

    setup = D_80150DA4_scene_cobalt_island;
    for (i = 0; i < 4; i++) {
        cobaltIslandSpawnItemType8(D_80134A34_scene_cobalt_island[i]);
    }
    cobaltIslandCreateAmbientAudio();
    cobaltIslandSpawnResidentType11();
    cobaltIslandSpawnResidentType18();
    for (i = 0; i < 10; i++) {
        cobaltIslandSpawnResidentType16(i);
    }
    stage = progressGetValue(0x76U);
    if (stage >= 4) {
        actorCreateForKind(ACTOR_KIND_BUTTERFREE, 220.0F, 41.0F, 111.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                           D_8014E12C_scene_cobalt_island, 0);
    }
    cobaltIslandCreateSceneProps();
    landmarkCreate(220.0F, 31.0F, 111.0F, RUNTIME_OBJECT_KIND_LANDMARK_CANNON);
}

/// Provide an island-scene callback that requires no action.
/// \returns Nothing.
void cobaltIslandSceneNoOp80134670(void) {
}

extern s32 *D_8018A28C_scene_cobalt_island[COBALT_ISLAND_GRID_LAYERS];
/// Look up the island spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x80134678 in the cobalt island overlay.
s32 cobaltIslandGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / COBALT_ISLAND_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= COBALT_ISLAND_GRID_LAYERS || (ix = (s32)((f64)x / COBALT_ISLAND_GRID_CELL_WIDTH), ix < 0) ||
        ix >= COBALT_ISLAND_GRID_SIDE || (iz = (s32)((f64)z / COBALT_ISLAND_GRID_CELL_WIDTH), iz < 0) ||
        iz >= COBALT_ISLAND_GRID_SIDE) {
        return 0;
    }
    layer = D_8018A28C_scene_cobalt_island[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * COBALT_ISLAND_GRID_SIDE) + ix];
}
