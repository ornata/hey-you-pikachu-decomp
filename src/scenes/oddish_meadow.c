#include "progress_field_ids.h"
#include "actor_kind.h"
/// Geometry of the oddish meadow spatial lookup grid.
enum { ODDISH_MEADOW_GRID_LAYERS = 16, ODDISH_MEADOW_GRID_SIDE = 16 };
/// Height of one lookup layer in world-coordinate units.
#define ODDISH_MEADOW_GRID_LAYER_HEIGHT 10.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define ODDISH_MEADOW_GRID_CELL_WIDTH 20.0

#include "random_scale.h"
#include "scene_types.h"
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

/// Mutable weighted item-kind choices retained by the scene overlay.
/// \note Original identifier: \c D_80157DB4_scene_oddish_meadow (0x80157DB4).
extern WeightedChoiceTable D_80157DB4_scene_oddish_meadow;
/// Borrowed world-position triples for scene item scattering.
/// \note Original identifier: \c D_80157DBC_scene_oddish_meadow (0x80157DBC).
extern f32 D_80157DBC_scene_oddish_meadow[];

#include "scene_configuration.h"

extern u8 D_8014E254_scene_oddish_meadow;
extern u8 D_80157C20_scene_oddish_meadow;
extern f32 D_80159454_scene_oddish_meadow;
extern f64 D_80159458_scene_oddish_meadow;
extern f32 D_80159460_scene_oddish_meadow;
extern f64 D_80159468_scene_oddish_meadow;
extern f64 D_80159470_scene_oddish_meadow;
extern f64 D_80159478_scene_oddish_meadow;
extern u8 D_8014F348_scene_oddish_meadow;

/// Spawn a meadow resident according to its saved growth state.
/// \param position Three-component scene position.
/// \param residentIndex Resident growth-slot index from zero through four.
/// \returns Nothing.
void oddishMeadowSpawnGrowthResident(f32 *position, s32 residentIndex) {
    f32 scale;
    s32 state;
    u8 *record;
    u32 random;

    state = progressGetValue(residentIndex + 0x6A);
    if (progressGetValue(0x12) == 0) {
        record = &D_8014E254_scene_oddish_meadow;
    } else {
        record = &D_80157C20_scene_oddish_meadow;
    }
    if (state < 2) {
        /* Keep the RNG result and scale-base initialization scheduled together. */
        // clang-format off
        random = randomNext(); scale = D_80159454_scene_oddish_meadow;
        // clang-format on
        actorCreateForKind(ACTOR_KIND_ODDISH, position[0], position[1], position[2], 0.0F,
                           (f32)((f64)random * RANDOM_U32_UNIT_SCALE * D_80159458_scene_oddish_meadow),
                           mathPowInteger(scale, state) * gCurrentAreaConfiguration->objectScale, record,
                           residentIndex);
        return;
    }
    if (state < 4) {
        /* Keep the RNG result and scale-base initialization scheduled together. */
        // clang-format off
        random = randomNext(); scale = D_80159460_scene_oddish_meadow;
        // clang-format on
        actorCreateForKind(ACTOR_KIND_GLOOM, position[0], position[1], position[2], 0.0F,
                           (f32)((f64)random * RANDOM_U32_UNIT_SCALE * D_80159468_scene_oddish_meadow),
                           mathPowInteger(scale, state - 2) * gCurrentAreaConfiguration->objectScale, record,
                           residentIndex);
        return;
    }
    random = randomNext();
    actorCreateForKind(ACTOR_KIND_VILEPLUME, position[0], position[1], position[2], 0.0F,
                       (f32)((f64)random * RANDOM_U32_UNIT_SCALE * D_80159470_scene_oddish_meadow),
                       gCurrentAreaConfiguration->objectScale, record, residentIndex);
}

/// Prepare scene resources for evolved meadow residents.
/// \returns Nothing.
void oddishMeadowPrepareEvolvedResidents(void) {
    objectRequestHierarchyDestruction(
        actorCreateForKind(ACTOR_KIND_GLOOM, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, NULL, 0));
    objectRequestHierarchyDestruction(
        actorCreateForKind(ACTOR_KIND_VILEPLUME, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, NULL, 0));
}

