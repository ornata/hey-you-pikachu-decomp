#include "actor_kind.h"
/// Geometry of the ochre deep spatial lookup grid.
enum { OCHRE_DEEP_GRID_LAYERS = 32, OCHRE_DEEP_GRID_SIDE = 32 };
/// Height of one lookup layer in world-coordinate units.
#define OCHRE_DEEP_GRID_LAYER_HEIGHT 10.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define OCHRE_DEEP_GRID_CELL_WIDTH 10.0

#include "actor_target.h"
#include "object_types.h"
#include "actor_script.h"
#include "game_heap.h"
#include "player_chase.h"
#include "ambient_audio.h"
#include "item_participation.h"
#include "maze_scatter.h"
#include "scene_region.h"
#include "door.h"
#include "common.h"
#include "item_object.h"
#include "actor_create.h"
#include "area_maze.h"
#include "heap_diagnostics.h"
#include "scene_types.h"

/// Mutable weighted item-kind choices retained by the scene overlay.
/// \note Original identifier: \c D_8014807C_scene_ochre_deep (0x8014807C).
extern WeightedChoiceTable D_8014807C_scene_ochre_deep;
/// Borrowed world-position triples for scene item scattering.
/// \note Original identifier: \c D_80147E3C_scene_ochre_deep (0x80147E3C).
extern f32 D_80147E3C_scene_ochre_deep[];
/// Borrowed world-position triples for scene item scattering.
/// \note Original identifier: \c D_80147C8C_scene_ochre_deep (0x80147C8C).
extern f32 D_80147C8C_scene_ochre_deep[];
/// Borrowed world-position triples for scene item scattering.
/// \note Original identifier: \c D_80147AF4_scene_ochre_deep (0x80147AF4).
extern f32 D_80147AF4_scene_ochre_deep[];

extern f32 D_8014A188_scene_ochre_deep[];
extern f32 D_80134C80_scene_ochre_deep[];
extern u8 D_80134C98_scene_ochre_deep[];
extern u8 D_80134CBC_scene_ochre_deep[];
extern u8 D_80148084_scene_ochre_deep[];
extern u8 D_801481AC_scene_ochre_deep[];
extern u8 D_80134CC8_scene_ochre_deep[];
extern u8 D_80134CD4_scene_ochre_deep[];
extern u8 D_801481BC_scene_ochre_deep[];
extern u8 D_8014859C_scene_ochre_deep[];
extern u8 D_801485AC_scene_ochre_deep[];
extern u8 D_801490E4_scene_ochre_deep[];
extern u8 D_801490F4_scene_ochre_deep[];
extern u8 D_80149BBC_scene_ochre_deep[];
extern u8 D_80149BCC_scene_ochre_deep[];
extern u8 D_80149C54_scene_ochre_deep[];
extern u8 D_80149C64_scene_ochre_deep[];
extern u8 D_8014A044_scene_ochre_deep[];
extern u8 D_8014A054_scene_ochre_deep[];
extern u8 D_8014A0DC_scene_ochre_deep[];

/// Create the deep-forest scene's ambient audio emitter.
/// \returns Nothing.
void ochreDeepCreateAmbientAudio(void) {
    ambientAudioCreate(D_8014A188_scene_ochre_deep, 0, 10000.0F, 0.0F, 0x88, 0x88, 1.0F, 2,
                       (Vec3f *)D_8014A188_scene_ochre_deep);
}

/// Initialize the deep-forest scene door.
/// \returns Nothing.
void ochreDeepInitializeDoor(void) {
    doorRegistryReset();
    doorCreate(D_80134C80_scene_ochre_deep, 1, 0xF, 1, 3);
}

/// Create the additional props for the third deep-forest scene variant.
/// \returns Nothing.
void ochreDeepCreateVariantTwoProps(void) {
    s32 i;
    for (i = 0; i < 3; i++) {
        mazeReplaceBlock(D_80134C98_scene_ochre_deep + (i * 12), D_80148084_scene_ochre_deep,
                         (MazeGrid *)D_801481AC_scene_ochre_deep, 0);
    }
}

