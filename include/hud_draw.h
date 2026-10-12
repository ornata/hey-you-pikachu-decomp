/// \file
/// Declares drawing of microphone and voice-recognition status.

#ifndef HUD_DRAW_H
#define HUD_DRAW_H

#include <ultra64.h>

/// Update and draw the enabled microphone and voice-recognition status indicators.
/// \param gdl Writable display-list cursor with sufficient command capacity; advances past appended commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022110 (0x80022110).
void hudDrawMicrophoneIndicators(Gfx **gdl);

#endif
