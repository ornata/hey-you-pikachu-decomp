#include "actor_kind.h"
/// Geometry of the olivine fishing spatial lookup grid.
enum { OLIVINE_FISHING_GRID_LAYERS = 8, OLIVINE_FISHING_GRID_SIDE = 8 };
/// Height of one lookup layer in world-coordinate units.
#define OLIVINE_FISHING_GRID_LAYER_HEIGHT 20.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define OLIVINE_FISHING_GRID_CELL_WIDTH 20.0

#include "random_scale.h"
#include "random.h"
#include "runtime_control.h"
#include "ambient_audio.h"
#include "progress_value.h"
#include "common.h"
#include "item_object.h"
#include "actor_create.h"
#include "scene_types.h"
#include "fishing_population.h"
#include "scene_configuration.h"
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_80142B94_scene_olivine_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_80144920_scene_olivine_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_80144E14_scene_olivine_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_80145088_scene_olivine_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_8014549C_scene_olivine_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_80145598_scene_olivine_fishing[];
extern FishingResidentChoice D_801455AC_scene_olivine_fishing[6];
extern FishingResidentChoice D_801455DC_scene_olivine_fishing[5];
extern FishingResidentChoice D_80145604_scene_olivine_fishing[7];
/// Checkpoint label stored in scene ROM data.
extern char D_80147B00_scene_olivine_fishing[];

/// Choose and spawn a fishing resident using saved progress and the selected population pool.
/// \param residentIndex Identifier assigned to the spawned resident within the scene population.
/// \param suppressSpecialPool Nonzero disables the special population pool even when saved progress enables it.
/// \returns Nothing.
void olivineFishingSpawnRandomResident(s32 residentIndex, s32 suppressSpecialPool) {
    s32 selected;
    s32 count;
    s32 offset;
    s32 i;
    s32 threshold;
    s32 kind;
    s32 roll;
    if (progressGetValue(0x9E) != 0 && suppressSpecialPool == 0) {
        offset = 0;
    } else if (progressGetValue(0x1F) != 0) {
        offset = 1;
    } else {
        offset = 2;
    }
    roll = (s32)(randomNext() * RANDOM_U32_UNIT_SCALE * 100.0);
    count = 7 - offset;
    /* Keep loop entry together to preserve IDO setup scheduling. */
    // clang-format off
    offset = 0; i = 0; if (count > 0) { do {
            // clang-format on
            if (progressGetValue(0x9E) != 0 && suppressSpecialPool == 0) {
                threshold = ((FishingResidentChoice *)((u8 *)&D_80145604_scene_olivine_fishing + offset))->threshold;
                kind = ((FishingResidentChoice *)((u8 *)&D_80145604_scene_olivine_fishing + offset))->kind;
            } else if (progressGetValue(0x1F) != 0) {
                threshold = ((FishingResidentChoice *)((u8 *)&D_801455AC_scene_olivine_fishing + offset))->threshold;
                kind = ((FishingResidentChoice *)((u8 *)&D_801455AC_scene_olivine_fishing + offset))->kind;
            } else {
                threshold = ((FishingResidentChoice *)((u8 *)&D_801455DC_scene_olivine_fishing + offset))->threshold;
                kind = ((FishingResidentChoice *)((u8 *)&D_801455DC_scene_olivine_fishing + offset))->kind;
            }
            if (threshold > roll) {
                selected = kind;
                break;
            }
            i++;
            offset += 8;
        } while (i != count);
    }
    if (selected != -1) {
        actorCreateForKind(selected, 90.0F, 3.5F, 40.0F, 0, 0, gCurrentAreaConfiguration->objectScale,
                           D_80142B94_scene_olivine_fishing, residentIndex);
        runtimeCheckResetCheckpoint((const char *)D_80147B00_scene_olivine_fishing, 1);
    }
}

/// Spawn the fixed type-nine resident for the fishing scene.
/// \returns Nothing.
void olivineFishingSpawnResidentType9(void) {
    actorCreateForKind(ACTOR_KIND_METAPOD, 90.0F, 3.5F, 35.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_80144920_scene_olivine_fishing, 0x63);
}

/// Spawn the fishing-scene item selected by saved progress.
/// \returns Nothing.
void olivineFishingSpawnProgressItem(void) {
    if (progressGetValue(0x1F) != 0) {
        itemCreate(90.0F, 8.0F, 72.0F, 0.0F, 0.0F, 0x53);
        return;
    }
    itemCreate(90.0F, 8.0F, 72.0F, 0.0F, 0.0F, 0x52);
}

/// Spawn the fixed type-twenty resident for the fishing scene.
/// \returns Nothing.
void olivineFishingSpawnResidentType20(void) {
    actorCreateForKind(ACTOR_KIND_FISHING_ROD, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_80144E14_scene_olivine_fishing, 0x62);
}

/// Spawn the optional type-three resident for the fishing scene.
/// \returns Nothing.
void olivineFishingSpawnResidentType3(void) {
    actorCreateForKind(ACTOR_KIND_SQUIRTLE, 113.0F, 10.0F, 67.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_80145088_scene_olivine_fishing, 0x46);
}

/// Spawn the fixed type-seventeen resident for the fishing scene.
/// \returns Nothing.
void olivineFishingSpawnResidentType17(void) {
    actorCreateForKind(ACTOR_KIND_HAUNTER, 130.0F, 20.0F, 40.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_8014549C_scene_olivine_fishing, 0x46);
}

/// Spawn the fixed type-ten resident for the fishing scene.
/// \returns Nothing.
void olivineFishingSpawnResidentType10(void) {
    actorCreateForKind(ACTOR_KIND_DIGLETT, 110.0F, 8.0F, 10.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_80145598_scene_olivine_fishing, 0x61);
}

/// Populate the fishing scene with randomized residents and its fixed residents and item.
/// \returns Nothing.
void olivineFishingInitializeObjects(void) {
    s32 var_s0;
    s32 var_s1;

    var_s0 = 0;
    var_s1 = 0;
    if ((randomNext() * RANDOM_U32_UNIT_SCALE) < 0.5) {
        var_s1 = 1;
    }
    do {
        olivineFishingSpawnRandomResident(var_s0, var_s1);
        var_s0 += 1;
    } while (var_s0 < 0xA);
    olivineFishingSpawnResidentType9();
    olivineFishingSpawnProgressItem();
    olivineFishingSpawnResidentType20();
    if (var_s1 != 0) {
        olivineFishingSpawnResidentType3();
    }
    olivineFishingSpawnResidentType17();
    olivineFishingSpawnResidentType10();
}

/// Provide the fishing-scene callback that requires no action.
/// \returns Nothing.
void olivineFishingSceneNoOp(void) {
}

extern s32 *D_8014D218_scene_olivine_fishing[OLIVINE_FISHING_GRID_LAYERS];
/// Look up the fishing-scene spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x801347A0 in the olivine fishing overlay.
s32 olivineFishingGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / OLIVINE_FISHING_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= OLIVINE_FISHING_GRID_LAYERS || (ix = (s32)((f64)x / OLIVINE_FISHING_GRID_CELL_WIDTH), ix < 0) ||
        ix >= OLIVINE_FISHING_GRID_SIDE || (iz = (s32)((f64)z / OLIVINE_FISHING_GRID_CELL_WIDTH), iz < 0) ||
        iz >= OLIVINE_FISHING_GRID_SIDE) {
        return 0;
    }
    layer = D_8014D218_scene_olivine_fishing[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * OLIVINE_FISHING_GRID_SIDE) + ix];
}