/// Spawn a meadow item with a randomized heading.
/// \param position Three-component scene position.
/// \param itemKind Supported item-definition identifier.
/// \returns Nothing.
void oddishMeadowSpawnItem(f32 *position, s32 itemKind) {
    f64 var_f6;
    s32 headingRandomBits;

    headingRandomBits = randomNext();
    var_f6 = (f64)(u32)headingRandomBits;
    itemCreate(position[0], position[1], position[2], 0.0F,
               (f32)(var_f6 * RANDOM_U32_UNIT_SCALE * D_80159478_scene_oddish_meadow), itemKind);
}

/// Provide the meadow scene callback that requires no action.
/// \returns Nothing.
void oddishMeadowSceneNoOp(void) {
}

/// Reset the five resident growth states when every resident has reached the final stage.
/// \returns Nothing.
void oddishMeadowResetCompletedGrowth(void) {
    s32 i;
    s32 count = 0;
    for (i = 0; i < 5; i++) {
        if (progressGetValue(i + 0x6A) >= 4) {
            count++;
        }
    }
    if (count == 5) {
        for (i = 0; i < 5; i++) {
            progressSetValue(i + 0x6A, 0);
        }
    }
}

typedef struct {
    s32 used[40];
} ScenePositions;
extern ScenePositions D_80157F0C_scene_oddish_meadow;
extern f32 D_80134DC0_scene_oddish_meadow[][3];
extern u8 D_8014E5D0_scene_oddish_meadow;
extern u8 D_8014EEA0_scene_oddish_meadow;
extern u8 D_8014F0BC_scene_oddish_meadow;
extern u8 D_80157D40_scene_oddish_meadow;

