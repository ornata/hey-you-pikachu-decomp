/// Geometry of the springleaf entrance spatial lookup grid.
enum { SPRINGLEAF_ENTRANCE_GRID_LAYERS = 32, SPRINGLEAF_ENTRANCE_GRID_SIDE = 32 };
/// Height of one lookup layer in world-coordinate units.
#define SPRINGLEAF_ENTRANCE_GRID_LAYER_HEIGHT 5.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define SPRINGLEAF_ENTRANCE_GRID_CELL_WIDTH 10.0

#include "random_scale.h"
#include "math_scalar.h"
#include "random.h"
#include "runtime_control.h"
#include "maze_scatter.h"
#include "story_chapter.h"
#include "progress_fields.h"
#include "landmark.h"
#include "object_lifecycle.h"
#include "common.h"
#include "item_object.h"
#include "actor_create.h"
#include "scene_types.h"

/// Mutable weighted item-kind choices retained by the scene overlay.
/// \note Original identifier: \c D_80157660_scene_springleaf_entrance (0x80157660).
extern WeightedChoiceTable D_80157660_scene_springleaf_entrance;
/// Borrowed world-position triples for scene item scattering.
/// \note Original identifier: \c D_80157668_scene_springleaf_entrance (0x80157668).
extern f32 D_80157668_scene_springleaf_entrance[];

#include "scene_configuration.h"

extern u8 D_8014DF74_scene_springleaf_entrance;
extern u8 D_80156F04_scene_springleaf_entrance;
extern f32 D_80158E30_scene_springleaf_entrance;
extern f64 D_80158E38_scene_springleaf_entrance;
extern f32 D_80158E40_scene_springleaf_entrance;
extern f64 D_80158E48_scene_springleaf_entrance;
extern f64 D_80158E50_scene_springleaf_entrance;
extern f64 D_80158E58_scene_springleaf_entrance;
extern u8 D_8014E2B0_scene_springleaf_entrance;
extern u8 D_801573B8_scene_springleaf_entrance;

/// Spawn an entrance resident according to its saved growth state.
/// \param position Three-component scene position.
/// \param residentIndex Resident growth-slot index from zero through four.
/// \returns Nothing.
void springleafEntranceSpawnGrowthResident(f32 *position, s32 residentIndex) {
    f32 scale;
    s32 state;
    u8 *record;
    u32 random;

    state = progressGetValue(residentIndex + 0x65);
    if (progressGetValue(0x12) == 0) {
        record = &D_8014DF74_scene_springleaf_entrance;
    } else {
        record = &D_80156F04_scene_springleaf_entrance;
    }
    if (state < 2) {
        /* Keep random sampling and scale selection together for IDO scheduling. */
        // clang-format off
        random = randomNext(); scale = D_80158E30_scene_springleaf_entrance;
        // clang-format on
        actorCreateForKind(5, position[0], position[1], position[2], 0.0F,
                           (f32)((f64)random * RANDOM_U32_UNIT_SCALE * D_80158E38_scene_springleaf_entrance),
                           mathPowInteger(scale, state) * gCurrentAreaConfiguration->objectScale, record,
                           residentIndex);
        return;
    }
    if (state < 4) {
        // clang-format off
        random = randomNext(); scale = D_80158E40_scene_springleaf_entrance;
        // clang-format on
        actorCreateForKind(6, position[0], position[1], position[2], 0.0F,
                           (f32)((f64)random * RANDOM_U32_UNIT_SCALE * D_80158E48_scene_springleaf_entrance),
                           mathPowInteger(scale, state - 2) * gCurrentAreaConfiguration->objectScale, record,
                           residentIndex);
        return;
    }
    random = randomNext();
    actorCreateForKind(7, position[0], position[1], position[2], 0.0F,
                       (f32)((f64)random * RANDOM_U32_UNIT_SCALE * D_80158E50_scene_springleaf_entrance),
                       gCurrentAreaConfiguration->objectScale, record, residentIndex);
}

/// Prepare scene resources for evolved entrance residents.
/// \returns Nothing.
void springleafEntrancePrepareEvolvedResidents(void) {
    objectRequestHierarchyDestruction(actorCreateForKind(6, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, NULL, 0));
    objectRequestHierarchyDestruction(actorCreateForKind(7, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, NULL, 0));
}

/// Spawn an entrance item with a randomized heading.
/// \param position Three-component scene position.
/// \param itemKind Supported item-definition identifier.
/// \returns Nothing.
void springleafEntranceSpawnItem(f32 *position, s32 itemKind) {
    f64 var_f6;
    s32 headingRandomBits;

    headingRandomBits = randomNext();
    var_f6 = (f64)(u32)headingRandomBits;
    itemCreate(position[0], position[1], position[2], 0.0F,
               (f32)(var_f6 * RANDOM_U32_UNIT_SCALE * D_80158E58_scene_springleaf_entrance), itemKind);
}

