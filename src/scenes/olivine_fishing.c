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
    if (progressGetValue(0x9E) != 0 && suppressSpecialPool == 0)
        offset = 0;
    else if (progressGetValue(0x1F) != 0)
        offset = 1;
    else
        offset = 2;
    roll = (s32)(randomNext() * 2.3283064365386963e-10 * 100.0);
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
        actorCreateForKind(selected, 90.0f, 3.5f, 40.0f, 0, 0, gCurrentAreaConfiguration->objectScale,
                           D_80142B94_scene_olivine_fishing, residentIndex);
        runtimeCheckResetCheckpoint((const char *)D_80147B00_scene_olivine_fishing, 1);
    }
}

/// Spawn the fixed type-nine resident for the fishing scene.
/// \returns Nothing.
void olivineFishingSpawnResidentType9(void) {
    actorCreateForKind(9, 90.0f, 3.5f, 35.0f, 0.0f, 0.0f, gCurrentAreaConfiguration->objectScale,
                       D_80144920_scene_olivine_fishing, 0x63);
}

/// Spawn the fishing-scene item selected by saved progress.
/// \returns Nothing.
void olivineFishingSpawnProgressItem(void) {
    if (progressGetValue(0x1F) != 0) {
        itemCreate(90.0f, 8.0f, 72.0f, 0.0f, 0.0f, 0x53);
        return;
    }
    itemCreate(90.0f, 8.0f, 72.0f, 0.0f, 0.0f, 0x52);
}

/// Spawn the fixed type-twenty resident for the fishing scene.
/// \returns Nothing.
void olivineFishingSpawnResidentType20(void) {
    actorCreateForKind(0x14, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, gCurrentAreaConfiguration->objectScale,
                       D_80144E14_scene_olivine_fishing, 0x62);
}

/// Spawn the optional type-three resident for the fishing scene.
/// \returns Nothing.
void olivineFishingSpawnResidentType3(void) {
    actorCreateForKind(3, 113.0f, 10.0f, 67.0f, 0.0f, 0.0f, gCurrentAreaConfiguration->objectScale,
                       D_80145088_scene_olivine_fishing, 0x46);
}

/// Spawn the fixed type-seventeen resident for the fishing scene.
/// \returns Nothing.
void olivineFishingSpawnResidentType17(void) {
    actorCreateForKind(0x11, 130.0f, 20.0f, 40.0f, 0.0f, 0.0f, gCurrentAreaConfiguration->objectScale,
                       D_8014549C_scene_olivine_fishing, 0x46);
}

/// Spawn the fixed type-ten resident for the fishing scene.
/// \returns Nothing.
void olivineFishingSpawnResidentType10(void) {
    actorCreateForKind(0xA, 110.0f, 8.0f, 10.0f, 0.0f, 0.0f, gCurrentAreaConfiguration->objectScale,
                       D_80145598_scene_olivine_fishing, 0x61);
}

/// Populate the fishing scene with randomized residents and its fixed residents and item.
/// \returns Nothing.
void olivineFishingInitializeObjects(void) {
    s32 var_s0;
    s32 var_s1;

    var_s0 = 0;
    var_s1 = 0;
    if ((randomNext() * 2.3283064365386963e-10) < 0.5) {
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

extern s32 *D_8014D218_scene_olivine_fishing[8];
/// Look up the fishing-scene spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 olivineFishingGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 20.0);
    if (iy < 0 || iy >= 8 || (ix = (s32)((f64)x / 20.0), ix < 0) || ix >= 8 || (iz = (s32)((f64)z / 20.0), iz < 0) ||
        iz >= 8)
        return 0;
    layer = D_8014D218_scene_olivine_fishing[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 8 + ix];
}
