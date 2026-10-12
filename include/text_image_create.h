/// \file
/// Declares creation of owned text bitmaps from encoded text and font resources.

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

#endif
