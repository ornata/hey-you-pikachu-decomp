/// \file
/// Defines the actor interpreter's shared value bank and its reset/assignment interface.

#ifndef SCRIPT_VM_GLOBALS_H
#define SCRIPT_VM_GLOBALS_H

#include "script_vm.h"

/// Number of shared packed values available to actor programs.
enum { SCRIPT_VM_GLOBAL_VALUE_COUNT = 500 };

/// Shared values addressed by scripts independently of their local frames.
/// Profile initialization and scriptVmResetGlobals restore the zero state.
/// Callers provide valid indices; referenced string storage retains its own lifetime.
/// \note Original identifier: \c D_800EF7A0 (0x800EF7A0).
extern ScriptValue gScriptVmGlobalValues[SCRIPT_VM_GLOBAL_VALUE_COUNT];

typedef char ScriptVmGlobalBankSizeCheck[(sizeof(gScriptVmGlobalValues) == 0x9C4) ? 1 : -1];

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
