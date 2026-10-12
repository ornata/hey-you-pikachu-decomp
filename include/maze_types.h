/// \file
/// Defines area hierarchy nodes, terrain samples and configured maze-cell resources.
#ifndef MAZE_TYPES_H
#define MAZE_TYPES_H
#include "object_types.h"

/// Borrowed terrain samples associated with a terminal maze region.
typedef struct MazeGrid {
    f32 *cells;       ///< Height samples, or triangle records in mode zero.
    void *attributes; ///< Borrowed surface-attribute samples.
    s16 w;            ///< Number of columns in the sample grid.
    s16 h;            ///< Number of rows in the sample grid.
    s32 mode;         ///< Sampling representation; zero selects triangles and two selects vertex samples.
} MazeGrid;

/// Owned octree region with borrowed terrain resources and associated scene objects.
/// Descendants and associated objects are released with the area hierarchy.
typedef struct MazeNode {
    ObjectReference objects[2];     ///< Primary region object and configured model hierarchy.
    f32 x, y, z;                    ///< World-space minimum corner of the region.
    f32 w, h, d;                    ///< Region extents along X, Y and Z.
    struct MazeNode *children[8];   ///< Owned octree descendants, or null for unused slots.
    s16 depth;                      ///< Remaining subdivision depth.
    s16 active;                     ///< Nonzero while associated objects are active.
    MazeGrid *terrain;              ///< Borrowed terrain surface, or null for an empty region.
    s32 (*event)(f32 *, u8 *, s32); ///< Optional position-event callback returning its response status.
    struct MazeNode *linkedNode;    ///< Borrowed region associated by a configured event link.
} MazeNode;

/// Borrowed resources and lifecycle callbacks for one occupancy-sampler result.
typedef struct SceneCellType {
    MazeGrid *terrain;        ///< Terrain surface assigned to the terminal region.
    void (*initialize)(u8 *); ///< Optional initializer receiving the new region address.
    void (*leave)(u8 *);      ///< Optional cleanup receiving the region before release.
    u8 *objectTemplate;       ///< Borrowed hierarchy template for the region's model.
    s32 unknown010;           ///< Offset 0x10; purpose remains unresolved.
} SceneCellType;

/// One configured item placement; a negative kind ends the placement list.
typedef struct SceneItemPlacement {
    f32 position[3]; ///< Item world position.
    s32 kind;        ///< Item kind, or a negative list terminator.
} SceneItemPlacement;

typedef char MazeGridSizeCheck[(sizeof(MazeGrid) == 0x10) ? 1 : -1];
typedef char MazeNodeSizeCheck[(sizeof(MazeNode) == 0x50) ? 1 : -1];
typedef char SceneCellTypeSizeCheck[(sizeof(SceneCellType) == 0x14) ? 1 : -1];
typedef char SceneItemPlacementSizeCheck[(sizeof(SceneItemPlacement) == 0x10) ? 1 : -1];
typedef char MazeChildrenOffsetCheck[(((u32) & ((MazeNode *)0)->children) == 0x20) ? 1 : -1];
typedef char MazeTerrainOffsetCheck[(((u32) & ((MazeNode *)0)->terrain) == 0x44) ? 1 : -1];
typedef char MazeEventOffsetCheck[(((u32) & ((MazeNode *)0)->event) == 0x48) ? 1 : -1];
typedef char MazeMinimumOffsetCheck[(((u32) & ((MazeNode *)0)->x) == 8) ? 1 : -1];
typedef char MazeExtentOffsetCheck[(((u32) & ((MazeNode *)0)->w) == 0x14) ? 1 : -1];
typedef char MazeDepthOffsetCheck[(((u32) & ((MazeNode *)0)->depth) == 0x40) ? 1 : -1];
typedef char MazeLinkedNodeOffsetCheck[(((u32) & ((MazeNode *)0)->linkedNode) == 0x4C) ? 1 : -1];
#endif
