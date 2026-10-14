/// \file
/// Defines recovered script values, VM state, and native command interfaces.

#ifndef SCRIPT_VM_H
#define SCRIPT_VM_H

#include <ultra64.h>
#include "script_vm_commands.h"

/// Recovered script value tags; tag 2 remains an unresolved integer-like form.
/// \ingroup ActorScriptVm
typedef enum ScriptValueTag {
    SCRIPT_VALUE_INTEGER = 0,
    SCRIPT_VALUE_FLOAT = 1,
    SCRIPT_VALUE_STRING_INDEX = 3,
    SCRIPT_VALUE_STRING_POINTER = 4
} ScriptValueTag;

/// Scheduling states retained by an actor's script context.
/// \ingroup ActorScriptVm
typedef enum ScriptVmExecutionState {
    SCRIPT_VM_READY = 0,              ///< Bytecode may run on the next actor update.
    SCRIPT_VM_YIELDED = 1,            ///< Bytecode resumes on a later actor update.
    SCRIPT_VM_WAITING_FOR_ACTION = 2, ///< Advance the actor's action handler.
    SCRIPT_VM_FINISHED = 4,           ///< Program finished; the next update resets the state.
    SCRIPT_VM_ERROR = 5               ///< Missing context or invalid opcode was reported.
} ScriptVmExecutionState;

/// A script operand or stack value with a tag and four payload bytes.
/// \ingroup ActorScriptVm
/// Numeric values, string indices, and borrowed string addresses share this
/// representation. The byte view supports copying complete values. Payloads
/// have byte alignment and must not be accessed through an aligned word pointer.
typedef union ScriptValue {
    struct {
        u8 tag;        ///< Value category, including the recovered ScriptValueTag values.
        u8 payload[4]; ///< Number, string-table index, or borrowed string address.
    } data;
    u8 bytes[5]; ///< Complete target representation of the value.
} ScriptValue;

struct ScriptVm;

/// Native script command called with an actor and its retained context.
/// \param actor Actor executing the command.
/// \param vm Script context providing command arguments and receiving results.
/// \returns A ScriptNativeCommandResult scheduling request; other nonzero values also yield.
typedef s32 (*ScriptNativeCommand)(u8 *actor, struct ScriptVm *vm);

/// Borrowed program resources and stack capacity supplied when creating a VM.
/// The observed prefix ends at 0x14; the complete resource allocation is unknown.
typedef struct ScriptVmConfiguration {
    u8 *program;                   ///< Bytecode retained for the context lifetime.
    char **stringTable;            ///< Borrowed table of borrowed null-terminated strings; both outlive the context.
    ScriptNativeCommand *commands; ///< Native-command table retained for the context lifetime.
    s32 unknown00C;                ///< Resource word whose purpose remains unresolved.
    s32 stackCapacity;             ///< Requested value capacity; nonpositive values select the default.
} ScriptVmConfiguration;

/// Recovered stack and return-location fields of a retained script context.
/// \ingroup ActorScriptVm
/// Unknown regions preserve the context extent without assigning semantics.
/// The context owns its value stack; saved depths and return offsets belong to
/// the same running script. This layout is specific to the original N64 ABI.
typedef struct ScriptVm {
    s32 executionState;            ///< Interpreter scheduling state retained between updates.
    u8 *program;                   ///< Borrowed script bytecode.
    char **stringTable;            ///< Borrowed table of borrowed null-terminated strings.
    f32 elapsed;                   ///< Time accumulated since the latest script reset.
    f32 totalElapsed;              ///< Time accumulated across resets.
    s32 instructionOffset;         ///< Current position in the borrowed script program.
    s32 stackDepth;                ///< Number of active values on the value stack.
    s32 stackCapacity;             ///< Maximum number of values in the owned stack.
    ScriptValue *values;           ///< Owned value-stack storage.
    s32 savedDepthCount;           ///< Number of saved value-stack depths.
    s32 savedDepths[64];           ///< Value-stack depths awaiting restoration.
    s32 returnCount;               ///< Number of saved script return locations.
    s32 returnOffsets[64];         ///< Program offsets awaiting a script return.
    s32 localFrameBases[32];       ///< Value-stack base for each local-variable frame.
    ScriptNativeCommand *commands; ///< Borrowed native-command dispatch table.
    s32 unknown2B0;                ///< Reset with the script context; its role remains unresolved.
    s32 messageStyle;              ///< Font style for subsequently queued script messages.
    f32 messageDuration;           ///< Display duration for subsequent messages in seconds.
    f32 actionTimer;               ///< Countdown retained while a script action handler waits.
    u32 actionHandler;             ///< Selector dispatched while waiting for an actor action.
    s32 actionInitializing;        ///< Nonzero on the handler's first update.
} ScriptVm;

