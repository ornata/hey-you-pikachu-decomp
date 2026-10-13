#include "vector3_types.h"
#include "runtime_lifecycle.h"
#include "area_names.h"
#include "game_heap.h"
#include "pikachu_actor.h"
#include "camera_storage.h"
#include "object_groups.h"
#include "vector_metrics.h"
#include "math_angles.h"
#include "random.h"
#include "area_transition.h"
#include "runtime_control.h"
#include "terrain_movement.h"
#include "terrain_height.h"
#include "terrain_surface.h"
#include "wisp_effect.h"
#include "falling_particle.h"
#include "item_mode.h"
#include "item_kind.h"
#include "maze_scatter.h"
#include "weighted_choice.h"
#include "progress_value.h"
#include "matrix_point.h"
#include "camera_visibility.h"
#include "object_flags.h"
#include "object_hierarchy_create.h"
#include "object_lifecycle.h"
#include "heap_free.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "item_object.h"
#include "runtime_clock.h"
#include "heap_diagnostics.h"
#include "item_spawn.h"
#include "heap_alloc.h"
#include "scene_types.h"
#include "area_maze.h"
#include "object_types.h"

// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/* Arguments to overlayLoadFromRom for one area's scene overlay. */
typedef struct {
    /* 0x00 */ u32 romStart;
    /* 0x04 */ u32 romEnd;
    /* 0x08 */ u8 *vramDst;
    /* 0x0C */ u8 *textStart;
    /* 0x10 */ u8 *textEnd;
    /* 0x14 */ u8 *dataStart;
    /* 0x18 */ u8 *dataEnd;
    /* 0x1C */ u8 *bssStart;
    /* 0x20 */ u8 *bssEnd;
} OverlayRec;

#define SCENE_SYMBOLS(name)                                                                                            \
    extern u8 scene_##name##_ROM_START[], scene_##name##_ROM_END[], scene_##name##_VRAM[];                             \
    extern u8 scene_##name##_TEXT_START[], scene_##name##_TEXT_END[], scene_##name##_DATA_START[];                     \
    extern u8 scene_##name##_RODATA_END[], scene_##name##_BSS_START[], scene_##name##_BSS_END[]
