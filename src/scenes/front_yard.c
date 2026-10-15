#include "area_services.h"
#include "actor_target.h"
#include "story_chapter.h"
#include "door.h"
#include "common.h"
#include "item_object.h"
#include "actor_create.h"
#include "actor_script.h"
#include "scene_types.h"

#include "toolbox.h"
extern f32 D_80134590_scene_front_yard[3];
extern f32 D_8013459C_scene_front_yard[3];
extern f32 D_801345A8_scene_front_yard[3];
extern ItemSpawn D_80158B90_scene_front_yard[];

/// Set up regular front-yard objects and destination gates according to game progress.
/// \returns Nothing.
void frontYardInitializeRegularObjects(void) {
    s32 enabled;

    toolboxClearCollection();
    mazeSpawnAvailableItems(D_80158B90_scene_front_yard);
    doorRegistryReset();
    doorCreate(D_80134590_scene_front_yard, 0, 0, 1, 0);
    doorCreate(D_8013459C_scene_front_yard, 2, 1, 0, 0);
    enabled = 1;
    if (progressGetCurrentChapter() >= 7) {
        enabled = 0;
    }
    doorCreate(D_801345A8_scene_front_yard, 2, 2, 0, enabled);
}

#include "scene_configuration.h"

/// Choose the regular front-yard setup or its special encounter according to game progress.
/// \returns Nothing.
void frontYardInitializeObjects(void) {
    u8 *companion;
    u8 *state;

    if (progressGetCurrentChapter() != 9) {
        if (progressGetCurrentChapter() == 8) {
            companion = itemCreate(145.0F, 1.5F, 150.0F, 0.0F, 0.0F, 0x8F);
            state = ((RuntimeObjectPrefix *)actorCreateForKind(0x15, 0.0F, 0.0F, 0.0F, 0.0F, 45.0F,
                                                               gCurrentAreaConfiguration->objectScale, 0, 0))
                        ->state.bytes;
            ((ActorScriptState *)state)->targetPosition[0] = ((RuntimeObjectPrefix *)companion)->worldPosition[0];
            ((ActorScriptState *)state)->targetPosition[1] = ((RuntimeObjectPrefix *)companion)->worldPosition[1];
            ((ActorScriptState *)state)->targetPosition[2] = ((RuntimeObjectPrefix *)companion)->worldPosition[2];
            ((ActorScriptState *)state)->targetHandle = ((RuntimeObjectPrefix *)companion)->handle;
            ((ActorScriptState *)state)->targetMode = ACTOR_TARGET_FOLLOW;
        } else {
            frontYardInitializeRegularObjects();
        }
    }
}

extern s32 *D_801670A8_scene_front_yard[32];
/// Look up the front-yard spatial-grid value at a world position.
/// \param x World-space X coordinate.
/// \param y World-space height coordinate.
/// \param z World-space Z coordinate.
/// \returns The spatial-cell value, or zero when the position maps outside the grid or to an absent layer.
s32 frontYardGetSpatialCell(f32 x, f32 y, f32 z) {
    s32 *layer;
    s32 iy;
    s32 ix;
    s32 iz;

    iy = (s32)((f64)y / 10.0);
    if (iy < 0 || iy >= 32 || (ix = (s32)((f64)x / 10.0), ix < 0) || ix >= 32 || (iz = (s32)((f64)z / 10.0), iz < 0) ||
        iz >= 32) {
        return 0;
    }
    layer = D_801670A8_scene_front_yard[iy];
    if (layer == NULL) {
        return 0;
    }
    return layer[(iz * 32) + ix];
}
