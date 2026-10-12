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

u8 *ambientAudioCreate(f32 *, s32, f32, f32, s32, s32, f32, s32, s32);
void doorRegistryReset(void);
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
    ambientAudioCreate(D_8014A188_scene_ochre_deep, 0, 10000.0f, 0.0f, 0x88, 0x88, 1.0f, 2,
                  (s32)D_8014A188_scene_ochre_deep);
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
        mazeReplaceBlock(D_80134C98_scene_ochre_deep + i * 12, D_80148084_scene_ochre_deep,
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

u8 playerGetChaseStage(void);
void itemHideAndSuspend(s32);
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
extern f32 D_8014AB90_scene_ochre_deep;
extern u8 D_801479C0_scene_ochre_deep[];
extern u8 D_801478D8_scene_ochre_deep[];
/// Create the special deep-forest encounter when saved progress and the scene variant permit it.
/// \returns One when the encounter is created; otherwise zero.
s32 ochreDeepTryCreateProgressEncounter(void) {
    u8 *item;
    u8 *effect;
    u8 *state;

    if (progressGetValue(0x15) == 0) {
        if (playerGetChaseStage() > 0) {
            item = itemCreate(D_8014AB90_scene_ochre_deep, 17.75f, 31.92f, 0.0f, 0.0f, 0x8F);
            effect = actorCreateForKind(0x17, 0, 0, 0, 0, 45.0f, *(f32 *)(gCurrentAreaConfiguration + 0xC) * 0.5,
                                        D_801479C0_scene_ochre_deep, 0);
            state = *(u8 **)(effect + 0x54);
            *(f32 *)(item + 8) = *(f32 *)(item + 4) = *(f32 *)(item + 0) = 0;
            *(f32 *)(state + 0x1EC) = *(f32 *)(item + 0x44);
            *(f32 *)(state + 0x1F0) = *(f32 *)(item + 0x48);
            *(f32 *)(state + 0x1F4) = *(f32 *)(item + 0x4C);
            *(s32 *)(state + 0x198) = *(s32 *)(item + 0x38);
            *(s32 *)(state + 0x19C) = 1;
            itemHideAndSuspend(*(s32 *)(item + 0x38));
            actorCreateForKind(0xF, 0, 0, 0, 0, 0, *(f32 *)(gCurrentAreaConfiguration + 0xC),
                               D_801478D8_scene_ochre_deep, 0);
            return 1;
        }
    }
    return 0;
}

s32 ochreDeepTryCreateProgressEncounter(void);
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;
extern u8 D_801475C8_scene_ochre_deep;
extern u8 D_80147AE0_scene_ochre_deep;


extern u8 D_8014A0EC_scene_ochre_deep;
extern u8 D_8014A110_scene_ochre_deep;
extern u8 D_8014A134_scene_ochre_deep;
extern u8 D_8014A164_scene_ochre_deep;
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

    actorCreateForKind(0x13, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       &D_801475C8_scene_ochre_deep, 0);
    if ((ochreDeepTryCreateProgressEncounter() == 0) && (progressGetValue(0x76) >= 2)) {
        actorCreateForKind(0xE, 83.5f, 20.8f, 52.0f, 0, 0,
                           ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                           &D_80147AE0_scene_ochre_deep, 0);
    }
    heapDebugReportFreeSpace(&gGameHeap, (const char *)&D_8014AB48_scene_ochre_deep);
    if (playerGetChaseStage() == 0) {
        mazeScatterItems(D_80147AF4_scene_ochre_deep, 0x22, &D_8014807C_scene_ochre_deep, 0.7f);
    } else if (playerGetChaseStage() == 1) {
        mazeScatterItems(D_80147AF4_scene_ochre_deep, 0x22, &D_8014807C_scene_ochre_deep, 0.4f);
        mazeScatterItems(D_80147C8C_scene_ochre_deep, 0x24, &D_8014807C_scene_ochre_deep, 0.7f);
    } else if (playerGetChaseStage() == 2) {
        mazeScatterItems(D_80147AF4_scene_ochre_deep, 0x22, &D_8014807C_scene_ochre_deep, 0.2f);
        mazeScatterItems(D_80147C8C_scene_ochre_deep, 0x24, &D_8014807C_scene_ochre_deep, 0.3f);
        mazeScatterItems(D_80147E3C_scene_ochre_deep, 0x22, &D_8014807C_scene_ochre_deep, 0.5f);
    }
    heapDebugReportFreeSpace(&gGameHeap, (const char *)&D_8014AB54_scene_ochre_deep);
    position[0] = 130.0f;
    position[1] = D_8014AB94_scene_ochre_deep;
    position[2] = D_8014AB98_scene_ochre_deep;
    sceneRegionCreate(position, 9.0f, 9.0f, (s32)&D_8014A0EC_scene_ochre_deep);
    position[0] = 217.0f;
    position[1] = D_8014AB9C_scene_ochre_deep;
    position[2] = 151.0f;
    sceneRegionCreate(position, 41.0f, 38.0f, (s32)&D_8014A110_scene_ochre_deep);
    position[0] = 74.0f;
    position[1] = D_8014ABA0_scene_ochre_deep;
    position[2] = 225.0f;
    sceneRegionCreate(position, 25.0f, 13.0f, (s32)&D_8014A134_scene_ochre_deep);
    position[0] = 83.5f;
    position[1] = D_8014ABA4_scene_ochre_deep;
    position[2] = 52.0f;
    sceneRegionCreate(position, 61.5f, 38.0f, (s32)&D_8014A164_scene_ochre_deep);
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

extern s32 *D_8018C048_scene_ochre_deep[32];
/// Look up the deep-forest spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 ochreDeepGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 10.0);
    if (iy < 0 || iy >= 32 || (ix = (s32)((f64)x / 10.0), ix < 0) || ix >= 32 || (iz = (s32)((f64)z / 10.0), iz < 0) ||
        iz >= 32)
        return 0;
    layer = D_8018C048_scene_ochre_deep[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 32 + ix];
}
