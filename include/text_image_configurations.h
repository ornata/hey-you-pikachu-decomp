/// \file
/// Declares borrowed font descriptors shared by text panels and message displays.

#ifndef TEXT_IMAGE_CONFIGURATIONS_H
#define TEXT_IMAGE_CONFIGURATIONS_H

#include "font_image_types.h"

/// Resident custom-font descriptors selected by supported text-style indices.
/// Entries and their glyph resources are borrowed while text is measured or rendered.
/// The complete table extent is not established by this declaration.
/// \note Original identifier: \c D_80074DE0 (0x80074DE0).
extern FontImage gTextImageConfigurations[];

#endif
