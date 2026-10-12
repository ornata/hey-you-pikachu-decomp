/// \file
/// Declares shared indexed-text drawing state and palette lifetimes.

#ifndef TEXT_DRAW_H
#define TEXT_DRAW_H

#include <ultra64.h>
#include "text_image.h"

/// Shared sixteen-entry RGBA5551 palette for indexed text drawing.
/// \note Original identifier: \c D_80063800 (0x80063800).
extern const u16 gDefaultTextPalette[16];

/// Select the palette used for indexed text images.
/// \param palette Borrowed sixteen-entry RGBA5551 palette, valid until all drawing commands that use it finish.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D580 (0x8003D580).
void textDrawSetPalette(const u16 *palette);

/// Draw an image at the text cursor using the current text scale.
/// \param image Borrowed bitmap descriptor; its pixels must remain valid until drawing commands finish.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D5EC (0x8003D5EC).
void textDrawImageAtCursor(TextImage *image);

#endif
