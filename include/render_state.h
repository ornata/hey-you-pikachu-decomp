/// \file
/// Declares the active rendering command stream and double-buffer selection.

#ifndef RENDER_STATE_H
#define RENDER_STATE_H

#include <ultra64.h>

/// Writable cursor advanced by drawing commands for the active frame.
/// Command storage remains live until its submitted graphics task completes.
/// \note This symbol also marks the end of camera storage in the target layout.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;

/// Start of the current command-list submission, advanced after submission.
/// \note Original identifier: \c D_800ABDD4 (0x800ABDD4).
extern Gfx *gRenderDisplayListStart;

/// Current framebuffer and command-buffer slot, zero or one.
/// Frame-presentation submission selects the other slot for subsequent drawing.
/// \note Original identifier: \c D_800F298C (0x800F298C).
extern s32 gGraphicsFramebufferIndex;

#endif