#define SCENE_OVERLAY(name)                                                                                            \
    {(u32)scene_##name##_ROM_START, (u32)scene_##name##_ROM_END, scene_##name##_VRAM,                                  \
     scene_##name##_TEXT_START,     scene_##name##_TEXT_END,     scene_##name##_DATA_START,                            \
     scene_##name##_RODATA_END,     scene_##name##_BSS_START,    scene_##name##_BSS_END}

SCENE_SYMBOLS(bedroom);
SCENE_SYMBOLS(entryway);
SCENE_SYMBOLS(front_yard);
SCENE_SYMBOLS(viridian_entrance);
SCENE_SYMBOLS(viridian_forest);
SCENE_SYMBOLS(viridian_deep);
SCENE_SYMBOLS(springleaf_entrance);
SCENE_SYMBOLS(oddish_meadow);
SCENE_SYMBOLS(springleaf_field);
SCENE_SYMBOLS(olivine_lakeshore);
SCENE_SYMBOLS(olivine_lake);
SCENE_SYMBOLS(ochre_entrance);
SCENE_SYMBOLS(ochre_deep);
SCENE_SYMBOLS(cobalt_island);
SCENE_SYMBOLS(cobalt_coast);
SCENE_SYMBOLS(abra_shop);
SCENE_SYMBOLS(wilderness_camp);
SCENE_SYMBOLS(ochre_fishing);
SCENE_SYMBOLS(olivine_fishing);
SCENE_SYMBOLS(cobalt_fishing);
SCENE_SYMBOLS(viridian_edge);
SCENE_SYMBOLS(playground);
SCENE_SYMBOLS(credits);

/// Octree child X placement in half-parent extents.
/// \note Original identifier: \c D_80123430 (0x80123430).
s32 gMazeChildOffsetX[8] = {0, 1, 0, 1, 0, 1, 0, 1};
/// Octree child Y placement in half-parent extents.
/// \note Original identifier: \c D_80123450 (0x80123450).
s32 gMazeChildOffsetY[8] = {0, 0, 0, 0, 1, 1, 1, 1};
/// Octree child Z placement in half-parent extents.
/// \note Original identifier: \c D_80123470 (0x80123470).
s32 gMazeChildOffsetZ[8] = {0, 0, 1, 1, 0, 0, 1, 1};

f32 gTerrainFlatHeightSamples[25] = {
    100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f,
    100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f,
};
/// Grid descriptor for the constant-height terrain samples.
/// \note Original identifier: \c D_801234F4 (0x801234F4).
MazeGrid gTerrainFlatHeightGrid = {gTerrainFlatHeightSamples, gTerrainFlatHeightSamples, 5, 5, 1};

f32 gTerrainMoundHeightSamples[36] = {
    4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f, //
    4.0f, 4.5f, 4.5f, 4.5f, 4.5f, 4.0f, //
    4.0f, 4.5f, 5.0f, 5.0f, 4.5f, 4.0f, //
    4.0f, 4.5f, 5.0f, 5.0f, 4.5f, 4.0f, //
    4.0f, 4.5f, 4.5f, 4.5f, 4.5f, 4.0f, //
    4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f, //
};
/// Grid descriptor used to interpolate solid maze block heights.
/// \note Original identifier: \c D_80123594 (0x80123594).
MazeGrid gTerrainMoundHeightGrid = {gTerrainMoundHeightSamples, gTerrainMoundHeightSamples, 6, 6, 1};

OverlayRec gAreaOverlayDescriptors[23] = {
    SCENE_OVERLAY(bedroom),
    SCENE_OVERLAY(entryway),
    SCENE_OVERLAY(front_yard),
    SCENE_OVERLAY(viridian_entrance),
    SCENE_OVERLAY(viridian_forest),
    SCENE_OVERLAY(viridian_deep),
    SCENE_OVERLAY(springleaf_entrance),
    SCENE_OVERLAY(oddish_meadow),
    SCENE_OVERLAY(springleaf_field),
    SCENE_OVERLAY(olivine_lakeshore),
    SCENE_OVERLAY(olivine_lake),
    SCENE_OVERLAY(ochre_entrance),
    SCENE_OVERLAY(ochre_deep),
    SCENE_OVERLAY(cobalt_island),
    SCENE_OVERLAY(cobalt_coast),
    SCENE_OVERLAY(abra_shop),
    SCENE_OVERLAY(wilderness_camp),
    SCENE_OVERLAY(ochre_fishing),
    SCENE_OVERLAY(olivine_fishing),
    SCENE_OVERLAY(cobalt_fishing),
    SCENE_OVERLAY(viridian_edge),
    SCENE_OVERLAY(playground),
    SCENE_OVERLAY(credits),
};

u8 *gAreaConfigurations[23] = {
    scene_bedroom_DATA_START + 0x21314,
    scene_entryway_DATA_START + 0x5A6C,
    scene_front_yard_DATA_START + 0x24A20,
    scene_viridian_entrance_DATA_START + 0x18904,
    scene_viridian_forest_DATA_START + 0x60C4,
    scene_viridian_deep_DATA_START + 0x15680,
    scene_springleaf_entrance_DATA_START + 0x22E9C,
    scene_oddish_meadow_DATA_START + 0x23368,
    scene_springleaf_field_DATA_START + 0x234B8,
    scene_olivine_lakeshore_DATA_START + 0xCC94,
    scene_olivine_lake_DATA_START + 0xCCE8,
    scene_ochre_entrance_DATA_START + 0x9858,
    scene_ochre_deep_DATA_START + 0x157A4,
    scene_cobalt_island_DATA_START + 0x1C698,
    scene_cobalt_coast_DATA_START + 0x12284,
    scene_abra_shop_DATA_START + 0xF114,
    scene_wilderness_camp_DATA_START + 0x64DC,
    scene_ochre_fishing_DATA_START + 0x111E0,
    scene_olivine_fishing_DATA_START + 0x10DB0,
    scene_cobalt_fishing_DATA_START + 0x10F50,
    scene_viridian_edge_DATA_START + 0xF2A8,
    scene_playground_DATA_START + 0xAA3C,
    scene_credits_DATA_START + 0xF4DC,
};

char *gAreaDisplayNames[23] = {
    "The Bedroom",
    "Entryway",
    "The Front Yard",
    "Entrance to Viridian Forest",
    "Viridian Forest",
    "Deep in Viridian Forest",
    "Entrance to Springleaf Field",
    "Oddish's Meadow",
    "Springleaf Field",
    "Olivine Lakeshore",
    "Olivine Lake",
    "Entrance to Ochre Woods",
    "Deep in Ochre Woods",
    "Cobalt Island",
    "Cobalt Coast",
    "Abra's Shop",
    "Wilderness Camp",
    "Ochre Fishing Hole",
    "Olivine Fishing Hole",
    "Cobalt Fishing Hole",
    "Viridian Forest Edge",
    "Pikachu's Playground",
    "Credits",
};
char *gAreaRegionNames[23] = {
    NULL,         NULL,      NULL,      "Viridian", "Viridian", "Viridian",   "Springleaf", "Springleaf",
    "Springleaf", "Olivine", "Olivine", "Ochre",    "Ochre",    "Cobalt",     "Cobalt",     "Abra's Shop",
    "Camp",       "Ochre",   "Olivine", "Cobalt",   "Viridian", "Playground", "The End",
};

/// Node selected by the most recent position event, used to suppress rapid repeats.
/// \note Original identifier: \c D_801239F4 (0x801239F4).
u8 *gAreaLastEventNode = NULL;
/// Event selector retained from the most recent position event.
/// \note Original identifier: \c D_801239F8 (0x801239F8).
s32 gAreaLastEventType = 0;

/// Elapsed-time checkpoint for suppression of repeated position events.
/// \note Original identifier: \c D_80123A00 (0x80123A00).
u64 gAreaLastEventMilliseconds = 0;
/// Octree child indices selected by comparisons with the node midpoint on each axis.
/// \note Original identifier: \c D_80123A08 (0x80123A08).
s32 gMazeOctantIndices[2][2][2] = {{{0, 2}, {4, 6}}, {{1, 3}, {5, 7}}};
/// Friction coefficients indexed by supported terrain material identifiers.
/// \note Original identifier: \c D_80123A28 (0x80123A28).
f32 gTerrainMaterialFriction[8] = {1.0f, 0.8f, 2.0f, 1.2f, 1.8f, 4.0f, 0.9f, 1.0f};
s32 gTerrainMaterialSounds[8][8] = {{-1, -1, -1, -1, -1, -1, -1, -1},       {-1, 141, 196, 178, 180, 199, 200, 151},
                                    {-1, 141, 196, 178, 180, 199, -1, 151}, {142, 141, 196, 178, 180, 199, 142, 141},
                                    {-1, 141, 196, 178, 180, 199, -1, 151}, {-1, -1, 165, 179, 179, 198, -1, -1},
                                    {-1, -1, 165, 179, 179, 198, -1, -1},   {-1, 141, 196, 178, 180, 199, -1, 151}};
/// Surface attribute masks that obstruct each horizontal collision policy.
/// \note Original identifier: \c D_80123B48 (0x80123B48).
s32 gTerrainCollisionPolicySurfaceMasks[10] = {0, 32, 0, 16, 64, 0, 0, 0, 0, 0};

/* .bss 0x80133DE0..0x80133E10, in address order (IDO lays globals out in declaration order) */
/// Owned root of the current area spatial hierarchy, cleared when the maze is destroyed.
/// \note Original identifier: \c D_80133DE0 (0x80133DE0).
MazeNode *gAreaMazeRoot;
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
struct SceneConfigurationPrefix *gCurrentAreaConfiguration;
/// Three retained adjacent words cleared during maze initialization; no reader is identified.
/// \note Their gameplay purpose is unresolved; the stores do not establish a movement vector.
/// \note Original identifier: \c D_80133DE8 (0x80133DE8).
f32 D_80133DE8;
s32 D_80133DE8_0;
s32 D_80133DE8_1;
/// Retained block-count diagnostic word reset during maze initialization.
/// \note Original identifier: \c D_80133DF4 (0x80133DF4).
s32 gMazeDiagnosticBlockCount;
/// Squared far-distance threshold used by area node visibility tests.
/// \note Original identifier: \c D_80133DF8 (0x80133DF8).
f32 gAreaFarVisibilityDistanceSquared;
/// Squared near-distance threshold used by area node visibility tests.
/// \note Original identifier: \c D_80133DFC (0x80133DFC).
f32 gAreaNearVisibilityDistanceSquared;
/// Maze visibility parameters containing the cell-radius scale and retained unknown storage.
/// \note Original identifier: \c D_80133E00 (0x80133E00).
struct {
    f32 radiusScale;
    s32 unknown04[3];
} gMazeVisibilityParameters;

#include "area_services.h"
#include "object_update.h"
#include "spatial_math.h"
/// Check whether a world position lies outside the current area's permitted bounds.
/// \param x World X coordinate.
/// \param y World Y coordinate.
/// \param z World Z coordinate.
/// \returns One if the position is outside the permitted bounds, or zero otherwise.
///
/// \note Requires an initialized current area.
/// \note Original decompilation identifier: \c func_8010AAF0 (0x8010AAF0).
s32 areaIsPositionOutsideBounds(f32 x, f32 y, f32 z) {
    u8 *root = (u8 *)gAreaMazeRoot;

    x -= ((MazeNode *)root)->x;
    y -= ((MazeNode *)root)->y;
    z -= ((MazeNode *)root)->z;
    if ((x < -10.0f) || ((((MazeNode *)root)->w + 10.0f) < x) || (y < -10.0f) ||
        (((2.0f * ((MazeNode *)root)->h) + 10.0f) < y) || (z < -10.0f) || ((((MazeNode *)root)->d + 10.0f) < z)) {
        return 1;
    }
    return 0;
}


/// Create an item at a random horizontal position near a supplied center.
/// \param center Center world position in X, Y, Z order; its Y coordinate is passed to item creation.
/// \param width Nonnegative side length of the square horizontal placement region.
/// \param kind Item kind identifier from zero through 165.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010ABB4 (0x8010ABB4).
void areaSpawnItemNearPosition(f32 *center, f32 width, s32 kind) {
    f32 v[3];

    v[0] = center[0] + randomNext() * 2.3283064365386963e-10 * width - width * 0.5f;
    v[1] = center[1];
    v[2] = center[2] + randomNext() * 2.3283064365386963e-10 * width - width * 0.5f;
    itemCreate(v[0], v[1], v[2], 0.0f, 0.0f, kind);
}

/// Create an item at a random horizontal position within an area region, using the region's top height.
/// \param region Area region providing world origin and dimensions.
/// \param kind Item kind identifier from zero through 165.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010ACD4 (0x8010ACD4).
void areaSpawnItemInRegion(f32 *region, s32 kind) {
    f32 x;
    f32 y;
    f32 z;

    x = region[2] + region[5] * (randomNext() * 2.3283064365386963e-10);
    y = region[3] + region[6];
    z = region[4] + region[7] * (randomNext() * 2.3283064365386963e-10);
    itemCreate(x, y, z, 0.0f, 0.0f, kind);
}

#include "contact_effect.h"

/// Create a contact effect at a random horizontal position within an area region.
/// \param region Area region providing world origin and dimensions; its top height relative to terrain
/// determines the effect height.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010ADD8 (0x8010ADD8).
void areaCreateContactEffectInRegion(f32 *region) {
    f32 x;
    f32 y;
    f32 z;

    x = region[2] + region[5] * (randomNext() * 2.3283064365386963e-10);
    y = region[3] + region[6];
    z = region[4] + region[7] * (randomNext() * 2.3283064365386963e-10);
    y -= terrainGetWorldHeight(x, y, z, 0);
    contactEffectCreate(x, y, z);
}


/// Create an item resting on terrain at a random horizontal position within an area region.
/// \param region Area region providing horizontal placement bounds and the terrain query height.
/// \param kind Item kind identifier from zero through 165.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010AEE0 (0x8010AEE0).
void areaSpawnGroundItemInRegion(f32 *region, s32 kind) {
    f32 x;
    f32 y;
    f32 z;
    f32 ground;
    u8 *fx;

    x = region[2] + region[5] * (randomNext() * 2.3283064365386963e-10);
    y = region[3] + region[6];
    z = region[4] + region[7] * (randomNext() * 2.3283064365386963e-10);
    ground = terrainGetWorldHeight(x, y, z, 0);
    fx = itemCreate(x, ground, z, 0.0f, 0.0f, kind);
    ((RuntimeObjectPrefix *)fx)->position[1] = ((RuntimeObjectPrefix *)fx)->radius + ground;
}

/// Raise an integer base to a nonnegative integer power.
/// \param base Integer base.
/// \param exp Nonnegative exponent.
/// \returns The integer power; callers must keep intermediate products within the signed integer range.
/// \note Original decompilation identifier: \c func_8010B010 (0x8010B010).
s32 integerPower(s32 base, s32 exp) {
    s32 result = 1;

    while (exp != 0) {
        if (exp & 1) {
            result *= base;
        }
        exp >>= 1;
        base *= base;
    }
    return result;
}

u8 *areaFindPositionNode(f32, f32, f32);
/// Dispatch a position-based area event, suppressing repeat events within the same node for one second.
/// \param position Readable world event position in X, Y, Z order; borrowed by the callback during this call.
/// \param eventType Event selector passed to the area's event handler.
/// \returns The event handler's result, zero when a repeat event is suppressed, or minus one when no handler
/// is present.
///
/// \note Requires an initialized current area. The callback receives the position, selected node, and event
/// selector.
/// \note Original decompilation identifier: \c func_8010B054 (0x8010B054).
s32 areaDispatchPositionEvent(f32 *position, s32 eventType) {
    u8 *node;
    MazePositionEvent callback;
    node = areaFindPositionNode(position[0], position[1], position[2]);
    if (node == gAreaLastEventNode && (u64)runtimeGetElapsedMilliseconds() - gAreaLastEventMilliseconds < 1000) {
        return 0;
    }
    gAreaLastEventMilliseconds = runtimeGetElapsedMilliseconds();
    gAreaLastEventType = eventType;
    gAreaLastEventNode = node;
    callback = ((MazeNode *)node)->event;
    if (callback != NULL) {
        return callback(position, node, eventType);
    }
    return -1;
}

/// Octree child indices selected by comparisons with the node midpoint on each axis.
/// \note Original identifier: \c D_80123A08 (0x80123A08).
extern s32 gMazeOctantIndices[2][2][2];

/// Find the most specific current-area region selected by a world position.
/// \param x World X coordinate.
/// \param y World Y coordinate.
/// \param z World Z coordinate.
/// \returns Borrowed selected area node, valid until its hierarchy is replaced or destroyed.
///
/// \note Requires an initialized, acyclic current-area hierarchy.
/// \note Original decompilation identifier: \c func_8010B124 (0x8010B124).
/// \note Checked member-offset reads preserve the matching target compiler expression ordering.
u8 *areaFindPositionNode(f32 x, f32 y, f32 z) {
    f32 mx;
    f32 my;
    f32 mz;
    s32 oct;
    u8 *child;
    u8 *node;

    node = (u8 *)gAreaMazeRoot;
    for (;;) {
        mx = (*(f32 *)(node + (u32) & ((MazeNode *)0)->w)) * 0.5f + (*(f32 *)(node + (u32) & ((MazeNode *)0)->x));
        my = (*(f32 *)(node + (u32) & ((MazeNode *)0)->h)) * 0.5f + (*(f32 *)(node + (u32) & ((MazeNode *)0)->y));
        mz = (*(f32 *)(node + (u32) & ((MazeNode *)0)->d)) * 0.5f + (*(f32 *)(node + (u32) & ((MazeNode *)0)->z));
        oct = gMazeOctantIndices[mx < x][my < y][mz < z];
        child = ((u8 **)(node + 0x20))[oct];
        if (child == NULL) {
            return node;
        }
        node = child;
    }
}

f32 terrainGetNodeLocalHeight(f32, f32, u8 *);
/// Find terrain at a world position and obtain its height relative to the selected area node.
/// \param x World X coordinate.
/// \param y World Y coordinate used to select the terrain region.
/// \param z World Z coordinate.
/// \param cachedNode Optional writable terrain-node cache; initialize to null or a live node in the current
/// area. Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns Terrain height relative to the selected node, or zero if no surface is found.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
/// \note Original decompilation identifier: \c func_8010B20C (0x8010B20C).
f32 terrainFindSurfaceLocalHeight(f32 x, f32 y, f32 z, u8 **cachedNode) {
    f32 deltaX;
    f32 height;
    f32 stepY;
    f32 deltaZ;
    f32 deltaY;
    f32 originalY;
    s32 searchDown;
    u8 *node;

    if ((cachedNode != NULL) && (*cachedNode != NULL)) {
        node = *cachedNode;
        deltaX = x - ((MazeNode *)node)->x;
        deltaY = y - ((MazeNode *)node)->y;
        deltaZ = z - ((MazeNode *)node)->z;
        if ((((MazeNode *)node)->terrain != 0) && (deltaX >= 0.0f) && (deltaX <= ((MazeNode *)node)->w) &&
            (deltaY >= 0.0f) && (deltaZ >= 0.0f) && (deltaZ <= ((MazeNode *)node)->d)) {
            height = terrainGetNodeLocalHeight(x, z, node);
        } else {
            node = NULL;
        }
    } else {
        node = NULL;
    }
    originalY = y;
    searchDown = 1;
    if (node == NULL) {
        while (1) {
            node = areaFindPositionNode(x, y, z);
            if ((node != NULL) && (((MazeNode *)node)->terrain != 0)) {
                height = terrainGetNodeLocalHeight(x, z, node);
                if (height > 0.001f) {
                    break;
                }
            }
            if (searchDown != 0) {
                stepY = gCurrentAreaConfiguration->cellDimensions[1];
                y -= stepY;
                if (y < 0.0f) {
                    searchDown = 0;
                    y = stepY + originalY;
                }
            } else {
                y += gCurrentAreaConfiguration->cellDimensions[1];
                if ((((MazeNode *)(u8 *)gAreaMazeRoot)->y + ((MazeNode *)(u8 *)gAreaMazeRoot)->h) < y) {
                    node = NULL;
                    height = 0.0f;
                    break;
                }
            }
        }
    }
    if (cachedNode != NULL) {
        *cachedNode = node;
    }
    return height;
}

#define GRID(m) (((MazeNode *)(m))->terrain)

/// Read a vertex height-map sample, using the nearest boundary sample for out-of-range coordinates.
/// \param x Horizontal sample column.
/// \param y Horizontal sample row along Z.
/// \param node Area node with a nonempty vertex height map.
/// \returns The sampled terrain height relative to the node.
/// \note Original decompilation identifier: \c func_8010B428 (0x8010B428).
/// \note Checked member-offset reads preserve the matching target compiler expression ordering.
f32 terrainSampleVertexHeight(s32 x, s32 y, u8 *node) {
    if (x < 0) {
        x = 0;
    } else if (x > (*(MazeGrid **)(node + (u32) & ((MazeNode *)0)->terrain))->w - 1) {
        x = (*(MazeGrid **)(node + (u32) & ((MazeNode *)0)->terrain))->w - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y > (*(MazeGrid **)(node + (u32) & ((MazeNode *)0)->terrain))->h - 1) {
        y = (*(MazeGrid **)(node + (u32) & ((MazeNode *)0)->terrain))->h - 1;
    }
    return *(y * (*(MazeGrid **)(node + (u32) & ((MazeNode *)0)->terrain))->w +
             ((*(MazeGrid **)(node + (u32) & ((MazeNode *)0)->terrain))->cells + x));
}

/// Read a cell height-map sample, using the nearest boundary sample for out-of-range coordinates.
/// \param x Horizontal sample column.
/// \param y Horizontal sample row along Z.
/// \param node Area node with a nonempty cell height map.
/// \returns The sampled terrain height relative to the node.
/// \note Original decompilation identifier: \c func_8010B4A8 (0x8010B4A8).
/// \note Checked member-offset reads preserve the matching target compiler expression ordering.
f32 terrainSampleCellHeight(s32 x, s32 y, u8 *node) {
    if (x < 0) {
        x = 0;
    } else if (x > (*(MazeGrid **)(node + (u32) & ((MazeNode *)0)->terrain))->w - 1) {
        x = (*(MazeGrid **)(node + (u32) & ((MazeNode *)0)->terrain))->w - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y > (*(MazeGrid **)(node + (u32) & ((MazeNode *)0)->terrain))->h - 1) {
        y = (*(MazeGrid **)(node + (u32) & ((MazeNode *)0)->terrain))->h - 1;
    }
    return *(y * (*(MazeGrid **)(node + (u32) & ((MazeNode *)0)->terrain))->w +
             ((*(MazeGrid **)(node + (u32) & ((MazeNode *)0)->terrain))->cells + x));
}

/// Evaluate the shared maze terrain height at a position within an area node.
/// \param x World X coordinate within the node.
/// \param z World Z coordinate within the node.
/// \param node Maze node with positive horizontal dimensions; the queried position must lie within its
/// horizontal bounds.
/// \returns Maze terrain height relative to the node.
/// \note Original decompilation identifier: \c func_8010B528 (0x8010B528).
f32 terrainGetMazeHeight(f32 x, f32 z, u8 *node) {
    s32 w, h;
    f32 fx, fz;
    f32 v00, v10, v01, v11;
    s32 ix, iz, nx, nz;
    f32 tx, tz;
    w = gTerrainMoundHeightGrid.w;
    fx = ((x - *(f32 *)(node + 8)) * (w - 1)) / *(f32 *)(node + 0x14);
    h = gTerrainMoundHeightGrid.h - 1;
    ix = fx;
    fz = ((z - *(f32 *)(node + 0x10)) * h) / *(f32 *)(node + 0x1C);
    iz = fz;
    nx = ix + 1;
    nz = iz + 1;
    if (nx < 0)
        nx = 0;
    else if ((w - 1) < nx)
        nx = (w - 1);
    if (nz < 0)
        nz = 0;
    else if (h < nz)
        nz = h;
    TRACE("ixz:(%d,%d) ixz_1:(%d,%d)\n", ix, iz, nx, nz);
    h = iz * w;
    v00 = gTerrainMoundHeightGrid.cells[ix + h];
    v10 = gTerrainMoundHeightGrid.cells[nx + h];
    h = nz * w;
    v01 = gTerrainMoundHeightGrid.cells[ix + h];
    v11 = gTerrainMoundHeightGrid.cells[nx + h];
    tx = fx - ix;
    tz = fz - iz;
    fz = ((1.0f - tx) * v00) + tx * v10;
    return ((1.0f - tz) * fz) + (tz * (((1.0f - tx) * v01) + tx * v11));
}

/* Mode-0 terrain triangle: XYZ vertices A, B and C, then the ground material. */
typedef struct {
    /* 0x00 */ f32 a[3];
    /* 0x0C */ f32 b[3];
    /* 0x18 */ f32 c[3];
    /* 0x24 */ u16 material;
    /* 0x26 */ u16 flags;
} TerrainTriangle; /* size 0x28 */

f32 fabsf(f32);
#pragma intrinsic(fabsf)

#define TRIANGLES(node) ((TerrainTriangle *)GRID(node)->cells)

/// Evaluate a node's triangle terrain at a world horizontal position.
/// \param x World X coordinate.
/// \param z World Z coordinate.
/// \param node Area node containing triangle terrain.
/// \returns Height relative to the node of the first matching terrain triangle, or zero when none matches.
/// \note Original decompilation identifier: \c func_8010B6D0 (0x8010B6D0).
f32 terrainGetTriangleHeight(f32 x, f32 z, u8 *node) {
    f32 ax, az, bx, bz, cx, cz, determinant, dx, dz, u, v;
    f32 lengthB, lengthC;
    s32 i;
    f32 baseY, by, cy;

    x -= *(f32 *)(node + 8);
    z -= *(f32 *)(node + 0x10);
    for (i = 0; i < GRID(node)->w; i++) {
        ax = TRIANGLES(node)[i].a[0];
        az = TRIANGLES(node)[i].a[2];
        bx = TRIANGLES(node)[i].b[0] - ax;
        bz = TRIANGLES(node)[i].b[2] - az;
        cx = TRIANGLES(node)[i].c[0] - ax;
        cz = TRIANGLES(node)[i].c[2] - az;
        determinant = bz * cx - cz * bx;
        if (fabsf(determinant) > 0.001f) {
            dx = x - ax;
            dz = z - az;
            u = (cx * dz - cz * dx) / determinant;
            if (u < 0.0)
                continue;
            v = (bx * dz - bz * dx) / -determinant;
            if (v < 0.0)
                continue;
            if (u + v > 1.0f)
                continue;
        } else {
            /* Degenerate projection: independent projections onto each edge. */
            lengthB = bx * bx + bz * bz;
            if (lengthB < 0.001f)
                continue;
            dx = x - ax;
            dz = z - az;
            u = (bx * dx + bz * dz) / lengthB;
            if (u < 0.0)
                continue;
            lengthC = cx * cx + cz * cz;
            if (lengthC < 0.001f)
                continue;
            v = (cx * dx + cz * dz) / lengthC;
            if (v < 0.0)
                continue;
            if (u + v > 1.0f)
                continue;
        }
        baseY = TRIANGLES(node)[i].a[1];
        by = TRIANGLES(node)[i].b[1] - baseY;
        cy = TRIANGLES(node)[i].c[1] - baseY;
        return by * u + cy * v + baseY;
    }
    return 0.0f;
}

/// Evaluate a node's height-map terrain at a world horizontal position.
/// \param x Finite world X coordinate whose scaled sample coordinate and successor fit in a signed integer.
/// \param z Finite world Z coordinate whose scaled sample coordinate and successor fit in a signed integer.
/// \param node Area node with a nonempty height map and nonzero horizontal dimensions.
/// \returns Terrain height relative to the node.
/// \note Original decompilation identifier: \c func_8010B9A4 (0x8010B9A4).
f32 terrainGetHeightMapHeight(f32 x, f32 z, u8 *node) {
    f32 fx;
    f32 fz;
    f32 tx;
    f32 tz;
    f32 v00;
    f32 v10;
    f32 v01;
    f32 v11;
    s32 ix;
    s32 iz;

    if (GRID(node)->mode == 2) {
        fx = ((x - ((MazeNode *)node)->x) * (GRID(node)->w - 1)) / ((MazeNode *)node)->w;
        ix = fx;
        fz = ((z - ((MazeNode *)node)->z) * (GRID(node)->h - 1)) / ((MazeNode *)node)->d;
        iz = fz;
        v00 = terrainSampleVertexHeight(ix, iz, node);
        v10 = terrainSampleVertexHeight(ix + 1, iz, node);
        v01 = terrainSampleVertexHeight(ix, iz + 1, node);
        v11 = terrainSampleVertexHeight(ix + 1, iz + 1, node);
    } else {
        fx = ((x - ((MazeNode *)node)->x) * GRID(node)->w) / ((MazeNode *)node)->w;
        ix = fx;
        fz = ((z - ((MazeNode *)node)->z) * GRID(node)->h) / ((MazeNode *)node)->d;
        iz = fz;
        v00 = terrainSampleCellHeight(ix, iz, node);
        v10 = terrainSampleCellHeight(ix + 1, iz, node);
        v01 = terrainSampleCellHeight(ix, iz + 1, node);
        v11 = terrainSampleCellHeight(ix + 1, iz + 1, node);
    }
    tx = fx - ix;
    tz = fz - iz;
    return ((1.0f - tz) * (((1.0f - tx) * v00) + (tx * v10))) + (tz * (((1.0f - tx) * v01) + (tx * v11)));
}

f32 terrainGetMazeHeight(f32, f32, u8 *);               /* extern */
f32 terrainGetTriangleHeight(f32, f32, u8 *);               /* extern */
f32 terrainGetHeightMapHeight(f32 arg0, f32 arg1, u8 *arg2); /* extern */
/// Evaluate terrain height within a supplied area node.
/// \param x World X coordinate.
/// \param z World Z coordinate.
/// \param node Live area node containing valid terrain data; mapped terrain requires positive horizontal
/// dimensions and nonempty grids.
/// \returns Terrain height relative to the node, or zero when the node has no terrain.
/// \note Original decompilation identifier: \c func_8010BBC4 (0x8010BBC4).
f32 terrainGetNodeLocalHeight(f32 x, f32 z, u8 *node) {
    u8 *terrainGrid;

    terrainGrid = (u8 *)((MazeNode *)node)->terrain;
    if ((terrainGrid == NULL) || (((MazeGrid *)terrainGrid)->cells == 0)) {
        return 0.0f;
    }
    if (((MazeNode *)node)->objects[0].address != 0) {
        return terrainGetMazeHeight(x, z, node);
    }
    if (((MazeGrid *)terrainGrid)->mode == 0) {
        return terrainGetTriangleHeight(x, z, node);
    }
    return terrainGetHeightMapHeight(x, z, node);
}

f32 terrainFindSurfaceLocalHeight(f32, f32, f32, u8 **);
/// Query terrain world height at a supplied position.
/// \param x World X coordinate.
/// \param y World Y coordinate used to select the terrain region.
/// \param z World Z coordinate.
/// \param previous Optional writable terrain-node cache; initialize to null or a live node in the current
/// area. Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns Terrain world height, or zero if no surface is found.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
/// \note Original decompilation identifier: \c func_8010BC40 (0x8010BC40).
f32 terrainGetWorldHeight(f32 x, f32 y, f32 z, u8 **previous) {
    u8 *node;
    f32 height;
    f32 px, pz;
    if (previous != NULL) {
        node = *previous;
    } else {
        node = NULL;
    }
    px = x;
    pz = z;
    x = terrainFindSurfaceLocalHeight(px, y, pz, &node);
    if (previous != NULL) {
        *previous = node;
    }
    if (node == NULL) {
        return 0.0f;
    }
    height = *(f32 *)(node + 0xC) + x;
    return height;
}

/// Query terrain world height through the caller's terrain-query interface.
/// \param unused Unused query argument.
/// \param x World X coordinate.
/// \param y World Y coordinate used to select the terrain region.
/// \param z World Z coordinate.
/// \param previous Optional writable terrain-node cache; initialize to null or a live node in the current
/// area. Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns Terrain world height, or zero if no surface is found.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
/// \note Original decompilation identifier: \c func_8010BCC0 (0x8010BCC0).
f32 terrainGetWorldHeightForQuery(s32 unused, f32 x, f32 y, f32 z, u8 **previous) {
    u8 *node;
    f32 height;
    f32 worldHeight;
    f32 position[3];

    if (previous != NULL) {
        node = *previous;
    } else {
        node = NULL;
    }
    height = terrainFindSurfaceLocalHeight(x, y, z, &node);
    if (previous != NULL) {
        *previous = node;
    }
    position[0] = x;
    position[1] = y;
    position[2] = z;
    if (node == NULL) {
        return 0.0f;
    } else {
        worldHeight = *(f32 *)(node + 0xC) + height;
    }
    return worldHeight;
}

#pragma intrinsic(fabsf)

/* Statement macro: triangle vertex A and the XZ edge vectors B - A and C - A. */
#define TRIANGLE_EDGES_XZ(tri, ax, az, bx, bz, cx, cz)                                                                 \
    do {                                                                                                               \
        (ax) = (tri)->a[0];                                                                                            \
        (az) = (tri)->a[2];                                                                                            \
        (bx) = (tri)->b[0] - (ax);                                                                                     \
        (bz) = (tri)->b[2] - (az);                                                                                     \
        (cx) = (tri)->c[0] - (ax);                                                                                     \
        (cz) = (tri)->c[2] - (az);                                                                                     \
    } while (0)

/// Query terrain material and surface attributes at a world position.
/// \param x World X coordinate.
/// \param y World Y coordinate used to select the terrain region.
/// \param z World Z coordinate.
/// \param cachedNode Optional writable terrain-node cache; initialize to null or a live node in the current
/// area. Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns The terrain material and attribute flags; 0x77 when no terrain is found, 0x170 for a maze block,
/// 0x8007 when a triangle surface cannot be resolved, or 0x4000 for terrain without supported attributes.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
/// \note Original decompilation identifier: \c func_8010BD64 (0x8010BD64).
s32 terrainGetSurfaceAttributes(f32 x, f32 y, f32 z, u8 **cachedNode) {
    u8 *node;
    f32 dx;
    f32 ax;
    f32 v;
    f32 lengthC;
    f32 az;
    f32 w;
    f32 bx;
    f32 bz;
    f32 det;
    f32 u;
    f32 dz;
    s32 offset;
    f32 cx;
    f32 cz;
    s32 maxRow;
    s32 mode;
    s32 maxColumn;
    s32 count;
    s32 column;
    s32 row;
    MazeGrid *grid;
    TerrainTriangle *triangle;
    s32 pad[3]; /* unused, sizes the frame */

    if (cachedNode != NULL) {
        node = *cachedNode;
    } else {
        node = NULL;
    }
    for (;;) {
        terrainFindSurfaceLocalHeight(x, y, z, &node);
        if (cachedNode != NULL) {
            *cachedNode = node;
        }
        if (node == NULL) {
            return 0x77;
        }
        offset = 0;
        if (*(s32 *)node != 0) {
            return 0x170;
        }
        grid = GRID(node);
        mode = grid->mode;
        if (mode == 0) {
            x -= *(f32 *)(node + 0x8);
            count = grid->w;
            z -= *(f32 *)(node + 0x10);
            if (count > 0) {
                triangle = (TerrainTriangle *)grid->cells;
                do {
                    offset += sizeof(TerrainTriangle);
                    TRIANGLE_EDGES_XZ(triangle, ax, az, bx, bz, cx, cz);
                    det = (bz * cx) - (cz * bx);
                    if (fabsf(det) > 0.001f) {
                        dx = x - ax;
                        dz = z - az;
                        u = ((cx * dz) - (cz * dx)) / det;
                        if ((f64)u < 0.0) {
                            continue;
                        }
                        v = ((bx * dz) - (bz * dx)) / -det;
                        if ((f64)v < 0.0) {
                            continue;
                        }
                        if ((u + v) > 1.0f) {
                            continue;
                        }
                    } else {
                        /* Degenerate projection: project onto each edge independently. */
                        det = (bx * bx) + (bz * bz);
                        if (det < 0.001f) {
                            continue;
                        }
                        dx = x - ax;
                        dz = z - az;
                        u = ((bx * dx) + (bz * dz)) / det;
                        if ((f64)u < 0.0) {
                            continue;
                        }
                        lengthC = (cx * cx) + (cz * cz);
                        if (lengthC < 0.001f) {
                            continue;
                        }
                        w = ((cx * dx) + (cz * dz)) / lengthC;
                        if ((f64)w < 0.0) {
                            continue;
                        }
                        if ((u + w) > 1.0f) {
                            continue;
                        }
                    }
                    return triangle->material | 0x8000;
                } while (triangle++, offset < count * (s32)sizeof(TerrainTriangle));
            }
            TRACE("fall through getGroundAttr, researching floor...\n");
            y -= gCurrentAreaConfiguration->cellDimensions[1];
            if (!((f64)y >= 0.0)) {
                TRACE("research failed.\n");
                return 0x8007;
            }
            continue;
        }
        if (mode == 2) {
            count = grid->w;
            maxRow = grid->h - 1;
            maxColumn = count - 1;
            dx = ((x - *(f32 *)(node + 0x8)) * (f32)maxColumn) / *(f32 *)(node + 0x14);
            dz = ((z - *(f32 *)(node + 0x10)) * (f32)maxRow) / *(f32 *)(node + 0x1C);
            column = dx;
            row = dz;
            if (column < 0) {
                column = 0;
            } else if (maxColumn < column) {
                column = maxColumn;
            }
            if (row < 0) {
                row = 0;
            } else if (maxRow < row) {
                row = maxRow;
            }
            return *(u16 *)((u8 *)grid->attributes + column * 2 + row * count * 2);
        }
        return 0x4000;
    }
}

/// Friction coefficients indexed by supported terrain material identifiers.
/// \note Original identifier: \c D_80123A28 (0x80123A28).
extern f32 gTerrainMaterialFriction[];

/// Look up the friction coefficient for a terrain material.
/// \param material Terrain material code whose low four bits identify a material from zero through seven.
/// \returns The configured friction coefficient.
///
/// \note Undefined behavior: identifiers eight through fifteen exceed the friction table bounds.
/// \note Original decompilation identifier: \c func_8010C18C (0x8010C18C).
f32 terrainGetFriction(s32 material) {
    return gTerrainMaterialFriction[material & 0xF];
}

/// Sound identifiers indexed by contact category and terrain material; minus one means no sound.
/// \note Original identifier: \c D_80123A48 (0x80123A48).
extern s32 gTerrainMaterialSounds[][8];

/// Look up a sound effect for a terrain material and contact category.
/// \param category Sound category identifier from zero through seven.
/// \param material Terrain material code with a material identifier from zero through seven.
/// \returns The configured sound effect identifier, or minus one when no sound is configured.
///
/// \note Undefined behavior: material identifiers eight through fifteen exceed the selected row's bounds.
/// \note Original decompilation identifier: \c func_8010C1A4 (0x8010C1A4).
s32 terrainGetMaterialSound(s32 category, s32 material) {
    material &= 0xF;
    return gTerrainMaterialSounds[category][material];
}

/// Create the contact effect appropriate to a terrain material when one is configured.
/// \param position World position in X, Y, Z order; supported ground effects may adjust Y to just above the
/// terrain.
/// \param material Terrain material code selecting the contact effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010C1C4 (0x8010C1C4).
void terrainCreateContactEffect(f32 *position, s32 material) {
    s32 variant;
    f32 spray[3];
    s32 sprayIndex;
    f32 dust[3];
    s32 dustIndex;
    f32 chips[3];
    s32 chipsIndex;
    s32 kind;

    switch (material & 0xF) {
    case 2:
        position[1] = terrainGetWorldHeight(position[0], position[1], position[2], NULL) + 0.5f;
        wispEffectCreate(7, position[0], position[1], position[2]);
        return;
    case 3:
        position[1] = (f32)(terrainGetWorldHeight(position[0], position[1], position[2], NULL) + 0.5f);
        sprayIndex = 0;
        if (((s32)randomBelow(6) + 2) > 0) {
            do {
                spray[0] = (f32)((((f64)randomNext() * 2.3283064365386963e-10) - 0.5) * 8.0);
                spray[1] = (f32)((f64)randomNext() * 2.3283064365386963e-10 * 5.0);
                spray[2] = (f32)((((f64)randomNext() * 2.3283064365386963e-10) - 0.5) * 8.0);
                kind = (s32)randomBelow(3);
                variant = (s32)randomBelow(3);
                fallingParticleCreate(
                    kind, variant, position, spray,
                    (f32)(((f64)randomNext() * 2.3283064365386963e-10 * 0.4000000059604645) + 0.800000011920929));
                sprayIndex += 1;
            } while (sprayIndex < ((s32)randomBelow(6) + 2));
            return;
        }
    default:
        return;
    case 1:
        position[1] = (f32)(terrainGetWorldHeight(position[0], position[1], position[2], NULL) + 0.5f);
        dustIndex = 0;
        if (((s32)randomBelow(3) + 3) > 0) {
            do {
                dust[0] = (f32)((((f64)randomNext() * 2.3283064365386963e-10) - 0.5) * 10.0);
                dust[1] = (f32)((f64)randomNext() * 2.3283064365386963e-10 * 3.0);
                dust[2] = (f32)((((f64)randomNext() * 2.3283064365386963e-10) - 0.5) * 10.0);
                variant = (s32)randomBelow(3);
                fallingParticleCreate(5, variant, position, dust,
                              (f32)(((f64)randomNext() * 2.3283064365386963e-10 * 0.5) + 0.6000000238418579));
                dustIndex += 1;
            } while (dustIndex < ((s32)randomBelow(3) + 3));
            return;
        }
        break;
    case 4:
        chipsIndex = 0;
        if (((s32)randomBelow(0xA) + 5) > 0) {
            do {
                chips[0] = (f32)((((f64)randomNext() * 2.3283064365386963e-10) - 0.5) * 10.0);
                chips[1] = (f32)((f64)randomNext() * 2.3283064365386963e-10 * 8.0);
                chips[2] = (f32)((((f64)randomNext() * 2.3283064365386963e-10) - 0.5) * 10.0);
                kind = (s32)randomBelow(2);
                variant = (s32)randomBelow(3);
                fallingParticleCreate(kind + 3, variant, position, chips,
                              (f32)(((f64)randomNext() * 2.3283064365386963e-10 * 0.5) + 1.0));
                chipsIndex += 1;
            } while (chipsIndex < ((s32)randomBelow(0xA) + 5));
            return;
        }
        break;
    case 5:
        wispEffectCreate(6, position[0], position[1], position[2]);
        break;
    }
}

/// Surface attribute masks that obstruct each horizontal collision policy.
/// \note Original identifier: \c D_80123B48 (0x80123B48).
extern s32 gTerrainCollisionPolicySurfaceMasks[];
/// Check horizontal clearance at a candidate world position under the requested collision policy.
/// \param type Collision policy from zero through nine; policy five also requires the active primary actor.
/// \param x Candidate world X coordinate.
/// \param y Candidate world Y coordinate.
/// \param z Candidate world Z coordinate.
/// \param width Horizontal clearance width.
/// \param height Vertical span used for clearance checks.
/// \param step Maximum permitted terrain world height.
/// \param dirX X movement sign: minus one, zero, or one.
/// \param dirZ Z movement sign: minus one, zero, or one.
/// \param previous Optional writable terrain-node cache; initialize to null or a live node in the current
/// area. Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns Directional obstruction flags, or zero when the tested position is clear.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
/// \note Original decompilation identifier: \c func_8010C788 (0x8010C788).
u16 terrainCheckHorizontalObstruction(s32 type, f32 x, f32 y, f32 z, f32 width, f32 height, f32 step, f32 dirX, f32 dirZ, u8 **previous) {
    u16 flags = 0;
    f32 halfWidth;
    f32 sample;
    f32 sideX;
    s32 *mask;
    f32 delta[3];
    f32 sideZ;

    halfWidth = width * 0.5f;
    sideZ = dirZ * halfWidth + z;
    sideX = dirX * halfWidth + x;
    if ((step < terrainGetWorldHeightForQuery(type, x, y, z, previous)) ||
        (mask = &gTerrainCollisionPolicySurfaceMasks[type], (terrainGetSurfaceAttributes(x, y, z, previous) & *mask)) ||
        (step < terrainGetWorldHeightForQuery(type, x, y + height, z, previous))) {
        flags = dirX > 0.0f ? 1 : 8;
        if (dirZ > 0.0f) {
            flags |= 4;
        } else {
            flags |= 0x20;
        }
    } else {
        if (dirZ != 0.0f) {
            sample = sideZ;
            if ((step < terrainGetWorldHeightForQuery(type, x, y, sample, previous)) ||
                (terrainGetSurfaceAttributes(x, y, sample, previous) & *mask)) {
                flags = dirZ > 0.0f ? 4 : 0x20;
            }
        }
        if (dirX != 0.0f) {
            sample = sideX;
            if ((step < terrainGetWorldHeightForQuery(type, sample, y, z, previous)) ||
                (terrainGetSurfaceAttributes(sample, y, z, previous) & *mask)) {
                if (dirX > 0.0f) {
                    flags |= 1;
                } else {
                    flags |= 8;
                }
            }
        }
    }
    if (type == 5) {
        delta[0] = *(f32 *)(((u8 *)gPikachuActor) + 0x44) - x;
        delta[1] = *(f32 *)(((u8 *)gPikachuActor) + 0x48) - y;
        delta[2] = *(f32 *)(((u8 *)gPikachuActor) + 0x4C) - z;
        if (vector3Length((f32 *)delta) < 5.0f) {
            flags |= 0x6D;
        }
    }
    return flags;
}

f32 sqrtf(f32);
#pragma intrinsic(sqrtf)
/// Constrain a requested world displacement to movement permitted by terrain and the selected collision
/// policy.
/// \param queryKind Collision policy from zero through nine; policy five also requires the active primary
/// actor.
/// \param x Starting world X coordinate.
/// \param y Starting world Y coordinate.
/// \param z Starting world Z coordinate.
/// \param width Nonnegative horizontal clearance width.
/// \param depth Vertical span used for horizontal clearance queries.
/// \param height Permitted clearance height above the starting Y coordinate.
/// \param deltaX Writable requested X displacement, replaced by allowed movement.
/// \param deltaY Writable requested Y displacement, replaced by allowed movement.
/// \param deltaZ Writable requested Z displacement, replaced by allowed movement.
/// \param hitNode Optional writable terrain-node cache; initialize to null or a live node in the current
/// area. Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns Collision flags from the final tested movement step, or zero when no collision is reported.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
///
/// \note Displacement outputs must be distinct. Inputs and horizontal distance must be finite, with the
/// movement step count representable as a signed integer.
/// \note Original decompilation identifier: \c func_8010CAA4 (0x8010CAA4).
s32 terrainConstrainMovement(s32 queryKind, f32 x, f32 y, f32 z, f32 width, f32 depth, f32 height, f32 *deltaX, f32 *deltaY,
                  f32 *deltaZ, u8 **hitNode) {
    f32 stepX;
    f32 stepY;
    f32 stepZ;
    s32 pad[3]; /* unused, sizes the frame */
    f32 startX;
    f32 startY;
    f32 startZ;
    f32 topY;
    f32 nextX;
    f32 nextY;
    f32 nextZ;
    f32 horizontalLength;
    f32 signX;
    f32 signZ;
    f32 inverseSteps;
    f32 movedX;
    f32 groundY;
    f32 movedY;
    f32 movedZ;
    u16 collision;
    u16 mask;
    s32 step;
    s32 stepCount;

    if (*deltaX == 0.0f && *deltaZ == 0.0f) {
        if (*deltaY == 0.0f) {
            return 0;
        }
        horizontalLength = 0.0f;
    } else {
        horizontalLength = sqrtf((*deltaX * *deltaX) + (*deltaZ * *deltaZ));
    }
    signX = (*deltaX > 0.0f) ? 1 : ((*deltaX < 0.0f) ? -1 : 0);
    stepCount = 1;
    signZ = (*deltaZ > 0.0f) ? 1 : ((*deltaZ < 0.0f) ? -1 : 0);
    if (horizontalLength < 1.0f) {
        stepX = *deltaX;
        stepY = *deltaY;
        stepZ = *deltaZ;
    } else {
        stepCount = (s32)(horizontalLength * 1);
        inverseSteps = 1.0f / (f32)stepCount;
        stepX = *deltaX * inverseSteps;
        stepY = *deltaY * inverseSteps;
        stepZ = *deltaZ * inverseSteps;
    }
    startX = x;
    startZ = z;
    mask = 0;
    step = 0;
    startY = y;
    topY = y + height;
    if (stepCount > 0) {
        do {
            nextX = x + stepX;
            nextY = y + stepY;
            nextZ = z + stepZ;

            if ((stepX != 0.0f) || (stepZ != 0.0f)) {
                collision = terrainCheckHorizontalObstruction(queryKind, nextX, nextY, nextZ, width, depth, topY, signX, signZ, hitNode);
                mask = collision;
                if (collision & 9) {
                    nextX = x;
                }
                if (collision & 0x24) {
                    nextZ = z;
                }
            }
            if (stepY != 0.0f) {
                groundY = terrainGetWorldHeight(nextX, y, nextZ, hitNode);
                if (nextY < groundY) {
                    mask = mask | 0x10;
                    nextY = groundY;
                } else {
                    if (groundY < terrainGetWorldHeight(nextX, nextY, nextZ, NULL)) {
                        mask = mask | 2;
                        nextY = y;
                    }
                }
            }
            step += 1;
            x = nextX;
            y = nextY;
            z = nextZ;
        } while (step != stepCount);
    }
    movedX = x - startX;
    *deltaX = movedX;
    if ((movedX < 0.001f) && (*deltaX > -0.001f)) {
        *deltaX = 0.0f;
    }
    movedY = y - startY;
    *deltaY = movedY;
    if ((movedY < 0.001f) && (movedY > -0.001f)) {
        *deltaY = 0.0f;
    }
    movedZ = z - startZ;
    *deltaZ = movedZ;
    if ((movedZ < 0.001f) && (movedZ > -0.001f)) {
        *deltaZ = 0.0f;
    }
    return mask;
}

#pragma intrinsic(fabsf)
/// Measure terrain height changes toward positive X and Z for motion responses.
/// \param x World X coordinate.
/// \param y World Y coordinate used to select terrain.
/// \param z World Z coordinate.
/// \param maxSlope Nonnegative largest absolute height difference to accept; larger differences produce zero.
/// \param step Horizontal sampling distance along each positive axis.
/// \param slopeX Writable center height minus the positive-X sample height, or zero for negligible or
/// excessive change.
/// \param slopeZ Distinct writable output for center height minus the positive-Z sample height, or zero for
/// negligible or excessive change.
/// \param node Optional writable terrain-node cache; initialize to null or a live node in the current area.
/// Receives a borrowed node, or null when no surface is found; invalidate it before area teardown.
/// \returns Nothing.
///
/// \note Requires initialized current-area terrain and a positive vertical search spacing.
/// \note Original decompilation identifier: \c func_8010CE98 (0x8010CE98).
void terrainGetDownhillHeightDifferences(f32 x, f32 y, f32 z, f32 maxSlope, f32 step, f32 *slopeX, f32 *slopeZ, u8 **node) {
    f32 heightX;
    f32 heightZ;
    f32 center;
    f32 deltaX;
    f32 deltaZ;
    s32 flatX;
    s32 flatZ;

    heightX = terrainGetWorldHeight(x + step, y, z, node);
    heightZ = terrainGetWorldHeight(x, y, z + step, node);
    center = terrainGetWorldHeight(x, y, z, node);
    step = 0.001f;
    flatX = fabsf(center - heightX) <= ((fabsf(heightX) * step) + step) ? 1 : 0;
    if ((flatX != 0) || (deltaX = center - heightX, (maxSlope < fabsf(deltaX)))) {
        *slopeX = 0.0f;
    } else {
        *slopeX = deltaX;
    }
    flatZ = fabsf(center - heightZ) <= ((fabsf(heightZ) * step) + step) ? 1 : 0;
    if ((flatZ != 0) || (deltaZ = center - heightZ, (maxSlope < fabsf(deltaZ)))) {
        *slopeZ = 0.0f;
    } else {
        *slopeZ = deltaZ;
    }
}

/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;
/// Camera storage viewed at retained matrix pointers, with 224 bytes per camera.
/// \note Original identifier: \c D_800ABD14 (0x800ABD14).
extern u8 gRenderCameraMatrixEntries[];
/// Transform a world position for the active camera and test the current area's view limits.
/// \param margin Allowance beyond the camera view boundaries.
/// \param position World position in X, Y, Z order.
/// \param out Output three-component camera-space position, written even when visibility is rejected.
/// \returns One if the view test accepts the position, or zero otherwise.
///
/// \note Requires an initialized current area and a valid active camera.
/// \note Original decompilation identifier: \c func_8010CFF8 (0x8010CFF8).
s32 areaTestCameraVisibility(f32 margin, f32 *position, f32 *out) {
    f32 x, y, z, far;
    matrix34TransformPointVector((f32 *)(((u8 *)gRenderCameras) + gRenderCameraIndex * 0xE0 + 0x58), position, out);
    far = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0x24) * 1.7f;
    z = -out[2];
    x = out[0];
    y = out[1];
    if (far < z)
        return 0;
    z += margin * *(f32 *)(gRenderCameraMatrixEntries + gRenderCameraIndex * 0xE0 + 0x64);
    if (x < z && -x < z && y < z && -y < z)
        return 1;
    return 0;
}


