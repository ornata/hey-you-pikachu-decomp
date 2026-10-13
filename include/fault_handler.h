/// \file
/// Declares startup of the game fault-reporting thread.

#ifndef FAULT_HANDLER_H
#define FAULT_HANDLER_H

#include <ultra64.h>

/// Start the fault-reporting thread.
/// \param threadId Identifier assigned to the fault-reporting thread.
/// \param priority Scheduling priority assigned to the fault-reporting thread.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800427C8 (0x800427C8).
void faultStartHandler(s32 threadId, s32 priority);

#endif
