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
void toolboxClearCollection(void);
void mazeSpawnAvailableItems(ItemSpawn *);
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
extern f32 D_80134520_scene_bedroom[3];
extern f32 D_8013452C_scene_bedroom[3];
extern ItemSpawn D_801554DC_scene_bedroom[];
extern u8 D_80158674_scene_bedroom[];
extern f32 D_801586DC_scene_bedroom;

/// Set up bedroom scene objects and door access according to saved game state.
/// \returns Nothing.
void bedroomInitializeObjects(void) {
    if (progressGetValue(0xF0) != 0) {
        actorCreateForKind(0x15, 93.75f, 31.93000030517578f, 52.189998626708984f, 0.0f, D_801586DC_scene_bedroom,
                           ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale, 0, 0);
    }
    toolboxClearCollection();
    mazeSpawnAvailableItems(D_801554DC_scene_bedroom);
    doorRegistryReset();
    if (progressGetValue(0xA0) != 0) {
        doorCreate(D_80134520_scene_bedroom, 0, 2, 0, 0);
    } else {
        interactionProxyCreate(D_80134520_scene_bedroom, 1, 4, 0, (char *)D_80158674_scene_bedroom);
    }
    doorCreate(D_8013452C_scene_bedroom, 3, 0, 0, 0);
}
extern s32 *D_8015E458_scene_bedroom[16];
/// Look up the bedroom spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 bedroomGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 20.0);
    if (iy < 0 || iy >= 16 || (ix = (s32)((f64)x / 10.0), ix < 0) || ix >= 16 || (iz = (s32)((f64)z / 10.0), iz < 0) ||
        iz >= 16) {
        return 0;
    }
    layer = D_8015E458_scene_bedroom[iy];
    if (layer == 0)
        return 0;
    return layer[iz * 16 + ix];
}
