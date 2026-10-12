/// \file
/// Declares creation and release of owned text bitmaps.

#ifndef TEXT_IMAGE_CREATE_H
#define TEXT_IMAGE_CREATE_H

#include "text_image.h"
#include "font_image_types.h"

/// Create a bitmap using the system font or a supplied custom font.
/// \param out Receives an owned image, or NULL for empty text; release images with textImageDestroy.
/// \param text Null-terminated encoded text with complete supported characters.
/// \param font Borrowed custom font, or NULL to use the system font.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003DDC0 (0x8003DDC0).
void textImageCreateFromText(TextImage **out, u8 *text, FontImage *font);

/// Release a generated text image and its pixels.
/// \param image Owned generated bitmap to release, or NULL.
/// \returns Nothing.
/// \pre Drawing commands that reference the image's pixels have finished.
/// \note Original decompilation identifier: \c func_8003F650 (0x8003F650).
void textImageDestroy(TextImage *image);

#endif