/// Create the props for the first deep-forest scene variant.
/// \returns Nothing.
void ochreDeepCreateVariantZeroProps(void) {
    mazeReplaceBlock(D_80134CBC_scene_ochre_deep, D_801481BC_scene_ochre_deep, (MazeGrid *)D_8014859C_scene_ochre_deep,
                     1);
    mazeReplaceBlock(D_80134CC8_scene_ochre_deep, D_801485AC_scene_ochre_deep, (MazeGrid *)D_801490E4_scene_ochre_deep,
                     1);
    mazeReplaceBlock(D_80134CD4_scene_ochre_deep, D_801490F4_scene_ochre_deep, (MazeGrid *)D_80149BBC_scene_ochre_deep,
                     1);
}

/// Create the props shared by the later deep-forest scene variants.
/// \returns Nothing.
void ochreDeepCreateAlternateVariantProps(void) {
    mazeReplaceBlock(D_80134CBC_scene_ochre_deep, D_80149BCC_scene_ochre_deep, (MazeGrid *)D_80149C54_scene_ochre_deep,
                     1);
    mazeReplaceBlock(D_80134CC8_scene_ochre_deep, D_80149C64_scene_ochre_deep, (MazeGrid *)D_8014A044_scene_ochre_deep,
                     1);
    mazeReplaceBlock(D_80134CD4_scene_ochre_deep, D_8014A054_scene_ochre_deep, (MazeGrid *)D_8014A0DC_scene_ochre_deep,
                     1);
}

#include "scene_configuration.h"
extern f32 D_8014AB90_scene_ochre_deep;
extern u8 D_801479C0_scene_ochre_deep[];
extern u8 D_801478D8_scene_ochre_deep[];
/// Create the special deep-forest encounter when saved progress and the scene variant permit it.
/// \returns One when the encounter is created; otherwise zero.
s32 ochreDeepTryCreateProgressEncounter(void) {
    u8 *item;
    u8 *encounterActor;
    ActorScriptState *actorState;

    if (progressGetValue(0x15) == 0) {
        if (playerGetChaseStage() > 0) {
            item = itemCreate(D_8014AB90_scene_ochre_deep, 17.75F, 31.92F, 0.0F, 0.0F, 0x8F);
            encounterActor =
                actorCreateForKind(ACTOR_KIND_TOGEPI_EGG, 0, 0, 0, 0, 45.0F, *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * 0.5,
                                   D_801479C0_scene_ochre_deep, 0);
            actorState = ((RuntimeObjectPrefix *)encounterActor)->state.actor;
            *(f32 *)(item + 8) = *(f32 *)(item + 4) = *(f32 *)(item + 0) = 0;
            actorState->targetPosition[0] = ((RuntimeObjectPrefix *)item)->worldPosition[0];
            actorState->targetPosition[1] = ((RuntimeObjectPrefix *)item)->worldPosition[1];
            actorState->targetPosition[2] = ((RuntimeObjectPrefix *)item)->worldPosition[2];
            actorState->targetHandle = ((RuntimeObjectPrefix *)item)->handle;
            actorState->targetMode = ACTOR_TARGET_FOLLOW;
            itemHideAndSuspend(((RuntimeObjectPrefix *)item)->handle);
            actorCreateForKind(ACTOR_KIND_ABRA, 0, 0, 0, 0, 0, *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC),
                               D_801478D8_scene_ochre_deep, 0);
            return 1;
        }
    }
    return 0;
}

extern u8 D_801475C8_scene_ochre_deep;
extern u8 D_80147AE0_scene_ochre_deep;


