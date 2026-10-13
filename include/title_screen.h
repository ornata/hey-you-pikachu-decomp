/// \file
/// Declares startup-screen drawing and title demonstration transitions.

#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include <ultra64.h>

/// Draw the current startup screen, title screen, or save-file menu.
/// \param cursor Writable display-list cursor with sufficient command capacity; advances past appended commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80034F6C (0x80034F6C).
void titleScreenDrawFrame(Gfx **cursor);

/// Begin the next demonstration transition when the title screen awaits Start.
/// \returns Nothing; requests outside the waiting title state have no effect.
/// \note Original decompilation identifier: \c func_80035234 (0x80035234).
void titleScreenBeginDemoTransition(void);

/// Query the selected title demonstration.
/// \returns Demonstration index from zero through three.
/// \note Original decompilation identifier: \c func_800352AC (0x800352AC).
s32 titleScreenGetDemoIndex(void);

#endif
