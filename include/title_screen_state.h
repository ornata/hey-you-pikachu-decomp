/// \file
/// Declares querying title state and requesting its save-file menu.

#ifndef TITLE_SCREEN_STATE_H
#define TITLE_SCREEN_STATE_H

#include <ultra64.h>

/// Query the current startup and title-screen state.
/// \returns The current title-screen state identifier.
/// \note Original decompilation identifier: \c func_800351C0 (0x800351C0).
s32 titleScreenGetState(void);

/// Request the main menu when the title screen is awaiting Start.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800351FC (0x800351FC).
void titleScreenRequestMenu(void);

#endif
