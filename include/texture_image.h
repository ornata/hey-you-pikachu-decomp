/// \file
/// Defines borrowed image storage and dimensions used by texture image rendering.

#ifndef TEXTURE_IMAGE_H
#define TEXTURE_IMAGE_H

#include "ultra64.h"

/// Caller-owned rendering descriptor borrowing its image and palette data.
/// Referenced data must remain RDP-readable until submitted rendering has finished.
typedef struct TextureImage {
    s32 image;        ///< Target address of borrowed image data.
    s32 palette;      ///< Target address of borrowed palette data, or zero for direct-color images.
    s32 format;       ///< N64 image-format selector.
    s32 size;         ///< N64 texel-size selector from zero through three.
    u32 width;        ///< Visible image width in texels.
    u32 height;       ///< Visible image height in texels.
    u32 alignedWidth; ///< Padded source row width in texels.
} TextureImage;

typedef char TextureImageSizeCheck[(sizeof(TextureImage) == 0x1C) ? 1 : -1];
typedef char TextureImageStrideOffsetCheck[((u32) & ((TextureImage *)0)->alignedWidth == 0x18) ? 1 : -1];

/// Initialize an image descriptor for texture rendering.
/// \param descriptor Caller-owned texture image descriptor; any previous borrowed image references are replaced.
/// \param image Borrowed texture data with rows padded for the selected texel size; must remain valid while used for rendering.
/// \param palette Borrowed palette for indexed images; must remain valid while used for rendering.
/// \param format N64 image format.
/// \param size N64 texel-size identifier from zero through three.
/// \param width Positive image width in texels whose padded width fits in signed 32 bits.
/// \param height Positive image height in texels.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80043000 (0x80043000).
void textureImageInitialize(TextureImage *descriptor, s32 image, s32 palette, s32 format, s32 size, s32 width,
                            s32 height);

/// Render a scaled texture with optional repetition and flipping.
/// \param out Display-list cursor with sufficient capacity; receives the drawing commands and advances past them.
/// \param texture Borrowed, initialized texture image descriptor.
/// \param x Destination screen X coordinate in pixels.
/// \param y Destination screen Y coordinate in pixels.
/// \param wrapS Texture addressing mode along the S axis.
/// \param wrapT Texture addressing mode along the T axis.
/// \param repeatX Horizontal repetition count with width-based wrapping; zero draws once without the wrapping mask.
/// \param repeatY Vertical repetition count with height-based wrapping; zero draws once without the wrapping mask.
/// \param scaleX Positive finite horizontal scale; the texture step and resulting coordinates must be representable.
/// \param scaleY Positive finite vertical scale; the texture step and resulting coordinates must be representable.
/// \param flipX Nonzero to flip the texture horizontally.
/// \param flipY Nonzero to flip the texture vertically.
/// \returns Nothing.
/// \pre Texture and palette data must remain RDP-readable until rendering finishes; source row padding must match the descriptor.
/// \pre Width and height must be positive. At least two source rows must fit texture memory unless the image has only one row.
/// \pre Four- and eight-bit images must use CI, I, or IA format; wrapped dimensions must be powers of two for whole-image repetition.
/// \note Original decompilation identifier: \c func_800430C4 (0x800430C4).
void textureDrawScaledRepeated(Gfx **out, TextureImage *texture, u32 x, u32 y, s32 wrapS, s32 wrapT, u32 repeatX,
                               u32 repeatY, f32 scaleX, f32 scaleY, s32 flipX, s32 flipY);

#endif
