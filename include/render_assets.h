/// \file
/// Declares owned RAM storage for shared and current-area rendering resources.

#ifndef RENDER_ASSETS_H
#define RENDER_ASSETS_H

#include <ultra64.h>

/// Owned shared rendering allocation, retained for the initialized renderer's lifetime.
/// \note Original identifier: \c D_800ABDDC (0x800ABDDC).
extern u8 *gSharedRenderAssets;

/// Owned current-area rendering allocation, or NULL after unloading.
/// \note Original identifier: \c D_800ABDE0 (0x800ABDE0).
extern u8 *gAreaRenderAssets;

#endif