/// Copy a typed value onto a script context's value stack.
/// \param vm Context with space for another value.
/// \param value Value to copy; its storage need not outlive the call.
/// \returns Nothing.
/// \note Original identifier: \c func_80115228 (0x80115228).
void scriptVmPushValue(ScriptVm *vm, ScriptValue *value);

/// Remove and return the top typed value from a script context's value stack.
/// \param vm Context with at least one active value.
/// \param value Destination receiving the removed value.
/// \returns Nothing.
/// \note Original identifier: \c func_801152DC (0x801152DC).
void scriptVmPopValue(ScriptVm *vm, ScriptValue *value);

/// Read the next numeric script operand and advance the instruction position.
/// \param vm Context positioned at a valid numeric operand.
/// \returns Operand converted to an integer.
/// \note Original identifier: \c func_8011566C (0x8011566C).
s32 scriptVmReadIntegerOperand(ScriptVm *vm);

typedef char ScriptVmConfigurationSizeCheck[(sizeof(ScriptVmConfiguration) == 0x14) ? 1 : -1];

typedef char ScriptValueSizeCheck[(sizeof(ScriptValue) == 5) ? 1 : -1];
typedef char ScriptVmSizeCheck[(sizeof(ScriptVm) == 0x2C8) ? 1 : -1];

#define SCRIPT_VM_OFFSET_CHECK(name, type, member, offset)                                                             \
    typedef char name[(((u32) & ((type *)0)->member) == (offset)) ? 1 : -1]
SCRIPT_VM_OFFSET_CHECK(ScriptVmConfigurationProgramOffsetCheck, ScriptVmConfiguration, program, 0x0);
SCRIPT_VM_OFFSET_CHECK(ScriptVmConfigurationStringTableOffsetCheck, ScriptVmConfiguration, stringTable, 0x4);
SCRIPT_VM_OFFSET_CHECK(ScriptVmConfigurationCommandsOffsetCheck, ScriptVmConfiguration, commands, 0x8);
SCRIPT_VM_OFFSET_CHECK(ScriptVmConfigurationStackCapacityOffsetCheck, ScriptVmConfiguration, stackCapacity, 0x10);
SCRIPT_VM_OFFSET_CHECK(ScriptVmProgramOffsetCheck, ScriptVm, program, 0x4);
SCRIPT_VM_OFFSET_CHECK(ScriptVmStringTableOffsetCheck, ScriptVm, stringTable, 0x8);
SCRIPT_VM_OFFSET_CHECK(ScriptValueDataTagOffsetCheck, ScriptValue, data.tag, 0x0);
SCRIPT_VM_OFFSET_CHECK(ScriptValueDataPayloadOffsetCheck, ScriptValue, data.payload, 0x1);
SCRIPT_VM_OFFSET_CHECK(ScriptVmInstructionOffsetOffsetCheck, ScriptVm, instructionOffset, 0x14);
SCRIPT_VM_OFFSET_CHECK(ScriptVmStackDepthOffsetCheck, ScriptVm, stackDepth, 0x18);
SCRIPT_VM_OFFSET_CHECK(ScriptVmStackCapacityOffsetCheck, ScriptVm, stackCapacity, 0x1C);
SCRIPT_VM_OFFSET_CHECK(ScriptVmValuesOffsetCheck, ScriptVm, values, 0x20);
SCRIPT_VM_OFFSET_CHECK(ScriptVmSavedDepthCountOffsetCheck, ScriptVm, savedDepthCount, 0x24);
SCRIPT_VM_OFFSET_CHECK(ScriptVmSavedDepthsOffsetCheck, ScriptVm, savedDepths, 0x28);
SCRIPT_VM_OFFSET_CHECK(ScriptVmReturnCountOffsetCheck, ScriptVm, returnCount, 0x128);
SCRIPT_VM_OFFSET_CHECK(ScriptVmReturnOffsetsOffsetCheck, ScriptVm, returnOffsets, 0x12C);
SCRIPT_VM_OFFSET_CHECK(ScriptVmLocalFrameBasesOffsetCheck, ScriptVm, localFrameBases, 0x22C);
SCRIPT_VM_OFFSET_CHECK(ScriptVmCommandsOffsetCheck, ScriptVm, commands, 0x2AC);
SCRIPT_VM_OFFSET_CHECK(ScriptVmMessageStyleOffsetCheck, ScriptVm, messageStyle, 0x2B4);
SCRIPT_VM_OFFSET_CHECK(ScriptVmActionTimerOffsetCheck, ScriptVm, actionTimer, 0x2BC);
SCRIPT_VM_OFFSET_CHECK(ScriptVmActionHandlerOffsetCheck, ScriptVm, actionHandler, 0x2C0);
SCRIPT_VM_OFFSET_CHECK(ScriptVmActionInitializingOffsetCheck, ScriptVm, actionInitializing, 0x2C4);
#undef SCRIPT_VM_OFFSET_CHECK

#endif
