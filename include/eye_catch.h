/// \file
/// Declares eye-catch scene initialization, drawing and scenario selection.

#ifndef EYE_CATCH_H
#define EYE_CATCH_H

#include <ultra64.h>

/// Choose an eye-catch group using the current selection weights.
/// \returns The selected group index from zero through five.
///
/// \note NONPORTABLE: preserve the observed 0x28-byte frame; the two upper words have no identified
/// value and are never read or written by the ROM.
/// \note Original decompilation identifier: \c func_800379A0 (0x800379A0).
s32 eyeCatchChooseGroup(void);

/// Prepare eye-catch layouts and reset interlude state once at startup.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80036A60 (0x80036A60).
void eyeCatchInitialize(void);

/// Draw the active eye-catch interlude during its visible stages.
/// \param displayList Display-list cursor receiving drawing commands and advanced past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800378A8 (0x800378A8).
void eyeCatchDraw(Gfx **displayList);

#endif
