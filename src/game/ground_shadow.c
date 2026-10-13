#include "render_state.h"
#include "game_heap.h"
#include "ground_shadow.h"
#include "actor_parts.h"
#include "terrain_height.h"
#include "object_create.h"
#include "heap_free.h"
#include "common.h"
#include "heap_alloc.h"
#include "object_types.h"

/// Owned terrain-conforming geometry and two frame-specific vertex views.
typedef struct GroundShadowGrid {
    s32 count;          ///< Vertices along each side of the square grid.
    f32 spacing;        ///< Distance between vertices before object scaling.
    Gfx *displayList;   ///< Owned rendering command allocation.
    Vtx *vertices[2];   ///< Two frame views into one owned vertex allocation.
} GroundShadowGrid;

typedef char GroundShadowGridSizeCheck[(sizeof(GroundShadowGrid) == 0x14) ? 1 : -1];
typedef char GroundShadowGridCommandsOffsetCheck[(((u32) & ((GroundShadowGrid *)0)->displayList) == 8) ? 1 : -1];
typedef char GroundShadowGridVerticesOffsetCheck[(((u32) & ((GroundShadowGrid *)0)->vertices) == 0xC) ? 1 : -1];

/// Conform the shadow surface to the terrain beneath its owner.
/// \param obj Ground-shadow object with a valid owner.
/// \param buffer Vertex-buffer index, zero or one.
/// \returns Nothing.
///
/// \note UB: the original texture-coordinate divisions require count != 1.
/// \note Original decompilation identifier: \c func_80036330 (0x80036330).
void groundShadowUpdateVertices(u8 *obj, s32 buffer) {
    GroundShadowGrid *grid;
    Vtx *vertex;
    s32 row;
    u8 *hit;
    s32 column;
    f32 x;
    f32 z;
    f32 height;

    grid = *(GroundShadowGrid **)(obj + 0x54);
    hit = 0;
    vertex = grid->vertices[buffer];
    for (row = 0; row < grid->count; row++) {
        column = 0;
        if (grid->count > 0) {
            do {
                x = (row - (grid->count - 1) * 0.5f) * grid->spacing;
                z = (column - (grid->count - 1) * 0.5f) * grid->spacing;
                height = terrainGetWorldHeight(*(f32 *)(obj + 0x18) + *(f32 *)(obj + 0) * x, *(f32 *)(obj + 0x1C),
                                       *(f32 *)(obj + 0x20) + *(f32 *)(obj + 8) * z, &hit);
                height = MIN(height, *(f32 *)(*(u8 **)(obj + 0x50) + 0x1C) + 0.3f);
                height -= *(f32 *)(obj + 0x1C);
                vertex->v.ob[0] = (s32)x;
                vertex->v.ob[1] = (s32)((height + 0.17f) / *(f32 *)(obj + 4));
                vertex->v.ob[2] = (s32)z;
                vertex->v.flag = 0;
                vertex->v.tc[0] = ((row << 6) / (grid->count - 1)) << 5;
                vertex->v.tc[1] = ((column * 64) / (grid->count - 1)) << 5;
                vertex->v.cn[3] = 0x50;
                vertex->v.cn[2] = 0;
                vertex->v.cn[1] = 0;
                vertex->v.cn[0] = 0;
                vertex++;
                column++;
            } while (column < grid->count);
        }
    }
}

/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
extern s32 gRuntimeGameMode;
void groundShadowUpdateVertices(u8 *, s32);

/// Prepare the ground shadow for rendering when the current scene mode permits it.
/// \param object Ground-shadow object to render.
/// \returns The shadow display-list address, or zero when shadows are suppressed.
/// \note Original decompilation identifier: \c func_80036594 (0x80036594).
s32 groundShadowPrepareDraw(u8 *object) {
    GroundShadowGrid *grid = *(GroundShadowGrid **)(object + 0x54);

    if (gRuntimeGameMode == 4 || gRuntimeGameMode == 5 || gRuntimeGameMode == 8 || gRuntimeGameMode == 9) {
        return 0;
    }
    groundShadowUpdateVertices(object, gGraphicsFramebufferIndex);
    gSPSegment(gRenderDisplayListCursor++, 3, osVirtualToPhysical(grid->vertices[gGraphicsFramebufferIndex]));
    return (s32)grid->displayList;
}


/// Keep the ground shadow positioned and scaled beneath its owner.
/// \param object Ground-shadow object with a valid owner.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80036640 (0x80036640).
void groundShadowUpdateTransform(u8 *object) {
    f32 *o = (f32 *)object;
    u8 *t;
    f32 *s;

    t = actorGetDefaultPart(*(u8 **)(object + 0x50));
    s = *(f32 **)(object + 0x50);
    o[6] = *(f32 *)(t + 0x44);
    o[7] = *(f32 *)(t + 0x48);
    o[8] = *(f32 *)(t + 0x4C);
    o[0] = s[0];
    o[1] = s[1];
    o[2] = s[2];
    o[7] = terrainGetWorldHeight(o[6], o[7], *(f32 *)(object + 0x20), NULL);
}

