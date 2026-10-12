/// \file
/// Declares drawing of startup screens and title menus.

#ifndef TITLE_SCREEN_H
#define TITLE_SCREEN_H

#include <ultra64.h>

/// Draw the current startup screen, title screen, or save-file menu.
/// \param cursor Writable display-list cursor with sufficient command capacity; advances past appended commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80034F6C (0x80034F6C).
void titleScreenDrawFrame(Gfx **cursor);

#endif
