/// \file
/// Declares horizontal ink-boundary queries for custom fonts.

#ifndef FONT_IMAGE_H
#define FONT_IMAGE_H

#include "font_image_types.h"

/// Get the left boundary used to draw a character in a supplied font.
/// \param code Encoded character to measure.
/// \param font Font containing character metrics.
/// \param spacingMode Spacing mode, with one selecting proportional spacing.
/// \returns Left boundary in pixels within the character cell.
/// \note Original decompilation identifier: \c func_8003F4FC (0x8003F4FC).
s32 fontImageGetCharacterLeft(s32 code, FontImage *font, s32 spacingMode);

/// Get the right boundary used to draw a character in a supplied font.
/// \param code Encoded character to measure.
/// \param font Borrowed font containing character dimensions and metrics.
/// \param spacingMode Spacing mode, with one selecting proportional spacing.
/// \returns Inclusive right boundary in pixels within the character cell.
/// \note Original decompilation identifier: \c func_8003F554 (0x8003F554).
s32 fontImageGetCharacterRight(s32 code, FontImage *font, s32 spacingMode);

#endif
