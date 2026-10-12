#include "common.h"
#include "actor_create.h"
#include "area_maze.h"

u8 *itemCreate(f32, f32, f32, f32, f32, s32);
void itemSetMode(u8 *, s32);
u8 playerGetChaseStage(void);
u32 randomNext(void);
u8 *ambientAudioCreate(f32 *, s32, f32, f32, s32, s32, f32, s32, s32);
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

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
    itemSetMode(itemCreate(x, y, z, 0.0f, 0.0f, 8), 2);
}

extern u8 D_8014D854_scene_cobalt_island[];
extern f64 D_80151E00_scene_cobalt_island;

/// Spawn a type-sixteen island resident with a random heading.
/// \param residentIndex Identifier assigned to the resident within the scene population.
/// \returns Nothing.
void cobaltIslandSpawnResidentType16(s32 residentIndex) {
    actorCreateForKind(0x10, 0.0f, 0.0f, 0.0f, 0,
                       (f32)((f64)randomNext() * 2.3283064365386963e-10 * D_80151E00_scene_cobalt_island),
                       *(f32 *)(gCurrentAreaConfiguration + 0xC), D_8014D854_scene_cobalt_island, residentIndex);
}

extern u8 D_8014DCE8_scene_cobalt_island[];

/// Spawn the fixed type-eleven resident for the island scene.
/// \returns Nothing.
void cobaltIslandSpawnResidentType11(void) {
    actorCreateForKind(0xB, 160.0f, -1.0f, 400.0f, 0.0f, 180.0f, *(f32 *)(gCurrentAreaConfiguration + 0xC),
                       D_8014DCE8_scene_cobalt_island, 0);
}

extern u8 D_8014E00C_scene_cobalt_island[];
extern f32 D_80151E08_scene_cobalt_island;

/// Spawn the fixed type-eighteen resident for the island scene.
/// \returns Nothing.
void cobaltIslandSpawnResidentType18(void) {
    actorCreateForKind(0x12, 301.0f, 6.66f, 61.0f, 0.0f, D_80151E08_scene_cobalt_island,
                       *(f32 *)(gCurrentAreaConfiguration + 0xC), D_8014E00C_scene_cobalt_island, 0);
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
    ambientAudioCreate(D_80150D80_scene_cobalt_island, 1, 4.0f, 4.0f, 0x2B, 0x2B, 1.0f, 3,
                  (s32)D_80150D80_scene_cobalt_island);
}

u8 *landmarkCreate(f32, f32, f32, s32);
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
        actorCreateForKind(0xE, 220.0f, 41.0f, 111.0f, 0.0f, 0.0f, *(f32 *)(gCurrentAreaConfiguration + 0xC),
                           D_8014E12C_scene_cobalt_island, 0);
    }
    cobaltIslandCreateSceneProps();
    landmarkCreate(220.0f, 31.0f, 111.0f, 0x8F);
}

/// Provide an island-scene callback that requires no action.
/// \returns Nothing.
void cobaltIslandSceneNoOp80134670(void) {
}

extern s32 *D_8018A28C_scene_cobalt_island[16];
/// Look up the island spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 cobaltIslandGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 15.0);
    if (iy < 0 || iy >= 16 || (ix = (s32)((f64)x / 20.0), ix < 0) || ix >= 16 || (iz = (s32)((f64)z / 20.0), iz < 0) ||
        iz >= 16)
        return 0;
    layer = D_8018A28C_scene_cobalt_island[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 16 + ix];
}
