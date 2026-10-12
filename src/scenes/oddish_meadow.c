#include "common.h"
#include "item_object.h"
#include "actor_create.h"
u32 randomNext(void);
s32 randomBelow(s32);
f32 mathPowInteger(f32, s32);
void objectRequestHierarchyDestruction(u8 *);
void runtimeCheckResetCheckpoint(u8 *, s32);
void landmarkCreate(f32, f32, f32, s32);
void mazeScatterItems(u8 *, s32, u8 *, f32);
s32 progressSetValue(s32, s32);
typedef struct {
    u8 unk0[12];
    f32 scale;
} SceneScale;
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern SceneScale *gCurrentAreaConfiguration;

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
        actorCreateForKind(5, position[0], position[1], position[2], 0.0f,
                           (f32)((f64)random * 2.3283064365386963e-10 * D_80159458_scene_oddish_meadow),
                           mathPowInteger(scale, state) * gCurrentAreaConfiguration->scale, record, residentIndex);
        return;
    }
    if (state < 4) {
        /* Keep the RNG result and scale-base initialization scheduled together. */
        // clang-format off
        random = randomNext(); scale = D_80159460_scene_oddish_meadow;
        // clang-format on
        actorCreateForKind(6, position[0], position[1], position[2], 0.0f,
                           (f32)((f64)random * 2.3283064365386963e-10 * D_80159468_scene_oddish_meadow),
                           mathPowInteger(scale, state - 2) * gCurrentAreaConfiguration->scale, record, residentIndex);
        return;
    }
    random = randomNext();
    actorCreateForKind(7, position[0], position[1], position[2], 0.0f,
                       (f32)((f64)(u32)random * 2.3283064365386963e-10 * D_80159470_scene_oddish_meadow),
                       gCurrentAreaConfiguration->scale, record, residentIndex);
}

/// Prepare scene resources for evolved meadow residents.
/// \returns Nothing.
void oddishMeadowPrepareEvolvedResidents(void) {
    objectRequestHierarchyDestruction(actorCreateForKind(6, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, NULL, 0));
    objectRequestHierarchyDestruction(actorCreateForKind(7, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, NULL, 0));
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
    itemCreate(position[0], position[1], position[2], 0.0f,
                  (f32)(var_f6 * 2.3283064365386963e-10 * D_80159478_scene_oddish_meadow), itemKind);
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
        if (progressGetValue(i + 0x6A) >= 4)
            count++;
    }
    if (count == 5) {
        for (i = 0; i < 5; i++)
            progressSetValue(i + 0x6A, 0);
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
extern u8 D_80157DB4_scene_oddish_meadow;
extern u8 D_80157DBC_scene_oddish_meadow;
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
        actorCreateForKind(0xE, 160.0f, 6.0f, 69.0f, 0.0f, 0.0f, gCurrentAreaConfiguration->scale,
                           &D_80157D40_scene_oddish_meadow, 0);
    }
    if (progressGetCurrentChapter() == 8) {
        actorCreateForKind(1, 188.0f, 8.5f, 65.0f, 0.0f, 20.0f, gCurrentAreaConfiguration->scale,
                           &D_8014F0BC_scene_oddish_meadow, 0);
    }
    actorCreateForKind(0xA, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, gCurrentAreaConfiguration->scale,
                       &D_8014E5D0_scene_oddish_meadow, 0);
    if (progressGetValue(0x12) == 0) {
        var_s0 = &D_8014EEA0_scene_oddish_meadow;
    } else {
        var_s0 = &D_80157C20_scene_oddish_meadow;
    }
    positionIndex = randomBelow(0x28);
    (sp64.used)[positionIndex] = 1;
    headingRandomBits = randomNext();
    growthProgress = progressGetValue(0x6F);
    residentPosition = D_80134DC0_scene_oddish_meadow[positionIndex];

    actorCreateForKind(5, residentPosition[0], residentPosition[1], residentPosition[2], 0.0f,
                       (f32)((f64)headingRandomBits * 2.3283064365386963e-10 * D_80159480_scene_oddish_meadow),
                       (f32)(((f64)((f32)growthProgress / 5.0f) + 1.0) * (f64)gCurrentAreaConfiguration->scale), var_s0,
                       5);
    runtimeCheckResetCheckpoint(&D_80159404_scene_oddish_meadow, 1);
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
        mazeScatterItems(&D_80157DBC_scene_oddish_meadow, 0x1C, &D_80157DB4_scene_oddish_meadow, 0.7f);
    } else {
        mazeScatterItems(&D_80157DBC_scene_oddish_meadow, 0x1C, &D_80157DB4_scene_oddish_meadow, 0.3f);
    }
    if (progressGetValue(0x12) == 0) {
        landmarkCreate(D_80159488_scene_oddish_meadow, 8.0f, 137.47f, 0x90);
        landmarkCreate(D_8015948C_scene_oddish_meadow, D_80159490_scene_oddish_meadow, 249.0f, 0x8E);
    }
}

/// Set up five final-stage residents for the alternate meadow scene.
/// \returns Nothing.
void oddishMeadowInitializeAlternateResidents(void) {
    s32 var_s0;

    for (var_s0 = 0; var_s0 < 5; var_s0++) {
        actorCreateForKind(7, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, gCurrentAreaConfiguration->scale,
                           &D_8014F348_scene_oddish_meadow, var_s0);
    }
}

/// Select and initialize the regular or alternate meadow setup from saved scene state.
/// \returns Nothing.
void oddishMeadowInitializeObjects(void) {
    if (progressGetValue(0x64) != 0) {
        oddishMeadowInitializeAlternateResidents();
        return;
    }
    oddishMeadowInitializeRegularObjects();
}

extern s32 *D_80189430_scene_oddish_meadow[16];
/// Look up the meadow spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 oddishMeadowGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 10.0);
    if (iy < 0 || iy >= 16 || (ix = (s32)((f64)x / 20.0), ix < 0) || ix >= 16 || (iz = (s32)((f64)z / 20.0), iz < 0) ||
        iz >= 16)
        return 0;
    layer = D_80189430_scene_oddish_meadow[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 16 + ix];
}
