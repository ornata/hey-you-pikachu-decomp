#include "landmark.h"
#include "cave.h"
#include "common.h"
#include "actor_create.h"
#include "scene_types.h"
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
extern u8 D_8013EE54_scene_playground[];
extern f32 D_801344B0_scene_playground[];
extern f32 D_801344BC_scene_playground[];
extern f32 D_801344D4_scene_playground[];
extern s32 *D_8015A04C_scene_playground[8];
/// Spawn the playground resident creature with its scene behavior.
/// \returns Nothing.
void playgroundSpawnResident(void) {
    actorCreateForKind(0x13, 320.0f, 10.0f, 225.0f, 0.0f, 0.0f,
                       ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale,
                       D_8013EE54_scene_playground, 0);
}
/// Create the playground cave, its linked landmark, and the resident creature.
/// \returns Nothing.
void playgroundInitializeObjects(void) {
    u8 *object;
    object = caveCreate(D_801344D4_scene_playground, D_801344B0_scene_playground, D_801344BC_scene_playground, 0);
    caveSetLinkedLandmark(object, landmarkCreate(D_801344B0_scene_playground[6], D_801344B0_scene_playground[7],
                                                      D_801344B0_scene_playground[8], 0x90));
    playgroundSpawnResident();
}
/// Look up the playground spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 playgroundGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;
    iy = (s32)((f64)y / 20.0);
    if (iy < 0 || iy >= 8 || (ix = (s32)((f64)x / 40.0), ix < 0) || ix >= 8 || (iz = (s32)((f64)z / 40.0), iz < 0) ||
        iz >= 8) {
        return 0;
    }
    layer = D_8015A04C_scene_playground[iy];
    if (layer == NULL)
        return 0;
    return layer[iz * 8 + ix];
}
