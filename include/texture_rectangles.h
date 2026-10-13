/// \file
/// Declares cached-texture rectangle lists and their display-list output contract.

#ifndef TEXTURE_RECTANGLES_H
#define TEXTURE_RECTANGLES_H

#include "texture_draw_types.h"

/// Render a list of textured screen rectangles.
/// \param table Texture-table index used by the rectangle records.
/// \param draw Rectangle records terminated by a negative texture index; scales must be nonzero.
/// \param gdl Display-list cursor receiving the drawing commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000C354 (0x8000C354).
void textureDrawRectangles(s32 table, TextureDrawC354 *draw, Gfx **gdl);

#endif
