/// \file
/// Declares drawing of the active scene backdrop.

#ifndef BACKDROP_DRAW_H
#define BACKDROP_DRAW_H

#include <ultra64.h>

/// Draw the active backdrop using its selected display mode.
/// \param list Writable display-list cursor with sufficient command capacity; advances past appended commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80033B78 (0x80033B78).
void backdropDraw(Gfx **list);

#endif