/// Deactivate an area node and its active descendants, disabling their associated objects.
/// \param node Initialized area node hierarchy to deactivate.
/// \returns Nothing.
///
/// \note The supplied hierarchy must be acyclic and its nonnull objects must be live.
/// \note Original decompilation identifier: \c func_8010D100 (0x8010D100).
void areaDeactivateNodeHierarchy(u8 *node) {
    s32 i;
    u8 *child;

    if (((MazeNode *)node)->active != 0) {
        ((MazeNode *)node)->active = 0;
        if ((((MazeNode *)node)->objects[0].bytes != NULL) &&
            !(((RuntimeObjectPrefix *)((MazeNode *)node)->objects[0].bytes)->flags & 0x80)) {
            objectSetHierarchyFlags(((MazeNode *)node)->objects[0].bytes, 0x80);
        }
        if ((((MazeNode *)node)->objects[1].bytes != NULL) &&
            !(((RuntimeObjectPrefix *)((MazeNode *)node)->objects[1].bytes)->flags & 0x80)) {
            objectSetHierarchyFlags(((MazeNode *)node)->objects[1].bytes, 0x80);
        }
        for (i = 0; i < 8; i++) {
            child = ((u8 **)(node + 0x20))[i];
            if (child != NULL) {
                areaDeactivateNodeHierarchy(child);
            }
        }
    }
}


