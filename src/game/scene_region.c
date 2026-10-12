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

/// Owned bounds and player-highlight state for a rectangular scene region.
typedef struct SceneRegionState {
    f32 halfWidth;        ///< Half-extent along the world X axis.
    f32 halfDepth;        ///< Half-extent along the world Z axis.
    u8 playerHighlighted; ///< Player occupies the region while the companion does not.
    u8 padding09[3];      ///< Unused alignment bytes; no observed reads or writes.
    s32 sceneData;        ///< Opaque scene association supplied by the creator.
} SceneRegionState;

typedef char SceneRegionStateSizeCheck[(sizeof(SceneRegionState) == 0x10) ? 1 : -1];
typedef char SceneRegionStateDataOffsetCheck[(((u32) & (((SceneRegionState *)0)->sceneData)) == 0xC) ? 1 : -1];

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
    obj = objectCreateHierarchyFromTemplate((ObjectTemplate *)&tmpl);
    *(f32 *)(obj + 0) = *(f32 *)(gCurrentAreaConfiguration + 0xC) * width;
    *(f32 *)(obj + 4) = *(f32 *)(gCurrentAreaConfiguration + 0xC);
    *(f32 *)(obj + 8) = *(f32 *)(gCurrentAreaConfiguration + 0xC) * depth;
    *(f32 *)(obj + 0x18) = position[0];
    *(f32 *)(obj + 0x1C) = position[1];
    *(f32 *)(obj + 0x20) = position[2];
    *(f32 *)(obj + 0x44) = position[0];
    *(f32 *)(obj + 0x48) = position[1];
    *(f32 *)(obj + 0x4C) = position[2];
    state = heapAllocLocal(&gGameHeap, 0x10, 0x33);
    state->halfWidth = width;
    state->halfDepth = depth;
    state->playerHighlighted = 0;
    state->sceneData = sceneData;
    *(SceneRegionState **)(obj + 0x54) = state;
    objectRegisterActiveHierarchy(obj);
}

/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern u8 *gPikachuActor;
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern f32 gPlayerState[];
/// Update the region highlight according to player and companion occupancy.
/// \param obj Scene-region object to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001F318 (0x8001F318).
void sceneRegionUpdateHighlight(u8 *obj) {
    SceneRegionState *state = *(SceneRegionState **)(obj + 0x54);
    f32 x = *(f32 *)(obj + 0x44);
    f32 y = *(f32 *)(obj + 0x48);
    f32 z = *(f32 *)(obj + 0x4C);
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
    px = *(f32 *)(gPikachuActor + 0x44);
    py = *(f32 *)(gPikachuActor + 0x48);
    pz = *(f32 *)(gPikachuActor + 0x4C);
    dx = x >= px ? x - px : -(x - px);
    width = state->halfWidth;
    if (dx < width) {
        dz = z >= pz ? z - pz : -(z - pz);
        if (dz < state->halfDepth && y <= py) {
            state->playerHighlighted = 0;
            return;
        }
    }
    dx = x >= cx ? x - cx : -(x - cx);
    if (dx < width) {
        dz = z >= cz ? z - cz : -(z - cz);
        if (dz < state->halfDepth && y <= cy) {
            state->playerHighlighted = 1;
            return;
        }
    }
    state->playerHighlighted = 0;
}
/// Prepare the region color for rendering its current highlight state.
/// \param obj Scene-region object to render.
/// \returns The region geometry display list.
/// \note Original decompilation identifier: \c func_8001F47C (0x8001F47C).
u8 *sceneRegionPrepareDraw(u8 *obj) {
    SceneRegionState *state = *(SceneRegionState **)(obj + 0x54);
    if (state->playerHighlighted == 0) {
        gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 0, 0, 0, 60);
    } else {
        gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 180, 0, 0, 180);
    }
    return gSceneRegionDisplayList;
}
