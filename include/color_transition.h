/// \file
/// Declares frame-timed transitions between four-component RGBA colors.

#ifndef COLOR_TRANSITION_H
#define COLOR_TRANSITION_H

#include "ultra64.h"

/// Advance an RGBA transition using the current frame duration.
/// \param color Four writable RGBA components to update.
/// \param target Four target RGBA components, each from zero to 255.
/// \param remaining Writable remaining transition time in seconds, updated toward zero.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800010C0 (0x800010C0).
void colorAdvanceTransition(u8 *color, u8 *target, f32 *remaining);

#endif