extern u8 D_80159404_scene_oddish_meadow;
extern f64 D_80159480_scene_oddish_meadow;
extern f32 D_80159488_scene_oddish_meadow;
extern f32 D_8015948C_scene_oddish_meadow;
extern f32 D_80159490_scene_oddish_meadow;
/// Set up the regular meadow residents, items, landmarks, and scene events.
/// \returns Nothing.
void oddishMeadowInitializeRegularObjects(void) {
    s32 positionIndex;
    s32 growthProgress;
    s32 var_a1;
    ScenePositions sp64;
    s32 i;
    s32 var_s2;
    u32 headingRandomBits;
    u8 *var_s0;
    f32 *residentPosition;

    sp64 = D_80157F0C_scene_oddish_meadow;
    if (progressGetValue(0x76) >= 5) {
        actorCreateForKind(ACTOR_KIND_BUTTERFREE, 160.0F, 6.0F, 69.0F, 0.0F, 0.0F,
                           gCurrentAreaConfiguration->objectScale, &D_80157D40_scene_oddish_meadow, 0);
    }
    if (progressGetCurrentChapter() == 8) {
        actorCreateForKind(ACTOR_KIND_BULBASAUR, 188.0F, 8.5F, 65.0F, 0.0F, 20.0F,
                           gCurrentAreaConfiguration->objectScale, &D_8014F0BC_scene_oddish_meadow, 0);
    }
    actorCreateForKind(ACTOR_KIND_DIGLETT, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       &D_8014E5D0_scene_oddish_meadow, 0);
    if (progressGetValue(0x12) == 0) {
        var_s0 = &D_8014EEA0_scene_oddish_meadow;
    } else {
        var_s0 = &D_80157C20_scene_oddish_meadow;
    }
    positionIndex = randomBelow(0x28);
    (sp64.used)[positionIndex] = 1;
    headingRandomBits = randomNext();
    growthProgress = progressGetValue(PROGRESS_FIELD_ODDISH_GROWTH_PROGRESS);
    residentPosition = D_80134DC0_scene_oddish_meadow[positionIndex];

    actorCreateForKind(ACTOR_KIND_ODDISH, residentPosition[0], residentPosition[1], residentPosition[2], 0.0F,
                       (f32)((f64)headingRandomBits * RANDOM_U32_UNIT_SCALE * D_80159480_scene_oddish_meadow),
                       (f32)(((f64)((f32)growthProgress / 5.0F) + 1.0) * (f64)gCurrentAreaConfiguration->objectScale),
                       var_s0, 5);
    runtimeCheckResetCheckpoint((const char *)&D_80159404_scene_oddish_meadow, 1);
    oddishMeadowPrepareEvolvedResidents();
    oddishMeadowResetCompletedGrowth();
    i = 0;
    do {
        positionIndex = randomBelow(0x28);
        if ((sp64.used)[positionIndex] == 0) {
            oddishMeadowSpawnGrowthResident(D_80134DC0_scene_oddish_meadow[positionIndex], i);
            (sp64.used)[positionIndex] = 1;
            i += 1;
        }
    } while (i < 5);
    i = 0;
    var_s2 = 0;
    for (; i < 5; i++) {
        if (progressGetValue(i + 0x6A) >= 2) {
            var_s2 += 1;
        }
    }
    i = 0;
    do {
        positionIndex = randomBelow(0x28);
        if ((sp64.used)[positionIndex] == 0) {
            var_a1 = 0x7A;
            if (i < var_s2) {
                var_a1 = 0x7B;
            }
            oddishMeadowSpawnItem(D_80134DC0_scene_oddish_meadow[positionIndex], var_a1);
            (sp64.used)[positionIndex] = 1;
            i += 1;
        }
    } while (i < 0xA);
    if (progressGetCurrentChapter() != 8) {
        mazeScatterItems(D_80157DBC_scene_oddish_meadow, 0x1C, &D_80157DB4_scene_oddish_meadow, 0.7F);
    } else {
        mazeScatterItems(D_80157DBC_scene_oddish_meadow, 0x1C, &D_80157DB4_scene_oddish_meadow, 0.3F);
    }
    if (progressGetValue(0x12) == 0) {
        landmarkCreate(D_80159488_scene_oddish_meadow, 8.0F, 137.47F, 0x90);
        landmarkCreate(D_8015948C_scene_oddish_meadow, D_80159490_scene_oddish_meadow, 249.0F, 0x8E);
    }
}

/// Set up five final-stage residents for the alternate meadow scene.
/// \returns Nothing.
void oddishMeadowInitializeAlternateResidents(void) {
    s32 var_s0;

    for (var_s0 = 0; var_s0 < 5; var_s0++) {
        actorCreateForKind(ACTOR_KIND_VILEPLUME, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                           &D_8014F348_scene_oddish_meadow, var_s0);
    }
}

/// Select and initialize the regular or alternate meadow setup from saved scene state.
/// \returns Nothing.
void oddishMeadowInitializeObjects(void) {
    if (progressGetValue(PROGRESS_FIELD_VILEPLUME_MEADOW_VISIT_PENDING) != 0) {
        oddishMeadowInitializeAlternateResidents();
        return;
    }
    oddishMeadowInitializeRegularObjects();
}

extern s32 *D_80189430_scene_oddish_meadow[ODDISH_MEADOW_GRID_LAYERS];
/// Look up the meadow spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x80134B68 in the oddish meadow overlay.
s32 oddishMeadowGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / ODDISH_MEADOW_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= ODDISH_MEADOW_GRID_LAYERS || (ix = (s32)((f64)x / ODDISH_MEADOW_GRID_CELL_WIDTH), ix < 0) ||
        ix >= ODDISH_MEADOW_GRID_SIDE || (iz = (s32)((f64)z / ODDISH_MEADOW_GRID_CELL_WIDTH), iz < 0) ||
        iz >= ODDISH_MEADOW_GRID_SIDE) {
        return 0;
    }
    layer = D_80189430_scene_oddish_meadow[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * ODDISH_MEADOW_GRID_SIDE) + ix];
}
