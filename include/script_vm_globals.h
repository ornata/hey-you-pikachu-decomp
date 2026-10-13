/// \file
/// Resets and assigns the actor interpreter's shared script-variable bank.

#ifndef SCRIPT_VM_GLOBALS_H
#define SCRIPT_VM_GLOBALS_H

#include <ultra64.h>

/// Restore all 500 shared script variables to their initial zero values.
/// \returns Nothing.
/// \note Original identifier: \c func_80116708 (0x80116708).
void scriptVmResetGlobals(void);

/// Assign an integer value to a shared script variable.
/// \param index Variable index from zero through 499.
/// \param value Integer value to assign.
/// \returns Nothing.
/// \note Original identifier: \c func_80116254 (0x80116254).
void scriptVmSetGlobalInteger(s32 index, s32 value);

#endif
