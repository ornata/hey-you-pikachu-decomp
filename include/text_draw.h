/// \file
/// Declares text-image drawing, texture preparation and shared palette lifetimes.

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

/// Prepare a complete text image for drawing.
/// \param cursor Display-list cursor to append to and advance.
/// \param image Nonnull borrowed bitmap; pixels must remain valid until drawing commands finish.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003DD84 (0x8003DD84).
void textImageLoadTexture(Gfx **cursor, TextImage *image);

/// Prepare a rectangular portion of a text image for drawing.
/// \param dl Display-list cursor to append to and advance.
/// \param image Nonnull borrowed bitmap; pixels must remain valid until drawing commands finish.
/// \param x0 Horizontal coordinate of the first source corner.
/// \param y0 Vertical coordinate of the first source corner.
/// \param x1 Inclusive horizontal coordinate of the opposite source corner.
/// \param y1 Inclusive vertical coordinate of the opposite source corner.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003DA3C (0x8003DA3C).
void textImageLoadRectangle(Gfx **dl, TextImage *image, s32 x0, s32 y0, s32 x1, s32 y1);

#endif
