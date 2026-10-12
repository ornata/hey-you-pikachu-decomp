/// \file
/// Declares retained bitmap slots shared by text-window initialization and drawing.

#ifndef TEXT_WINDOW_IMAGES_H
#define TEXT_WINDOW_IMAGES_H

#include "text_image.h"

/// Owned double-buffered page images, with the original extended cleanup view.
/// Only the first five row pairs are page images. Cleanup visits a sixth pair
/// whose first slot aliases gTextWindowPromptImage at 0x800F10A8; the purpose of
/// the final slot at 0x800F10AC remains unresolved.
/// \note Original identifier: \c D_800F1080 (0x800F1080).
extern TextImage *gTextWindowPageImages[6][2];

/// Owned prompt bitmap, also reached through the extended page cleanup view.
/// \note Original identifier: \c D_800F10A8 (0x800F10A8).
extern TextImage *gTextWindowPromptImage;

/// Selected page-image buffer, zero or one.
/// \note Original identifier: \c D_800F1ABC (0x800F1ABC).
extern s32 gTextWindowPageImageBuffer;

#endif
