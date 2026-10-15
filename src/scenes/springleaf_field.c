#include "actor_kind.h"
/// Geometry of the springleaf field spatial lookup grid.
enum { SPRINGLEAF_FIELD_GRID_LAYERS = 32, SPRINGLEAF_FIELD_GRID_SIDE = 32 };
/// Height of one lookup layer in world-coordinate units.
#define SPRINGLEAF_FIELD_GRID_LAYER_HEIGHT 5.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define SPRINGLEAF_FIELD_GRID_CELL_WIDTH 10.0

#include "random_scale.h"
#include "scene_types.h"
#include "math_scalar.h"
#include "random.h"
#include "runtime_control.h"
#include "maze_scatter.h"
#include "story_chapter.h"
#include "progress_fields.h"
#include "progress_value.h"
#include "landmark.h"
#include "object_lifecycle.h"
#include "door.h"
#include "common.h"
#include "item_object.h"
#include "actor_create.h"

/// Mutable weighted item-kind choices retained by the scene overlay.
/// \note Original identifier: \c D_80157CEC_scene_springleaf_field (0x80157CEC).
extern WeightedChoiceTable D_80157CEC_scene_springleaf_field;
/// Borrowed world-position triples for scene item scattering.
/// \note Original identifier: \c D_80157CF4_scene_springleaf_field (0x80157CF4).
extern f32 D_80157CF4_scene_springleaf_field[];

/// Entrance index used to initialize the player and companion in the current area.
/// \note Original identifier: \c D_800AE4F4 (0x800AE4F4).
extern s32 gRuntimeCurrentEntrance;
extern f32 D_80134D24_scene_springleaf_field[];
extern f64 D_801594E8_scene_springleaf_field;

#include "scene_configuration.h"
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
        actorCreateForKind(ACTOR_KIND_ODDISH, position[0], position[1], position[2], 0.0F,
                           (f32)((f64)random * RANDOM_U32_UNIT_SCALE * D_801594C8_scene_springleaf_field),
                           mathPowInteger(scale, state) * gCurrentAreaConfiguration->objectScale, record,
                           residentIndex);
        return;
    }
    if (state < 4) {
        // clang-format off
        random = randomNext(); scale = D_801594D0_scene_springleaf_field;
        // clang-format on
        actorCreateForKind(ACTOR_KIND_GLOOM, position[0], position[1], position[2], 0.0F,
                           (f32)((f64)random * RANDOM_U32_UNIT_SCALE * D_801594D8_scene_springleaf_field),
                           mathPowInteger(scale, state - 2) * gCurrentAreaConfiguration->objectScale, record,
                           residentIndex);
        return;
    }
    random = randomNext();
    actorCreateForKind(ACTOR_KIND_VILEPLUME, position[0], position[1], position[2], 0.0F,
                       (f32)((f64)random * RANDOM_U32_UNIT_SCALE * D_801594E0_scene_springleaf_field),
                       gCurrentAreaConfiguration->objectScale, record, residentIndex);
}

/// Prepare scene resources for evolved field residents.
/// \returns Nothing.
void springleafFieldPrepareEvolvedResidents(void) {
    objectRequestHierarchyDestruction(actorCreateForKind(ACTOR_KIND_GLOOM, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, NULL, 0));
    objectRequestHierarchyDestruction(actorCreateForKind(ACTOR_KIND_VILEPLUME, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, NULL, 0));
}

/// Spawn a field item with a randomized heading.
/// \param position Three-component scene position.
/// \param itemKind Supported item-definition identifier.
/// \returns Nothing.
void springleafFieldSpawnItem(f32 *position, s32 itemKind) {
    itemCreate(position[0], position[1], position[2], 0.0F,
               (f32)((f64)randomNext() * RANDOM_U32_UNIT_SCALE * D_801594E8_scene_springleaf_field), itemKind);
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
        if (progressGetValue(i + 0x70) >= 4) {
            count++;
        }
    }
    if (gRuntimeCurrentEntrance != 2 && count == 5) {
        for (i = 0; i != 5; i++) {
            progressSetValue(i + 0x70, 0);
        }
    }
}

