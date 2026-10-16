/// \file
/// Declares backdrop image allocation, per-view selection and active horizontal scrolling.

#ifndef BACKDROP_VIEW_H
#define BACKDROP_VIEW_H

#include "ultra64.h"

/// Allocate a backdrop image buffer for a view.
/// \param viewIndex Valid view index receiving the buffer.
/// \param width Requested image width in pixels.
/// \param height Requested image height in pixels.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033784 (0x80033784).
void backdropAllocateViewBuffer(s32 viewIndex, u32 width, u32 height);

/// Load a backdrop for a view and select its display settings.
/// \param viewIndex Valid view index with an allocated backdrop buffer.
/// \param backdropId Retail descriptor index from one through 23; zero has no loadable source.
/// \param sharedScratch Nonzero for shared-workspace asset loading, or zero for ordinary loading.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033A70 (0x80033A70).
void backdropSelectForView(s32 viewIndex, s32 backdropId, s32 sharedScratch);

/// Advance the active backdrop horizontal scroll within the image.
/// Requires an active backdrop with a positive image width.
/// \param delta Horizontal scroll change in pixels.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033118 (0x80033118).
void backdropAdvanceScroll(f32 delta);

#endif