/// Activate an area node and enable its associated objects.
/// \param node Initialized area node to activate.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_8010D1B4 (0x8010D1B4).
void areaActivateNode(u8 *node) {
    u8 *primaryObject;
    u8 *secondaryObject;

    if (((MazeNode *)node)->active == 0) {
        ((MazeNode *)node)->active = 1;
        primaryObject = ((MazeNode *)node)->objects[0].bytes;
        if ((primaryObject != NULL) && (((RuntimeObjectPrefix *)primaryObject)->flags & 0x80)) {
            objectClearHierarchyFlags(primaryObject, 0x80);
        }
        secondaryObject = ((MazeNode *)node)->objects[1].bytes;
        if ((secondaryObject != NULL) && (((RuntimeObjectPrefix *)secondaryObject)->flags & 0x80)) {
            objectClearHierarchyFlags(secondaryObject, 0x80);
        }
    }
}

#include "scene_configuration.h"
/// Squared far-distance threshold used by area node visibility tests.
/// \note Original identifier: \c D_80133DF8 (0x80133DF8).
extern f32 gAreaFarVisibilityDistanceSquared;
/// Squared near-distance threshold used by area node visibility tests.
/// \note Original identifier: \c D_80133DFC (0x80133DFC).
extern f32 gAreaNearVisibilityDistanceSquared;

