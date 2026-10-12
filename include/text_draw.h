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

/// Initialize text drawing for a screen and establish its default text area.
/// \param width Screen width in pixels.
/// \param height Screen height in pixels.
/// \param cachedGlyphCount Positive wide-glyph cache capacity whose byte allocation fits a signed 32-bit value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003CD30 (0x8003CD30).
void textDrawInitialize(s32 width, s32 height, s32 cachedGlyphCount);

/// Begin appending text drawing commands to a display list using the current text color and default palette.
/// \param displayList Writable command storage with enough room for the requested text drawing, retained until another textDrawBegin call.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003CE24 (0x8003CE24).
void textDrawBegin(Gfx *displayList);

/// Restore the default text area, cursor, color, font, spacing, scale, and palette.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003CF78 (0x8003CF78).
void textDrawReset(void);

/// Set the color and opacity used for text drawing.
/// \param red Red component from zero to 255.
/// \param green Green component from zero to 255.
/// \param blue Blue component from zero to 255.
/// \param alpha Opacity from zero to 255.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D4A0 (0x8003D4A0).
void textDrawSetColor(s32 red, s32 green, s32 blue, s32 alpha);

/// Select the font used for single-byte text characters.
/// \param font Valid single-byte font-bank index for the characters to be drawn.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D58C (0x8003D58C).
void textDrawSetFont(s32 font);

/// Set the horizontal and vertical scale used for text drawing.
/// \param scaleX Finite positive horizontal text scale compatible with representable drawing coordinates and texture steps.
/// \param scaleY Finite positive vertical text scale whose sixteen-pixel row advance rounds to at least one pixel.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D598 (0x8003D598).
void textDrawSetScale(f32 scaleX, f32 scaleY);

/// Select proportional or fixed-cell text spacing.
/// \param mode One for proportional spacing; other values select fixed-cell spacing.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D5AC (0x8003D5AC).
void textDrawSetSpacingMode(s32 mode);

/// Retrieve the default text area's inclusive screen bounds.
/// \param left Receives the left pixel coordinate.
/// \param top Receives the top pixel coordinate.
/// \param right Receives the inclusive right pixel coordinate.
/// \param bottom Receives the inclusive bottom pixel coordinate.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003D5B8 (0x8003D5B8).
void textDrawGetDefaultArea(s32 *left, s32 *top, s32 *right, s32 *bottom);

/// Draw formatted text at the current cursor, supporting single-byte characters, wide characters, and newlines.
/// \param format Printf-style format string; the result including its terminator must fit in 512 bytes and contain complete supported characters.
/// \param ... Values used by the format string.
/// \returns Nothing.
///
/// \note Portability/UB: Variadic forwarding reads 512 bytes after the format argument, relying on the original IDO stack layout instead of a portable va_list.
/// \note Original decompilation identifier: \c func_8003D030 (0x8003D030).
void textDrawPrintf(const char *format, ...);

#endif
