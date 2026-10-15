#include "progress_field_ids.h"
#include "actor_kind.h"
/// Geometry of the abra shop spatial lookup grid.
enum { ABRA_SHOP_GRID_LAYERS = 8, ABRA_SHOP_GRID_SIDE = 8 };
/// Height of one lookup layer in world-coordinate units.
#define ABRA_SHOP_GRID_LAYER_HEIGHT 20.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define ABRA_SHOP_GRID_CELL_WIDTH 20.0

#include "scene_types.h"
#include "actor_target.h"
#include "object_types.h"
#include "actor_script.h"
#include "story_chapter.h"
#include "common.h"
#include "item_object.h"
#include "actor_create.h"

#include "scene_configuration.h"
extern u8 D_801431F8_scene_abra_shop[], D_8014363C_scene_abra_shop[];

/// Set up shop residents and any special encounter enabled by saved game state.
/// \returns Nothing.
void abraShopInitializeObjects(void) {
    s32 level;
    f32 scale;
    u8 *trackedItem;
    ActorScriptState *actorState;

    actorCreateForKind(ACTOR_KIND_ABRA, 100.0F, 1.2F, 95.0F, 0, 0, gCurrentAreaConfiguration->objectScale * 1.5F,
                       D_801431F8_scene_abra_shop, 0);
    if (progressGetValue(0x15) != 0 && progressGetValue(PROGRESS_FIELD_TOGEPI_HATCHED) == 0) {
        level = progressGetValue(PROGRESS_FIELD_TOGEPI_EGG_GROWTH_STAGE);
        scale = (f32)level;
        actorCreateForKind(ACTOR_KIND_TOGEPI_EGG, 100.0F, 0.0F, 0.0F, 0, 0,
                           ((scale / 5.0F + 1.0F) * (gCurrentAreaConfiguration->objectScale * 0.5F)),
                           D_8014363C_scene_abra_shop, 0);
    }
    if (progressGetCurrentChapter() == 8) {
        trackedItem = itemCreate(145.0F, 1.5F, 150.0F, 0.0F, 0.0F, 0x8F);
        actorState = ((RuntimeObjectPrefix *)actorCreateForKind(ACTOR_KIND_TELESCOPE, 0.0F, 0.0F, 0.0F, 0.0F, 45.0F,
                                                                gCurrentAreaConfiguration->objectScale, 0, 0))
                         ->state.actor;
        actorState->targetPosition[0] = ((RuntimeObjectPrefix *)trackedItem)->worldPosition[0];
        actorState->targetPosition[1] = ((RuntimeObjectPrefix *)trackedItem)->worldPosition[1];
        actorState->targetPosition[2] = ((RuntimeObjectPrefix *)trackedItem)->worldPosition[2];
        actorState->targetHandle = ((RuntimeObjectPrefix *)trackedItem)->handle;
        actorState->targetMode = ACTOR_TARGET_FOLLOW;
    }
}

/// Provide the shop scene callback that requires no action.
/// \returns Nothing.
void abraShopSceneNoOp(void) {
}

extern s32 *D_8014743C_scene_abra_shop[ABRA_SHOP_GRID_LAYERS];
/// Look up the shop spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x8013443C in the abra shop overlay.
s32 abraShopGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;

    iy = (s32)((f64)y / ABRA_SHOP_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= ABRA_SHOP_GRID_LAYERS || (ix = (s32)((f64)x / ABRA_SHOP_GRID_CELL_WIDTH), ix < 0) ||
        ix >= ABRA_SHOP_GRID_SIDE || (iz = (s32)((f64)z / ABRA_SHOP_GRID_CELL_WIDTH), iz < 0) ||
        iz >= ABRA_SHOP_GRID_SIDE) {
        return 0;
    }
    layer = D_8014743C_scene_abra_shop[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * ABRA_SHOP_GRID_SIDE) + ix];
}