/// Refresh visibility distance limits from the current area's configuration.
/// \param unused Unused callback argument.
/// \returns Nothing.
///
/// \note Requires a loaded current-area configuration.
/// \note Original decompilation identifier: \c func_8010D234 (0x8010D234).
void areaRefreshVisibilityDistances(s32 unused) {
    f32 farClip;
    f32 nearClip;

    farClip = gCurrentAreaConfiguration->farClip;
    gAreaFarVisibilityDistanceSquared = farClip * farClip * 3.0f;
    nearClip = gCurrentAreaConfiguration->nearClip;
    gAreaNearVisibilityDistanceSquared = nearClip * nearClip * 3.0f;
}


/// Check whether a world position lies within the active camera's horizontal view allowance.
/// \param x World X coordinate.
/// \param y World Y coordinate.
/// \param z World Z coordinate.
/// \param margin Horizontal allowance beyond the camera view.
/// \returns One if the horizontal view test accepts the position, or zero otherwise.
///
/// \note Requires an initialized current area and a valid active camera.
/// \note Original decompilation identifier: \c func_8010D27C (0x8010D27C).
s32 areaIsPositionInHorizontalView(f32 x, f32 y, f32 z, f32 margin) {
    f32 ox;
    f32 oy;
    f32 oz;
    f32 d;

    matrix34TransformPoint((f32(*)[3])(gRenderCameraIndex * 0xE0 + 0x58 + ((u8 *)gRenderCameras)), x, y, z, &ox, &oy, &oz);
    d = margin - oz;
    if ((ox < d) && (-ox < d)) {
        return 1;
    }
    return 0;
}