typedef struct {
    s32 used[27];
} FieldOccupancy;
extern FieldOccupancy D_80157F70_scene_springleaf_field;
/// Borrowed VM configuration for a resident that repeatedly requests Pikachu's attention.
/// The scene overlay retains this resource for the resident's lifetime.
/// \note Original identifier: \c D_801571E4_scene_springleaf_field (0x801571E4).
extern u8 gResidentAttentionScript_scene_springleaf_field[];
extern u8 D_80157B58_scene_springleaf_field[], D_80157C78_scene_springleaf_field[], D_8014E354_scene_springleaf_field[], D_80159474_scene_springleaf_field[];
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
        actorCreateForKind(ACTOR_KIND_SQUIRTLE, 203.45F, 0.5F, 35.97F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                           gResidentAttentionScript_scene_springleaf_field, 0);
    }
    if (progressGetCurrentChapter() == 8) {
        actorCreateForKind(ACTOR_KIND_HAUNTER, 0, 0, 0, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                           D_80157B58_scene_springleaf_field, 0);
    }
    if (progressGetValue(0x76) >= 3) {
        actorCreateForKind(ACTOR_KIND_BUTTERFREE, 130.0F, 16.0F, 150.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                           D_80157C78_scene_springleaf_field, 0);
    }
    actorCreateForKind(ACTOR_KIND_DIGLETT, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_8014E354_scene_springleaf_field, 0);
    runtimeCheckResetCheckpoint((const char *)D_80159474_scene_springleaf_field, 1);
    springleafFieldPrepareEvolvedResidents();
    springleafFieldResetCompletedGrowth();
    if (progressGetValue(0x4F) == 0) {
        position[0] = 72.0F;
        position[1] = D_801594F0_scene_springleaf_field;
        position[2] = 59.0F;
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
        if (progressGetValue(i + 0x70) >= 2) {
            count++;
        }
        i++;
    } while (i < 5);
    i = 0;
    do {
        selected = randomBelow(27);
        if (!occupied.used[selected]) {
            kind = 0x7A;
            if (i < count) {
                kind = 0x7B;
            }
            springleafFieldSpawnItem(D_80134BE0_scene_springleaf_field[selected], kind);
            occupied.used[selected] = 1;
            i++;
        }
    } while (i < 7);
    mazeScatterItems(D_80157CF4_scene_springleaf_field, 0x35, &D_80157CEC_scene_springleaf_field, 0.7F);
    if (progressGetValue(0x12) == 0) {
        landmarkCreate(D_801594F4_scene_springleaf_field, 10.5F, 35.97F, 0x90);
        landmarkCreate(D_801594F8_scene_springleaf_field, D_801594FC_scene_springleaf_field, 91.0F, 0x8E);
        springleafFieldInitializeDoor();
    }
}

extern s32 *D_8017D658_scene_springleaf_field[SPRINGLEAF_FIELD_GRID_LAYERS];
/// Look up the field spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x80134A44 in the springleaf field overlay.
s32 springleafFieldGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / SPRINGLEAF_FIELD_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= SPRINGLEAF_FIELD_GRID_LAYERS ||
        (ix = (s32)((f64)x / SPRINGLEAF_FIELD_GRID_CELL_WIDTH), ix < 0) || ix >= SPRINGLEAF_FIELD_GRID_SIDE ||
        (iz = (s32)((f64)z / SPRINGLEAF_FIELD_GRID_CELL_WIDTH), iz < 0) || iz >= SPRINGLEAF_FIELD_GRID_SIDE) {
        return 0;
    }
    layer = D_8017D658_scene_springleaf_field[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * SPRINGLEAF_FIELD_GRID_SIDE) + ix];
}
