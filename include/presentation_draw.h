/// \file
/// Declares card and collection-result presentation drawing.

#ifndef PRESENTATION_DRAW_H
#define PRESENTATION_DRAW_H

#include <ultra64.h>

/// Draw the active actor and current card presentation when card data is available; requires initialized area and actor state.
/// \param gdl Display-list cursor to receive drawing commands and advance past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80028AD4 (0x80028AD4).
void presentationDrawCardScreen(Gfx **gdl);

/// Draws the collection presentation for its current stage.
/// \param gdl Display-list cursor to receive drawing commands and advance past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800294C4 (0x800294C4).
void presentationDrawCollectionScreen(Gfx **gdl);

#endif
