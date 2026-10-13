/// \file
/// Declares HUD drawing, feature selection and animation controls.

#ifndef HUD_H
#define HUD_H

#include <ultra64.h>

/// Keep the horizontal movement indicator visible for at least the requested remaining duration.
/// \param seconds Nonnegative minimum remaining horizontal-indicator display time in seconds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80023100 (0x80023100).
void hudExtendHorizontalMotionDisplay(f32 seconds);

/// Set whether the current area name is visible.
/// \param visible Nonzero to show the area label, or zero to hide it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022F68 (0x80022F68).
void hudSetAreaLabelVisible(s32 visible);

/// Provide the inactive HUD program-initialization hook for this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800215A0 (0x800215A0).
void hudProgramInitializeNoOp(void);

/// Provide the inactive per-player HUD update hook for this build.
/// \param player Player identifier supplied by the runtime; unused in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800215A8 (0x800215A8).
void hudPlayerUpdateNoOp(s32 player);

/// Select the HUD guide style, playing selection feedback when a different guide is enabled.
/// \param style Guide style identifier, from 1 through 4, or zero to disable the guide.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80021CDC (0x80021CDC).
void hudSetGuideStyle(s32 style);

/// Draw the current area name when its label is visible.
/// Requires a prepared area-label image when the label is shown.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022DBC (0x80022DBC).
void hudDrawAreaLabel(Gfx **gdl);

/// Select the alternate HUD overlay, disabling microphone input when enabled.
/// \param enabled Nonzero to select the alternate overlay and disable microphone input, or zero to restore normal HUD drawing without re-enabling microphone input.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022F74 (0x80022F74).
void hudSetOverlayMode(s32 enabled);

/// Set whether the auxiliary HUD icon is visible.
/// \param visible Nonzero to show the auxiliary icon, or zero to hide it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022FB8 (0x80022FB8).
void hudSetAuxiliaryIconVisible(s32 visible);

/// Select which movement and microphone HUD indicators are enabled.
/// \param features Feature mask: bit zero enables the selected action indicator, bit one enables movement indicators, and bit two enables microphone indicators.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022FC4 (0x80022FC4).
void hudSetFeatureMask(s32 features);

/// Draw the currently enabled HUD indicators and overlays.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022FD0 (0x80022FD0).
void hudDraw(Gfx **gdl);

#endif
