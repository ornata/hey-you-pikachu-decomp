#include "common.h"
#include "actor_create.h"
#include "scene_types.h"
u8 *itemCreate(f32, f32, f32, f32, f32, s32);
u32 randomNext(void);
s32 randomBelow(s32);
f32 mathPowInteger(f32, s32);
void objectRequestHierarchyDestruction(u8 *);
void runtimeCheckResetCheckpoint(u8 *, s32);
void landmarkCreate(f32, f32, f32, s32);
void mazeScatterItems(u8 *, s32, u8 *, f32);
s32 progressSetValue(s32, s32);
typedef struct {
    u8 unknown[12];
    f32 scale;
} SceneScale;
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern SceneScale *gCurrentAreaConfiguration;

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
        actorCreateForKind(5, position[0], position[1], position[2], 0.0f,
                           (f32)((f64)random * 2.3283064365386963e-10 * D_80158E38_scene_springleaf_entrance),
                           mathPowInteger(scale, state) * gCurrentAreaConfiguration->scale, record, residentIndex);
        return;
    }
    if (state < 4) {
        // clang-format off
        random = randomNext(); scale = D_80158E40_scene_springleaf_entrance;
        // clang-format on
        actorCreateForKind(6, position[0], position[1], position[2], 0.0f,
                           (f32)((f64)random * 2.3283064365386963e-10 * D_80158E48_scene_springleaf_entrance),
                           mathPowInteger(scale, state - 2) * gCurrentAreaConfiguration->scale, record, residentIndex);
        return;
    }
    random = randomNext();
    actorCreateForKind(7, position[0], position[1], position[2], 0.0f,
                       (f32)((f64)(u32)random * 2.3283064365386963e-10 * D_80158E50_scene_springleaf_entrance),
                       gCurrentAreaConfiguration->scale, record, residentIndex);
}

/// Prepare scene resources for evolved entrance residents.
/// \returns Nothing.
void springleafEntrancePrepareEvolvedResidents(void) {
    objectRequestHierarchyDestruction(actorCreateForKind(6, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, NULL, 0));
    objectRequestHierarchyDestruction(actorCreateForKind(7, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, NULL, 0));
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
    itemCreate(position[0], position[1], position[2], 0.0f,
                  (f32)(var_f6 * 2.3283064365386963e-10 * D_80158E58_scene_springleaf_entrance), itemKind);
}

/// Spawn the entrance resident at the location selected by saved scene state.
/// \returns Nothing.
void springleafEntranceSpawnFixedResident(void) {
    if (progressGetValue(0x12) == 0) {
        actorCreateForKind(4, 165.5f, 38.0f, 225.0f, 0.0f, 0.0f,
                           ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                           &D_8014E2B0_scene_springleaf_entrance, 0);
        return;
    }
    actorCreateForKind(4, 128.5f, 38.0f, 200.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
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
        if (progressGetValue(i + 0x65) >= 4)
            count++;
    }
    if (count == 5) {
        for (i = 0; i < 5; i++)
            progressSetValue(i + 0x65, 0);
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
extern u8 D_80157660_scene_springleaf_entrance;
extern u8 D_80157668_scene_springleaf_entrance;
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
        actorCreateForKind(0xE, 125.0f, 16.2f, 267.0f, 0.0f, 0.0f,
                           ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                           &D_801575EC_scene_springleaf_entrance, 0);
    }
    springleafEntranceSpawnFixedResident();
    if (progressGetCurrentChapter() == 8) {
        actorCreateForKind(0xC, 207.69f, 10.5f, 239.56f, 0.0f, 0.0f,
                           ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                           &D_801574CC_scene_springleaf_entrance, 0);
    }
    actorCreateForKind(0xA, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       &D_8014E62C_scene_springleaf_entrance, 0);
    runtimeCheckResetCheckpoint(&D_80158DE4_scene_springleaf_entrance, 1);
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
    mazeScatterItems(&D_80157668_scene_springleaf_entrance, 0x3C, &D_80157660_scene_springleaf_entrance, 0.7f);
    if (progressGetValue(0x12) == 0) {
        landmarkCreate(D_80158E60_scene_springleaf_entrance, 10.5f, 239.56f, 0x90);
        landmarkCreate(115.0f, D_80158E64_scene_springleaf_entrance, 267.0f, 0x8E);
    }
}

extern s32 *D_8016C76C_scene_springleaf_entrance[32];
/// Look up the entrance spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 springleafEntranceGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 5.0);
    if (iy < 0 || iy >= 32 || (ix = (s32)((f64)x / 10.0), ix < 0) || ix >= 32 || (iz = (s32)((f64)z / 10.0), iz < 0) ||
        iz >= 32)
        return 0;
    layer = D_8016C76C_scene_springleaf_entrance[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 32 + ix];
}
