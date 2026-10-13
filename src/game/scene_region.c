#include "pikachu_actor.h"
#include "scene_region.h"
#include "scene_types.h"
#include "object_types.h"
#include "object_hierarchy_create.h"
#include "object_template.h"
#include "object_lifecycle.h"
#include "common.h"
#include "heap_alloc.h"
#include "game_heap.h"
/// Geometry display list used to render scene-region occupancy highlights.
/// \note Original identifier: \c D_100CCB0 (0x0100CCB0).
extern u8 gSceneRegionDisplayList[];
/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;

/// Hierarchy template used to create rectangular scene interaction regions.
/// \note Original identifier: \c D_80088E60 (0x80088E60).
extern ObjectTemplate gSceneRegionObjectTemplate;
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

/// Create a rectangular scene region with its associated scene data.
/// \param position Three-component world-space region center.
/// \param width Region half-extent along the X axis.
/// \param depth Region half-extent along the Z axis.
/// \param sceneData Opaque scene-data value associated with the region.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F1F0 (0x8001F1F0).
void sceneRegionCreate(f32 *position, f32 width, f32 depth, s32 sceneData) {
    ObjectTemplate tmpl;
    u8 *obj;
    SceneRegionState *state;

    tmpl = gSceneRegionObjectTemplate;
    obj = objectCreateHierarchyFromTemplate(&tmpl);
    ((RuntimeObjectPrefix *)obj)->scale[0] = ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale * width;
    ((RuntimeObjectPrefix *)obj)->scale[1] = ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale;
    ((RuntimeObjectPrefix *)obj)->scale[2] = ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale * depth;
    ((RuntimeObjectPrefix *)obj)->position[0] = position[0];
    ((RuntimeObjectPrefix *)obj)->position[1] = position[1];
    ((RuntimeObjectPrefix *)obj)->position[2] = position[2];
    ((RuntimeObjectPrefix *)obj)->worldPosition[0] = position[0];
    ((RuntimeObjectPrefix *)obj)->worldPosition[1] = position[1];
    ((RuntimeObjectPrefix *)obj)->worldPosition[2] = position[2];
    state = heapAllocLocal(&gGameHeap, 0x10, 0x33);
    state->width = width;
    state->depth = depth;
    state->highlighted = 0;
    state->sceneData = sceneData;
    ((RuntimeObjectPrefix *)obj)->state.pointer = state;
    objectRegisterActiveHierarchy(obj);
}

/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern f32 gPlayerState[];
/// Update the region highlight according to player and companion occupancy.
/// \param obj Scene-region object to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F318 (0x8001F318).
void sceneRegionUpdateHighlight(u8 *obj) {
    SceneRegionState *state = ((RuntimeObjectPrefix *)obj)->state.pointer;
    f32 x = ((RuntimeObjectPrefix *)obj)->worldPosition[0];
    f32 y = ((RuntimeObjectPrefix *)obj)->worldPosition[1];
    f32 z = ((RuntimeObjectPrefix *)obj)->worldPosition[2];
    f32 *other = gPlayerState;
    f32 cx = other[0];
    f32 cy = other[1];
    f32 cz = other[2];
    f32 px;
    f32 py;
    f32 pz;
    f32 dx;
    f32 dz;
    f32 width;
    px = ((RuntimeObjectPrefix *)((u8 *)gPikachuActor))->worldPosition[0];
    py = ((RuntimeObjectPrefix *)((u8 *)gPikachuActor))->worldPosition[1];
    pz = ((RuntimeObjectPrefix *)((u8 *)gPikachuActor))->worldPosition[2];
    dx = x >= px ? x - px : -(x - px);
    width = state->width;
    if (dx < width) {
        dz = z >= pz ? z - pz : -(z - pz);
        if (dz < state->depth && y <= py) {
            state->highlighted = 0;
            return;
        }
    }
    dx = x >= cx ? x - cx : -(x - cx);
    if (dx < width) {
        dz = z >= cz ? z - cz : -(z - cz);
        if (dz < state->depth && y <= cy) {
            state->highlighted = 1;
            return;
        }
    }
    state->highlighted = 0;
}
/// Prepare the region color for rendering its current highlight state.
/// \param obj Scene-region object to render.
/// \returns The region geometry display list.
/// \note Original decompilation identifier: \c func_8001F47C (0x8001F47C).
u8 *sceneRegionPrepareDraw(u8 *obj) {
    SceneRegionState *state = ((RuntimeObjectPrefix *)obj)->state.pointer;
    if (state->highlighted == 0) {
        gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 0, 0, 0, 60);
    } else {
        gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 180, 0, 0, 180);
    }
    return gSceneRegionDisplayList;
}
