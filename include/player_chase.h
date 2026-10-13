/// \file
/// Declares current chase-stage selection and saved furthest-stage progress.

#ifndef PLAYER_CHASE_H
#define PLAYER_CHASE_H

#include "ultra64.h"

/// Select the current chase stage and retain the furthest stage reached in saved progress.
/// \param stage Requested chase stage; its low eight bits become the current stage, while saved furthest
/// progress uses the full value.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_801127DC (0x801127DC).
void playerSetChaseStage(s32 stage);

/// Read the currently selected chase stage.
/// \returns The current chase stage.
/// \note Original decompilation identifier: \c func_80112824 (0x80112824).
u8 playerGetChaseStage(void);

#endif
