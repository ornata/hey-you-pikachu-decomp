/// \file
/// Defines timed action commands and the per-actor action sequencer state.

#ifndef ACTION_SCRIPT_TYPES_H
#define ACTION_SCRIPT_TYPES_H

#include <ultra64.h>

/// Fixed-size command borrowed from an action-script resource.
/// Argument meanings depend on the opcode; this is separate from actor VM bytecode.
typedef struct ActionScriptCommand {
    s32 opcode; ///< Command handler selector; four terminates command execution.
    f32 time; ///< Earliest command time in seconds.
    s32 args[2]; ///< Opcode-specific operands, including action and sound identifiers.
} ActionScriptCommand;

/// Actor-owned sequencer state beginning at actor-state offset 0x158.
/// The command cursor borrows its resource and does not own command storage.
/// Stopping command execution does not imply that the last requested action ended.
typedef struct ActionScriptState {
    s32 index; ///< Selected script identifier, or minus one after an explicit stop.
    f32 elapsed; ///< Elapsed script time in seconds.
    f32 duration; ///< Time limit in seconds; a negative value disables the limit.
    f32 commandTime; ///< Command clock in seconds, held while a command waits.
    ActionScriptCommand *command; ///< Current borrowed command, or null when execution has stopped.
    s32 soundHandle; ///< Sound playback handle tracked by the sequencer, or zero.
    u32 lastRequestId; ///< Last action request recorded by a start-action command, or zero.
    u32 pendingRequestId; ///< Request being awaited by the start-action command, or zero.
} ActionScriptState;

struct ActorScriptState;

/// Execute a timed command for an actor.
/// \param object Borrowed actor object executing the script.
/// \param state Actor-owned model and action state.
/// \param script Actor-owned sequencer state with a valid current command.
/// \returns Nonzero when the command may advance, or zero to retry on a later frame.
typedef s32 (*ActionScriptCommandHandler)(u8 *object, struct ActorScriptState *state, ActionScriptState *script);

typedef char ActionScriptCommandSizeCheck[(sizeof(ActionScriptCommand) == 0x10) ? 1 : -1];
typedef char ActionScriptStateSizeCheck[(sizeof(ActionScriptState) == 0x20) ? 1 : -1];
#define ACTION_SCRIPT_OFFSET(field) ((u32) & (((ActionScriptState *)0)->field))
typedef char ActionScriptCursorOffsetCheck[(ACTION_SCRIPT_OFFSET(command) == 0x10) ? 1 : -1];
typedef char ActionScriptSoundOffsetCheck[(ACTION_SCRIPT_OFFSET(soundHandle) == 0x14) ? 1 : -1];
typedef char ActionScriptLastRequestOffsetCheck[(ACTION_SCRIPT_OFFSET(lastRequestId) == 0x18) ? 1 : -1];
typedef char ActionScriptPendingRequestOffsetCheck[(ACTION_SCRIPT_OFFSET(pendingRequestId) == 0x1C) ? 1 : -1];
#undef ACTION_SCRIPT_OFFSET

#endif
