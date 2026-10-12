#include "door.h"
#include "common.h"
#include "item_object.h"
#include "actor_create.h"
void objectRequestHierarchyDestruction(u8 *);
void doorRegistryReset(void);
u32 randomNext(void);
s32 progressGetValue(s32);
s32 progressSetValue(s32, s32);
/// Entrance index used to initialize the player and companion in the current area.
/// \note Original identifier: \c D_800AE4F4 (0x800AE4F4).
extern s32 gRuntimeCurrentEntrance;
extern f32 D_80134D24_scene_springleaf_field[];
extern f64 D_801594E8_scene_springleaf_field;

f32 mathPowInteger(f32, s32);
typedef struct {
    u8 unknown[12];
    f32 scale;
} SceneScale;
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern SceneScale *gCurrentAreaConfiguration;
extern u8 D_8014DFD8_scene_springleaf_field, D_80156C2C_scene_springleaf_field;
extern f32 D_801594C4_scene_springleaf_field, D_801594D0_scene_springleaf_field;
extern f64 D_801594C8_scene_springleaf_field, D_801594D8_scene_springleaf_field, D_801594E0_scene_springleaf_field;

/// Spawn a field resident according to its saved growth state.
/// \param position Three-component scene position.
/// \param residentIndex Resident growth-slot index from zero through four.
/// \returns Nothing.
void springleafFieldSpawnGrowthResident(f32 *position, s32 residentIndex) {
    f32 scale;
    s32 state;
    u8 *record;
    u32 random;

    state = progressGetValue(residentIndex + 0x70);
    if (progressGetValue(0x12) == 0) {
        record = &D_8014DFD8_scene_springleaf_field;
    } else {
        record = &D_80156C2C_scene_springleaf_field;
    }
    if (state < 2) {
        /* Keep random sampling and scale selection together for IDO scheduling. */
        // clang-format off
        random = randomNext(); scale = D_801594C4_scene_springleaf_field;
        // clang-format on
        actorCreateForKind(5, position[0], position[1], position[2], 0.0f,
                           (f32)((f64)random * 2.3283064365386963e-10 * D_801594C8_scene_springleaf_field),
                           mathPowInteger(scale, state) * gCurrentAreaConfiguration->scale, record, residentIndex);
        return;
    }
    if (state < 4) {
        // clang-format off
        random = randomNext(); scale = D_801594D0_scene_springleaf_field;
        // clang-format on
        actorCreateForKind(6, position[0], position[1], position[2], 0.0f,
                           (f32)((f64)random * 2.3283064365386963e-10 * D_801594D8_scene_springleaf_field),
                           mathPowInteger(scale, state - 2) * gCurrentAreaConfiguration->scale, record, residentIndex);
        return;
    }
    random = randomNext();
    actorCreateForKind(7, position[0], position[1], position[2], 0.0f,
                       (f32)((f64)(u32)random * 2.3283064365386963e-10 * D_801594E0_scene_springleaf_field),
                       gCurrentAreaConfiguration->scale, record, residentIndex);
}

/// Prepare scene resources for evolved field residents.
/// \returns Nothing.
void springleafFieldPrepareEvolvedResidents(void) {
    objectRequestHierarchyDestruction(actorCreateForKind(6, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, NULL, 0));
    objectRequestHierarchyDestruction(actorCreateForKind(7, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, NULL, 0));
}

/// Spawn a field item with a randomized heading.
/// \param position Three-component scene position.
/// \param itemKind Supported item-definition identifier.
/// \returns Nothing.
void springleafFieldSpawnItem(f32 *position, s32 itemKind) {
    itemCreate(position[0], position[1], position[2], 0.0f,
                  (f32)((f64)randomNext() * 2.3283064365386963e-10 * D_801594E8_scene_springleaf_field), itemKind);
}

/// Initialize the field scene door.
/// \returns Nothing.
void springleafFieldInitializeDoor(void) {
    doorRegistryReset();
    doorCreate(D_80134D24_scene_springleaf_field, 1, 15, 1, 3);
}

/// Reset completed field growth for the next cycle when the current mode permits it.
/// \returns Nothing.
void springleafFieldResetCompletedGrowth(void) {
    s32 i;
    s32 count = 0;
    for (i = 0; i < 5; i++) {
        if (progressGetValue(i + 0x70) >= 4)
            count++;
    }
    if (gRuntimeCurrentEntrance != 2 && count == 5) {
        for (i = 0; i != 5; i++)
            progressSetValue(i + 0x70, 0);
    }
}

