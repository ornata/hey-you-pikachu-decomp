#include "player_profile.h"
#include "player_state.h"
#include "render_state.h"
#include "pikachu_actor.h"
#include "object_groups.h"
#include "math_angles.h"
#include "camera_overlay.h"
#include "terrain_height.h"
#include "terrain_surface.h"
#include "text_window.h"
#include "progress_value.h"
#include "action_script_playback.h"
#include "object_transform.h"
#include "object_matrix_buffers.h"
#include "object_create.h"
#include "object_lifecycle.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "text_image_create.h"
#include "object_lookup.h"
#include "heap_alloc.h"
#include "game_heap.h"
#include "object_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

#include "scene_configuration.h"
/// Friendship messages indexed by the cleared chapter.
/// \note Original identifier: \c D_800999F0 (0x800999F0).
extern s32 gChapterFriendshipMessages[];


/// Separate view of the message-bar image slot prepared for the next transition.
/// \note Original identifier: \c D_800F0B1C (0x800F0B1C).
extern s32 gMessageBarWorkImageIndex;


/// Interior availability progress-flag view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125AB2 (0x80125AB2).
extern u8 gItemKindAvailabilityFlagEntries[];

/// Format string used to create the collection title text image.
/// \note Original identifier: \c D_800A558C (0x800A558C).
extern s8 gCollectionTitleTextFormat;
/// Owned text image for the collection title.
/// \note Original identifier: \c D_800F0BE8 (0x800F0BE8).
extern s32 gCollectionPresentationTitleImage;

/// Format string used to create the collection score text image.
/// \note Original identifier: \c D_800A559C (0x800A559C).
extern s8 gCollectionScoreTextFormat;
/// Accumulated collection value awarded to the player at the end of results.
/// \note Original identifier: \c D_800F0BD8 (0x800F0BD8).
extern s32 gCollectionPresentationScore;

/// Live item displayed during collection results, or zero when none is shown.
/// \note Original identifier: \c D_800F0BDC (0x800F0BDC).
extern s32 gCollectionPresentationItem;

/// Interior collection-value view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125AAC (0x80125AAC).
extern s32 gItemKindCollectionValueEntries[];


/// Owned render state of one terrain diagnostic cell.
typedef struct TerrainDebugCellState {
    s32 enabled;          ///< Nonzero when this cell contributes geometry.
    u32 matrixIndices[4]; ///< Root hierarchy matrix indices for the four corners.
    u8 color[3];          ///< Terrain-property diagnostic red, green and blue values.
    u8 padding17;         ///< Retained alignment byte.
} TerrainDebugCellState;

typedef char TerrainDebugCellStateSizeCheck[(sizeof(TerrainDebugCellState) == 0x18) ? 1 : -1];
typedef char TerrainDebugCellStateColorOffsetCheck[(((u32) & (((TerrainDebugCellState *)0)->color)) == 0x14) ? 1 : -1];

/// Initialize the root state of a terrain diagnostic grid.
/// \param object Grid root object to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002AF50 (0x8002AF50).
void terrainDebugGridInitializeRoot(u8 *object) {
    TRACE("meshRootInit:%p\n", object);
    ((RuntimeObjectPrefix *)object)->state.pointer = heapAllocLocal(&gGameHeap, 4, 0x38);
}

/// Small player view preserving the diagnostic grid's matching load scheduling.
/// \note The full player prefix changes generated code in terrainDebugGridUpdatePosition.
typedef struct PlayerGridView {
    f32 x; ///< Primary player world X position.
    f32 pad4[2]; ///< Y and Z position storage accessed through separate symbols.
    f32 yaw; ///< View yaw in degrees.
    f32 pitchOffset; ///< Walking pitch offset in degrees.
    f32 pitch; ///< View pitch in degrees.
    f32 viewShake; ///< Horizontal shake added to yaw in degrees.
} PlayerGridView;
typedef char PlayerGridViewSizeCheck[(sizeof(PlayerGridView) == 0x1C) ? 1 : -1];
typedef char PlayerGridYawOffsetCheck[(((u32)&((PlayerGridView *)0)->yaw) == 0xC) ? 1 : -1];
typedef char PlayerGridShakeOffsetCheck[(((u32)&((PlayerGridView *)0)->viewShake) == 0x18) ? 1 : -1];
/// Separate symbol view of the primary player record's position Y.
/// \note Separate position symbols preserve the original independent address loads.
/// \note Original identifier: \c D_800EF6A4 (0x800EF6A4).
extern f32 gPlayerPositionY;
/// Separate symbol view of the primary player record's position Z.
/// \note Original identifier: \c D_800EF6A8 (0x800EF6A8).
extern f32 gPlayerPositionZ;

