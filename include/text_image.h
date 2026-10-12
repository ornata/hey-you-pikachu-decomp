/// \file
/// Defines text bitmap images and the handles shared by their builders and renderers.

#ifndef TEXT_IMAGE_H
#define TEXT_IMAGE_H

#include <ultra64.h>

/* Allocated by the text bitmap builders and consumed by their renderers. */
typedef struct TextImage {
    u8 *pixels;
    s32 width;
    s32 height;
    s32 format;
} TextImage;

/* A queue slot exposes the original integer handle and its owned image. */
typedef union TextImageHandle {
    TextImage *image;
    s32 address;
} TextImageHandle;

#endif