/// Release the rendering resources owned by a ground shadow.
/// \param object Ground-shadow object whose geometry is released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800366BC (0x800366BC).
void groundShadowReleaseGeometry(u8 *object) {
    GroundShadowGrid *shadowState = *(GroundShadowGrid **)(object + 0x54);
    heapFreeLocal(&gGameHeap, shadowState->displayList);
    heapFreeLocal(&gGameHeap, shadowState->vertices[0]);
}

void _bzero(void *, s32);

/// Initialize the state of a ground-shadow object.
/// \param object Ground-shadow object to initialize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80036700 (0x80036700).
void groundShadowInitialize(u8 *object) {
    u8 *sp1C;
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x14, 0xAC);
    sp1C = allocation;
    _bzero(allocation, 0x14);
    ((RuntimeObjectPrefix *)object)->state.bytes = sp1C;
}

/// 64-by-64 four-bit intensity texture used by ground-shadow grids.
/// \note Original identifier: \c D_1007B90 (0x01007B90).
extern u8 gGroundShadowIntensityTexture[];

/// Create the rendering commands for a ground-shadow grid.
/// \param grid Shadow grid with its dimensions and vertex buffers configured.
/// \returns Nothing.
///
/// \note NONPORTABLE: paired texture setup commands share one line for IDO scheduling.
/// \note Original decompilation identifier: \c func_8003674C (0x8003674C).
void groundShadowBuildDisplayList(GroundShadowGrid *grid) {
    Gfx *g;
    s32 row;
    s32 column;
    s32 index;
    s32 size = grid->count * grid->count * sizeof(Gfx) + 0x80;

    g = (Gfx *)heapAllocLocal(&gGameHeap, size, 0xBB);
    grid->displayList = g;
    gDPPipeSync(g++);
    gSPGeometryMode(g++, 0xFFFFFF, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gDPSetCombineLERP(g++, 0, 0, 0, SHADE, TEXEL0, 0, SHADE, 0, 0, 0, 0, COMBINED, 0, 0, 0, COMBINED);
    gDPSetRenderMode(g++, 0, G_RM_AA_ZB_XLU_SURF2);

    // clang-format off
    gSPTexture(g++, 0x7FFF, 0, 0, G_TX_RENDERTILE, G_ON); gMoveWd(g++, G_MW_FOG, 0x10, 0x7FFF);
    // clang-format on
    gDPSetTextureLUT(g++, G_TT_NONE);
    gDPLoadTextureBlock_4b(g++, gGroundShadowIntensityTexture, G_IM_FMT_I, 64, 64, 0, G_TX_CLAMP, G_TX_CLAMP, 6, 6,
                           G_TX_NOLOD, G_TX_NOLOD);
    gSPVertex(g++, (void *)0x03000000, grid->count * grid->count, 0);
    for (row = 0; row < grid->count - 1; row++) {
        index = row * grid->count;
        for (column = 0; column < grid->count - 1; column++) {
            gSP2Triangles(g++, index, index + 1, index + grid->count + 1, 0, index, index + grid->count + 1,
                          index + grid->count, 0);
            index++;
        }
    }
    gSPTexture(g++, 0x7FFF, 0, 0, G_TX_RENDERTILE, G_OFF);
    gMoveWd(g++, G_MW_FOG, 0x10, 0x7FFF);
    gSPEndDisplayList(g++);
}

/// Object template used to create terrain-conforming ground shadows.
/// \note Original identifier: \c D_8009B540 (0x8009B540).
extern u8 gGroundShadowObjectTemplate[];
void groundShadowBuildDisplayList(GroundShadowGrid *);

/// Create a ground shadow with the requested grid resolution and spacing.
/// \param count Number of vertices along each grid side; must exceed one and fit the renderer vertex limit.
/// \param spacing Distance between adjacent vertices before object scaling.
/// \returns The created ground-shadow object.
/// \note Original decompilation identifier: \c func_800369D4 (0x800369D4).
u8 *groundShadowCreate(s32 count, f32 spacing) {
    u8 *obj = objectCreateFromTemplate((ObjectTemplate *)gGroundShadowObjectTemplate);
    GroundShadowGrid *grid = *(GroundShadowGrid **)(obj + 0x54);
    s32 size;

    grid->count = count;
    grid->spacing = spacing;
    size = count * count * 0x10;
    grid->vertices[0] = (Vtx *)heapAllocLocal(&gGameHeap, size * 2, 0x11B);
    grid->vertices[1] = (Vtx *)((s32)grid->vertices[0] + size);
    groundShadowBuildDisplayList(grid);
    return obj;
}
