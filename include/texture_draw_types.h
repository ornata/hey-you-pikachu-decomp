/// \file
/// Defines screen rectangles consumed by the cached-texture drawing interface.

#ifndef TEXTURE_DRAW_TYPES_H
#define TEXTURE_DRAW_TYPES_H

#include <ultra64.h>

/// Textured screen rectangle; a negative texture index terminates a list.
typedef struct TextureRectangle {
    s32 texture;        ///< Index into the selected texture table.
    s32 x, y;           ///< Destination origin in screen pixels.
    s32 width, height;  ///< Destination dimensions in pixels; negative values use source dimensions.
    s32 s, t;           ///< Texture-coordinate origin.
    f32 scaleX, scaleY; ///< Nonzero horizontal and vertical texture scale.
} TextureRectangle;

typedef char TextureDrawSizeCheck[(sizeof(TextureRectangle) == 0x24) ? 1 : -1];
typedef char TextureDrawScaleOffsetCheck[((u32) & ((TextureRectangle *)0)->scaleX == 0x1C) ? 1 : -1];

#endif