#define D324_VISIBLE_IF(cond)                                                                                          \
    do {                                                                                                               \
        if (cond) {                                                                                                    \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)
/// Check whether an area node should remain active under the current camera and visibility limits.
/// \param node Initialized area node to test.
/// \returns One if the node is accepted for visibility, or zero otherwise.
///
/// \note Requires an initialized current area and a valid active camera.
///
/// \note Portability: the two visibility exits preserve the original IDO optimization boundaries.
/// \note Original decompilation identifier: \c func_8010D324 (0x8010D324).
s32 areaIsNodeVisible(u8 *node) {
    u8 *camera;
    f32 z;
    f32 dz;
    f32 center[3];
    f32 dx;
    f32 dy;
    f32 distance;
    u8 *base;

    base = ((u8 *)gRenderCameras);
    if (*(s16 *)(node + 0x40) >= 3) {
        return 1;
    }
    camera = gRenderCameraIndex * 0xE0 + base;
    z = *(f32 *)(node + 0x1C) * 0.5f + *(f32 *)(node + 0x10);
    dz = *(f32 *)(camera + 8) - z;
    center[2] = *(f32 *)(node + 0x14);
    center[1] = center[2] * 0.5f + *(f32 *)(node + 8);
    dx = *(f32 *)(camera + 0) - center[1];
    center[0] = *(f32 *)(node + 0x18) * 0.5f + *(f32 *)(node + 0xC);
    dy = *(f32 *)(camera + 4) - center[0];
    distance = dz * dz + (dx * dx + dy * dy);
    if (gAreaFarVisibilityDistanceSquared < distance) {
        return 0;
    }
    D324_VISIBLE_IF(distance < gAreaNearVisibilityDistanceSquared);
    D324_VISIBLE_IF(areaIsPositionInHorizontalView(center[1], center[0], z,
                                                   gMazeVisibilityParameters.radiusScale * center[2]) != 0);
    return 0;
}
#undef D324_VISIBLE_IF

void areaDeactivateNodeHierarchy(u8 *arg0); /* extern */
void areaActivateNode(u8 *arg0); /* extern */
s32 areaIsNodeVisible(u8 *arg0);  /* extern */
/// Activate an area node and update descendant participation according to camera visibility.
/// \param node Initialized area hierarchy whose visibility is refreshed.
/// \returns Nothing.
///
/// \note The supplied hierarchy must be acyclic and its nonnull objects must be live.
/// \note Original decompilation identifier: \c func_8010D468 (0x8010D468).
void areaUpdateNodeVisibility(u8 *node) {
    s32 var_s1;
    u8 *childNode;
    u8 *var_s2;

    areaActivateNode(node);
    var_s1 = 0;
    var_s2 = node;
    do {
        childNode = (*(u8 **)(var_s2 + (u32) & ((MazeNode *)0)->children[0]));
        if (childNode != NULL) {
            if (areaIsNodeVisible(childNode) != 0) {
                areaUpdateNodeVisibility(childNode);
            } else {
                areaDeactivateNodeHierarchy(childNode);
            }
        }
        var_s1 += 4;
        var_s2 += 4;
    } while (var_s1 != 0x20);
}

void _bzero(void *, s32); /* extern */

/// Allocate an empty, active area node for subsequent configuration.
/// \returns The newly allocated node owned by the area hierarchy.
/// \note Original decompilation identifier: \c func_8010D4F0 (0x8010D4F0).
u8 *areaAllocateNode(void) {
    u8 *sp1C;

    sp1C = heapAllocLocal(&gGameHeap, 0x50, 0x81A);
    _bzero(sp1C, 0x50);
    ((MazeNode *)sp1C)->depth = -1;
    ((MazeNode *)sp1C)->active = 1;
    return sp1C;
}

/// Borrowed scalar view of the initial scene cell dimensions used during root initialization.
/// The view describes only the shared configuration prefix's first twelve bytes.
typedef char SceneRootZViewOffsetCheck
    [((u32) & ((Vec3f *)0)->z == (u32) & ((SceneConfigurationPrefix *)0)->cellDimensions[2]) ? 1 : -1];
/// Create the maze root region and configure its geometry and visibility scale for the current area.
/// \returns Nothing; requires a loaded area configuration and no existing maze root to replace.
///
/// \note Configured cell dimensions must be positive; the nonnegative maze depth must keep all integer-power
/// intermediate products within the signed integer range.
/// \note Original decompilation identifier: \c func_8010D540 (0x8010D540).
void mazeInitializeRoot(void) {
    s32 integerScale;
    Vec3f dims;
    f32 length;
    gAreaMazeRoot = (MazeNode *)areaAllocateNode();
    integerScale = integerPower(2, *(s32 *)(((u8 *)gCurrentAreaConfiguration) + 0x10));
    ((MazeNode *)gAreaMazeRoot)->x = 0.0f;
    ((MazeNode *)gAreaMazeRoot)->y = 0.0f;
    ((MazeNode *)gAreaMazeRoot)->z = 0.0f;
    ((MazeNode *)gAreaMazeRoot)->w = (f32)integerScale * ((Vec3f *)((u8 *)gCurrentAreaConfiguration))->x;
    ((MazeNode *)gAreaMazeRoot)->h = (f32)integerScale * ((Vec3f *)((u8 *)gCurrentAreaConfiguration))->y;
    ((MazeNode *)gAreaMazeRoot)->d = (f32)integerScale * ((Vec3f *)((u8 *)gCurrentAreaConfiguration))->z;
    gAreaMazeRoot->depth = *(s32 *)(((u8 *)gCurrentAreaConfiguration) + 0x10);
    dims.x = ((Vec3f *)((u8 *)gCurrentAreaConfiguration))->x;
    dims.y = ((Vec3f *)((u8 *)gCurrentAreaConfiguration))->y;
    dims.z = ((Vec3f *)((u8 *)gCurrentAreaConfiguration))->z;
    length = vector3Length((f32 *)&dims);
    gMazeVisibilityParameters.radiusScale =
        length * 0.5f / (mathSinDegrees(36.0f) * ((Vec3f *)((u8 *)gCurrentAreaConfiguration))->x);
    TRACE("blockSize: %f,%f,%f  _clipZFactor:%f  clipFar:%f clipNear:%f clipFactor:%f\n", dims.x, dims.y, dims.z,
          *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0x2C), *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0x24),
          *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0x28), gMazeVisibilityParameters.radiusScale);
}


/// Configure a maze node for its selected kind, creating its model and running the configured initializer
/// when present.
/// \param node Maze node with initialized world bounds and no existing model to replace.
/// \param kind Valid kind index in the current maze configuration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010D658 (0x8010D658).
void mazeConfigureNodeModel(MazeNode *node, s32 kind) {
    u8 *root;
    u8 *part;
    s32 offset = kind * sizeof(SceneCellType);
    node->terrain = (*(SceneCellType *)((u8 *)gCurrentAreaConfiguration->cellTypes + offset)).terrain;
    if ((*(SceneCellType *)((u8 *)gCurrentAreaConfiguration->cellTypes + offset)).objectTemplate != NULL) {
        root = objectCreateHierarchyFromTemplate(
            (ObjectTemplate *)(*(SceneCellType *)((u8 *)gCurrentAreaConfiguration->cellTypes + offset)).objectTemplate);
        root = objectRegisterActiveHierarchy(root);
        node->objects[1].bytes = root;
        *(f32 *)(root + 0x14) = 0.0f;
        *(f32 *)(root + 0x10) = 0.0f;
        *(f32 *)(root + 0xC) = 0.0f;
        *(f32 *)(root + 0x18) = node->x;
        *(f32 *)(root + 0x1C) = node->y;
        part = *(u8 **)(root + 0x28);
        *(f32 *)(root + 0x20) = node->z;
        *(f32 *)(root + 0) *= gCurrentAreaConfiguration->objectScale;
        *(f32 *)(root + 4) *= gCurrentAreaConfiguration->objectScale;
        *(f32 *)(root + 8) *= gCurrentAreaConfiguration->objectScale;
        while (part != NULL) {
            *(f32 *)(part + 0x18) = *(f32 *)(part + 0x18) * gCurrentAreaConfiguration->objectScale + node->x;
            *(f32 *)(part + 0x1C) = *(f32 *)(part + 0x1C) * gCurrentAreaConfiguration->objectScale + node->y;
            *(f32 *)(part + 0x20) = *(f32 *)(part + 0x20) * gCurrentAreaConfiguration->objectScale + node->z;
            *(f32 *)(part + 0) = *(f32 *)(part + 0) * *(f32 *)(root + 0);
            *(f32 *)(part + 4) = *(f32 *)(part + 4) * *(f32 *)(root + 4);
            *(f32 *)(part + 8) = *(f32 *)(part + 8) * *(f32 *)(root + 8);
            part = *(u8 **)(part + 0x28);
        }
    }
    if ((*(SceneCellType *)((u8 *)gCurrentAreaConfiguration->cellTypes + offset)).initialize != NULL) {
        (*(SceneCellType *)((u8 *)gCurrentAreaConfiguration->cellTypes + offset)).initialize((u8 *)node);
    }
}

/// Configure a maze child region within its parent.
/// \param parent Maze parent with initialized bounds and an allocated child at the selected index.
/// \param index Child region index from zero through seven.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010D7FC (0x8010D7FC).
void mazeConfigureChildBounds(MazeNode *parent, s32 index) {
    MazeNode *child = parent->children[index];
    child->x = gMazeChildOffsetX[index] * parent->w * 0.5f + parent->x;
    child->y = gMazeChildOffsetY[index] * parent->h * 0.5f + parent->y;
    child->z = gMazeChildOffsetZ[index] * parent->d * 0.5f + parent->z;
    child->w = parent->w * 0.5f;
    child->h = parent->h * 0.5f;
    child->d = parent->d * 0.5f;
    child->depth = parent->depth - 1;
}

/// Matching-only borrowed view of scene dimensions and the occupancy callback.
/// Shared-array field access changes mazeClassifyNode instructions. This smaller view must not be used
/// to allocate or copy configurations; all used offsets are checked against SceneConfigurationPrefix.
typedef struct {
    f32 width, height, depth; ///< Cell dimensions in world units along X, Y and Z.
    u8 omittedFields[0x30];   ///< Intervening configuration storage, not owned by this view.
    SceneCellSampler sample;  ///< Borrowed occupancy classification callback.
} SceneOccupancyView;
typedef char SceneOccupancyViewSizeCheck[(sizeof(SceneOccupancyView) == 0x40) ? 1 : -1];
typedef char SceneOccupancyXViewOffsetCheck
    [((u32) & ((SceneOccupancyView *)0)->width == (u32) & ((SceneConfigurationPrefix *)0)->cellDimensions[0]) ? 1 : -1];
typedef char SceneOccupancyYViewOffsetCheck[((u32) & ((SceneOccupancyView *)0)->height == (u32) &
                                             ((SceneConfigurationPrefix *)0)->cellDimensions[1])
                                                ? 1
                                                : -1];
typedef char SceneOccupancyZViewOffsetCheck
    [((u32) & ((SceneOccupancyView *)0)->depth == (u32) & ((SceneConfigurationPrefix *)0)->cellDimensions[2]) ? 1 : -1];
typedef char SceneOccupancyCallbackViewOffsetCheck
    [((u32) & ((SceneOccupancyView *)0)->sample == (u32) & ((SceneConfigurationPrefix *)0)->sampleCell) ? 1 : -1];

/// Classify a maze region using the current area's occupancy sampler.
/// \param node Maze region with initialized bounds and nonnegative remaining depth.
/// \returns Zero for an empty region, a configured model kind for a terminal region, or minus one when
/// subdivision is requested.
///
/// \note Requires a loaded configuration with a callable occupancy sampler and positive cell dimensions. The
/// depth must keep integer-power intermediate products within the signed integer range.
/// \note Original decompilation identifier: \c func_8010D8D8 (0x8010D8D8).
s32 mazeClassifyNode(MazeNode *node) {
    f32 width;
    s32 result;
    f32 height;
    f32 depth;
    s32 count;
    s32 x;
    s32 y;
    s32 z;

    count = integerPower(2, node->depth);
    result = ((SceneOccupancyView *)((u8 *)gCurrentAreaConfiguration))
                 ->sample(node->x + ((SceneOccupancyView *)((u8 *)gCurrentAreaConfiguration))->width * 0.5f,
                          node->y + ((SceneOccupancyView *)((u8 *)gCurrentAreaConfiguration))->height * 0.5f,
                          node->z + ((SceneOccupancyView *)((u8 *)gCurrentAreaConfiguration))->depth * 0.5f);
    if (count >= 2) {
        for (x = 0; x < count; x++) {
            for (y = 0; y < count; y++) {
                for (z = 0; z < count; z++) {
                    if (x == 0 && y == 0 && z == 0)
                        continue;
                    {
                        f32 px, py, pz;
                        width = ((SceneOccupancyView *)((u8 *)gCurrentAreaConfiguration))->width;
                        height = ((SceneOccupancyView *)((u8 *)gCurrentAreaConfiguration))->height;
                        depth = ((SceneOccupancyView *)((u8 *)gCurrentAreaConfiguration))->depth;
                        px = node->x + x * width + width * 0.5f;
                        py = node->y + y * height + height * 0.5f;
                        pz = node->z + z * depth + depth * 0.5f;
                        if (((SceneOccupancyView *)((u8 *)gCurrentAreaConfiguration))->sample(px, py, pz) != 0) {
                            return -1;
                        }
                    }
                }
            }
        }
    }
    return result;
}


/// Build a maze region's descendants and models from the current area's configuration.
/// \param node New maze node with initialized bounds and remaining depth.
/// \param depth Nonnegative recursive construction depth.
/// \param forceSplit Nonzero requests subdivision without occupancy classification.
/// \returns Nothing.
///
/// \note Requires a loaded maze configuration and a fresh node without owned descendants or models.
/// Portability: the empty per-child debug check preserves the original IDO code generation.
/// \note Original decompilation identifier: \c func_8010DA74 (0x8010DA74).
void mazeBuildNodeHierarchy(u8 *node, s32 depth, s32 forceSplit) {
    s32 kind;
    s32 i;
    if (depth >= 4) {
        runtimeCheckResetCheckpoint("pre-digMazeTree", 1);
    }
    if (forceSplit == 0) {
        kind = mazeClassifyNode((MazeNode *)node);
    } else {
        kind = -1;
    }
    if (kind == 0) {
        *(s32 *)(node + 0) = 0;
        *(s32 *)(node + 4) = 0;
        *(s32 *)(node + 0x44) = 0;
        *(s32 *)(node + 0x48) = 0;
    } else if (kind == -1) {
        for (i = 0; i != 8; i++) {
            ((MazeNode *)node)->children[i] = (MazeNode *)areaAllocateNode();
            mazeConfigureChildBounds((MazeNode *)node, i);
            if (depth > 0) {
                mazeBuildNodeHierarchy((u8 *)((MazeNode *)node)->children[i], depth - 1, forceSplit);
            }
            if (forceSplit) {
            }
        }
    } else {
        mazeConfigureNodeModel((MazeNode *)node, kind);
    }
}

u8 *areaFindPositionNode(f32 arg0, f32 arg1, f32 arg2); /* extern */

