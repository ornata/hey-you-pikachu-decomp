#include "common.h"
#include "actor_create.h"
#include "scene_types.h"

#include "scene_configuration.h"
extern u8 D_8013A878_scene_wilderness_camp[];
extern s32 *D_8013CE38_scene_wilderness_camp[16];

/// Spawn the Wilderness Camp resident creature with its scene behavior.
/// \returns Nothing.
void wildernessCampSpawnResident(void) {
    actorCreateForKind(0x11, 125.0f, 10.0f, 122.27f, 0.0f, 0.0f, gCurrentAreaConfiguration->objectScale,
                       D_8013A878_scene_wilderness_camp, 0);
}

/// Provide the Wilderness Camp scene callback that requires no action.
/// \returns Nothing.
void wildernessCampSceneNoOp(void) {
}

/// Look up the Wilderness Camp spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 wildernessCampGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 20.0);
    if (iy < 0 || iy >= 16 || (ix = (s32)((f64)x / 20.0), ix < 0) || ix >= 16 || (iz = (s32)((f64)z / 20.0), iz < 0) ||
        iz >= 16) {
        return 0;
    }
    layer = D_8013CE38_scene_wilderness_camp[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 16 + ix];
}
