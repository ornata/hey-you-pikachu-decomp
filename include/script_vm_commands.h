/// \file
/// Defines native actor-script scheduling results and command dispatch contracts.

#ifndef SCRIPT_VM_COMMANDS_H
#define SCRIPT_VM_COMMANDS_H

#include <ultra64.h>

/// Scheduling requests returned by native commands to the bytecode interpreter.
/// These describe command completion; they are distinct from retained VM states.
typedef enum ScriptNativeCommandResult {
    SCRIPT_COMMAND_CONTINUE = 0,       ///< Continue bytecode during the current update.
    SCRIPT_COMMAND_YIELD = 1,          ///< Resume bytecode on a later update.
    SCRIPT_COMMAND_WAIT_FOR_ACTION = 2 ///< Advance the selected action handler before resuming bytecode.
} ScriptNativeCommandResult;

/// Invoke the next native command selected by an actor's bytecode.
/// \param actor Actor with a live context, a valid command-table selector and
/// arguments satisfying the selected command's contract.
/// \returns The command's scheduling request: zero continues, two waits for an
/// action, and any other nonzero result yields until a later update.
/// \note Original identifier: \c func_8011BD50 (0x8011BD50).
s32 actorScriptDispatchRegisteredCommand(u8 *actor);

#endif
