#include "door.h"
#include "random.h"
#include "runtime_control.h"
#include "actor_color.h"
#include "common.h"
#include "actor_create.h"
#include "area_maze.h"
#include "scene_types.h"
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
void lakeshorePlaceSceneProp(u8 *, s32);
void lakeshoreSpawnColoredResident(f32 *, f32);
extern u8 *D_8014142C_scene_olivine_lakeshore[];
extern u8 *D_80141438_scene_olivine_lakeshore[];
typedef struct {
    s32 count;
    u8 *positions;
} SpawnGroup;
extern SpawnGroup D_8013495C_scene_olivine_lakeshore[];
extern s32 D_80141424_scene_olivine_lakeshore[];
extern u8 D_80141448_scene_olivine_lakeshore[][4];
extern f32 D_8013489C_scene_olivine_lakeshore[];
extern u8 D_801402D0_scene_olivine_lakeshore;
extern u8 D_8014069C_scene_olivine_lakeshore;
extern u8 D_801410FC_scene_olivine_lakeshore;
extern s32 D_80141444_scene_olivine_lakeshore;
extern f32 D_8014145C_scene_olivine_lakeshore[];
extern f32 D_80141468_scene_olivine_lakeshore[];
extern f32 D_80141474_scene_olivine_lakeshore[];
extern char D_801420A4_scene_olivine_lakeshore[];
extern f64 D_801420E8_scene_olivine_lakeshore;

/// Place a lakeshore prop using the selected scene style.
/// \param position Three-component scene position.
/// \param style Valid index into the scene prop-style tables.
/// \returns Nothing.
void lakeshorePlaceSceneProp(u8 *position, s32 style) {
    mazeReplaceBlock(position, D_8014142C_scene_olivine_lakeshore[style],
                     (MazeGrid *)D_80141438_scene_olivine_lakeshore[style], 0);
}

/// Provide a lakeshore scene callback that requires no action.
/// \returns Nothing.
void lakeshoreSceneNoOp342B0(void) {
}
/// Provide a second lakeshore scene callback that requires no action.
/// \returns Nothing.
void lakeshoreSceneNoOp342B8(void) {
}
/// Place props for the selected lakeshore spawn groups.
/// \returns Nothing.
void lakeshoreInitializeProps(void) {
    s32 count;
    s32 index;
    s32 offset;
    s32 style;
    s32 *selection;

    for (style = 1, selection = D_80141424_scene_olivine_lakeshore; style != 3; style++, selection++) {
        index = 0;
        count = D_8013495C_scene_olivine_lakeshore[*selection].count;
        offset = 0;
        if (count > 0) {
            do {
                lakeshorePlaceSceneProp(&D_8013495C_scene_olivine_lakeshore[*selection].positions[offset], style);
                index += 1;
                offset += 0xC;
            } while (index != count);
        }
    }
}

/// Spawn a lakeshore resident with the next color in the scene palette.
/// \param position Three-component scene position.
/// \param heading Initial heading in degrees.
/// \returns Nothing.
void lakeshoreSpawnColoredResident(f32 *position, f32 heading) {
    s32 index;
    u8 *object;
    u8 *color;
    s32 *counter;

    counter = &D_80141444_scene_olivine_lakeshore;
    object = actorCreateForKind(0xC, position[0], position[1], position[2], 0.0f, heading,
                                ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                                &D_801402D0_scene_olivine_lakeshore, *counter);
    index = *counter;
    if (*counter < 0) {
        do {
            index += 5;
        } while (index < 0);
    }
    if (index >= 5) {
        do {
            index -= 5;
        } while (index >= 5);
    }
    color = D_80141448_scene_olivine_lakeshore[index];
    actorTransitionModelEnvironmentColor(object, color[0], color[1], color[2], color[3], 0.5f);
    (*counter)++;
}

/// Spawn a colored lakeshore resident with a randomized heading.
/// \param position Three-component scene position.
/// \returns Nothing.
void lakeshoreSpawnColoredResidentRandomHeading(f32 *position) {
    lakeshoreSpawnColoredResident(position,
                                  (f32)(randomNext() * 2.3283064365386963e-10 * D_801420E8_scene_olivine_lakeshore));
}

/// Spawn the lakeshore type-three resident at a requested position and heading.
/// \param position Three-component scene position.
/// \param heading Initial heading in degrees.
/// \returns Nothing.
void lakeshoreSpawnResidentType3(f32 *position, f32 heading) {
    actorCreateForKind(3, position[0], position[1], position[2], 0.0f, heading,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       &D_8014069C_scene_olivine_lakeshore, 0);
}

/// Spawn the lakeshore type-seventeen resident at a requested position.
/// \param position Three-component scene position.
/// \returns Nothing.
void lakeshoreSpawnResidentType17(f32 *position) {
    actorCreateForKind(0x11, position[0], position[1], position[2], 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       &D_801410FC_scene_olivine_lakeshore, 0);
}

/// Spawn four colored residents at randomly selected scene positions, allowing repeated positions.
/// \returns Nothing.
void lakeshoreSpawnRandomResidents(void) {
    s32 i;
    s32 chosen;
    /* Original code clears this table but never marks a selected position. */
    s32 occupied[16];
    for (i = 0; i < 16; i++)
        occupied[i] = 0;
    for (i = 0; i != 4; i++) {
        chosen = randomBelow(16);
        while (occupied[chosen] != 0) {
            chosen++;
            while (chosen < 0)
                chosen += 16;
            while (chosen >= 16)
                chosen -= 16;
        }
        lakeshoreSpawnColoredResidentRandomHeading((f32 *)((u8 *)D_8013489C_scene_olivine_lakeshore + chosen * 12));
    }
}

/// Initialize lakeshore props and resident creatures.
/// \returns Nothing.
void lakeshoreInitializeObjects(void) {
    doorRegistryReset();
    lakeshoreInitializeProps();
    runtimeCheckResetCheckpoint((const char *)D_801420A4_scene_olivine_lakeshore, 1);
    lakeshoreSpawnResidentType3(D_8014145C_scene_olivine_lakeshore, 310.0f);
    lakeshoreSpawnRandomResidents();
    lakeshoreSpawnResidentType17(D_80141474_scene_olivine_lakeshore);
    lakeshoreSpawnColoredResident(D_80141468_scene_olivine_lakeshore, 10.0f);
}
extern s32 *D_801550F4_scene_olivine_lakeshore[32];
/// Look up the lakeshore spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 lakeshoreGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 5.0);
    if (iy < 0 || iy >= 32 || (ix = (s32)((f64)x / 10.0), ix < 0) || ix >= 32 || (iz = (s32)((f64)z / 10.0), iz < 0) ||
        iz >= 32)
        return 0;
    layer = D_801550F4_scene_olivine_lakeshore[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 32 + ix];
}
