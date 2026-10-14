#include "random.h"
#include "common.h"
#include "actor_create.h"

#include "togepi.h"
#include "scene_configuration.h"
#include "scene_types.h"
extern u8 D_8013D380_scene_ochre_entrance[], D_8013DD80_scene_ochre_entrance[];
extern f32 D_8013E8A0_scene_ochre_entrance;

/// Populate the forest entrance with residents appropriate to saved progress.
/// \returns Nothing.
void ochreEntranceInitializeResidents(void) {
    switch (progressGetValue(0x56)) {
    case 0:
        break;
    default:
        if (progressGetValue(0x16) != 0 && randomBelow(3) == 0) {
            togepiCreate(D_8013E8A0_scene_ochre_entrance, 18.25f, 149.5f, 0.0f, 90.0f, D_8013DD80_scene_ochre_entrance,
                          3);
        } else {
            actorCreateForKind(0xC, 192.5f, 18.25f, 149.5f, 0.0f, 90.0f, gCurrentAreaConfiguration->objectScale * 1.5f,
                               D_8013DD80_scene_ochre_entrance, 3);
        }
    case 2:
        actorCreateForKind(3, 187.5f, 18.25f, 164.0f, 0.0f, 100.0f, gCurrentAreaConfiguration->objectScale,
                           D_8013DD80_scene_ochre_entrance, 2);
    case 1:
        actorCreateForKind(4, 181.5f, 18.25f, 164.5f, 0.0f, 100.0f, gCurrentAreaConfiguration->objectScale,
                           D_8013DD80_scene_ochre_entrance, 1);
        break;
    }
    actorCreateForKind(1, 168.0f, 18.0f, 144.5f, 0.0f, 0.0f, gCurrentAreaConfiguration->objectScale,
                       D_8013D380_scene_ochre_entrance, 0);
}

extern s32 *D_80174674_scene_ochre_entrance[16];
/// Look up the forest-entrance spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 ochreEntranceGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;

    iy = (s32)((f64)y / 20.0);
    if (iy < 0 || iy >= 16 || (ix = (s32)((f64)x / 20.0), ix < 0) || ix >= 16 || (iz = (s32)((f64)z / 20.0), iz < 0) ||
        iz >= 16) {
        return 0;
    }
    layer = D_80174674_scene_ochre_entrance[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[iz * 16 + ix];
}
