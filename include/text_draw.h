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

/// Retrieve the current text drawing command position.
/// \param cursor Receives the display-list position after commands emitted so far.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003CF68 (0x8003CF68).
void textDrawGetDisplayListCursor(Gfx **cursor);

/// Set the screen-bounded text area and reset the cursor to its upper-left corner.
/// \param x0 X coordinate of one corner.
/// \param y0 Y coordinate of one corner.
/// \param x1 X coordinate of the opposite corner, inclusive.
/// \param y1 Y coordinate of the opposite corner, inclusive.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D31C (0x8003D31C).
void textDrawSetArea(s32 x0, s32 y0, s32 x1, s32 y1);

/// Position the text cursor relative to the text area's origin, allowing room for the next glyph.
/// \param x Horizontal offset from the text area's left edge.
/// \param y Vertical offset from the text area's top edge.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D3E4 (0x8003D3E4).
void textDrawSetCursor(s32 x, s32 y);

/// Draw part of a text image in a screen rectangle with scaling and clipping.
/// \param gfx Display-list cursor to append to and advance.
/// \param image Text image to draw.
/// \param x0 Left screen coordinate of the destination rectangle.
/// \param y0 Top screen coordinate of the destination rectangle.
/// \param x1 Inclusive right screen coordinate of the destination rectangle.
/// \param y1 Inclusive bottom screen coordinate of the destination rectangle.
/// \param s Nonnegative horizontal starting coordinate in the source image.
/// \param t Nonnegative vertical starting coordinate in the source image.
/// \param scaleX Positive horizontal image scale.
/// \param scaleY Positive vertical image scale.
/// \returns Nothing.
/// \pre The drawable source width is at most 4096 pixels and scaled texture steps fit signed integers.
/// \note Original decompilation identifier: \c func_8003D764 (0x8003D764).
void textImageDrawRectangle(Gfx **gfx, TextImage *image, s32 x0, s32 y0, s32 x1, s32 y1, s32 s, s32 t, f32 scaleX,
                            f32 scaleY);

#endif
