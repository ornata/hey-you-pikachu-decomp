#include "progress_field_ids.h"
#include "actor_kind.h"
/// Geometry of the bedroom spatial lookup grid.
enum { BEDROOM_GRID_LAYERS = 16, BEDROOM_GRID_SIDE = 16 };
/// Height of one lookup layer in world-coordinate units.
#define BEDROOM_GRID_LAYER_HEIGHT 20.0
/// Width of a lookup cell along either horizontal axis in world-coordinate units.
#define BEDROOM_GRID_CELL_WIDTH 10.0

#include "interaction_proxy.h"
#include "door.h"
#include "common.h"
#include "actor_create.h"
#include "scene_types.h"
#include "item_spawn.h"
/// Provide the bedroom scene callback that requires no action.
/// \returns Nothing.
void bedroomSceneNoOp(void) {
}
#include "area_services.h"
#include "toolbox.h"
#include "scene_configuration.h"
extern f32 D_80134520_scene_bedroom[3];
extern f32 D_8013452C_scene_bedroom[3];
extern ItemSpawn D_801554DC_scene_bedroom[];
extern u8 D_80158674_scene_bedroom[];
extern f32 D_801586DC_scene_bedroom;

/// Set up bedroom scene objects and door access according to saved game state.
/// \returns Nothing.
void bedroomInitializeObjects(void) {
    if (progressGetValue(0xF0) != 0) {
        actorCreateForKind(ACTOR_KIND_TELESCOPE, 93.75F, 31.93000030517578F, 52.189998626708984F, 0.0F,
                           D_801586DC_scene_bedroom, gCurrentAreaConfiguration->objectScale, 0, 0);
    }
    toolboxClearCollection();
    mazeSpawnAvailableItems(D_801554DC_scene_bedroom);
    doorRegistryReset();
    if (progressGetValue(PROGRESS_FIELD_FRONT_YARD_DOOR_ENABLED) != 0) {
        doorCreate(D_80134520_scene_bedroom, 0, 2, 0, 0);
    } else {
        interactionProxyCreate(D_80134520_scene_bedroom, 1, 4, 0, (char *)D_80158674_scene_bedroom);
    }
    doorCreate(D_8013452C_scene_bedroom, 3, 0, 0, 0);
}
extern s32 *D_8015E458_scene_bedroom[BEDROOM_GRID_LAYERS];
/// Look up the bedroom spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
/// \pre Coordinates are finite and scaled indices fit a signed 32-bit integer.
/// \note Negative positions within one cell of zero can select the zero-index cell.
/// \note Original address: \c 0x80134374 in the bedroom overlay.
s32 bedroomGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / BEDROOM_GRID_LAYER_HEIGHT);
    if (iy < 0 || iy >= BEDROOM_GRID_LAYERS || (ix = (s32)((f64)x / BEDROOM_GRID_CELL_WIDTH), ix < 0) ||
        ix >= BEDROOM_GRID_SIDE || (iz = (s32)((f64)z / BEDROOM_GRID_CELL_WIDTH), iz < 0) || iz >= BEDROOM_GRID_SIDE) {
        return 0;
    }
    layer = D_8015E458_scene_bedroom[iy];
    if (layer == 0) {
        return 0;
    }
    return layer[(iz * BEDROOM_GRID_SIDE) + ix];
}
