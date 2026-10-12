#include "story_chapter.h"
#include "door.h"
#include "common.h"
#include "item_object.h"
#include "actor_create.h"
#include "actor_script.h"
#include "scene_types.h"

void doorRegistryReset(void);
void toolboxClearCollection(void);
void mazeSpawnAvailableItems(u8 *);
extern f32 D_80134590_scene_front_yard[3];
extern f32 D_8013459C_scene_front_yard[3];
extern f32 D_801345A8_scene_front_yard[3];
extern u8 D_80158B90_scene_front_yard[];

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

/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

/// Choose the regular front-yard setup or its special encounter according to game progress.
/// \returns Nothing.
void frontYardInitializeObjects(void) {
    u8 *companion;
    u8 *state;

    if (progressGetCurrentChapter() != 9) {
        if (progressGetCurrentChapter() == 8) {
            companion = itemCreate(145.0f, 1.5f, 150.0f, 0.0f, 0.0f, 0x8F);
            state = ((RuntimeObjectPrefix *)actorCreateForKind(
                         0x15, 0.0f, 0.0f, 0.0f, 0.0f, 45.0f,
                         ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale, 0, 0))
                        ->state.bytes;
            ((ActorScriptState *)state)->targetPosition[0] = (f32)((RuntimeObjectPrefix *)companion)->worldPosition[0];
            ((ActorScriptState *)state)->targetPosition[1] = (f32)((RuntimeObjectPrefix *)companion)->worldPosition[1];
            ((ActorScriptState *)state)->targetPosition[2] = (f32)((RuntimeObjectPrefix *)companion)->worldPosition[2];
            ((ActorScriptState *)state)->targetHandle = (s32)((RuntimeObjectPrefix *)companion)->handle;
            ((ActorScriptState *)state)->targetMode = 1;
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
    return layer[iz * 32 + ix];
}
