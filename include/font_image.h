/// \file
/// Declares text measurement and horizontal ink-boundary queries for custom fonts.

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

/// Measure encoded text using a supplied custom font.
/// \param text Borrowed null-terminated text with complete supported characters
/// and optional position/color controls.
/// \param font Borrowed descriptor with valid metrics for every supplied character.
/// \param mode Spacing mode, with one selecting proportional spacing.
/// \returns Final horizontal position in pixels after processing the text.
/// \note Original identifier: \c func_8003F6DC (0x8003F6DC).
s32 textMeasureWithFont(u8 *text, FontImage *font, s32 mode);

#endif
