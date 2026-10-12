#include "landmark.h"
#include "item_object.h"
#include "common.h"
#include "actor_create.h"
#include "scene_types.h"
u8 *toolboxTakeObject(s32, s32);
/// Remove the removable items held in the tracked scene-item slots.
/// \returns Nothing.
void viridianDeepRemoveTrackedItems(void) {
    s32 i, j;
    u8 *object;
    for (i = 0; i != 2; i++) {
        for (j = 0; j != 10; j++) {
            object = toolboxTakeObject(i, j);
            if (object != NULL)
                itemDestroy(object);
        }
    }
}
f32 mathSinDegrees(f32);
f32 mathCosDegrees(f32);
f32 mathPowInteger(f32, s32);
typedef struct {
    u8 header[12];
    f32 scale;
} SceneScale;
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern SceneScale *gCurrentAreaConfiguration;
extern u8 D_80148ABC_scene_viridian_deep[], D_80148DAC_scene_viridian_deep[], D_80149170_scene_viridian_deep[];
extern f32 D_8014B020_scene_viridian_deep;
extern f32 D_8014989C_scene_viridian_deep[][4];
typedef struct {
    f32 x, y, z;
} Position;
/// Spawn a deep-forest resident group appropriate to its saved growth and population state.
/// \param position World-space center of the resident group.
/// \param index Resident-group index from zero through four.
/// \returns Nothing.
void viridianDeepSpawnResidentGroup(Position *position, s32 index) {
    s32 growth;
    s32 first;
    s32 secondIndex;
    s32 second;
    f32 angle;
    u8 *record;
    f32 scale;
    f32 cosine;
    f32 sine;
    growth = progressGetValue(index + 0x77);
    first = progressGetValue(index + 0x7C);
    secondIndex = index + 5;
    second = progressGetValue(index + 0x81);
    if (progressGetCurrentChapter() != 4)
        record = D_80148ABC_scene_viridian_deep;
    else
        record = D_80148DAC_scene_viridian_deep;
    angle = -135.0f - index * 60.0f;
    sine = mathSinDegrees(angle);
    cosine = mathCosDegrees(angle);
    scale = mathPowInteger(D_8014B020_scene_viridian_deep, growth);
    actorCreateForKind(8, sine * 8.0f + position->x, position->y, cosine * 8.0f + position->z, 0.0f, angle + 180.0f,
                       (scale *= gCurrentAreaConfiguration->scale) * 2.0f, record, index);
    if (progressGetCurrentChapter() != 4) {
        if (first > 0)
            actorCreateForKind(9, D_8014989C_scene_viridian_deep[index][0], D_8014989C_scene_viridian_deep[index][1],
                               D_8014989C_scene_viridian_deep[index][2], 0.0f, D_8014989C_scene_viridian_deep[index][3],
                               gCurrentAreaConfiguration->scale, D_80149170_scene_viridian_deep, index);
        if (second > 0)
            actorCreateForKind(
                9, D_8014989C_scene_viridian_deep[secondIndex][0], D_8014989C_scene_viridian_deep[secondIndex][1],
                D_8014989C_scene_viridian_deep[secondIndex][2], 0.0f, D_8014989C_scene_viridian_deep[secondIndex][3],
                gCurrentAreaConfiguration->scale, D_80149170_scene_viridian_deep, secondIndex);
    }
}

void mazeScatterItems(u8 *, s32, u8 *, f32);
void viridianDeepSpawnResidentGroup(Position *, s32);
extern u8 D_80149888_scene_viridian_deep[], D_8014993C_scene_viridian_deep[], D_80149DB8_scene_viridian_deep[];
extern f32 D_8014B024_scene_viridian_deep, D_8014B028_scene_viridian_deep, D_8014B02C_scene_viridian_deep;
/// Populate the deep forest with resident groups and progress-dependent items and landmarks.
/// \returns Nothing.
void viridianDeepInitializeObjects(void) {
    s32 i;
    Position position;
    for (i = 0; i != 5; i++) {
        position.x = 162.0f;
        position.y = 1.25f;
        position.z = 151.0f;
        viridianDeepSpawnResidentGroup(&position, i);
    }
    if (progressGetCurrentChapter() != 4) {
        actorCreateForKind(14, 160.0f, 5.0f, 145.0f, 0.0f, D_8014B024_scene_viridian_deep,
                           ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                           D_80149888_scene_viridian_deep, 0);
        mazeScatterItems(D_8014993C_scene_viridian_deep, 0x5B, D_80149DB8_scene_viridian_deep, 0.5f);
        landmarkCreate(162.0f, 1.25f, 151.0f, 0x8D);
        landmarkCreate(D_8014B028_scene_viridian_deep, D_8014B02C_scene_viridian_deep, 175.27f, 0x8E);
    } else
        viridianDeepRemoveTrackedItems();
}

extern s32 *D_8015DBC8_scene_viridian_deep[32];
/// Look up the deep-forest spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 viridianDeepGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 5.0);
    if (iy < 0 || iy >= 32 || (ix = (s32)((f64)x / 10.0), ix < 0) || ix >= 32 || (iz = (s32)((f64)z / 10.0), iz < 0) ||
        iz >= 32)
        return 0;
    layer = D_8015DBC8_scene_viridian_deep[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 32 + ix];
}