/// Position the terrain diagnostic grid in front of the player.
/// \param object Grid root transform to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002AF88 (0x8002AF88).
void terrainDebugGridUpdatePosition(u8 *object) {
    f32 pad; /* unused, but sizes the frame (angle lands at 0x18) */
    f32 angle = ((PlayerGridView *)&gPlayerState)->yaw + ((PlayerGridView *)&gPlayerState)->viewShake;

    ((f32 *)object)[6] = (mathSinDegrees(angle) * 15.0F) + ((PlayerGridView *)&gPlayerState)->x;
    ((f32 *)object)[7] = gPlayerPositionY + 0.1F;
    ((f32 *)object)[8] = (mathCosDegrees(angle) * 15.0F) + gPlayerPositionZ;
}

/// Initialize the state of a terrain diagnostic grid cell.
/// \param object Grid cell object to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002B020 (0x8002B020).
void terrainDebugGridInitializeCell(u8 *object) {
    ((RuntimeObjectPrefix *)object)->state.pointer = heapAllocLocal(&gGameHeap, 0x18, 0x59);
}

/// Terrain-property view selecting the diagnostic grid cell colors.
/// \note Original identifier: \c D_80099D20 (0x80099D20).
extern s32 gTerrainDebugGridPropertyView;
/// Align a diagnostic cell to the terrain and color it for the selected terrain-property view.
/// \param obj Grid cell object with a valid grid parent.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002B058 (0x8002B058).
void terrainDebugGridUpdateCell(u8 *obj) {
    u8 *parent;
    TerrainDebugCellState *data;
    u32 flags;
    u8 *node;
    parent = ((RuntimeObjectPrefix *)obj)->root.bytes;
    data = (TerrainDebugCellState *)((RuntimeObjectPrefix *)obj)->state.pointer;
    node = NULL;
    ((RuntimeObjectPrefix *)obj)->position[1] =
        terrainGetWorldHeight(((RuntimeObjectPrefix *)obj)->worldPosition[0],
                              ((RuntimeObjectPrefix *)parent)->position[1],
                              ((RuntimeObjectPrefix *)obj)->worldPosition[2], &node) -
        ((RuntimeObjectPrefix *)parent)->position[1];
    flags = terrainGetSurfaceAttributes(((RuntimeObjectPrefix *)obj)->worldPosition[0],
                                        ((RuntimeObjectPrefix *)parent)->position[1],
                                        ((RuntimeObjectPrefix *)obj)->worldPosition[2], &node);
    switch (gTerrainDebugGridPropertyView) {
    case 1:
        data->color[0] = data->color[1] = data->color[2] = 0;
        if (flags & 2) {
            data->color[0] = 255;
        }
        if (flags & 4) {
            data->color[1] = 255;
        }
        if (flags & 1) {
            data->color[2] = 255;
        }
        break;
    case 2:
        data->color[0] = data->color[1] = data->color[2] = 255;
        if (flags & 0x8000) {
            data->color[0] = 0;
        }
        if (flags & 0x100) {
            data->color[1] = 0;
        }
        if (flags & 0x4000) {
            data->color[2] = 0;
        }
        break;
    case 3:
        data->color[0] = data->color[1] = data->color[2] = 255;
        if (flags & 0x20) {
            data->color[0] = 0;
        }
        if (flags & 0x10) {
            data->color[1] = 0;
        }
        if (flags & 0x40) {
            data->color[2] = 0;
        }
        break;
    default:
        TRACE("ERROR: Unknown meshType:%d\n", gTerrainDebugGridPropertyView);
        break;
    }
}

typedef struct Rgb8 {
    u8 r, g, b;
} Rgb8;
extern Vtx D_100CAD8[];
extern Gfx D_100CBB8[];

