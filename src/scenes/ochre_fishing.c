#include "ambient_audio.h"
#include "progress_value.h"
#include "common.h"
#include "item_object.h"
#include "actor_create.h"
#include "scene_types.h"
#include "fishing_population.h"
u32 randomNext(void);
void runtimeCheckResetCheckpoint(s32, s32);
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_801430E4_scene_ochre_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_80144EF4_scene_ochre_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_801453E8_scene_ochre_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_8014565C_scene_ochre_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_801458F8_scene_ochre_fishing[];
/// Packed actor-script data borrowed from the scene ROM.
extern u8 D_801459F4_scene_ochre_fishing[];
extern FishingResidentChoice D_80145A08_scene_ochre_fishing[5];
extern FishingResidentChoice D_80145A30_scene_ochre_fishing[4];
extern f32 D_80145A50_scene_ochre_fishing;
/// Checkpoint label stored in scene ROM data.
extern char D_801480FC_scene_ochre_fishing[];

/// Choose and spawn a fishing resident using the population pool enabled by saved progress.
/// \param residentIndex Identifier assigned to the spawned resident within the scene population.
/// \returns Nothing.
void ochreFishingSpawnRandomResident(s32 residentIndex) {
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
        offset = 1;
    roll = (s32)(randomNext() * 2.3283064365386963e-10 * 100.0);
    count = 5 - offset;
    /* Keep loop entry together to preserve IDO setup scheduling. */
    // clang-format off
    offset = 0; i = 0; if (count > 0) { do {
            // clang-format on
            if (progressGetValue(0x1F) != 0) {
                threshold = ((FishingResidentChoice *)((u8 *)&D_80145A08_scene_ochre_fishing + offset))->threshold;
                kind = ((FishingResidentChoice *)((u8 *)&D_80145A08_scene_ochre_fishing + offset))->kind;
            } else {
                threshold = ((FishingResidentChoice *)((u8 *)&D_80145A30_scene_ochre_fishing + offset))->threshold;
                kind = ((FishingResidentChoice *)((u8 *)&D_80145A30_scene_ochre_fishing + offset))->kind;
            }
            i++;
            if (threshold >= roll) {
                selected = kind;
                break;
            }
            offset += 8;
        } while (i != count);
    }
    if (selected != -1) {
        actorCreateForKind(selected, 54.0f, 8.0f, 72.0f, 0, 0,
                           ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                           D_801430E4_scene_ochre_fishing, residentIndex);
        runtimeCheckResetCheckpoint((s32)D_801480FC_scene_ochre_fishing, 1);
    }
}

/// Spawn the fixed type-nine resident for the fishing scene.
/// \returns Nothing.
void ochreFishingSpawnResidentType9(void) {
    actorCreateForKind(9, 54.0f, 8.0f, 67.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       D_80144EF4_scene_ochre_fishing, 0x63);
}

/// Spawn the fishing-scene item selected by saved progress.
/// \returns Nothing.
void ochreFishingSpawnProgressItem(void) {
    if (progressGetValue(0x1F) != 0) {
        itemCreate(54.0f, 12.0f, 104.0f, 0.0f, 0.0f, 0x53);
        return;
    }
    itemCreate(54.0f, 12.0f, 104.0f, 0.0f, 0.0f, 0x52);
}

/// Spawn the fixed type-twenty resident for the fishing scene.
/// \returns Nothing.
void ochreFishingSpawnResidentType20(void) {
    actorCreateForKind(0x14, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       D_801453E8_scene_ochre_fishing, 0x62);
}

/// Spawn the optional type-one resident for the fishing scene.
/// \returns Nothing.
void ochreFishingSpawnResidentType1(void) {
    actorCreateForKind(1, 64.0f, 12.0f, 45.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       D_8014565C_scene_ochre_fishing, 0x46);
}

/// Spawn the fixed type-nineteen resident for the fishing scene.
/// \returns Nothing.
void ochreFishingSpawnResidentType19(void) {
    actorCreateForKind(0x13, 104.0f, 24.0f, 72.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       D_801458F8_scene_ochre_fishing, 0x46);
}

/// Spawn the fixed type-ten resident for the fishing scene.
/// \returns Nothing.
void ochreFishingSpawnResidentType10(void) {
    actorCreateForKind(0xA, 74.0f, 12.0f, 42.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       D_801459F4_scene_ochre_fishing, 0x61);
}

/// Create the fishing scene's ambient audio emitter.
/// \returns Nothing.
void ochreFishingCreateAmbientAudio(void) {
    ambientAudioCreate(&D_80145A50_scene_ochre_fishing, 0, 10000.0f, 0.0f, 0x88, 0x88, 1.0f, 1,
                  (Vec3f *)&D_80145A50_scene_ochre_fishing);
}

/// Populate the fishing scene with randomized residents, fixed residents, an item, and ambient audio.
/// \returns Nothing.
void ochreFishingInitializeObjects(void) {
    s32 var_s0;

    var_s0 = 0;
    do {
        ochreFishingSpawnRandomResident(var_s0);
        var_s0 += 1;
    } while (var_s0 != 0xA);
    ochreFishingSpawnResidentType9();
    ochreFishingSpawnProgressItem();
    ochreFishingSpawnResidentType20();
    if ((randomNext() * 2.3283064365386963e-10) < 0.5) {
        ochreFishingSpawnResidentType1();
    }
    ochreFishingSpawnResidentType19();
    ochreFishingSpawnResidentType10();
    ochreFishingCreateAmbientAudio();
}
/// Provide the fishing-scene callback that requires no action.
/// \returns Nothing.
void ochreFishingSceneNoOp(void) {
}

extern s32 *D_801551B0_scene_ochre_fishing[8];
/// Look up the fishing-scene spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 ochreFishingGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 20.0);
    if (iy < 0 || iy >= 8 || (ix = (s32)((f64)x / 20.0), ix < 0) || ix >= 8 || (iz = (s32)((f64)z / 20.0), iz < 0) ||
        iz >= 8)
        return 0;
    layer = D_801551B0_scene_ochre_fishing[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 8 + ix];
}
