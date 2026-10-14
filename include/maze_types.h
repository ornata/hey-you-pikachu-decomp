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

/// Handle a position event in a maze region.
/// \param position Borrowed world-space X, Y, Z coordinates, valid for the call.
/// \param node Borrowed maze region receiving the event, valid for the call.
/// \param eventType Event selector supplied by the event source.
/// \returns The handler's response status.
typedef s32 (*MazePositionEvent)(f32 *position, u8 *node, s32 eventType);

/// One borrowed area event-link definition; a null event terminates the list.
/// Source cells identify regions in the configured maze. A negative target-cell component disables
/// the optional destination link. Storage remains live through maze event-link initialization.
/// Installed handlers must remain callable until their maze regions are released.
typedef struct SceneEventLink {
    s32 sourceCell[3];       ///< Source cell indices in X, Y, Z order.
    MazePositionEvent event; ///< Handler installed on the source region, or null to end the list.
    s32 targetCell[3];       ///< Optional destination cell indices in X, Y, Z order.
} SceneEventLink;

typedef char SceneEventLinkSizeCheck[(sizeof(SceneEventLink) == 0x1C) ? 1 : -1];
typedef char SceneEventLinkHandlerOffsetCheck[((u32) & ((SceneEventLink *)0)->event == 0xC) ? 1 : -1];
typedef char SceneEventLinkTargetOffsetCheck[((u32) & ((SceneEventLink *)0)->targetCell == 0x10) ? 1 : -1];

/// Owned octree region with borrowed terrain resources and associated scene objects.
/// Descendants and associated objects are released with the area hierarchy.
typedef struct MazeNode {
    ObjectReference objects[2];   ///< Primary region object and configured model hierarchy.
    f32 x, y, z;                  ///< World-space minimum corner of the region.
    f32 w, h, d;                  ///< Region extents along X, Y and Z.
    struct MazeNode *children[8]; ///< Owned octree descendants, or null for unused slots.
    s16 depth;                    ///< Remaining subdivision depth.
    s16 active;                   ///< Nonzero while associated objects are active.
    MazeGrid *terrain;            ///< Borrowed terrain surface, or null for an empty region.
    MazePositionEvent event;      ///< Optional position-event callback returning its response status.
    struct MazeNode *linkedNode;  ///< Borrowed region associated by a configured event link.
} MazeNode;

/// Initialize or release resources associated with a terminal maze region.
/// \param node Borrowed live region address; initialization sees established bounds and terrain,
/// while cleanup runs before the region and its associated objects are released.
/// \returns Nothing.
/// \note The callback and its scene resources must remain loaded until the region is released.
typedef void (*SceneCellLifecycleHandler)(u8 *node);

/// Borrowed resources and lifecycle callbacks for one occupancy-sampler result.
typedef struct SceneCellType {
    MazeGrid *terrain;        ///< Terrain surface assigned to the terminal region.
    SceneCellLifecycleHandler initialize; ///< Optional initializer receiving the new region address.
    SceneCellLifecycleHandler leave; ///< Optional cleanup receiving the region before release.
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
typedef char SceneCellInitializeOffsetCheck[((u32) & ((SceneCellType *)0)->initialize == 4) ? 1 : -1];
typedef char SceneCellLeaveOffsetCheck[((u32) & ((SceneCellType *)0)->leave == 8) ? 1 : -1];
#endif