extern f32 D_8014A0EC_scene_ochre_deep[];
extern f32 D_8014A110_scene_ochre_deep[];
extern f32 D_8014A134_scene_ochre_deep[];
extern f32 D_8014A164_scene_ochre_deep[];
extern u8 D_8014AB48_scene_ochre_deep;
extern u8 D_8014AB54_scene_ochre_deep;
extern f32 D_8014AB94_scene_ochre_deep;
extern f32 D_8014AB98_scene_ochre_deep;
extern f32 D_8014AB9C_scene_ochre_deep;
extern f32 D_8014ABA0_scene_ochre_deep;
extern f32 D_8014ABA4_scene_ochre_deep;
/// Populate the deep forest with its encounter, residents, scattered items, props, ambient audio, and door.
/// \returns Nothing.
void ochreDeepInitializeObjects(void) {
    f32 position[3];

    actorCreateForKind(ACTOR_KIND_MAGNEMITE, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       &D_801475C8_scene_ochre_deep, 0);
    if ((ochreDeepTryCreateProgressEncounter() == 0) && (progressGetValue(0x76) >= 2)) {
        actorCreateForKind(ACTOR_KIND_BUTTERFREE, 83.5F, 20.8F, 52.0F, 0, 0, gCurrentAreaConfiguration->objectScale,
                           &D_80147AE0_scene_ochre_deep, 0);
    }
    heapDebugReportFreeSpace(&gGameHeap, (const char *)&D_8014AB48_scene_ochre_deep);
    if (playerGetChaseStage() == 0) {
        mazeScatterItems(D_80147AF4_scene_ochre_deep, 0x22, &D_8014807C_scene_ochre_deep, 0.7F);
    } else if (playerGetChaseStage() == 1) {
        mazeScatterItems(D_80147AF4_scene_ochre_deep, 0x22, &D_8014807C_scene_ochre_deep, 0.4F);
        mazeScatterItems(D_80147C8C_scene_ochre_deep, 0x24, &D_8014807C_scene_ochre_deep, 0.7F);
    } else if (playerGetChaseStage() == 2) {
        mazeScatterItems(D_80147AF4_scene_ochre_deep, 0x22, &D_8014807C_scene_ochre_deep, 0.2F);
        mazeScatterItems(D_80147C8C_scene_ochre_deep, 0x24, &D_8014807C_scene_ochre_deep, 0.3F);
        mazeScatterItems(D_80147E3C_scene_ochre_deep, 0x22, &D_8014807C_scene_ochre_deep, 0.5F);
    }
    heapDebugReportFreeSpace(&gGameHeap, (const char *)&D_8014AB54_scene_ochre_deep);
    position[0] = 130.0F;
    position[1] = D_8014AB94_scene_ochre_deep;
    position[2] = D_8014AB98_scene_ochre_deep;
    sceneRegionCreate(position, 9.0F, 9.0F, D_8014A0EC_scene_ochre_deep);
    position[0] = 217.0F;
    position[1] = D_8014AB9C_scene_ochre_deep;
    position[2] = 151.0F;
    sceneRegionCreate(position, 41.0F, 38.0F, D_8014A110_scene_ochre_deep);
    position[0] = 74.0F;
    position[1] = D_8014ABA0_scene_ochre_deep;
    position[2] = 225.0F;
    sceneRegionCreate(position, 25.0F, 13.0F, D_8014A134_scene_ochre_deep);
    position[0] = 83.5F;
    position[1] = D_8014ABA4_scene_ochre_deep;
    position[2] = 52.0F;
    sceneRegionCreate(position, 61.5F, 38.0F, D_8014A164_scene_ochre_deep);
    if (playerGetChaseStage() == 2) {
        ochreDeepCreateVariantTwoProps();
    }
    if (playerGetChaseStage() == 0) {
        ochreDeepCreateVariantZeroProps();
    } else {
        ochreDeepCreateAlternateVariantProps();
    }
    ochreDeepCreateAmbientAudio();
    ochreDeepInitializeDoor();
}

extern s32 *D_8018C048_scene_ochre_deep[OCHRE_DEEP_GRID_LAYERS];
/// Look up the deep-forest spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x801348FC in the ochre deep overlay.
s32 ochreDeepGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / OCHRE_DEEP_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= OCHRE_DEEP_GRID_LAYERS || (ix = (s32)((f64)x / OCHRE_DEEP_GRID_CELL_WIDTH), ix < 0) ||
        ix >= OCHRE_DEEP_GRID_SIDE || (iz = (s32)((f64)z / OCHRE_DEEP_GRID_CELL_WIDTH), iz < 0) ||
        iz >= OCHRE_DEEP_GRID_SIDE) {
        return 0;
    }
    layer = D_8018C048_scene_ochre_deep[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * OCHRE_DEEP_GRID_SIDE) + ix];
}
