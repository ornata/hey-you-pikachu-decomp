/// \file
/// Declares cached texture setup and display-list cursor advancement.

#ifndef TEXTURE_SETUP_H
#define TEXTURE_SETUP_H

#include "ultra64.h"

/// Prepare a table texture for rendering.
/// \param displayList Display-list cursor advanced by the texture setup commands.
/// \param table Texture-table index.
/// \param index Valid texture index within the selected table.
/// \returns The display-list cursor before texture setup.
/// \note Original decompilation identifier: \c func_8000C26C (0x8000C26C).
Gfx *textureLoadForDraw(Gfx **displayList, s32 table, s32 index);

#endif
