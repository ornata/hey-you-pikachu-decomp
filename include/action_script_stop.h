/// \file
/// Declares action-script stopping and the borrowed completion-state reference.

#ifndef ACTION_SCRIPT_STOP_H
#define ACTION_SCRIPT_STOP_H

#include "ultra64.h"

/// Stop script command execution and request the current action to finish.
/// \param object Actor whose action script is stopped.
/// \returns A borrowed pointer to the actor's script identifier, now set to -1; valid while its state lives.
/// \note Original decompilation identifier: \c func_800120AC (0x800120AC).
s32 *actionScriptStop(u8 *object);

#endif
