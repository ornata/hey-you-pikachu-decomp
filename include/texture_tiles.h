/// \file
/// Declares texture tile uploads and RGBA16 rectangle copies.

#ifndef TEXTURE_TILES_H
#define TEXTURE_TILES_H

#include <ultra64.h>

/// Copy a region of an RGBA16 image to the screen.
/// \param displayList Display-list cursor with sufficient capacity; receives drawing commands and advances past them.
/// \param image Borrowed RDP-readable RGBA16 image with padded rows and a readable requested region; retained until rendering finishes.
/// \param x Destination screen X coordinate in pixels.
/// \param y Destination screen Y coordinate in pixels.
/// \param imageWidth Positive source width in texels; at least two padded rows must fit texture memory unless height is one.
/// \param drawWidth Positive destination rectangle width in pixels.
/// \param height Positive number of readable source rows to draw.
/// \param sourceX Source X offset in texels.
/// \param sourceY Source Y offset in texels.
/// \param wrapS Texture addressing mode along S.
/// \param maskWidth Nonzero to enable width-based horizontal addressing.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800473B4 (0x800473B4).
void textureCopyRgba16Region(Gfx **displayList, s32 image, s32 x, s32 y, u32 imageWidth, s32 drawWidth, u32 height,
                             s32 sourceX, s32 sourceY, s32 wrapS, s32 maskWidth);

/// Make an image and any required palette ready for textured drawing.
/// \param displayList Display-list cursor with sufficient capacity; receives the texture setup commands and advances past them.
/// \param image RDP-readable texture data with padded rows; borrowed until these commands finish rendering.
/// \param palette RDP-readable palette for indexed images: 16 entries for CI4 or 256 for CI8; borrowed until rendering finishes.
/// \param format N64 image format.
/// \param size N64 texel-size identifier from zero through three.
/// \param width Positive image width in texels; the padded image must fit available texture memory.
/// \param height Positive image height in texels; the full image must fit available texture memory.
/// \param wrapS Texture addressing mode along the S axis.
/// \param wrapT Texture addressing mode along the T axis.
/// \param useMaskS Nonzero to enable a width-based S addressing mask.
/// \param useMaskT Nonzero to enable a height-based T addressing mask.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80042840 (0x80042840).
void textureUploadTile(Gfx **displayList, s32 image, s32 palette, s32 format, s32 size, u32 width, u32 height,
                       s32 wrapS, s32 wrapT, s32 useMaskS, s32 useMaskT);

#endif
