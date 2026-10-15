#include "door.h"
#include "random.h"
#include "runtime_control.h"
#include "actor_color.h"
#include "common.h"
#include "actor_create.h"
#include "area_maze.h"
#include "scene_types.h"
#include "scene_configuration.h"
void olivineLakeSpawnTintedResidentType12(void *, f32);
extern f32 D_80134ABC_scene_olivine_lake[];
typedef struct {
    s32 count;
    u8 *positions;
} LakeGroup;
extern LakeGroup D_80134B70_scene_olivine_lake[];
extern u8 D_801404AC_scene_olivine_lake;
extern u8 D_80140854_scene_olivine_lake;
extern u8 D_801412B4_scene_olivine_lake;
extern s32 D_80141624_scene_olivine_lake[2];
extern u8 *D_8014162C_scene_olivine_lake[];
extern u8 *D_80141638_scene_olivine_lake[];
extern s32 D_80141644_scene_olivine_lake;
extern u8 D_80141648_scene_olivine_lake[];
extern f32 D_8014166C_scene_olivine_lake[];
extern f32 D_80141678_scene_olivine_lake[];
extern char D_80142304_scene_olivine_lake[];

/// Create a lake prop of the selected style at the requested position.
/// \param position Three-component world-space position.
/// \param style Prop-style index selecting the appearance and behavior.
/// \returns Nothing.
void olivineLakeCreateProp(u8 *position, s32 style) {
    mazeReplaceBlock(position, D_8014162C_scene_olivine_lake[style], (MazeGrid *)D_80141638_scene_olivine_lake[style],
                     0);
}

/// Populate the selected lake prop groups with randomized styles.
/// \returns Nothing.
void olivineLakeCreateRandomizedGroupProps(void) {
    s32 *selection;
    s32 count;
    s32 offset;
    s32 index;
    s32 style;

    for (selection = D_80141624_scene_olivine_lake, style = 1; style != 3; style++, selection++) {
        index = 0;
        count = D_80134B70_scene_olivine_lake[*selection].count;
        offset = 0;
        if (count > 0) {
            do {
                if (0.3 < randomNext() * 2.3283064365386963e-10) {
                    olivineLakeCreateProp(D_80134B70_scene_olivine_lake[*selection].positions + offset, 0);
                } else {
                    olivineLakeCreateProp(D_80134B70_scene_olivine_lake[*selection].positions + offset, style);
                }
                index++;
                offset += 12;
            } while (index != count);
        }
    }
}

typedef struct SpawnPair {
    s32 group, position;
} SpawnPair;
extern SpawnPair D_80141578_scene_olivine_lake[3][7];
extern s32 D_80141620_scene_olivine_lake[];
/// Create one of the lake scene prop arrangements.
/// \returns Nothing.
void olivineLakeCreatePropArrangement(void) {
    s32 randomRow;
    s32 i;
    s32 group;
    s32 point;
    f32 position[3];
    f32 *p;
    randomRow = randomBelow(3);
    for (i = 0; i < 7; i++) {
        group = D_80141578_scene_olivine_lake[randomRow][i].group;
        point = D_80141578_scene_olivine_lake[randomRow][i].position;
        if (group < 0) {
            continue;
        }
        p = ((f32(*)[3])D_80134B70_scene_olivine_lake[D_80141620_scene_olivine_lake[group]].positions)[point];
        position[0] = p[0];
        position[1] = p[1];
        position[2] = p[2];
        olivineLakeCreateProp((u8 *)position, group);
    }
}

