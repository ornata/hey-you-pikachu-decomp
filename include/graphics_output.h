/// \file
/// Declares video-mode configuration and framebuffer diagnostic text drawing.

#ifndef GRAPHICS_OUTPUT_H
#define GRAPHICS_OUTPUT_H

#include <ultra64.h>

/// Draw diagnostic text into both framebuffers.
/// \param text Null-terminated text; letters, digits, and colons are displayed, with other characters left blank. Each character occupies eight by ten pixels.
/// \param x Left screen coordinate in pixels.
/// \param y Top screen coordinate in pixels; the entire text rectangle must fit both initialized 16-bit framebuffers.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003CA98 (0x8003CA98).
void debugTextDrawFramebuffers(u8 *text, s32 x, s32 y);

/// Configure the video mode for a framebuffer and blank the display until rendering begins.
/// \param mode Base video mode to configure.
/// \param width Positive framebuffer width in pixels; video-mode integer calculations must remain representable.
/// \param height Positive framebuffer height in pixels.
/// \param depth Supported framebuffer color depth, 16 or 32 bits per pixel.
/// \returns Nothing.
///
/// \note NONPORTABLE: comRegs.width and xScale are written through their own symbols (D_800F29B8,
/// D_800F29D0), aliases of D_800F29B0's fields
/// \note Original decompilation identifier: \c func_8003C89C (0x8003C89C).
void videoConfigureMode(OSViMode mode, s32 width, s32 height, s32 depth);

#endif
