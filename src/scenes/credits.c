/// Geometry of the credits spatial lookup grid.
enum { CREDITS_GRID_LAYERS = 4, CREDITS_GRID_SIDE = 4 };
/// Height of one lookup layer in world-coordinate units.
#define CREDITS_GRID_LAYER_HEIGHT 20.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define CREDITS_GRID_CELL_WIDTH 20.0

#include "runtime_control.h"
#include "actor_color.h"
#include "object_flags.h"
#include "common.h"
#include "actor_create.h"
#include "scene_types.h"
/// Current area scenario index shared by actor scripts, scene population, cards and collection titles.
/// \note Original identifier: \c D_800F0B70 (0x800F0B70).
extern s32 gAreaScenarioIndex;
extern u8 D_8013B17C_scene_credits[], D_8013B58C_scene_credits[], D_8013BC80_scene_credits[],
    D_8013C0D0_scene_credits[], D_8014382C_scene_credits[], D_80143CD0_scene_credits[], D_801440C0_scene_credits[];
extern u8 D_80143D04_scene_credits[][4];
#include "scene_configuration.h"
/// Provide a credits-scene callback that requires no action.
/// \returns Nothing.
void creditsSceneNoOp80134270(void) {
}
/// Provide a credits-scene callback that requires no action.
/// \returns Nothing.
void creditsSceneNoOp80134278(void) {
}
/// Spawn the type-4 resident for the credits scene.
/// \returns Nothing.
void creditsSpawnResidentType4(void) {
    actorCreateForKind(4, 40.0F, 1.0F, 60.0F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_8013B17C_scene_credits, 0);
}
/// Spawn the type-1 resident for the credits scene.
/// \returns Nothing.
void creditsSpawnResidentType1(void) {
    actorCreateForKind(1, 0, 1.0F, 0, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale, D_8013B17C_scene_credits, 1);
}
/// Spawn the type-3 resident for the credits scene.
/// \returns Nothing.
void creditsSpawnResidentType3(void) {
    actorCreateForKind(3, 0, 1.0F, 0, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale, D_8013B17C_scene_credits, 2);
}
/// Spawn the type-five resident group for the credits scene.
/// \returns Nothing.
void creditsSpawnResidentType5Group(void) {
    s32 i;
    for (i = 0; i != 10; i++) {
        actorCreateForKind(5, 0, 0, 0, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale, D_8013B58C_scene_credits, i);
    }
}
/// Spawn the colored type-twelve resident group for the credits scene.
/// \returns Nothing.
void creditsSpawnTintedResidentGroup(void) {
    s32 i;
    s32 color;
    u8 *object;
    u8 *rgba;
    for (i = 0; i != 5; i++) {
        object = actorCreateForKind(12, 0, 0, (f32)i, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                                    D_8013BC80_scene_credits, i);
        color = i;
        while (color < 0) {
            color += 5;
        }
        while (color >= 5) {
            color -= 5;
        }
        rgba = D_80143D04_scene_credits[color];
        actorTransitionModelEnvironmentColor(object, rgba[0], rgba[1], rgba[2], rgba[3], 0.5F);
    }
}
/// Spawn the type-17 resident for the credits scene.
/// \returns Nothing.
void creditsSpawnResidentType17(void) {
    actorCreateForKind(17, 0, 0, 0, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale, D_8013C0D0_scene_credits, 0);
}
/// Spawn the type-fourteen resident for the alternate credits mode.
/// \returns Nothing.
void creditsSpawnAlternateResidentType14(void) {
    objectSetHierarchyFlags(actorCreateForKind(14, 0, 0, 0, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                                               D_8014382C_scene_credits, 0),
                            OBJECT_DRAW_GROUP_ALTERNATE | OBJECT_UPDATE_GROUP_ALTERNATE);
}
/// Spawn the type-five resident for the alternate credits mode.
/// \returns Nothing.
void creditsSpawnAlternateResidentType5(void) {
    objectSetHierarchyFlags(
        actorCreateForKind(5, 0, 0, 0, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale, D_80143CD0_scene_credits, 0),
        OBJECT_DRAW_GROUP_ALTERNATE | OBJECT_UPDATE_GROUP_ALTERNATE);
}
/// Populate the credits scene for the current game mode.
/// \returns Nothing.
void creditsInitializeObjects(void) {
    if (gAreaScenarioIndex == 0) {
        creditsSpawnResidentType4();
        creditsSpawnResidentType5Group();
        creditsSpawnTintedResidentGroup();
        runtimeCheckResetCheckpoint((const char *)D_801440C0_scene_credits, 1);
        creditsSpawnResidentType17();
        creditsSpawnResidentType1();
        creditsSpawnResidentType3();
    } else if (gAreaScenarioIndex == 1) {
        creditsSpawnAlternateResidentType14();
        creditsSpawnAlternateResidentType5();
    }
}
/// Provide a credits-scene callback that requires no action.
/// \returns Nothing.
void creditsSceneNoOp80134734(void) {
}
extern s32 *D_80144218_scene_credits[CREDITS_GRID_LAYERS];
/// Look up the credits-scene spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x8013473C in the credits overlay.
s32 creditsGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / CREDITS_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= CREDITS_GRID_LAYERS || (ix = (s32)((f64)x / CREDITS_GRID_CELL_WIDTH), ix < 0) ||
        ix >= CREDITS_GRID_SIDE || (iz = (s32)((f64)z / CREDITS_GRID_CELL_WIDTH), iz < 0) || iz >= CREDITS_GRID_SIDE) {
        return 0;
    }
    layer = D_80144218_scene_credits[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * CREDITS_GRID_SIDE) + ix];
}