/// Spawn a type-twelve lake resident with the next scene color.
/// \param position Three-component world-space position.
/// \param heading Resident heading in degrees.
/// \returns Nothing.
void olivineLakeSpawnTintedResidentType12(void *position, f32 heading) {
    s32 index;
    u8 *object;
    u8 *color;
    s32 *counter;

    counter = &D_80141644_scene_olivine_lake;
    object = actorCreateForKind(0xC, ((f32 *)position)[0], ((f32 *)position)[1], ((f32 *)position)[2], 0.0F, heading,
                                gCurrentAreaConfiguration->objectScale, &D_801404AC_scene_olivine_lake, *counter);
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
    color = (u8 *)D_80141648_scene_olivine_lake + (index * 4);
    actorTransitionModelEnvironmentColor(object, color[0], color[1], color[2], color[3], 0.5F);
    (*counter)++;
}

/// Spawn a type-twelve lake resident with a random heading and scene color.
/// \param position Three-component world-space position.
/// \returns Nothing.
void olivineLakeSpawnRandomHeadingResidentType12(void *position) {
    olivineLakeSpawnTintedResidentType12(position, (f32)(randomNext() * 2.3283064365386963e-10 * 360.0));
}

/// Spawn a type-thirteen lake resident at the requested position and heading.
/// \param position Three-component world-space position.
/// \param heading Resident heading in degrees.
/// \returns Nothing.
void olivineLakeSpawnResidentType13(f32 *position, f32 heading) {
    actorCreateForKind(0xD, position[0], position[1], position[2], 0.0F, heading,
                       gCurrentAreaConfiguration->objectScale, &D_80140854_scene_olivine_lake, 0);
}

/// Spawn a type-seventeen lake resident at the requested position.
/// \param position Three-component world-space position.
/// \returns Nothing.
void olivineLakeSpawnResidentType17(f32 *position) {
    actorCreateForKind(0x11, position[0], position[1], position[2], 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       &D_801412B4_scene_olivine_lake, 0);
}

/// Spawn the lake resident group at randomized scene positions.
/// \returns Nothing.
void olivineLakeSpawnRandomResidentGroup(void) {
    s32 i;
    s32 chosen;
    /* Original code clears this table but never marks a selected position. */
    s32 occupied[15];
    s32 randomIndex;
    for (i = 0; i < 15; i++) {
        occupied[i] = 0;
    }
    for (i = 0; i != 5; i++) {
        randomIndex = randomBelow(15);
        chosen = randomIndex;
        while (occupied[chosen] != 0) {
            chosen++;
            while (chosen < 0) {
                chosen += 15;
            }
            while (chosen >= 15) {
                chosen -= 15;
            }
        }
        olivineLakeSpawnRandomHeadingResidentType12((f32 *)((u8 *)D_80134ABC_scene_olivine_lake + (chosen * 12)));
    }
}

/// Provide the lake-scene callback that requires no action.
/// \returns Nothing.
void olivineLakeSceneNoOp(void) {
}
typedef struct {
    f32 x, y, z;
} Vec3;
/// Set up lake props, residents, and scene events.
/// \returns Nothing.
void olivineLakeInitializeObjects(void) {
    Vec3 a = *(Vec3 *)D_8014166C_scene_olivine_lake;
    Vec3 b = *(Vec3 *)D_80141678_scene_olivine_lake;
    doorRegistryReset();
    olivineLakeCreateRandomizedGroupProps();
    olivineLakeCreatePropArrangement();
    runtimeCheckResetCheckpoint((const char *)D_80142304_scene_olivine_lake, 1);
    olivineLakeSpawnResidentType13((f32 *)&a, 227.0F);
    olivineLakeSpawnResidentType17((f32 *)&b);
    olivineLakeSpawnRandomResidentGroup();
}
extern s32 *D_80150ED4_scene_olivine_lake[32];
/// Look up the lake spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 olivineLakeGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 5.0);
    if (iy < 0 || iy >= 32 || (ix = (s32)((f64)x / 10.0), ix < 0) || ix >= 32 || (iz = (s32)((f64)z / 10.0), iz < 0) ||
        iz >= 32) {
        return 0;
    }
    layer = D_80150ED4_scene_olivine_lake[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * 32) + ix];
}
