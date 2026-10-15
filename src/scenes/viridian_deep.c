#include "actor_kind.h"
/// Geometry of the viridian deep spatial lookup grid.
enum { VIRIDIAN_DEEP_GRID_LAYERS = 32, VIRIDIAN_DEEP_GRID_SIDE = 32 };
/// Height of one lookup layer in world-coordinate units.
#define VIRIDIAN_DEEP_GRID_LAYER_HEIGHT 5.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define VIRIDIAN_DEEP_GRID_CELL_WIDTH 10.0

#include "math_scalar.h"
#include "math_angles.h"
#include "maze_scatter.h"
#include "story_chapter.h"
#include "landmark.h"
#include "item_object.h"
#include "common.h"
#include "actor_create.h"
#include "scene_types.h"

/// Mutable weighted item-kind choices retained by the scene overlay.
/// \note Original identifier: \c D_80149DB8_scene_viridian_deep (0x80149DB8).
extern WeightedChoiceTable D_80149DB8_scene_viridian_deep;
/// Borrowed world-position triples for scene item scattering.
/// \note Original identifier: \c D_8014993C_scene_viridian_deep (0x8014993C).
extern f32 D_8014993C_scene_viridian_deep[];

#include "toolbox.h"
/// Remove the removable items held in the tracked scene-item slots.
/// \returns Nothing.
void viridianDeepRemoveTrackedItems(void) {
    s32 i;
    s32 j;
    u8 *object;
    for (i = 0; i != 2; i++) {
        for (j = 0; j != 10; j++) {
            object = toolboxTakeObject(i, j);
            if (object != NULL) {
                itemDestroy(object);
            }
        }
    }
}
#include "scene_configuration.h"
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
    if (progressGetCurrentChapter() != 4) {
        record = D_80148ABC_scene_viridian_deep;
    } else {
        record = D_80148DAC_scene_viridian_deep;
    }
    angle = -135.0F - (index * 60.0F);
    sine = mathSinDegrees(angle);
    cosine = mathCosDegrees(angle);
    scale = mathPowInteger(D_8014B020_scene_viridian_deep, growth);
    actorCreateForKind(ACTOR_KIND_CATERPIE, (sine * 8.0F) + position->x, position->y, (cosine * 8.0F) + position->z,
                       0.0F, angle + 180.0F, (scale *= gCurrentAreaConfiguration->objectScale) * 2.0F, record, index);
    if (progressGetCurrentChapter() != 4) {
        if (first > 0) {
            actorCreateForKind(ACTOR_KIND_METAPOD, D_8014989C_scene_viridian_deep[index][0],
                               D_8014989C_scene_viridian_deep[index][1], D_8014989C_scene_viridian_deep[index][2], 0.0F,
                               D_8014989C_scene_viridian_deep[index][3], gCurrentAreaConfiguration->objectScale,
                               D_80149170_scene_viridian_deep, index);
        }
        if (second > 0) {
            actorCreateForKind(ACTOR_KIND_METAPOD, D_8014989C_scene_viridian_deep[secondIndex][0],
                               D_8014989C_scene_viridian_deep[secondIndex][1],
                               D_8014989C_scene_viridian_deep[secondIndex][2], 0.0F,
                               D_8014989C_scene_viridian_deep[secondIndex][3], gCurrentAreaConfiguration->objectScale,
                               D_80149170_scene_viridian_deep, secondIndex);
        }
    }
}

extern u8 D_80149888_scene_viridian_deep[];
extern f32 D_8014B024_scene_viridian_deep, D_8014B028_scene_viridian_deep, D_8014B02C_scene_viridian_deep;
/// Populate the deep forest with resident groups and progress-dependent items and landmarks.
/// \returns Nothing.
void viridianDeepInitializeObjects(void) {
    s32 i;
    Position position;
    for (i = 0; i != 5; i++) {
        position.x = 162.0F;
        position.y = 1.25F;
        position.z = 151.0F;
        viridianDeepSpawnResidentGroup(&position, i);
    }
    if (progressGetCurrentChapter() != 4) {
        actorCreateForKind(ACTOR_KIND_BUTTERFREE, 160.0F, 5.0F, 145.0F, 0.0F, D_8014B024_scene_viridian_deep,
                           gCurrentAreaConfiguration->objectScale, D_80149888_scene_viridian_deep, 0);
        mazeScatterItems(D_8014993C_scene_viridian_deep, 0x5B, &D_80149DB8_scene_viridian_deep, 0.5F);
        landmarkCreate(162.0F, 1.25F, 151.0F, 0x8D);
        landmarkCreate(D_8014B028_scene_viridian_deep, D_8014B02C_scene_viridian_deep, 175.27F, 0x8E);
    } else {
        viridianDeepRemoveTrackedItems();
    }
}

extern s32 *D_8015DBC8_scene_viridian_deep[VIRIDIAN_DEEP_GRID_LAYERS];
/// Look up the deep-forest spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x80134658 in the viridian deep overlay.
s32 viridianDeepGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / VIRIDIAN_DEEP_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= VIRIDIAN_DEEP_GRID_LAYERS || (ix = (s32)((f64)x / VIRIDIAN_DEEP_GRID_CELL_WIDTH), ix < 0) ||
        ix >= VIRIDIAN_DEEP_GRID_SIDE || (iz = (s32)((f64)z / VIRIDIAN_DEEP_GRID_CELL_WIDTH), iz < 0) ||
        iz >= VIRIDIAN_DEEP_GRID_SIDE) {
        return 0;
    }
    layer = D_8015DBC8_scene_viridian_deep[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * VIRIDIAN_DEEP_GRID_SIDE) + ix];
}
