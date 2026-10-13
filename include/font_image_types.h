/// \file
/// Defines custom-font cell dimensions and borrowed glyph resources.

#ifndef FONT_IMAGE_TYPES_H
#define FONT_IMAGE_TYPES_H

#include <ultra64.h>

/// Glyph pixel offset and signed horizontal ink boundaries.
typedef struct FontGlyph {
    s32 offset;   ///< Byte offset from the font's cartridge pixel base.
    s8 left;      ///< Inclusive left ink boundary within the character cell.
    s8 right;     ///< Inclusive right ink boundary within the character cell.
    s8 unknown06; ///< Retained byte with no established consumer.
    s8 unknown07; ///< Retained byte with no established consumer.
} FontGlyph;

/// Borrowed custom-font descriptor, valid while its text is measured or rendered.
typedef struct FontImage {
    s32 width;         ///< Character-cell width in pixels.
    s32 height;        ///< Character-cell height in pixels.
    u8 *pixels;        ///< Cartridge base of packed glyph pixel data.
    FontGlyph *glyphs; ///< Borrowed metrics indexed by supported character codes.
} FontImage;

typedef char FontGlyphSizeCheck[(sizeof(FontGlyph) == 8) ? 1 : -1];
typedef char FontImageSizeCheck[(sizeof(FontImage) == 0x10) ? 1 : -1];
typedef char FontImageWidthOffsetCheck[(((u32) & (((FontImage *)0)->width)) == 0) ? 1 : -1];
typedef char FontImageHeightOffsetCheck[(((u32) & (((FontImage *)0)->height)) == 4) ? 1 : -1];
typedef char FontImagePixelsOffsetCheck[(((u32) & (((FontImage *)0)->pixels)) == 8) ? 1 : -1];
typedef char FontImageGlyphsOffsetCheck[(((u32) & (((FontImage *)0)->glyphs)) == 0xC) ? 1 : -1];

#endif