/// Render an enabled terrain diagnostic grid cell.
/// \param object Grid cell object to render.
/// \returns Null.
/// \note Original decompilation identifier: \c func_8002B1A4 (0x8002B1A4).
Gfx *terrainDebugGridDrawCell(u8 *object) {
    TerrainDebugCellState *d = (TerrainDebugCellState *)((RuntimeObjectPrefix *)object)->state.pointer;
    u8 *o = ((RuntimeObjectPrefix *)object)->root.bytes;
    Rgb8 *c;
    s32 i;

    if (d->enabled == 0) {
        return 0;
    }
    gSPDisplayList(gRenderDisplayListCursor++, D_100CBB8);
    for (i = 0; i < 4; i++) {
        gSPMatrix(gRenderDisplayListCursor++,
                  (s32)((RuntimeObjectPrefix *)o)->matrixBuffers[(((RuntimeObjectPrefix *)o)->flags & 0x8000) ? 1 : 0] +
                      (d->matrixIndices[i] << 6),
                  G_MTX_LOAD);
        gSPVertex(gRenderDisplayListCursor++, &D_100CAD8[i], 1, i);
    }
    c = (Rgb8 *)d->color;
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, c->r, c->g, c->b, 0xFF);
    gSP2Triangles(gRenderDisplayListCursor++, 0, 1, 2, 0, 0, 2, 3, 0);
    return 0;
}

typedef struct {
    s32 words[14];
} GridTemplate;
/// Object template for the terrain diagnostic grid hierarchy root.
/// \note Original identifier: \c D_80099D24 (0x80099D24).
extern GridTemplate gTerrainDebugGridRootTemplate;
/// Object template for individual terrain diagnostic grid cells.
/// \note Original identifier: \c D_80099D5C (0x80099D5C).
extern GridTemplate gTerrainDebugGridCellTemplate;
void terrainDebugGridSetEnabled(u8 *, s32);
/// Create a terrain diagnostic grid in front of the player.
/// \param size Number of diagnostic cells along each grid side.
/// \returns The created grid root object.
/// \note Original decompilation identifier: \c func_8002B380 (0x8002B380).
u8 *terrainDebugGridCreate(s32 size) {
    GridTemplate rootTemplate = gTerrainDebugGridRootTemplate;
    GridTemplate cellTemplate = gTerrainDebugGridCellTemplate;
    u8 *root;
    u8 *tail;
    u8 *cell;
    TerrainDebugCellState *data;
    s32 *dimensions;
    s32 row;
    s32 col;
    s32 index;
    root = objectCreateFromTemplate((ObjectTemplate *)(u8 *)&rootTemplate);
    dimensions = ((RuntimeObjectPrefix *)root)->state.words;
    *dimensions = size;
    tail = root;
    index = 1;
    for (row = 0; row < size + 1; row++) {
        for (col = 0; col < size + 1; col++) {
            cell = objectCreateFromTemplate((ObjectTemplate *)(u8 *)&cellTemplate);
            data = (TerrainDebugCellState *)((RuntimeObjectPrefix *)cell)->state.pointer;
            ((RuntimeObjectPrefix *)cell)->position[0] = col - (size / 2);
            ((RuntimeObjectPrefix *)cell)->position[2] = row - (size / 2);
            data->matrixIndices[0] = index;
            data->matrixIndices[1] = index + size + 1;
            data->matrixIndices[2] = index + size + 2;
            data->matrixIndices[3] = index + 1;
            ((RuntimeObjectPrefix *)tail)->sibling.bytes = cell;
            index++;
            tail = cell;
        }
    }
    ((RuntimeObjectPrefix *)root)->child.bytes = ((RuntimeObjectPrefix *)root)->sibling.bytes;
    ((RuntimeObjectPrefix *)root)->sibling.bytes = NULL;
    objectMakeHierarchyRoot(root);
    objectAllocateHierarchyMatrixBuffers(root);
    objectRegisterActiveHierarchy(root);
    terrainDebugGridUpdatePosition((u8 *)root);
    objectRefreshHierarchyWorldPositions(root);
    terrainDebugGridSetEnabled(root, 1);
    TRACE("meshNum:%d\n", index);
    return root;
}

/// Set whether the terrain diagnostic grid cells are rendered.
/// \param object Initialized grid root object.
/// \param enabled Nonzero to enable rendering; zero to disable it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002B574 (0x8002B574).
void terrainDebugGridSetEnabled(u8 *object, s32 enabled) {
    s32 *dimensions = ((RuntimeObjectPrefix *)object)->state.words;
    s32 size = *dimensions;
    u8 *node = ((RuntimeObjectPrefix *)object)->child.bytes;
    s32 row;
    s32 col;
    TerrainDebugCellState *cell;
    row = 0;
    for (; row < size + 1; row++) {
        for (col = 0; col < size + 1; col++) {
            cell = (TerrainDebugCellState *)((RuntimeObjectPrefix *)node)->state.pointer;
            if (col < size && row < size) {
                cell->enabled = enabled;
            } else {
                cell->enabled = 0;
            }
            node = ((RuntimeObjectPrefix *)node)->sibling.bytes;
        }
    }
}
