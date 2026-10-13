/// \file
/// Declares camera overlay color transitions, reset controls and drawing.

#ifndef CAMERA_OVERLAY_H
#define CAMERA_OVERLAY_H

#include "ultra64.h"

/// Set the target color and duration for a camera overlay transition.
/// \param cameraIndex Valid camera slot to configure.
/// \param red Target red intensity, limited to zero through 255.
/// \param green Target green intensity, limited to zero through 255.
/// \param blue Target blue intensity, limited to zero through 255.
/// \param alpha Target opacity, limited to zero through 255.
/// \param duration Transition time in seconds; near-zero values request immediate completion on update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80001530 (0x80001530).
void cameraFadeOverlayToColor(s32 cameraIndex, s32 red, s32 green, s32 blue, s32 alpha, f32 duration);

/// Reset a camera overlay to a specified color and clear its additional blend contribution.
/// \param cameraIndex Valid camera slot to configure.
/// \param red Red intensity, limited to zero through 255.
/// \param green Green intensity, limited to zero through 255.
/// \param blue Blue intensity, limited to zero through 255.
/// \param alpha Opacity, limited to zero through 255.
/// \returns Nothing.
/// \note Pending transition targets and countdowns are retained.
/// \note Original decompilation identifier: \c func_800015D4 (0x800015D4).
void cameraResetOverlayColor(s32 cameraIndex, s32 red, s32 green, s32 blue, s32 alpha);

/// Set the target color and duration of camera slot zero’s additional overlay blend.
/// \param red Target red intensity, limited to zero through 255.
/// \param green Target green intensity, limited to zero through 255.
/// \param blue Target blue intensity, limited to zero through 255.
/// \param alpha Target opacity, limited to zero through 255.
/// \param duration Transition time in seconds; near-zero values request completion on the next applicable camera update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000159C (0x8000159C).
void cameraFadePrimaryOverlayBlend(s32 red, s32 green, s32 blue, s32 alpha, f32 duration);

/// Set the target color and duration of camera slot zero’s independent secondary overlay.
/// \param red Target red intensity, limited to zero through 255.
/// \param green Target green intensity, limited to zero through 255.
/// \param blue Target blue intensity, limited to zero through 255.
/// \param alpha Target opacity, limited to zero through 255.
/// \param duration Transition time in seconds; near-zero values request completion on the next applicable camera update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80001678 (0x80001678).
void cameraFadePrimarySecondaryOverlay(s32 red, s32 green, s32 blue, s32 alpha, f32 duration);

/// Advance camera overlay transitions and refresh the blended overlay color for the current frame.
/// \param cameraIndex Valid camera slot whose overlay colors are updated.
/// \param advanceBlend Nonzero also advances the additional blend transition; zero retains its current contribution.
/// \returns Nothing.
/// \note Original address: 0x8000114C.
void cameraUpdateOverlayColors(s32 cameraIndex, s32 advanceBlend);

/// Draw the camera’s blended color overlay when its opacity is nonzero.
/// \param index Valid camera slot whose overlay is drawn in the current drawing context.
/// \returns Nothing.
/// \note Original address: 0x80001218.
void cameraDrawOverlay(s32 index);

/// Draw the camera’s independent secondary color overlay when its opacity is nonzero.
/// \param index Valid camera slot whose secondary overlay is drawn in the current drawing context.
/// \returns Nothing.
/// \note Original address: 0x80001324.
void cameraDrawSecondaryOverlay(s32 index);

#endif
