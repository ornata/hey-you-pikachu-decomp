#include "actor_kind.h"
/// Geometry of the ochre fishing spatial lookup grid.
enum { OCHRE_FISHING_GRID_LAYERS = 8, OCHRE_FISHING_GRID_SIDE = 8 };
/// Height of one lookup layer in world-coordinate units.
#define OCHRE_FISHING_GRID_LAYER_HEIGHT 20.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define OCHRE_FISHING_GRID_CELL_WIDTH 20.0

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
    if (progressGetValue(0x1F) != 0) {
        offset = 0;
    } else {
        offset = 1;
    }
    roll = (s32)(randomNext() * RANDOM_U32_UNIT_SCALE * 100.0);
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
        actorCreateForKind(selected, 54.0F, 8.0F, 72.0F, 0, 0, gCurrentAreaConfiguration->objectScale,
                           D_801430E4_scene_ochre_fishing, residentIndex);
        runtimeCheckResetCheckpoint((const char *)D_801480FC_scene_ochre_fishing, 1);
    }
}

/// Spawn the fixed type-nine resident for the fishing scene.
/// \returns Nothing.
void ochreFishingSpawnResidentType9(void) {
    actorCreateForKind(ACTOR_KIND_METAPOD, 54.0F, 8.0F, 67.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_80144EF4_scene_ochre_fishing, 0x63);
}

/// Spawn the fishing-scene item selected by saved progress.
/// \returns Nothing.
void ochreFishingSpawnProgressItem(void) {
    if (progressGetValue(0x1F) != 0) {
        itemCreate(54.0F, 12.0F, 104.0F, 0.0F, 0.0F, 0x53);
        return;
    }
    itemCreate(54.0F, 12.0F, 104.0F, 0.0F, 0.0F, 0x52);
}

/// Spawn the fixed type-twenty resident for the fishing scene.
/// \returns Nothing.
void ochreFishingSpawnResidentType20(void) {
    actorCreateForKind(ACTOR_KIND_FISHING_ROD, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_801453E8_scene_ochre_fishing, 0x62);
}

/// Spawn the optional type-one resident for the fishing scene.
/// \returns Nothing.
void ochreFishingSpawnResidentType1(void) {
    actorCreateForKind(ACTOR_KIND_BULBASAUR, 64.0F, 12.0F, 45.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_8014565C_scene_ochre_fishing, 0x46);
}

/// Spawn the fixed type-nineteen resident for the fishing scene.
/// \returns Nothing.
void ochreFishingSpawnResidentType19(void) {
    actorCreateForKind(ACTOR_KIND_MAGNEMITE, 104.0F, 24.0F, 72.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_801458F8_scene_ochre_fishing, 0x46);
}

/// Spawn the fixed type-ten resident for the fishing scene.
/// \returns Nothing.
void ochreFishingSpawnResidentType10(void) {
    actorCreateForKind(ACTOR_KIND_DIGLETT, 74.0F, 12.0F, 42.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_801459F4_scene_ochre_fishing, 0x61);
}

/// Create the fishing scene's ambient audio emitter.
/// \returns Nothing.
void ochreFishingCreateAmbientAudio(void) {
    ambientAudioCreate(&D_80145A50_scene_ochre_fishing, 0, 10000.0F, 0.0F, 0x88, 0x88, 1.0F, 1,
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
    if ((randomNext() * RANDOM_U32_UNIT_SCALE) < 0.5) {
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

extern s32 *D_801551B0_scene_ochre_fishing[OCHRE_FISHING_GRID_LAYERS];
/// Look up the fishing-scene spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x80134790 in the ochre fishing overlay.
s32 ochreFishingGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / OCHRE_FISHING_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= OCHRE_FISHING_GRID_LAYERS || (ix = (s32)((f64)x / OCHRE_FISHING_GRID_CELL_WIDTH), ix < 0) ||
        ix >= OCHRE_FISHING_GRID_SIDE || (iz = (s32)((f64)z / OCHRE_FISHING_GRID_CELL_WIDTH), iz < 0) ||
        iz >= OCHRE_FISHING_GRID_SIDE) {
        return 0;
    }
    layer = D_801551B0_scene_ochre_fishing[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * OCHRE_FISHING_GRID_SIDE) + ix];
}
