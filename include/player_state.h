/// \file
/// Defines the primary player's shared control-state storage and borrowed lifetime.

#ifndef PLAYER_STATE_H
#define PLAYER_STATE_H

#include "player_types.h"

/// Primary player record, borrowed by gameplay, scripts, menus and rendering.
/// The prefix describes established fields, not the size of the complete allocation.
/// Runtime initialization and state restoration may replace its contents.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern PlayerControlPrefix gPlayerState;

#endif