/// Create and register a block object in the maze region selected by a world position.
/// \param position World position in X, Y, Z order selecting a region with no existing block object to
/// replace.
/// \param objectTemplate Address of a valid object hierarchy template for the new block.
/// \returns Nothing.
///
/// \pre The maze is initialized.
/// \note Original decompilation identifier: \c func_8010DB68 (0x8010DB68).
void mazePlaceBlockObject(u8 *position, u8 *objectTemplate) {
    u8 *sp1C;
    u8 *blockHierarchy;
    u8 *registeredHierarchy;

    sp1C = areaFindPositionNode(((f32 *)position)[0], ((f32 *)position)[1], ((f32 *)position)[2]);
    blockHierarchy = objectCreateHierarchyFromTemplate((ObjectTemplate *)objectTemplate);
    ((RuntimeObjectPrefix *)blockHierarchy)->kind = 0x88;
    registeredHierarchy = objectRegisterActiveHierarchy(blockHierarchy);
    ((RuntimeObjectPrefix *)registeredHierarchy)->position[0] = (f32)((MazeNode *)sp1C)->x;
    ((RuntimeObjectPrefix *)registeredHierarchy)->position[1] = (f32)((MazeNode *)sp1C)->y;
    ((RuntimeObjectPrefix *)registeredHierarchy)->position[2] = (f32)((MazeNode *)sp1C)->z;
    ((RuntimeObjectPrefix *)registeredHierarchy)->scale[0] *= gCurrentAreaConfiguration->objectScale;
    ((RuntimeObjectPrefix *)registeredHierarchy)->scale[1] *= gCurrentAreaConfiguration->objectScale;
    ((RuntimeObjectPrefix *)registeredHierarchy)->scale[2] *= gCurrentAreaConfiguration->objectScale;
    ((MazeNode *)sp1C)->objects[0].bytes = registeredHierarchy;
}

/// Remove a maze block's area association and request destruction of its object hierarchy.
/// \param object Live block object associated with its current maze region.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010DC14 (0x8010DC14).
void mazeRemoveBlockObject(u8 *object) {
    *(s32 *)areaFindPositionNode(
        gCurrentAreaConfiguration->cellDimensions[0] / 2 + ((RuntimeObjectPrefix *)object)->position[0],
        gCurrentAreaConfiguration->cellDimensions[1] / 2 + ((RuntimeObjectPrefix *)object)->position[1],
        gCurrentAreaConfiguration->cellDimensions[2] / 2 + ((RuntimeObjectPrefix *)object)->position[2]) = 0;
    objectRequestHierarchyDestruction(object);
}

void mazeBuildNodeHierarchy(u8 *, s32, s32);
void mazePrepareNodeObjects(u8 *);

/// Replace the model and terrain of a maze block at the requested subdivision level.
/// \param pos World-space XYZ origin of the block to replace.
/// \param modelTemplate Replacement object template, or NULL to create no new model.
/// \param terrain Borrowed terrain definition address, or zero for no terrain; must remain live while the
/// block uses it.
/// \param level Requested remaining subdivision depth, within the configured maze depth.
/// \returns Nothing.
///
/// \pre The maze is initialized.
/// \note Original decompilation identifier: \c func_8010DC88 (0x8010DC88).
void mazeReplaceBlock(u8 *pos, u8 *modelTemplate, MazeGrid *terrain, s32 level) {
    u8 *node;
    u8 *model;
    u8 *child;

    heapDebugReportFreeSpace(&gGameHeap, (const char *) "pre_changeMazeBlock");
    node = areaFindPositionNode(*(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0x0) / 2 + *(f32 *)(pos + 0x0),
                                *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0x4) / 2 + *(f32 *)(pos + 0x4),
                                *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0x8) / 2 + *(f32 *)(pos + 0x8));
    if (level < *(s16 *)(node + 0x40)) {
        TRACE("block level mismatch (level %d, [%3.2f, %3.2f, %3.2f]), force to digging...\n", level,
              *(f32 *)(pos + 0x0), *(f32 *)(pos + 0x4), *(f32 *)(pos + 0x8));
        mazeBuildNodeHierarchy(node, *(s16 *)(node + 0x40) - level - 1, 1);
        node = areaFindPositionNode(*(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0x0) / 2 + *(f32 *)(pos + 0x0),
                                    *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0x4) / 2 + *(f32 *)(pos + 0x4),
                                    *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0x8) / 2 + *(f32 *)(pos + 0x8));
    }
    if (*(u8 **)(node + 0x4) != NULL) {
        objectRequestHierarchyDestruction(*(u8 **)(node + 0x4));
    }
    ((MazeNode *)node)->terrain = terrain;
    if (modelTemplate != NULL) {
        model = objectRegisterActiveHierarchy(objectCreateHierarchyFromTemplate((ObjectTemplate *)modelTemplate));
        *(u8 **)(node + 0x4) = model;
        *(f32 *)(model + 0x14) = 0.0f;
        *(f32 *)(model + 0x10) = 0.0f;
        *(f32 *)(model + 0xC) = 0.0f;
        child = *(u8 **)(model + 0x28);
        *(f32 *)(model + 0x18) = *(f32 *)(node + 0x8);
        *(f32 *)(model + 0x1C) = *(f32 *)(node + 0xC);
        *(f32 *)(model + 0x20) = *(f32 *)(node + 0x10);
        *(f32 *)(model + 0x0) = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * *(f32 *)(model + 0x0);
        *(f32 *)(model + 0x4) = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * *(f32 *)(model + 0x4);
        *(f32 *)(model + 0x8) = *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * *(f32 *)(model + 0x8);
        if (child != NULL) {
            do {
                *(f32 *)(child + 0x18) =
                    *(f32 *)(node + 0x8) + *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * *(f32 *)(child + 0x18);
                *(f32 *)(child + 0x1C) =
                    *(f32 *)(node + 0xC) + *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * *(f32 *)(child + 0x1C);
                *(f32 *)(child + 0x20) =
                    *(f32 *)(node + 0x10) + *(f32 *)(((u8 *)gCurrentAreaConfiguration) + 0xC) * *(f32 *)(child + 0x20);
                *(f32 *)(child + 0x0) = *(f32 *)(child + 0x0) * *(f32 *)(model + 0x0);
                *(f32 *)(child + 0x4) = *(f32 *)(child + 0x4) * *(f32 *)(model + 0x4);
                *(f32 *)(child + 0x8) = *(f32 *)(child + 0x8) * *(f32 *)(model + 0x8);
                child = *(u8 **)(child + 0x28);
            } while (child != NULL);
        }
        mazePrepareNodeObjects(node);
        heapDebugReportFreeSpace(&gGameHeap, (const char *) "aft_changeMazeBlock");
    }
}

/// Configure maze event flags and destination links from the current area's link definitions.
/// \returns Nothing; requires a constructed maze hierarchy and a complete link list terminated by a null
/// callback, or a null list.
///
/// \note Portability: the shared initialization and loop-label line preserves the original IDO scheduling.
/// Callback addresses use the original 32-bit pointer ABI. The byte cursor retains target null-plus-zero
/// behavior when the configured event list is absent.
/// \note Original decompilation identifier: \c func_8010DEF8 (0x8010DEF8).
void mazeConfigureEventLinks(void) {
    f32 width;
    f32 height;
    f32 depth;
    f32 x;
    f32 y;
    f32 z;
    s32 offset;
    SceneEventLink *link;
    u8 *node;



    // clang-format off
    offset = 0; nextLink:
    // clang-format on
    link = (SceneEventLink *)((u8 *)gCurrentAreaConfiguration->eventLinks + offset);
    if ((link != NULL) && (link->event != NULL)) {
        width = (gCurrentAreaConfiguration->cellDimensions[0]);
        height = (gCurrentAreaConfiguration->cellDimensions[1]);
        depth = (gCurrentAreaConfiguration->cellDimensions[2]);
        x = ((f32)(link->sourceCell[0]) * width) + (width * 0.5f);
        y = ((f32)(link->sourceCell[1]) * height) + (height * 0.5f);
        z = ((f32)(link->sourceCell[2]) * depth) + (depth * 0.5f);
        node = areaFindPositionNode(x, y, z);
        ((MazeNode *)node)->event = link->event;
        width = (gCurrentAreaConfiguration->cellDimensions[0]);
        height = (gCurrentAreaConfiguration->cellDimensions[1]);
        depth = (gCurrentAreaConfiguration->cellDimensions[2]);
        x = ((f32)(link->targetCell[0]) * width) + (width * 0.5f);
        y = ((f32)(link->targetCell[1]) * height) + (height * 0.5f);
        z = ((f32)(link->targetCell[2]) * depth) + (depth * 0.5f);
        if (!((f64)x < 0.0) && !((f64)y < 0.0) && !((f64)z < 0.0)) {
            ((MazeNode *)node)->linkedNode = (MazeNode *)areaFindPositionNode(x, y, z);
        }
        offset += (s32)sizeof(SceneEventLink);
        goto nextLink;
    }
}


/// Release a maze node and its associated objects, invoking its configured leave callback when present.
/// \param node Owned maze node to release after its descendants have been handled; invalid after this call.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010E0E0 (0x8010E0E0).
/// \note Checked member-offset reads preserve the matching target compiler expression ordering.
void mazeDestroyNode(u8 *node) {
    u8 *pendingObject;
    s32 type = gCurrentAreaConfiguration->sampleCell(
        (*(f32 *)(((u8 *)gCurrentAreaConfiguration) + (u32) & ((SceneConfigurationPrefix *)0)->cellDimensions[0])) *
                0.5f +
            (*(f32 *)(node + (u32) & ((MazeNode *)0)->x)),
        gCurrentAreaConfiguration->cellDimensions[1] * 0.5f + (*(f32 *)(node + (u32) & ((MazeNode *)0)->y)),
        (*(f32 *)(((u8 *)gCurrentAreaConfiguration) + (u32) & ((SceneConfigurationPrefix *)0)->cellDimensions[2])) *
                0.5f +
            (*(f32 *)(node + (u32) & ((MazeNode *)0)->z)));

    if (gCurrentAreaConfiguration->cellTypes[type].leave != NULL) {
        gCurrentAreaConfiguration->cellTypes[type].leave(node);
    }
    pendingObject = ((MazeNode *)node)->objects[0].bytes;
    if (pendingObject != NULL) {
        objectRequestHierarchyDestruction(pendingObject);
    }
    pendingObject = ((MazeNode *)node)->objects[1].bytes;
    if (pendingObject != NULL) {
        objectRequestHierarchyDestruction(pendingObject);
    }
    heapFreeLocal(&gGameHeap, node);
}

void mazeDestroyNode(u8 *); /* extern */

/// Release a maze node hierarchy and all of its associated objects.
/// \param node Owned maze hierarchy root to release; the root and descendants are invalid after this call.
/// \returns Nothing.
///
/// \note The supplied hierarchy must be acyclic and its nonnull objects must be live.
/// \note Original decompilation identifier: \c func_8010E1D0 (0x8010E1D0).
void mazeDestroyNodeHierarchy(u8 *node) {
    s32 var_s0;
    u8 *childNode;
    u8 *var_s1;

    var_s0 = 0;
    var_s1 = node;
    do {
        childNode = (*(u8 **)(var_s1 + (u32) & ((MazeNode *)0)->children[0]));
        if (childNode != NULL) {
            mazeDestroyNodeHierarchy(childNode);
        }
        var_s0 += 4;
        var_s1 += 4;
    } while (var_s0 != 0x20);
    mazeDestroyNode(node);
}

void mazeInitializeRoot(void);        /* extern */
void mazeConfigureEventLinks(void); /* extern */

/// Construct the current area's maze, its models, and configured event links.
/// \returns Nothing; requires a loaded area configuration and no existing maze to replace.
/// \note Original decompilation identifier: \c func_8010E234 (0x8010E234).
void mazeBuild(void) {
    mazeInitializeRoot();
    runtimeCheckResetCheckpoint("aft-buildMazeFoundation", 1);
    mazeBuildNodeHierarchy((u8 *)gAreaMazeRoot, gCurrentAreaConfiguration->subdivisionPower, 0);
    runtimeCheckResetCheckpoint("aft-digMazeTree", 1);
    objectPrepareDistributedKindSearches(0, 0xA);
    objectPrepareDistributedKindSearches(0x98, 4);
    objectPrepareDistributedKindSearches(0x95, 6);
    objectPrepareDistributedKindSearches(0x81, 6);
    objectPrepareDistributedKindSearches(0x80, 1);
    mazeConfigureEventLinks();
    runtimeCheckResetCheckpoint("aft-setMazeEvent", 1);
}


