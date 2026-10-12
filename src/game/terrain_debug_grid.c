/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "object_lookup.h"
#include "heap_alloc.h"
#include "object_types.h"
void presentationDrawAreaLabel(Gfx **);

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

typedef struct {
    f32 f0;
    f32 f4;
    s32 f8;
    s32 fC;
    f32 f10;
    u8 pad14[0x444];
    s32 f458;
    volatile s32 f45C;
    u8 pad460[0x48];
} Unk4A8;
/// Shared queued-message, timing, scrolling and text-image state for the message bar.
/// \note Original identifier: \c D_800F06C0 (0x800F06C0).
extern Unk4A8 gMessageBarState;
void cameraUpdateOverlayColors(s32, s32);
void cameraDrawSecondaryOverlay(s32);
void objectDrawHierarchyGroup(s32, s32);
void presentationDrawCardDecorations(Gfx **);
void presentationDrawCurrentCardPanels(Gfx **);
void backdropDraw();
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
void actionScriptStart(s32, s32);          /* extern */
void textWindowAppendText(s32);                       /* extern */
s32 textWindowOpen(s32, s32, s32); /* extern */
s32 textWindowShowFirstPage();                          /* extern */
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern s32 gPikachuActor;
/// Friendship messages indexed by the cleared chapter.
/// \note Original identifier: \c D_800999F0 (0x800999F0).
extern s32 gChapterFriendshipMessages[];
/// Separate symbol view of the primary player friendship-title level.
/// \note Original identifier: \c D_800EF796 (0x800EF796).
extern u8 gPlayerFriendshipLevel;
/// Separate symbol view of the last awarded primary player friendship milestone, or minus one when none awaits presentation.
/// \note Original identifier: \c D_800EF798 (0x800EF798).
extern s32 gPlayerFriendshipMilestone;

/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

/// Separate view of the message-bar image slot prepared for the next transition.
/// \note Original identifier: \c D_800F0B1C (0x800F0B1C).
extern s32 gMessageBarWorkImageIndex;

/// Separate view of the three retained message-bar text-image handles.
/// \note Original identifier: \c D_800F0B20 (0x800F0B20).
extern s32 gMessageBarImageHandles[];

/// Interior availability progress-flag view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125AB2 (0x80125AB2).
extern u8 gItemKindAvailabilityFlagEntries[];

void textImageCreateFromText(s32 *, s8 *, s32); /* extern */
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
/// Owned text image for the current collection score, or zero before creation.
/// \note Original identifier: \c D_800F0BE4 (0x800F0BE4).
extern s32 gCollectionPresentationScoreImage;

/// Live item displayed during collection results, or zero when none is shown.
/// \note Original identifier: \c D_800F0BDC (0x800F0BDC).
extern s32 gCollectionPresentationItem;

/// Interior collection-value view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125AAC (0x80125AAC).
extern s32 gItemKindCollectionValueEntries[];

s32 progressGetValue(u32); /* extern */




/// Initialize the root state of a terrain diagnostic grid.
/// \param object Grid root object to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002AF50 (0x8002AF50).
void terrainDebugGridInitializeRoot(u8 *object) {
    TRACE("meshRootInit:%p\n", object);
    ((RuntimeObjectPrefix *)object)->state.address = heapAllocLocal(&gGameHeap, 4, 0x38);
}

typedef struct {
    f32 x;
    f32 pad4[2];
    f32 unkC;
    f32 unk10;
    f32 unk14;
    f32 unk18;
} Cam6A0;
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern Cam6A0 gPlayerState;
/* y and z are referenced as their own symbols: the ROM gives each its own
 * lui, which a struct member access of D_800EF6A0 would CSE away. */
/// Separate symbol view of the primary player record's position Y.
/// \note Original identifier: \c D_800EF6A4 (0x800EF6A4).
extern f32 gPlayerPositionY;
/// Separate symbol view of the primary player record's position Z.
/// \note Original identifier: \c D_800EF6A8 (0x800EF6A8).
extern f32 gPlayerPositionZ;
f32 mathSinDegrees(f32);
f32 mathCosDegrees(f32);

/// Position the terrain diagnostic grid in front of the player.
/// \param object Grid root transform to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002AF88 (0x8002AF88).
void terrainDebugGridUpdatePosition(f32 *object) {
    f32 pad; /* unused, but sizes the frame (angle lands at 0x18) */
    f32 angle = gPlayerState.unkC + gPlayerState.unk18;

    object[6] = mathSinDegrees(angle) * 15.0f + gPlayerState.x;
    object[7] = gPlayerPositionY + 0.1f;
    object[8] = mathCosDegrees(angle) * 15.0f + gPlayerPositionZ;
}


/// Initialize the state of a terrain diagnostic grid cell.
/// \param object Grid cell object to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002B020 (0x8002B020).
void terrainDebugGridInitializeCell(u8 *object) {
    ((RuntimeObjectPrefix *)object)->state.address = heapAllocLocal(&gGameHeap, 0x18, 0x59);
}

