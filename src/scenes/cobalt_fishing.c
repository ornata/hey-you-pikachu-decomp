#include "progress_field_ids.h"
#include "actor_kind.h"
/// Geometry of the cobalt fishing spatial lookup grid.
enum { COBALT_FISHING_GRID_LAYERS = 8, COBALT_FISHING_GRID_SIDE = 8 };
/// Height of one lookup layer in world-coordinate units.
#define COBALT_FISHING_GRID_LAYER_HEIGHT 20.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define COBALT_FISHING_GRID_CELL_WIDTH 20.0

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
    if (progressGetValue(PROGRESS_FIELD_LUCKY_HOOK_UNLOCKED) != 0) {
        offset = 0;
    } else {
        offset = 4;
    }
    roll = (s32)(randomNext() * RANDOM_U32_UNIT_SCALE * 100.0);
    count = 9 - offset;
    /* Keep loop entry together to preserve IDO setup scheduling. */
    // clang-format off
    offset = 0; i = 0; if (count > 0) { do {
            // clang-format on
            if (progressGetValue(PROGRESS_FIELD_LUCKY_HOOK_UNLOCKED) != 0) {
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
        actorCreateForKind(selected, 93.0F, 5.0F, 35.0F, 0, 0, gCurrentAreaConfiguration->objectScale,
                           D_80142E90_scene_cobalt_fishing, residentIndex);
        runtimeCheckResetCheckpoint((const char *)D_80147C94_scene_cobalt_fishing, 1);
    }
}

/// Spawn the fixed type-nine resident for the fishing scene.
/// \returns Nothing.
void cobaltFishingSpawnResidentType9(void) {
    actorCreateForKind(ACTOR_KIND_METAPOD, 93.0F, 5.0F, 30.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_80144CA4_scene_cobalt_fishing, 0x63);
}

/// Spawn the fishing-scene item selected by saved progress.
/// \returns Nothing.
void cobaltFishingSpawnProgressItem(void) {
    if (progressGetValue(PROGRESS_FIELD_LUCKY_HOOK_UNLOCKED) != 0) {
        itemCreate(93.0F, 8.0F, 67.0F, 0.0F, 0.0F, 0x53);
        return;
    }
    itemCreate(93.0F, 8.0F, 67.0F, 0.0F, 0.0F, 0x52);
}

/// Spawn the fixed type-twenty resident for the fishing scene.
/// \returns Nothing.
void cobaltFishingSpawnResidentType20(void) {
    actorCreateForKind(ACTOR_KIND_FISHING_ROD, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_80145198_scene_cobalt_fishing, 0x62);
}

/// Spawn the optional type-eighteen resident for the fishing scene.
/// \returns Nothing.
void cobaltFishingSpawnResidentType18(void) {
    actorCreateForKind(ACTOR_KIND_SLOWPOKE, 110.0F, 11.0F, 79.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_80145390_scene_cobalt_fishing, 0x63);
}

/// Spawn the fixed type-eleven resident for the fishing scene.
/// \returns Nothing.
void cobaltFishingSpawnResidentType11(void) {
    actorCreateForKind(ACTOR_KIND_LAPRAS, 130.0F, 5.25F, 28.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_80145640_scene_cobalt_fishing, 0x63);
}

/// Spawn the fixed type-ten resident for the fishing scene.
/// \returns Nothing.
void cobaltFishingSpawnResidentType10(void) {
    actorCreateForKind(ACTOR_KIND_DIGLETT, 113.0F, 8.0F, 5.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_8014573C_scene_cobalt_fishing, 0x61);
}

/// Create the fishing scene's ambient audio emitter.
/// \returns Nothing.
void cobaltFishingCreateAmbientAudio(void) {
    ambientAudioCreate(&D_801457C0_scene_cobalt_fishing, 1, 7.0F, 3.0F, 0x2B, 0x2B, 1.0F, 1,
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
    if ((randomNext() * RANDOM_U32_UNIT_SCALE) < D_80147CD0_scene_cobalt_fishing) {
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

extern s32 *D_80151708_scene_cobalt_fishing[COBALT_FISHING_GRID_LAYERS];
/// Look up the fishing-scene spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x8013478C in the cobalt fishing overlay.
s32 cobaltFishingGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / COBALT_FISHING_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= COBALT_FISHING_GRID_LAYERS || (ix = (s32)((f64)x / COBALT_FISHING_GRID_CELL_WIDTH), ix < 0) ||
        ix >= COBALT_FISHING_GRID_SIDE || (iz = (s32)((f64)z / COBALT_FISHING_GRID_CELL_WIDTH), iz < 0) ||
        iz >= COBALT_FISHING_GRID_SIDE) {
        return 0;
    }
    layer = D_80151708_scene_cobalt_fishing[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * COBALT_FISHING_GRID_SIDE) + ix];
}
