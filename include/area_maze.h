/// \file
/// Declares ownership of the current area's spatial hierarchy.

#ifndef AREA_MAZE_H
#define AREA_MAZE_H

#include "maze_types.h"

/// Owned root of the loaded area's octree, or NULL after maze destruction.
/// \note Original identifier: \c D_80133DE0 (0x80133DE0).
extern MazeNode *gAreaMazeRoot;

#endif
