/// \file
/// Defines text bitmap images and the handles shared by their builders and renderers.

#ifndef TEXT_IMAGE_H
#define TEXT_IMAGE_H

#include <ultra64.h>

/// Bitmap descriptor shared by text builders and image renderers.
/// Generated text images own their descriptor and pixel allocation.
typedef struct TextImage {
    u8 *pixels; ///< Pixel storage referenced by the bitmap.
    s32 width;  ///< Bitmap width in pixels.
    s32 height; ///< Bitmap height in pixels.
    s32 format; ///< Texture encoding selected by the image renderer.
} TextImage;

/// Queue-slot views of the original target-width address and owned image.
typedef union TextImageHandle {
    TextImage *image;
    s32 address;
} TextImageHandle;

typedef char TextImageSizeCheck[(sizeof(TextImage) == 0x10) ? 1 : -1];
typedef char TextImageHandleSizeCheck[(sizeof(TextImageHandle) == 4) ? 1 : -1];

#endif
