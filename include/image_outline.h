/// \file
/// Declares offset outlines for four-bit text bitmap images.

#ifndef IMAGE_OUTLINE_H
#define IMAGE_OUTLINE_H

#include <ultra64.h>
#include "text_image.h"

/// Add a colored outline at a requested offset in a four-bit image.
/// \param image Writable image with an even pixel width and byte-aligned rows; unsupported formats or absent pixel data are ignored.
/// \param dx Horizontal outline offset in pixels.
/// \param dy Vertical outline offset in pixels.
/// \param background Background pixel value from zero through fifteen that may receive the outline.
/// \param outline Outline pixel value from zero through fifteen.
/// \returns Nothing.
/// \note Portability/UB: Original clipping temporarily forms a pointer before a pixel row. Target access bounds do not establish portable C pointer validity.
/// \note Original decompilation identifier: \c func_80012F8C (0x80012F8C).
void image4BitAddOffsetOutline(TextImage *image, s32 dx, s32 dy, s32 background, s32 outline);

#endif
