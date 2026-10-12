/// \file
/// Declares the primary actor script clock query used by presentation code.

#ifndef ACTOR_SCRIPT_TIME_H
#define ACTOR_SCRIPT_TIME_H

#include "ultra64.h"

/// Query the primary actor script elapsed time.
/// \pre The primary actor and its script context must be initialized.
/// \returns Total elapsed time in seconds accumulated by the primary actor script context.
/// \note Original decompilation identifier: \c func_80014B3C (0x80014B3C).
f32 actorGetPrimaryScriptElapsedTime(void);

#endif