f32 terrainGetWorldHeight(f32, f32, f32, u8 **);
u32 terrainGetSurfaceAttributes(f32, f32, f32, u8 **);
/// Terrain-property view selecting the diagnostic grid cell colors.
/// \note Original identifier: \c D_80099D20 (0x80099D20).
extern s32 gTerrainDebugGridPropertyView;
/// Align a diagnostic cell to the terrain and color it for the selected terrain-property view.
/// \param obj Grid cell object with a valid grid parent.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002B058 (0x8002B058).
void terrainDebugGridUpdateCell(u8 *obj) {
    u8 *parent;
    u8 *data;
    u32 flags;
    u8 *node;
    parent = *(u8 **)(obj + 0x50);
    data = *(u8 **)(obj + 0x54);
    node = NULL;
    *(f32 *)(obj + 0x1C) = terrainGetWorldHeight(*(f32 *)(obj + 0x44), *(f32 *)(parent + 0x1C), *(f32 *)(obj + 0x4C), &node) -
                           *(f32 *)(parent + 0x1C);
    flags = terrainGetSurfaceAttributes(*(f32 *)(obj + 0x44), *(f32 *)(parent + 0x1C), *(f32 *)(obj + 0x4C), &node);
    switch (gTerrainDebugGridPropertyView) {
    case 1:
        data[0x14] = data[0x15] = data[0x16] = 0;
        if (flags & 2)
            data[0x14] = 255;
        if (flags & 4)
            data[0x15] = 255;
        if (flags & 1)
            data[0x16] = 255;
        break;
    case 2:
        data[0x14] = data[0x15] = data[0x16] = 255;
        if (flags & 0x8000)
            data[0x14] = 0;
        if (flags & 0x100)
            data[0x15] = 0;
        if (flags & 0x4000)
            data[0x16] = 0;
        break;
    case 3:
        data[0x14] = data[0x15] = data[0x16] = 255;
        if (flags & 0x20)
            data[0x14] = 0;
        if (flags & 0x10)
            data[0x15] = 0;
        if (flags & 0x40)
            data[0x16] = 0;
        break;
    default:
        TRACE("ERROR: Unknown meshType:%d\n", gTerrainDebugGridPropertyView);
        break;
    }
}

typedef struct Rgb8 {
    u8 r, g, b;
} Rgb8;
/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;
extern Vtx D_100CAD8[];
extern Gfx D_100CBB8[];

/// Render an enabled terrain diagnostic grid cell.
/// \param object Grid cell object to render.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_8002B1A4 (0x8002B1A4).
s32 terrainDebugGridDrawCell(u8 *object) {
    u8 *d = *(u8 **)(object + 0x54);
    u8 *o = *(u8 **)(object + 0x50);
    Rgb8 *c;
    s32 i;

    if (*(s32 *)(d + 0) == 0) {
        return 0;
    }
    gSPDisplayList(gRenderDisplayListCursor++, D_100CBB8);
    for (i = 0; i < 4; i++) {
        gSPMatrix(gRenderDisplayListCursor++,
                  ((s32 *)(o + 0x3C))[(*(u16 *)(o + 0x36) & 0x8000) ? 1 : 0] + (*(u32 *)(d + 4 + i * 4) << 6),
                  G_MTX_LOAD);
        gSPVertex(gRenderDisplayListCursor++, &D_100CAD8[i], 1, i);
    }
    c = (Rgb8 *)(d + 0x14);
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
u8 *objectCreateFromTemplate(u8 *);
u8 *objectMakeHierarchyRoot(u8 *);
void objectAllocateHierarchyMatrixBuffers(s32);
u8 *objectRegisterActiveHierarchy(u8 *);
s32 objectRefreshHierarchyWorldPositions(s32);
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
    s32 *data;
    s32 *dimensions;
    s32 row;
    s32 col;
    s32 index;
    root = objectCreateFromTemplate((u8 *)&rootTemplate);
    dimensions = *(s32 **)(root + 0x54);
    *dimensions = size;
    tail = root;
    index = 1;
    for (row = 0; row < size + 1; row++) {
        for (col = 0; col < size + 1; col++) {
            cell = objectCreateFromTemplate((u8 *)&cellTemplate);
            data = *(s32 **)(cell + 0x54);
            *(f32 *)(cell + 0x18) = col - size / 2;
            *(f32 *)(cell + 0x20) = row - size / 2;
            data[1] = index;
            data[2] = index + size + 1;
            data[3] = index + size + 2;
            data[4] = index + 1;
            *(u8 **)(tail + 0x28) = cell;
            index++;
            tail = cell;
        }
    }
    *(u8 **)(root + 0x2C) = *(u8 **)(root + 0x28);
    *(u8 **)(root + 0x28) = NULL;
    objectMakeHierarchyRoot(root);
    objectAllocateHierarchyMatrixBuffers((s32)root);
    objectRegisterActiveHierarchy(root);
    terrainDebugGridUpdatePosition((f32 *)root);
    objectRefreshHierarchyWorldPositions((s32)root);
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
    s32 *dimensions = *(s32 **)(object + 0x54);
    s32 size = *dimensions;
    u8 *node = *(u8 **)(object + 0x2C);
    s32 row;
    s32 col;
    s32 *cell;
    row = 0;
    for (; row < size + 1; row++) {
        for (col = 0; col < size + 1; col++) {
            cell = *(s32 **)(node + 0x54);
            if (col < size && row < size)
                *cell = enabled;
            else
                *cell = 0;
            node = *(u8 **)(node + 0x28);
        }
    }
}
