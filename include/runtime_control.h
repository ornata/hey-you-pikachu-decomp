/// \file
/// Declares game-mode transition requests and cooperative system-reset checkpoints.

#ifndef RUNTIME_CONTROL_H
#define RUNTIME_CONTROL_H

#include "ultra64.h"

/// Request a game-mode transition and account for elapsed play time.
/// \param mode Valid game-mode identifier to enter.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000B2C8 (0x8000B2C8).
void runtimeRequestGameModeChange(s32 mode);

/// Honor a pending system reset when runtime execution permits shutdown.
/// \param label Diagnostic checkpoint label; unused in this build.
/// \param unused Unused diagnostic argument retained by the interface.
/// \returns Nothing; halts if a pending reset can be honored.
/// \note Original decompilation identifier: \c func_8000BB48 (0x8000BB48).
void runtimeCheckResetCheckpoint(const char *label, s32 unused);

/// Request a runtime program transition and reset transient input and recognition state.
/// \param program Valid identifier in the runtime program table.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000B1D8 (0x8000B1D8).
void runtimeRequestProgramChange(s32 program);

#endif
