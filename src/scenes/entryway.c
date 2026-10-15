#include "common.h"
#include "actor_create.h"
#include "scene_types.h"

#include "toolbox.h"
#include "scene_configuration.h"
extern u8 D_80139E38_scene_entryway[];

/// Initialize entryway scene state and spawn its resident creature.
/// \returns Nothing.
void entrywayInitializeObjects(void) {
    toolboxClearCollection();
    actorCreateForKind(1, 82.26F, 3.0F, 104.37F, 0.0F, 0.0F, gCurrentAreaConfiguration->objectScale,
                       D_80139E38_scene_entryway, 0);
}

extern s32 *D_8013C888_scene_entryway[16];
/// Look up the entryway spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 entrywayGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;

    iy = (s32)((f64)y / 21.0);
    if (iy < 0 || iy >= 16 || (ix = (s32)((f64)x / 10.0), ix < 0) || ix >= 16 || (iz = (s32)((f64)z / 10.0), iz < 0) ||
        iz >= 16) {
        return 0;
    }
    layer = D_8013C888_scene_entryway[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * 16) + ix];
}
