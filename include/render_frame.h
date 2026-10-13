/// \file
/// Declares frame rendering setup, submission and elapsed-time camera tint.

#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

#include <ultra64.h>

/// Fill a rectangle in the current framebuffer with a solid color.
/// \param left Left pixel coordinate; empty or reversed rectangles are ignored.
/// \param top Top pixel coordinate.
/// \param right Inclusive right pixel coordinate, limited to 319.
/// \param bottom Inclusive bottom pixel coordinate, limited to 239.
/// \param r Red intensity from zero through 255.
/// \param g Green intensity from zero through 255.
/// \param b Blue intensity from zero through 255.
/// \param a One-bit alpha value, zero or one.
/// \returns Nothing.
/// \note Original address: 0x80000B38.
void renderFillFramebufferRectangle(u32 left, u32 top, u32 right, u32 bottom, u8 r, u8 g, u8 b, u8 a);

/// Begin a frame with the current background and camera, then invoke the active runtime drawing callback.
/// \returns Nothing; requires initialized rendering resources and a valid active runtime mode.
/// \note Original address: 0x80002058.
void renderBeginFrame(void);

/// Draw the frame’s right and bottom border and submit the finishing graphics task.
/// \returns Nothing; requires an active display list and initialized rendering resources.
/// \note Original address: 0x80002384.
void renderSubmitFrameBorder(void);

/// Load shared rendering assets and initialize camera and display-list resources.
/// \returns Nothing; requires an initialized local heap and must run before rendering resources are used.
/// \note Original address: 0x80000518.
void renderInitializeResources(void);

/// Enable the camera tint that grows with the primary actor’s elapsed script time.
/// \param initialElapsed Initial elapsed time in seconds used when the effect first starts; negative values are ignored.
/// \returns Nothing.
/// \note Original address: 0x80001430.
void cameraEnableElapsedTimeTint(f32 initialElapsed);

#endif
