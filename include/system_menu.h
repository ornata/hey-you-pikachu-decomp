/// \file
/// Declares system-menu entry and drawing of its surrounding scene.

#ifndef SYSTEM_MENU_H
#define SYSTEM_MENU_H

#include <ultra64.h>

/// Draw the system menu and its surrounding scene for the current menu phase.
/// \param displayList Writable display-list cursor with sufficient command capacity; advances past appended commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119FC4 (0x80119FC4).
void systemMenuDrawFrame(Gfx **displayList);

/// Enter the system menu and select its root list.
/// \returns Nothing; changes runtime mode and configures menu controller input.
/// Requires initialized menu models/text, player, camera, area, audio and controller services.
/// \note Original decompilation identifier: \c func_8011A878 (0x8011A878).
void systemMenuEnter(void);

#endif
