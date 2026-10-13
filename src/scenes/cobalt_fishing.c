#include "runtime_control.h"
#include "ambient_audio.h"
#include "progress_value.h"
#include "common.h"
#include "item_object.h"
#include "actor_create.h"
#include "scene_types.h"
#include "fishing_population.h"
u32 randomNext(void);
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_80142E90_scene_cobalt_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_80144CA4_scene_cobalt_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_80145198_scene_cobalt_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_80145390_scene_cobalt_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_80145640_scene_cobalt_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_8014573C_scene_cobalt_fishing[];
extern FishingResidentChoice D_80145750_scene_cobalt_fishing[9];
extern FishingResidentChoice D_80145798_scene_cobalt_fishing[5];
extern f32 D_801457C0_scene_cobalt_fishing;
/// Checkpoint label stored in scene ROM data.
extern char D_80147C94_scene_cobalt_fishing[];
extern f64 D_80147CD0_scene_cobalt_fishing;

/// Choose and spawn a fishing resident using the population pool enabled by saved progress.
/// \param residentIndex Identifier assigned to the spawned resident within the scene population.
/// \returns Nothing.
void cobaltFishingSpawnRandomResident(s32 residentIndex) {
    s32 selected;
    s32 count;
    s32 offset;
    s32 i;
    s32 threshold;
    s32 kind;
    s32 roll;
    if (progressGetValue(0x1F) != 0)
        offset = 0;
    else
        offset = 4;
    roll = (s32)(randomNext() * 2.3283064365386963e-10 * 100.0);
    count = 9 - offset;
    /* Keep loop entry together to preserve IDO setup scheduling. */
    // clang-format off
    offset = 0; i = 0; if (count > 0) { do {
            // clang-format on
            if (progressGetValue(0x1F) != 0) {
                threshold = ((FishingResidentChoice *)((u8 *)&D_80145750_scene_cobalt_fishing + offset))->threshold;
                kind = ((FishingResidentChoice *)((u8 *)&D_80145750_scene_cobalt_fishing + offset))->kind;
            } else {
                threshold = ((FishingResidentChoice *)((u8 *)&D_80145798_scene_cobalt_fishing + offset))->threshold;
                kind = ((FishingResidentChoice *)((u8 *)&D_80145798_scene_cobalt_fishing + offset))->kind;
            }
            i++;
            if (threshold > roll) {
                selected = kind;
                break;
            }
            offset += 8;
        } while (i != count);
    }
    if (selected != -1) {
        actorCreateForKind(selected, 93.0f, 5.0f, 35.0f, 0, 0,
                           ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                           D_80142E90_scene_cobalt_fishing, residentIndex);
        runtimeCheckResetCheckpoint((const char *)D_80147C94_scene_cobalt_fishing, 1);
    }
}

/// Spawn the fixed type-nine resident for the fishing scene.
/// \returns Nothing.
void cobaltFishingSpawnResidentType9(void) {
    actorCreateForKind(9, 93.0f, 5.0f, 30.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       D_80144CA4_scene_cobalt_fishing, 0x63);
}

/// Spawn the fishing-scene item selected by saved progress.
/// \returns Nothing.
void cobaltFishingSpawnProgressItem(void) {
    if (progressGetValue(0x1F) != 0) {
        itemCreate(93.0f, 8.0f, 67.0f, 0.0f, 0.0f, 0x53);
        return;
    }
    itemCreate(93.0f, 8.0f, 67.0f, 0.0f, 0.0f, 0x52);
}

/// Spawn the fixed type-twenty resident for the fishing scene.
/// \returns Nothing.
void cobaltFishingSpawnResidentType20(void) {
    actorCreateForKind(0x14, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       D_80145198_scene_cobalt_fishing, 0x62);
}

/// Spawn the optional type-eighteen resident for the fishing scene.
/// \returns Nothing.
void cobaltFishingSpawnResidentType18(void) {
    actorCreateForKind(0x12, 110.0f, 11.0f, 79.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       D_80145390_scene_cobalt_fishing, 0x63);
}

/// Spawn the fixed type-eleven resident for the fishing scene.
/// \returns Nothing.
void cobaltFishingSpawnResidentType11(void) {
    actorCreateForKind(0xB, 130.0f, 5.25f, 28.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       D_80145640_scene_cobalt_fishing, 0x63);
}

/// Spawn the fixed type-ten resident for the fishing scene.
/// \returns Nothing.
void cobaltFishingSpawnResidentType10(void) {
    actorCreateForKind(0xA, 113.0f, 8.0f, 5.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       D_8014573C_scene_cobalt_fishing, 0x61);
}

/// Create the fishing scene's ambient audio emitter.
/// \returns Nothing.
void cobaltFishingCreateAmbientAudio(void) {
    ambientAudioCreate(&D_801457C0_scene_cobalt_fishing, 1, 7.0f, 3.0f, 0x2B, 0x2B, 1.0f, 1,
                  (Vec3f *)&D_801457C0_scene_cobalt_fishing);
}

/// Populate the fishing scene with randomized residents, fixed residents, an item, and ambient audio.
/// \returns Nothing.
void cobaltFishingInitializeObjects(void) {
    s32 var_s0;

    var_s0 = 0;
    do {
        cobaltFishingSpawnRandomResident(var_s0);
        var_s0 += 1;
    } while (var_s0 != 0xA);
    cobaltFishingSpawnResidentType9();
    cobaltFishingSpawnProgressItem();
    cobaltFishingSpawnResidentType20();
    if ((randomNext() * 2.3283064365386963e-10) < D_80147CD0_scene_cobalt_fishing) {
        cobaltFishingSpawnResidentType18();
    }
    cobaltFishingSpawnResidentType11();
    cobaltFishingSpawnResidentType10();
    cobaltFishingCreateAmbientAudio();
}

/// Provide the fishing-scene callback that requires no action.
/// \returns Nothing.
void cobaltFishingSceneNoOp(void) {
}

extern s32 *D_80151708_scene_cobalt_fishing[8];
/// Look up the fishing-scene spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 cobaltFishingGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 20.0);
    if (iy < 0 || iy >= 8 || (ix = (s32)((f64)x / 20.0), ix < 0) || ix >= 8 || (iz = (s32)((f64)z / 20.0), iz < 0) ||
        iz >= 8)
        return 0;
    layer = D_80151708_scene_cobalt_fishing[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 8 + ix];
}