/// Spawn the initial items listed in the current area's maze configuration.
/// \returns Nothing.
///
/// \note Requires a loaded current-area configuration; a nonnull placement list must end with a negative item
/// kind and contain valid item kinds before it.
/// \note Original decompilation identifier: \c func_8010E2E4 (0x8010E2E4).
void mazeSpawnConfiguredItems(void) {
    s32 itemKind;
    u8 *var_s0;

    var_s0 = (u8 *)gCurrentAreaConfiguration->items;
    if (var_s0 != NULL) {
    loop_2:
        itemKind = ((SceneItemPlacement *)var_s0)->kind;
        if (itemKind >= 0) {
            itemSetMode(itemCreate(((SceneItemPlacement *)var_s0)->position[0],
                                   ((SceneItemPlacement *)var_s0)->position[1],
                                   ((SceneItemPlacement *)var_s0)->position[2], 0.0f, 0.0f, itemKind),
                        ITEM_MODE_STATIONARY);
            var_s0 += 0x10;
            goto loop_2;
        }
    }
}

void areaRefreshVisibilityDistances(s32 arg0); /* extern */
void areaUpdateNodeVisibility(u8 *arg0); /* extern */

/// Refresh current-area maze visibility for the active camera.
/// \param unused Unused callback argument.
/// \returns Nothing; requires an initialized maze and active camera.
/// \note Original decompilation identifier: \c func_8010E360 (0x8010E360).
void mazeUpdateVisibility(s32 unused) {
    areaRefreshVisibilityDistances(gRenderCameraIndex);
    areaUpdateNodeVisibility((u8 *)gAreaMazeRoot);
}

/// Stop hierarchy updates for static maze object kinds after initial preparation.
/// \param object First object in the hierarchy to prepare, or null.
/// \returns Nothing.
///
/// \note The supplied hierarchy must be acyclic and its nonnull objects must be live.
/// \note Original decompilation identifier: \c func_8010E394 (0x8010E394).
void mazeFreezeStaticObjects(u8 *object) {
    u16 objectKind;
    u8 *firstChild;

    while (object != NULL) {
        objectKind = ((RuntimeObjectPrefix *)object)->kind;
        if ((objectKind == 0) || (objectKind == 0x98) || (objectKind == 0x95)) {
            firstChild = ((RuntimeObjectPrefix *)object)->child.bytes;
            ((RuntimeObjectPrefix *)object)->flags |= 0x40;
            if (firstChild != NULL) {
                mazeFreezeStaticObjects(firstChild);
            }
        }
        object = ((RuntimeObjectPrefix *)object)->sibling.bytes;
    }
}


/// Prepare a maze object hierarchy for use and freeze its static objects.
/// \param object Live object hierarchy address to prepare.
/// \returns Nothing.
///
/// \note Portability: the object address is passed as a 32-bit integer and converted to a pointer.
/// \note Original decompilation identifier: \c func_8010E414 (0x8010E414).
void mazePrepareObjectHierarchy(s32 object) {
    objectUpdateHierarchy((RuntimeObjectPrefix *)object);
    objectUpdateHierarchy((RuntimeObjectPrefix *)object);
    mazeFreezeStaticObjects((u8 *)object);
}

void mazePrepareObjectHierarchy(s32 arg0); /* extern */
/// Prepare the objects associated with a maze node and all descendants.
/// \param node Maze hierarchy root to prepare, or null.
/// \returns Nothing.
///
/// \note The supplied hierarchy must be acyclic and its nonnull objects must be live.
/// \note Original decompilation identifier: \c func_8010E444 (0x8010E444).
void mazePrepareNodeObjects(u8 *node) {
    s32 i;

    if (node != NULL) {
        if (((MazeNode *)node)->objects[0].address != 0) {
            mazePrepareObjectHierarchy(((MazeNode *)node)->objects[0].address);
        }
        if (((MazeNode *)node)->objects[1].address != 0) {
            mazePrepareObjectHierarchy(((MazeNode *)node)->objects[1].address);
        }
        for (i = 0; i < 8; i++) {
            mazePrepareNodeObjects(((u8 **)(node + 0x20))[i]);
        }
    }
}

void mazeBuild(void);        /* extern */
void mazeSpawnConfiguredItems(void);        /* extern */
void mazePrepareNodeObjects(u8 *arg0);    /* extern */
extern f32 D_80133DE8;
/// Retained block-count diagnostic word reset during maze initialization.
/// \note Original identifier: \c D_80133DF4 (0x80133DF4).
extern s32 gMazeDiagnosticBlockCount;

/// Initialize the current area's maze, prepare its objects, and spawn its configured items.
/// \returns Nothing; requires a loaded area configuration and no existing maze to replace.
/// \note Original decompilation identifier: \c func_8010E4C0 (0x8010E4C0).
/// \note The retained vector spans three adjacent linker symbols; its byte layout is target-specific
/// and the indexed access relies on the original target compiler aliasing behavior.
void mazeInitialize(void) {
    u8 *mazeObject;

    gMazeDiagnosticBlockCount = 0;
    mazeObject = objectCreateDefault();
    ((RuntimeObjectPrefix *)mazeObject)->kind = 0x83;
    objectRegisterActiveHierarchy(objectMakeHierarchyRoot(mazeObject));
    mazeBuild();
    mazePrepareNodeObjects((u8 *)gAreaMazeRoot);
    runtimeCheckResetCheckpoint("aft-calcMazeFirst", 1);
    TRACE("totalBlockNum = %d\n", gMazeDiagnosticBlockCount);
    TRACE("sizeof(mazeBlockT) = %d\n", 0x50);
    mazeSpawnConfiguredItems();
    ((f32 *)&D_80133DE8)[0] = 0.0f;
    ((f32 *)&D_80133DE8)[1] = 0.0f;
    ((f32 *)&D_80133DE8)[2] = 0.0f;
}

/// Invoke the area's optional initializer after common gameplay setup.
/// \returns Nothing; has no effect when the callback is absent.
///
/// \note Requires a loaded current-area configuration and initialized common gameplay systems.
/// Release the previous area-owned objects before repeating scene initialization.
/// \note Original decompilation identifier: \c func_8010E53C (0x8010E53C).
void areaRunSceneInitializer(void) {
    void (*initializer)(void);

    initializer = gCurrentAreaConfiguration->initializeScene;
    if (initializer != NULL) {
        initializer();
    }
}

void mazeDestroyNodeHierarchy(u8 *arg0); /* extern */

/// Release the current maze hierarchy and its associated objects.
/// \returns Nothing; requires an initialized maze root.
/// \note Original decompilation identifier: \c func_8010E570 (0x8010E570).
void mazeDestroy(void) {
    mazeDestroyNodeHierarchy((u8 *)gAreaMazeRoot);
    gAreaMazeRoot = 0;
}

/// Area identifier requested for the next overlay load.
/// \note Original identifier: \c D_800AE500 (0x800AE500).
extern s32 gRuntimeNextArea;
/// Entrance index requested for the next area load.
/// \note Original identifier: \c D_800AE4FC (0x800AE4FC).
extern s32 gRuntimeNextEntrance;

/// Select the destination area and spawn for the next area load.
/// \param area Destination area identifier, wrapped through the 23 supported areas.
/// \param spawn Valid spawn index in the destination area's configuration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010E598 (0x8010E598).
void areaSetNextDestination(s32 area, s32 spawn) {
    gRuntimeNextArea = area;
    while (gRuntimeNextArea < 0) {
        gRuntimeNextArea += 23;
    }
    while (gRuntimeNextArea >= 23) {
        gRuntimeNextArea -= 23;
    }
    gRuntimeNextEntrance = spawn;
}

/// Request a transition to the selected area and destination spawn.
/// \param area Destination area identifier, wrapped through the 23 supported areas.
/// \param spawn Valid spawn index in the destination area's configuration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010E5E4 (0x8010E5E4).
void areaRequestTransition(s32 area, s32 spawn) {
    areaSetNextDestination(area, spawn);
    runtimeRequestGameModeChange(0xB);
}

/// Identifier of the currently loaded area.
/// \note Original identifier: \c D_800AE4F8 (0x800AE4F8).
extern s32 gRuntimeCurrentArea;
/// Entrance index used to initialize the player and companion in the current area.
/// \note Original identifier: \c D_800AE4F4 (0x800AE4F4).
extern s32 gRuntimeCurrentEntrance;

/// Make the selected destination and its loaded configuration the current area.
/// \returns Nothing; requires the destination area's overlay to be loaded.
/// \note Original decompilation identifier: \c func_8010E60C (0x8010E60C).
void areaActivateDestination(void) {
    gRuntimeCurrentArea = gRuntimeNextArea;
    gRuntimeCurrentEntrance = gRuntimeNextEntrance;
    gCurrentAreaConfiguration = (SceneConfigurationPrefix *)gAreaConfigurations[gRuntimeCurrentArea];
    TRACE("initArea(%p) curAreaNum:%d\n", ((u8 *)gCurrentAreaConfiguration), gRuntimeCurrentArea);
}

#include "overlay_loader.h"

/// Load and initialize the selected destination area's overlay.
/// \returns The start of available memory following the loaded overlay, aligned to sixteen bytes.
///
/// \note Requires a selected destination from zero through 22 and available destination memory. Loading
/// replaces the previous scene overlay; its references must no longer be in use.
/// \note Original decompilation identifier: \c func_8010E650 (0x8010E650).
void *areaLoadNextOverlay(void) {
    OverlayRec *ovl = &gAreaOverlayDescriptors[gRuntimeNextArea];

    return overlayLoadFromRom(ovl->romStart, ovl->romEnd, ovl->vramDst, ovl->textStart, ovl->textEnd, ovl->dataStart,
                              ovl->dataEnd, ovl->bssStart, ovl->bssEnd);
}


/// Spawn listed maze items whose saved availability permits them to appear.
/// \param spawn Readable placement list terminated by item kind zero; preceding kinds must be valid catalog
/// entries.
/// \returns Nothing.
///
/// \note Requires initialized item and progress systems. Portability: the empty debug type check preserves
/// IDO register allocation; its historical comparison constant is unrecoverable.
/// \note Original decompilation identifier: \c func_8010E6C4 (0x8010E6C4).
void mazeSpawnAvailableItems(ItemSpawn *spawn) {
    s32 i;
    ItemSpawn *entry;

    for (i = 0;; i++) {
        entry = &spawn[i];
        if (entry->type == 0) {
            break;
        }
        if (progressGetValue(gItemKinds[entry->type].availabilityField)) {
            itemSetMode(itemCreate(entry->x, entry->y, entry->z, entry->yaw, entry->pitch, entry->type), ITEM_MODE_STATIONARY);
        } else {
            TRACE("ERROR : not keeping item, %s\n", (char *)&gItemKinds[entry->type]);
        }
        if (entry->type == 100) {
        }
    }
}

/// Scatter weighted item choices across a random selection of candidate positions, optionally adding contact
/// effects at unused positions.
/// \param positions Readable array of count consecutive world positions in X, Y, Z order.
/// \param count Nonnegative number of candidate positions; array size and requested quota must fit in a
/// signed integer.
/// \param table Initialized weighted-choice table containing valid item kinds.
/// \param fraction Fraction of candidate positions to fill, from zero through one; the requested count is
/// rounded down.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010E768 (0x8010E768).
void mazeScatterItems(f32 *positions, s32 count, WeightedChoiceTable *table, f32 fraction) {
    s32 quota;
    s32 index;
    s32 remaining;
    f32 *position;
    quota = (s32)((f32)count * fraction);
    runtimeCheckResetCheckpoint("pre-mazeScatterItem", 1);
    index = 0;
    if (count > 0) {
        remaining = count;
        do {
            position = (f32 *)((u8 *)positions + index * 12);
            if ((randomNext() * 2.3283064365386963e-10) < (f64)((f32)quota / (f32)remaining)) {
                quota--;
                itemSetMode(itemCreate(position[0], position[1], position[2], 0.0f, 0.0f, weightedChoiceSelectKey(table)),
                              ITEM_MODE_STATIONARY);
            } else {
                if ((randomNext() * 2.3283064365386963e-10) > 0.5) {
                    position = (f32 *)((u8 *)positions + index * 12);
                    contactEffectCreate(position[0], position[1], position[2]);
                }
            }
            index++;
            remaining--;
        } while (index != count);
    }
    objectPrepareDistributedKindSearches(0x86, 0xA);
    runtimeCheckResetCheckpoint("aft-mazeScatterItem", 1);
}

/// Look up the display name of an area.
/// \param area Area identifier from zero through 22.
/// \returns The persistent area display-name string.
/// \note Original decompilation identifier: \c func_8010E958 (0x8010E958).
char *areaGetDisplayName(s32 area) {
    return gAreaDisplayNames[area];
}

/// Look up the short region name associated with an area.
/// \param area Area identifier from zero through 22.
/// \returns The persistent region-name string, or null when the area has no short region name.
/// \note Original decompilation identifier: \c func_8010E96C (0x8010E96C).
char *areaGetRegionName(s32 area) {
    return gAreaRegionNames[area];
}
