/// \file
/// Declares drawing of the system menu and its surrounding scene.

#ifndef SYSTEM_MENU_H
#define SYSTEM_MENU_H

#include <ultra64.h>

/// Draw the system menu and its surrounding scene for the current menu phase.
/// \param displayList Writable display-list cursor with sufficient command capacity; advances past appended commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119FC4 (0x80119FC4).
void systemMenuDrawFrame(Gfx **displayList);

#endif