/// Spawn the entrance resident at the location selected by saved scene state.
/// \returns Nothing.
void springleafEntranceSpawnFixedResident(void) {
    if (progressGetValue(0x12) == 0) {
        actorCreateForKind(4, 165.5F, 38.0F, 225.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                           &D_8014E2B0_scene_springleaf_entrance, 0);
        return;
    }
    actorCreateForKind(4, 128.5F, 38.0F, 200.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       &D_801573B8_scene_springleaf_entrance, 0);
}

/// Provide the entrance scene callback that requires no action.
/// \returns Nothing.
void springleafEntranceSceneNoOp(void) {
}

/// Reset the five resident growth states when every resident has reached the final stage.
/// \returns Nothing.
void springleafEntranceResetCompletedGrowth(void) {
    s32 i;
    s32 count = 0;
    for (i = 0; i < 5; i++) {
        if (progressGetValue(i + 0x65) >= 4) {
            count++;
        }
    }
    if (count == 5) {
        for (i = 0; i < 5; i++) {
            progressSetValue(i + 0x65, 0);
        }
    }
}

typedef struct {
    s32 used[22];
} ScenePositions;
extern ScenePositions D_80157938_scene_springleaf_entrance;
extern f32 D_80134BB0_scene_springleaf_entrance[][3];
extern u8 D_8014E62C_scene_springleaf_entrance;
extern u8 D_801574CC_scene_springleaf_entrance;
extern u8 D_801575EC_scene_springleaf_entrance;


extern u8 D_80158DE4_scene_springleaf_entrance;
extern f32 D_80158E60_scene_springleaf_entrance;
extern f32 D_80158E64_scene_springleaf_entrance;
/// Set up entrance residents, items, landmarks, and scene events.
/// \returns Nothing.
void springleafEntranceInitializeObjects(void) {
    s32 positionIndex;
    s32 var_a1;
    s32 i;
    ScenePositions sp54;
    s32 var_s2;

    sp54 = D_80157938_scene_springleaf_entrance;
    if (progressGetValue(0x76) > 0) {
        actorCreateForKind(0xE, 125.0F, 16.2F, 267.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                           &D_801575EC_scene_springleaf_entrance, 0);
    }
    springleafEntranceSpawnFixedResident();
    if (progressGetCurrentChapter() == 8) {
        actorCreateForKind(0xC, 207.69F, 10.5F, 239.56F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                           &D_801574CC_scene_springleaf_entrance, 0);
    }
    actorCreateForKind(0xA, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       &D_8014E62C_scene_springleaf_entrance, 0);
    runtimeCheckResetCheckpoint((const char *)&D_80158DE4_scene_springleaf_entrance, 1);
    springleafEntrancePrepareEvolvedResidents();
    springleafEntranceResetCompletedGrowth();
    i = 0;
    do {
        positionIndex = randomBelow(0x16);
        if ((sp54.used)[positionIndex] == 0) {
            springleafEntranceSpawnGrowthResident(D_80134BB0_scene_springleaf_entrance[positionIndex], i);
            (sp54.used)[positionIndex] = 1;
            i += 1;
        }
    } while (i < 5);
    i = 0;
    var_s2 = 0;
    for (; i < 5; i++) {
        if (progressGetValue(i + 0x65) >= 2) {
            var_s2 += 1;
        }
    }
    i = 0;
    do {
        positionIndex = randomBelow(0x16);
        if ((sp54.used)[positionIndex] == 0) {
            var_a1 = 0x7A;
            if (i < var_s2) {
                var_a1 = 0x7B;
            }
            springleafEntranceSpawnItem(D_80134BB0_scene_springleaf_entrance[positionIndex], var_a1);
            (sp54.used)[positionIndex] = 1;
            i += 1;
        }
    } while (i < 5);
    mazeScatterItems(D_80157668_scene_springleaf_entrance, 0x3C, &D_80157660_scene_springleaf_entrance, 0.7F);
    if (progressGetValue(0x12) == 0) {
        landmarkCreate(D_80158E60_scene_springleaf_entrance, 10.5F, 239.56F, 0x90);
        landmarkCreate(115.0F, D_80158E64_scene_springleaf_entrance, 267.0F, 0x8E);
    }
}

extern s32 *D_8016C76C_scene_springleaf_entrance[SPRINGLEAF_ENTRANCE_GRID_LAYERS];
/// Look up the entrance spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x80134A14 in the springleaf entrance overlay.
s32 springleafEntranceGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / SPRINGLEAF_ENTRANCE_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= SPRINGLEAF_ENTRANCE_GRID_LAYERS ||
        (ix = (s32)((f64)x / SPRINGLEAF_ENTRANCE_GRID_CELL_WIDTH), ix < 0) || ix >= SPRINGLEAF_ENTRANCE_GRID_SIDE ||
        (iz = (s32)((f64)z / SPRINGLEAF_ENTRANCE_GRID_CELL_WIDTH), iz < 0) || iz >= SPRINGLEAF_ENTRANCE_GRID_SIDE) {
        return 0;
    }
    layer = D_8016C76C_scene_springleaf_entrance[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * SPRINGLEAF_ENTRANCE_GRID_SIDE) + ix];
}