s32 randomBelow(s32);
void runtimeCheckResetCheckpoint(u8 *, s32);
void mazeScatterItems(u8 *, s32, u8 *, f32);
void landmarkCreate(f32, f32, f32, s32);
typedef struct {
    s32 used[27];
} FieldOccupancy;
extern FieldOccupancy D_80157F70_scene_springleaf_field;
extern u8 D_801571E4_scene_springleaf_field[], D_80157B58_scene_springleaf_field[], D_80157C78_scene_springleaf_field[],
    D_8014E354_scene_springleaf_field[], D_80159474_scene_springleaf_field[], D_80157CF4_scene_springleaf_field[],
    D_80157CEC_scene_springleaf_field[];
extern f32 D_801594F0_scene_springleaf_field, D_801594F4_scene_springleaf_field, D_801594F8_scene_springleaf_field,
    D_801594FC_scene_springleaf_field;
extern f32 D_80134BE0_scene_springleaf_field[][3];
/// Set up field residents, items, landmarks, door access, and scene events.
/// \returns Nothing.
void springleafFieldInitializeObjects(void) {
    s32 selected;
    s32 i;
    s32 count;
    FieldOccupancy occupied;
    f32 position[3];
    s32 kind;
    occupied = D_80157F70_scene_springleaf_field;
    if (progressGetValue(0x12) != 0) {
        actorCreateForKind(3, 203.45f, 0.5f, 35.97f, 0.0f, 0.0f, gCurrentAreaConfiguration->scale,
                           D_801571E4_scene_springleaf_field, 0);
    }
    if (progressGetCurrentChapter() == 8) {
        actorCreateForKind(17, 0, 0, 0, 0.0f, 0.0f, gCurrentAreaConfiguration->scale, D_80157B58_scene_springleaf_field,
                           0);
    }
    if (progressGetValue(0x76) >= 3) {
        actorCreateForKind(14, 130.0f, 16.0f, 150.0f, 0.0f, 0.0f, gCurrentAreaConfiguration->scale,
                           D_80157C78_scene_springleaf_field, 0);
    }
    actorCreateForKind(10, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, gCurrentAreaConfiguration->scale,
                       D_8014E354_scene_springleaf_field, 0);
    runtimeCheckResetCheckpoint(D_80159474_scene_springleaf_field, 1);
    springleafFieldPrepareEvolvedResidents();
    springleafFieldResetCompletedGrowth();
    if (progressGetValue(0x4F) == 0) {
        position[0] = 72.0f;
        position[1] = D_801594F0_scene_springleaf_field;
        position[2] = 59.0f;
        springleafFieldSpawnGrowthResident(position, 0);
        i = 1;
    } else {
        i = 0;
    }
    while (i < 5) {
        selected = randomBelow(27);
        if (!occupied.used[selected]) {
            springleafFieldSpawnGrowthResident(D_80134BE0_scene_springleaf_field[selected], i);
            occupied.used[selected] = 1;
            i++;
        }
    }
    i = 0;
    count = 0;
    do {
        if (progressGetValue(i + 0x70) >= 2)
            count++;
        i++;
    } while (i < 5);
    i = 0;
    do {
        selected = randomBelow(27);
        if (!occupied.used[selected]) {
            kind = 0x7A;
            if (i < count)
                kind = 0x7B;
            springleafFieldSpawnItem(D_80134BE0_scene_springleaf_field[selected], kind);
            occupied.used[selected] = 1;
            i++;
        }
    } while (i < 7);
    mazeScatterItems(D_80157CF4_scene_springleaf_field, 0x35, D_80157CEC_scene_springleaf_field, 0.7f);
    if (progressGetValue(0x12) == 0) {
        landmarkCreate(D_801594F4_scene_springleaf_field, 10.5f, 35.97f, 0x90);
        landmarkCreate(D_801594F8_scene_springleaf_field, D_801594FC_scene_springleaf_field, 91.0f, 0x8E);
        springleafFieldInitializeDoor();
    }
}

extern s32 *D_8017D658_scene_springleaf_field[32];
/// Look up the field spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 springleafFieldGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 5.0);
    if (iy < 0 || iy >= 32 || (ix = (s32)((f64)x / 10.0), ix < 0) || ix >= 32 || (iz = (s32)((f64)z / 10.0), iz < 0) ||
        iz >= 32)
        return 0;
    layer = D_8017D658_scene_springleaf_field[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 32 + ix];
}
