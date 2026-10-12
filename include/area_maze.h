/// \file
/// Declares ownership of the current area's spatial hierarchy.

#ifndef AREA_MAZE_H
#define AREA_MAZE_H

#include "maze_types.h"

/// Owned root of the loaded area's octree, or NULL after maze destruction.
/// \note Original identifier: \c D_80133DE0 (0x80133DE0).
extern MazeNode *gAreaMazeRoot;

/// Create and register a block object in the maze region selected by a world position.
/// \param position World position in X, Y, Z order selecting a region with no existing block object to
/// replace.
/// \param objectTemplate Address of a valid object hierarchy template for the new block.
/// \returns Nothing.
///
/// \pre The maze is initialized.
/// \note Original decompilation identifier: \c func_8010DB68 (0x8010DB68).
void mazePlaceBlockObject(u8 *position, u8 *objectTemplate);

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
void mazeReplaceBlock(u8 *pos, u8 *modelTemplate, MazeGrid *terrain, s32 level);

#endif
