#include "game_heap.h"
#include "message_bar.h"
#include "actor_script_action.h"
#include "heap_free.h"
/// \file
/// Cooperative actor behavior interpreter and script-value operations.
/// \ingroup ActorScriptVm
///
/// See docs/VOICE_AND_SCRIPT_VM.md for the subsystem walkthrough.

/// \defgroup ActorScriptVm Actor script execution
/// Executes actor behavior programs across gameplay updates.
///
/// Programs evaluate expressions, inspect gameplay state, and invoke native
/// commands. Execution continues until a command yields, an action requires
/// further updates, or the program finishes. Actors retain their execution
/// context between updates; native commands define their gameplay effects.
///
/// The actor VM chooses behavior. The separate action sequencer schedules
/// timed action requests and sounds while motion playback advances poses.
/// \see VoiceInteraction
/// \see actionScriptUpdate

/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "actor_script.h"
#include "heap_alloc.h"
#include "object_types.h"
#include "heap_types.h"
#include "script_vm.h"
#include "script_vm_stack.h"
#include "script_vm_lifecycle.h"
#include "script_vm_execution.h"
#include "script_vm_globals.h"
#include "script_value.h"

/// Recovered operations encoded by one byte of actor-script bytecode.
/// Binary operations use the second-from-top value as the left operand.
/// Bytes zero through two are accepted without a visible operation; their
/// distinct source-language roles remain unresolved and retain numeric spelling.
typedef enum ScriptOpcode {
    SCRIPT_OP_NEGATE = 3,            ///< Numeric negation.
    SCRIPT_OP_BITWISE_NOT = 4,       ///< Integer bitwise complement; floating input produces integer zero.
    SCRIPT_OP_ADD = 5,               ///< Left operand plus right operand.
    SCRIPT_OP_SUBTRACT = 6,          ///< Left operand minus right operand.
    SCRIPT_OP_MULTIPLY = 7,          ///< Product of the operands.
    SCRIPT_OP_DIVIDE = 8,            ///< Left operand divided by right operand; integer division truncates toward zero.
    SCRIPT_OP_REMAINDER = 9,         ///< Integer remainder; floating input produces floating zero.
    SCRIPT_OP_STORE_VARIABLE = 10,   ///< Assign a popped value to a referenced variable.
    SCRIPT_OP_EQUAL = 11,            ///< Equality, using the retained tolerance for floating values.
    SCRIPT_OP_NOT_EQUAL = 12,        ///< Inequality, using the retained tolerance for floating values.
    SCRIPT_OP_GREATER_THAN = 13,     ///< Left operand greater than right operand.
    SCRIPT_OP_LESS_THAN = 14,        ///< Left operand less than right operand.
    SCRIPT_OP_GREATER_OR_EQUAL = 15, ///< Left operand greater than or equal to right operand.
    SCRIPT_OP_LESS_OR_EQUAL = 16,    ///< Left operand less than or equal to right operand.
    SCRIPT_OP_LOGICAL_NOT = 17,      ///< Logical negation of one operand.
    SCRIPT_OP_LOGICAL_AND = 18,      ///< Logical conjunction of the converted integer operands.
    SCRIPT_OP_LOGICAL_OR = 19,       ///< Logical disjunction of the converted integer operands.
    SCRIPT_OP_BITWISE_AND = 20,      ///< Integer bitwise conjunction.
    SCRIPT_OP_BITWISE_OR = 21,       ///< Integer bitwise disjunction.
    SCRIPT_OP_SHIFT_RIGHT = 22,      ///< Signed integer right shift by the right operand.
    SCRIPT_OP_SHIFT_LEFT = 23,       ///< Integer left shift by the right operand.
    SCRIPT_OP_PUSH_LITERAL = 24,     ///< Push an encoded value from the bytecode.
    SCRIPT_OP_LOAD_VARIABLE = 25,    ///< Push the value of a referenced variable.
    SCRIPT_OP_CALL = 26,             ///< Call a bytecode offset with a saved return location.
    SCRIPT_OP_CALL_NATIVE = 27,      ///< Invoke a registered native command.
    SCRIPT_OP_JUMP = 28,             ///< Continue at the encoded bytecode offset.
    SCRIPT_OP_JUMP_IF_ZERO = 29,     ///< Branch to the encoded offset when the popped condition is zero.
    SCRIPT_OP_RETURN = 30,           ///< Return a value and restore the caller frame and return location.
    SCRIPT_OP_DISCARD = 31,          ///< Discard the top value.
    SCRIPT_OP_ENTER_FRAME = 32,      ///< Establish the called function's argument and local-variable frame.
    SCRIPT_OP_LOAD_ELEMENT = 33,     ///< Push an indexed value from a referenced value array.
    SCRIPT_OP_STORE_ELEMENT = 34,    ///< Assign a popped value to an indexed element of a referenced array.
    SCRIPT_OP_FINISH = 35,           ///< Finish the program and clear its retained execution depths.
} ScriptOpcode;

/// Default number of values for configurations without a positive capacity.
enum { SCRIPT_VM_DEFAULT_STACK_CAPACITY = 200 };

/// Relative and absolute tolerances used by floating equality comparisons.
#define SCRIPT_VALUE_RELATIVE_EQUALITY_TOLERANCE 0.001F
#define SCRIPT_VALUE_ABSOLUTE_EQUALITY_TOLERANCE 0.001F

/// Shared 32-byte numeric conversion text, overwritten by the next numeric string conversion.
/// \note Original identifier: \c D_801340B0 (0x801340B0).
struct {
    s32 w[8];
} gScriptVmNumericTextBuffer;

/// Number of live registered items in the current area.
/// \note Original identifier: \c D_80134040 (0x80134040).
extern s32 gLiveItemCount;

/* Debug trace: compiled out in this build, as a statement-safe macro; each
 * format string is still emitted to .rodata in source order. */
#define TRACE(args)                                                                                                    \
    do {                                                                                                               \
        if (0) {                                                                                                       \
            osSyncPrintf args;                                                                                         \
        }                                                                                                              \
    } while (0)

void _bzero(void *, s32); /* extern */
/// Reset script execution state and local values for reuse, retaining total elapsed time.
/// \param vm Script context to reset, or null.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115070 (0x80115070).
void scriptVmReset(ScriptVm *vm) {
    if (vm != NULL) {
        vm->executionState = SCRIPT_VM_READY;
        vm->instructionOffset = 0;
        vm->stackDepth = 0;
        vm->elapsed = 0.0F;
        _bzero(vm->values, vm->stackCapacity * sizeof(ScriptValue));
        vm->savedDepthCount = 0;
        _bzero(vm->savedDepths, sizeof(vm->savedDepths));
        vm->returnCount = 0;
        _bzero(vm->returnOffsets, sizeof(vm->returnOffsets));
        _bzero(vm->localFrameBases, sizeof(vm->localFrameBases));
        vm->unknown2B0 = 0;
        vm->messageStyle = 0;
        vm->actionHandler = 0;
        vm->actionInitializing = 0;
        vm->messageDuration = 3.5F;
        vm->actionTimer = 0.0F;
    }
}

/// Create a script context with a specified value-stack capacity.
/// \param config Configuration supplying borrowed program, string, and command tables that must outlive the context.
/// \param capacity Positive maximum number of values in the script stack.
/// \returns An owned initialized context to release with scriptVmDestroy.
/// \note Original decompilation identifier: \c func_80115114 (0x80115114).
ScriptVm *scriptVmCreateWithCapacity(ScriptVmConfiguration *config, s32 capacity) {
    ScriptVm *vm;
    vm = heapAllocLocal((&gGameHeap), sizeof(ScriptVm), 0xB3);
    _bzero(vm, sizeof(ScriptVm));
    vm->totalElapsed = 0.0F;
    vm->commands = config->commands;
    vm->program = config->program;
    vm->stringTable = config->stringTable;
    vm->stackCapacity = capacity;
    vm->values = (ScriptValue *)heapAllocLocal((&gGameHeap), capacity * sizeof(ScriptValue), 0xC6);
    scriptVmReset(vm);
    return vm;
}

/// Create a script context using its configured stack capacity.
/// \param config Configuration with borrowed program, string, and command tables that must outlive the context; a nonpositive stack capacity selects 200 values.
/// \returns An owned initialized context to release with scriptVmDestroy.
/// \note Original decompilation identifier: \c func_801151B0 (0x801151B0).
ScriptVm *scriptVmCreate(ScriptVmConfiguration *config) {
    s32 configuredCapacity = config->stackCapacity;
    s32 capacity;

    if (configuredCapacity <= 0) {
        TRACE(("WARNING : stack size out of range, %d\n", configuredCapacity));
        capacity = SCRIPT_VM_DEFAULT_STACK_CAPACITY;
    } else {
        capacity = configuredCapacity;
    }
    return scriptVmCreateWithCapacity(config, capacity);
}

/// Release a script context and its owned storage.
/// \param vm Script context to release, or null.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801151E4 (0x801151E4).
void scriptVmDestroy(ScriptVm *vm) {
    if (vm != NULL) {
        heapFreeLocal(&gGameHeap, vm->values);
        heapFreeLocal(&gGameHeap, vm);
    }
}

#define VM_SP(vm) ((vm)->stackDepth)
#define VM_STACK(vm) ((vm)->values)

/// Push a typed value onto the script stack.
/// \param vm Script context with room for another value.
/// \param value Typed value to copy onto the stack.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115228 (0x80115228).
void scriptVmPushValue(ScriptVm *vm, ScriptValue *value) {
    s32 i;

    for (i = 0; i < SCRIPT_VALUE_BYTES; i++) {
        VM_STACK(vm)[VM_SP(vm)].bytes[i] = value->bytes[i];
    }
    VM_SP(vm)++;
}

/// Remove the top typed value from the script stack.
/// \param vm Script context with a nonempty value stack.
/// \param value Output receiving the removed typed value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801152DC (0x801152DC).
void scriptVmPopValue(ScriptVm *vm, ScriptValue *value) {
    s32 i;

    VM_SP(vm)--;
    for (i = 0; i < SCRIPT_VALUE_BYTES; i++) {
        value->bytes[i] = VM_STACK(vm)[VM_SP(vm)].bytes[i];
    }
}

/// Save the script value-stack depth for later restoration.
/// \param vm Script context with fewer than 64 saved stack depths.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011538C (0x8011538C).
void scriptVmSaveStackDepth(ScriptVm *vm) {
    vm->savedDepths[vm->savedDepthCount] = vm->stackDepth;
    vm->savedDepthCount = (s32)(vm->savedDepthCount + 1);
}

/// Restore the most recently saved script value-stack depth.
/// \param vm Script context with a saved stack depth to restore.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801153B0 (0x801153B0).
void scriptVmRestoreStackDepth(ScriptVm *vm) {
    vm->savedDepthCount--;
    vm->stackDepth = vm->savedDepths[vm->savedDepthCount];
}

/// Save the current script instruction location for a later return.
/// \param vm Script context with fewer than 64 saved return locations.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801153D0 (0x801153D0).
void scriptVmSaveReturnLocation(ScriptVm *vm) {
    vm->returnOffsets[vm->returnCount] = vm->instructionOffset;
    vm->returnCount = (s32)(vm->returnCount + 1);
}

/// Restore the most recent script return location when one is available.
/// \param vm Script context whose return location is restored.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801153F4 (0x801153F4).
void scriptVmRestoreReturnLocation(ScriptVm *vm) {
    if (vm->returnCount > 0) {
        vm->returnCount--;
        vm->instructionOffset = vm->returnOffsets[vm->returnCount];
    }
}

/// Provide an inactive script-runtime hook for this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011541C (0x8011541C).
void scriptVmNoOp(void) {
}

/// Read a numeric script value as an integer.
/// \param value Numeric script value to read; floating-point values must be representable as an integer.
/// \returns The integer value, with any floating-point fraction discarded.
///
/// \note Original decompilation identifier: \c func_80115424 (0x80115424).
s32 scriptValueGetInteger(ScriptValue *value) {
    s32 integerValue;
    f32 floatValue;
    s32 isInteger;

    if ((value->data.tag == SCRIPT_VALUE_INTEGER) || (value->data.tag == SCRIPT_VALUE_CODE_REFERENCE)) {
        isInteger = 1;
    } else {
        isInteger = 0;
    }
    if (isInteger != 0) {
        bcopy(value->data.payload, &integerValue, SCRIPT_VALUE_PAYLOAD_BYTES);
        return integerValue;
    }
    bcopy(value->data.payload, &floatValue, SCRIPT_VALUE_PAYLOAD_BYTES);
    return floatValue;
}

/// Read a numeric script value as a floating-point number.
/// \param value Numeric script value to read.
/// \returns The floating-point value.
/// \note Original decompilation identifier: \c func_8011549C (0x8011549C).
f32 scriptValueGetFloat(ScriptValue *value) {
    f32 floatValue;
    s32 integerValue;
    s32 isFloat;

    if (value->data.tag == SCRIPT_VALUE_FLOAT) {
        isFloat = 1;
    } else {
        isFloat = 0;
    }
    if (isFloat) {
        bcopy(value->data.payload, &floatValue, SCRIPT_VALUE_PAYLOAD_BYTES);
        return floatValue;
    }
    bcopy(value->data.payload, &integerValue, SCRIPT_VALUE_PAYLOAD_BYTES);
    return integerValue;
}

/// Assign a floating-point number to a script value.
/// \param value Script value to replace.
/// \param number Floating-point number to assign.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011550C (0x8011550C).
void scriptValueSetFloat(ScriptValue *value, f32 number) {
    value->data.tag = SCRIPT_VALUE_FLOAT;
    bcopy(&number, value->data.payload, SCRIPT_VALUE_PAYLOAD_BYTES);
}

/// Assign an integer to a script value.
/// \param value Script value to replace.
/// \param number Integer to assign.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115544 (0x80115544).
void scriptValueSetInteger(ScriptValue *value, s32 number) {
    value->data.tag = SCRIPT_VALUE_INTEGER;
    bcopy(&number, value->data.payload, SCRIPT_VALUE_PAYLOAD_BYTES);
}

/// Assign a string pointer to a script value.
/// \param value Script value to replace.
/// \param text Borrowed string address that must remain valid while the value is used.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115578 (0x80115578).
void scriptValueSetStringPointer(ScriptValue *value, char *text) {
    value->data.tag = SCRIPT_VALUE_STRING_POINTER;
    bcopy(&text, value->data.payload, SCRIPT_VALUE_PAYLOAD_BYTES);
}

s32 stdlibParseUnsignedDecimal(s32); /* extern */

/// Parse a decimal number from a script string value.
/// \param vm Script context used to resolve string-table references.
/// \param value Null-terminated string value or valid string-table reference to parse.
/// \returns The parsed decimal value, or zero when the value is not a string.
/// \note Original decompilation identifier: \c func_801155B0 (0x801155B0).
s32 scriptValueParseDecimal(ScriptVm *vm, ScriptValue *value) {
    s32 stringIndex;
    s32 isString;
    char *text;
    s32 isPointer;
    s32 parsedValue;

    if ((value->data.tag == SCRIPT_VALUE_STRING_INDEX) || (value->data.tag == SCRIPT_VALUE_STRING_POINTER)) {
        isString = 1;
    } else {
        isString = 0;
    }
    if (isString == 0) {
        return 0;
    }
    if (value->data.tag == SCRIPT_VALUE_STRING_POINTER) {
        isPointer = 1;
    } else {
        isPointer = 0;
    }
    if (isPointer != 0) {
        bcopy(&value->bytes[1], &text, SCRIPT_VALUE_PAYLOAD_BYTES);
        parsedValue = stdlibParseUnsignedDecimal((s32)text);
    } else {
        bcopy(&value->bytes[1], &stringIndex, SCRIPT_VALUE_PAYLOAD_BYTES);
        text = vm->stringTable[stringIndex];
        parsedValue = stdlibParseUnsignedDecimal((s32)text);
    }
    return parsedValue;
}

/// Read the next script operand as an integer and advance execution past it.
/// \param vm Script context positioned at a valid numeric operand.
/// \returns The operand converted to an integer.
/// \note Original decompilation identifier: \c func_8011566C (0x8011566C).
s32 scriptVmReadIntegerOperand(ScriptVm *vm) {
    s32 operandOffset;
    s32 operandAddress;

    operandOffset = vm->instructionOffset;
    operandAddress = (s32)vm->program + operandOffset;
    vm->instructionOffset = operandOffset + SCRIPT_VALUE_BYTES;
    return scriptValueGetInteger((ScriptValue *)(operandAddress));
}

/// Global script-variable storage containing 500 packed five-byte values.
/// \note Original identifier: \c D_800EF7A0 (0x800EF7A0).
extern ScriptValue gScriptVmGlobalValues[500];

/// Read the next variable reference and resolve it to a writable script value.
/// \param vm Context positioned at a valid variable reference, with frame -1 for globals or 0 through 31 for locals and an in-range variable index.
/// \returns A borrowed writable value owned by the context or global variable storage.
///
/// \note UB: a frame below -1 returns an uninitialised pointer.
/// \note Original decompilation identifier: \c func_801156A0 (0x801156A0).
ScriptValue *scriptVmReadVariableReference(ScriptVm *vm) {
    s32 frame;
    s32 valueIndex;
    ScriptValue *value;

    frame = scriptVmReadIntegerOperand(vm);
    valueIndex = scriptVmReadIntegerOperand(vm);
    if (frame >= 0) {
        valueIndex = vm->localFrameBases[frame] + valueIndex;
        value = &vm->values[valueIndex];
    } else if (frame == -1) {
        value = &gScriptVmGlobalValues[valueIndex];
    }
    return value;
}

/// Replace the top script value with the result of a unary operation.
/// \param vm Context with a numeric or valid decimal-string operand on its stack; integer negation requires a representable result.
/// \param op SCRIPT_OP_NEGATE, SCRIPT_OP_BITWISE_NOT, or SCRIPT_OP_LOGICAL_NOT.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115724 (0x80115724).
void scriptVmApplyUnaryOperator(ScriptVm *vm, u8 op) {
    ScriptValue *box;
    s32 test;
    f32 f;
    s32 i;
    box = VM_STACK(vm) + VM_SP(vm) - 1;
    if (box->data.tag == SCRIPT_VALUE_STRING_INDEX || box->data.tag == SCRIPT_VALUE_STRING_POINTER) {
        test = 1;
    } else {
        test = 0;
    }
    if (test) {
        scriptValueSetInteger(box, scriptValueParseDecimal(vm, box));
    }
    if (box->data.tag == SCRIPT_VALUE_FLOAT) {
        test = 1;
    } else {
        test = 0;
    }
    if (test) {
        f = scriptValueGetFloat(box);
        switch (op) {
        case SCRIPT_OP_NEGATE:
            scriptValueSetFloat(box, -f);
            break;
        case SCRIPT_OP_LOGICAL_NOT:
            scriptValueSetFloat(box, !f);
            break;
        case SCRIPT_OP_BITWISE_NOT:
            scriptValueSetInteger(box, 0);
            break;
        }
    } else {
        i = scriptValueGetInteger(box);
        switch (op) {
        case SCRIPT_OP_NEGATE:
            scriptValueSetInteger(box, -i);
            break;
        case SCRIPT_OP_LOGICAL_NOT:
            scriptValueSetInteger(box, !i);
            break;
        case SCRIPT_OP_BITWISE_NOT:
            scriptValueSetInteger(box, ~i);
            break;
        }
    }
}

/// Replace two script operands with their arithmetic result.
/// \param vm Context with a numeric left operand and a numeric or valid decimal-string right operand.
/// \pre Integer results must be representable; integer division and remainder require a nonzero divisor and exclude the minimum integer divided by -1.
/// \param op Arithmetic ScriptOpcode; SCRIPT_OP_REMAINDER produces zero for floating-point operands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801158D0 (0x801158D0).
void scriptVmApplyArithmeticOperator(ScriptVm *vm, u8 op) {
    ScriptValue right;
    ScriptValue left;
    ScriptValue result;
    f32 fv;
    f32 fr;
    f32 fl;
    s32 iv;
    s32 ir;
    s32 il;
    scriptVmPopValue(vm, &right);
    if ((right.data.tag == SCRIPT_VALUE_STRING_INDEX || right.data.tag == SCRIPT_VALUE_STRING_POINTER) ? 1 : 0) {
        scriptValueSetInteger(&right, scriptValueParseDecimal(vm, &right));
    }
    scriptVmPopValue(vm, &left);
    if ((right.data.tag == SCRIPT_VALUE_STRING_INDEX || right.data.tag == SCRIPT_VALUE_STRING_POINTER) ? 1 : 0) {
        scriptValueSetInteger(&right, scriptValueParseDecimal(vm, &right));
    }
    if ((right.data.tag == SCRIPT_VALUE_FLOAT ? 1 : 0) || (left.data.tag == SCRIPT_VALUE_FLOAT ? 1 : 0)) {
        fr = scriptValueGetFloat(&right);
        fl = scriptValueGetFloat(&left);
        switch (op) {
        case SCRIPT_OP_ADD:
            fv = fl + fr;
            break;
        case SCRIPT_OP_SUBTRACT:
            fv = fl - fr;
            break;
        case SCRIPT_OP_MULTIPLY:
            fv = fl * fr;
            break;
        case SCRIPT_OP_DIVIDE:
            fv = fl / fr;
            break;
        case SCRIPT_OP_REMAINDER:
            fv = 0.0F;
            break;
        }
        scriptValueSetFloat(&result, fv);
    } else {
        ir = scriptValueGetInteger(&right);
        il = scriptValueGetInteger(&left);
        switch (op) {
        case SCRIPT_OP_ADD:
            iv = il + ir;
            break;
        case SCRIPT_OP_SUBTRACT:
            iv = il - ir;
            break;
        case SCRIPT_OP_MULTIPLY:
            iv = il * ir;
            break;
        case SCRIPT_OP_DIVIDE:
            iv = il / ir;
            break;
        case SCRIPT_OP_REMAINDER:
            iv = il % ir;
            break;
        }
        scriptValueSetInteger(&result, iv);
    }
    scriptVmPushValue(vm, &result);
}

/// Replace two script operands with their bitwise-operation result.
/// \param vm Context with a numeric left operand and a numeric or valid decimal-string right operand; either floating-point operand produces zero.
/// \pre Integer shift counts must be 0 through 31; left shifts require a nonnegative operand and a representable result.
/// \param op SCRIPT_OP_BITWISE_AND, SCRIPT_OP_BITWISE_OR, SCRIPT_OP_SHIFT_RIGHT, or SCRIPT_OP_SHIFT_LEFT.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115B38 (0x80115B38).
void scriptVmApplyBitwiseOperator(ScriptVm *vm, u8 op) {
    ScriptValue right;
    ScriptValue left;
    ScriptValue result;
    s32 value;
    s32 rhs;
    s32 lhs;
    scriptVmPopValue(vm, &right);
    if ((right.data.tag == SCRIPT_VALUE_STRING_INDEX || right.data.tag == SCRIPT_VALUE_STRING_POINTER) ? 1 : 0) {
        scriptValueSetInteger(&right, scriptValueParseDecimal(vm, &right));
    }
    scriptVmPopValue(vm, &left);
    if ((right.data.tag == SCRIPT_VALUE_STRING_INDEX || right.data.tag == SCRIPT_VALUE_STRING_POINTER) ? 1 : 0) {
        scriptValueSetInteger(&right, scriptValueParseDecimal(vm, &right));
    }
    if ((right.data.tag == SCRIPT_VALUE_FLOAT ? 1 : 0) || (left.data.tag == SCRIPT_VALUE_FLOAT ? 1 : 0)) {
        scriptValueSetInteger(&result, 0);
    } else {
        rhs = scriptValueGetInteger(&right);
        lhs = scriptValueGetInteger(&left);
        switch (op) {
        case SCRIPT_OP_BITWISE_AND:
            value = lhs & rhs;
            break;
        case SCRIPT_OP_BITWISE_OR:
            value = lhs | rhs;
            break;
        case SCRIPT_OP_SHIFT_RIGHT:
            value = lhs >> rhs;
            break;
        case SCRIPT_OP_SHIFT_LEFT:
            value = lhs << rhs;
            break;
        }
        scriptValueSetInteger(&result, value);
    }
    scriptVmPushValue(vm, &result);
}

f32 fabsf(f32);
#pragma intrinsic(fabsf)
/// Replace two script operands with an integer comparison or logical result.
/// \param vm Context with a numeric left operand and a numeric or valid decimal-string right operand.
/// \pre Floating-point operands of logical operations must be representable as integers; their fractional parts do not affect truth values.
/// \param op A comparison or logical ScriptOpcode; floating equality uses relative and absolute tolerances.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115CBC (0x80115CBC).
void scriptVmApplyComparisonOperator(ScriptVm *vm, u8 op) {
    ScriptValue right;
    ScriptValue left;
    ScriptValue result;
    s32 value;
    f32 fr;
    f32 fl;
    s32 ir;
    s32 il;
    scriptVmPopValue(vm, &right);
    if ((right.data.tag == SCRIPT_VALUE_STRING_INDEX || right.data.tag == SCRIPT_VALUE_STRING_POINTER) ? 1 : 0) {
        scriptValueSetInteger(&right, scriptValueParseDecimal(vm, &right));
    }
    scriptVmPopValue(vm, &left);
    if ((right.data.tag == SCRIPT_VALUE_STRING_INDEX || right.data.tag == SCRIPT_VALUE_STRING_POINTER) ? 1 : 0) {
        scriptValueSetInteger(&right, scriptValueParseDecimal(vm, &right));
    }
    value = 0;
    if ((right.data.tag == SCRIPT_VALUE_FLOAT ? 1 : 0) || (left.data.tag == SCRIPT_VALUE_FLOAT ? 1 : 0)) {
        fr = scriptValueGetFloat(&right);
        fl = scriptValueGetFloat(&left);
        switch (op) {
        case SCRIPT_OP_EQUAL:
            if (fabsf(fr - fl) <=
                (fabsf(fl) * SCRIPT_VALUE_RELATIVE_EQUALITY_TOLERANCE) + SCRIPT_VALUE_ABSOLUTE_EQUALITY_TOLERANCE) {
                value = 1;
            }
            break;
        case SCRIPT_OP_NOT_EQUAL:
            if (!(fabsf(fr - fl) <=
                  (fabsf(fl) * SCRIPT_VALUE_RELATIVE_EQUALITY_TOLERANCE) + SCRIPT_VALUE_ABSOLUTE_EQUALITY_TOLERANCE)) {
                value = 1;
            }
            break;
        case SCRIPT_OP_GREATER_THAN:
            if (fr < fl) {
                value = 1;
            }
            break;
        case SCRIPT_OP_LESS_THAN:
            if (fl < fr) {
                value = 1;
            }
            break;
        case SCRIPT_OP_GREATER_OR_EQUAL:
            if (fr <= fl) {
                value = 1;
            }
            break;
        case SCRIPT_OP_LESS_OR_EQUAL:
            if (fl <= fr) {
                value = 1;
            }
            break;
        case SCRIPT_OP_LOGICAL_AND:
            value = (s32)fr && (s32)fl;
            break;
        case SCRIPT_OP_LOGICAL_OR:
            value = (s32)fr || (s32)fl;
            break;
        }
    } else {
        ir = scriptValueGetInteger(&right);
        il = scriptValueGetInteger(&left);
        switch (op) {
        case SCRIPT_OP_EQUAL:
            if (il == ir) {
                value = 1;
            }
            break;
        case SCRIPT_OP_NOT_EQUAL:
            if (il != ir) {
                value = 1;
            }
            break;
        case SCRIPT_OP_GREATER_THAN:
            if (ir < il) {
                value = 1;
            }
            break;
        case SCRIPT_OP_LESS_THAN:
            if (il < ir) {
                value = 1;
            }
            break;
        case SCRIPT_OP_GREATER_OR_EQUAL:
            if (il >= ir) {
                value = 1;
            }
            break;
        case SCRIPT_OP_LESS_OR_EQUAL:
            if (ir >= il) {
                value = 1;
            }
            break;
        case SCRIPT_OP_LOGICAL_AND:
            value = ir && il;
            break;
        case SCRIPT_OP_LOGICAL_OR:
            value = ir || il;
            break;
        }
    }
    scriptValueSetInteger(&result, value);
    scriptVmPushValue(vm, &result);
}

/// Push an integer onto the script value stack.
/// \param vm Script context with stack capacity available.
/// \param value Integer to push.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011601C (0x8011601C).
void scriptVmPushInteger(ScriptVm *vm, s32 value) {
    ScriptValue valueBox;

    scriptValueSetInteger(&valueBox, value);
    scriptVmPushValue(vm, &valueBox);
}

/// Push a floating-point number onto the script value stack.
/// \param vm Script context with stack capacity available.
/// \param value Floating-point number to push.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011604C (0x8011604C).
void scriptVmPushFloat(ScriptVm *vm, f32 value) {
    ScriptValue valueBox;

    scriptValueSetFloat(&valueBox, value);
    scriptVmPushValue(vm, &valueBox);
}

/// Push a string pointer onto the script value stack.
/// \param vm Script context with stack capacity available.
/// \param text Borrowed string that must remain valid while the stacked value is used.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80116084 (0x80116084).
void scriptVmPushStringPointer(ScriptVm *vm, char *text) {
    ScriptValue valueBox;

    scriptValueSetStringPointer(&valueBox, text);
    scriptVmPushValue(vm, &valueBox);
}

/// Pop a numeric script value and convert it to an integer.
/// \param vm Script context with a numeric value on its stack.
/// \returns The popped integer value, with any floating-point fraction discarded.
/// \note Original decompilation identifier: \c func_801160B4 (0x801160B4).
s32 scriptVmPopInteger(ScriptVm *vm) {
    ScriptValue valueBox;

    scriptVmPopValue(vm, &valueBox);
    return scriptValueGetInteger(&valueBox);
}

/// Pop a numeric script value and convert it to a floating-point number.
/// \param vm Script context with a numeric value on its stack.
/// \returns The popped floating-point value.
/// \note Original decompilation identifier: \c func_801160DC (0x801160DC).
f32 scriptVmPopFloat(ScriptVm *vm) {
    ScriptValue valueBox;

    scriptVmPopValue(vm, &valueBox);
    return scriptValueGetFloat(&valueBox);
}

/// Pop a script value and obtain its string representation.
/// \param vm Context with a valid string or numeric value on its stack; numeric text must fit in 32 bytes including its terminator.
/// \returns The borrowed referenced string, or shared temporary text for a numeric value that a later numeric conversion overwrites.
/// \note Original decompilation identifier: \c func_80116104 (0x80116104).
char *scriptVmPopString(ScriptVm *vm) {
    ScriptValue box;
    char *text;
    s32 index;
    s32 test;
    scriptVmPopValue(vm, &box);
    if (box.data.tag == SCRIPT_VALUE_STRING_INDEX || box.data.tag == SCRIPT_VALUE_STRING_POINTER) {
        test = 1;
    } else {
        test = 0;
    }
    if (test) {
        if (box.data.tag == SCRIPT_VALUE_STRING_POINTER) {
            test = 1;
        } else {
            test = 0;
        }
        if (test) {
            bcopy(&box.bytes[1], &text, SCRIPT_VALUE_PAYLOAD_BYTES);
            return text;
        }
        bcopy(&box.bytes[1], &index, SCRIPT_VALUE_PAYLOAD_BYTES);
        return vm->stringTable[index];
    }
    if (box.data.tag == SCRIPT_VALUE_INTEGER || box.data.tag == SCRIPT_VALUE_CODE_REFERENCE) {
        test = 1;
    } else {
        test = 0;
    }
    if (test) {
        sprintf((char *)&gScriptVmNumericTextBuffer, "%d", scriptValueGetInteger(&box));
    } else {
        sprintf((char *)&gScriptVmNumericTextBuffer, "%f", scriptValueGetFloat(&box));
    }
    return (char *)&gScriptVmNumericTextBuffer;
}

/// Evaluate a global script variable as an integer.
/// \param index Valid index of a numeric global script variable.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80116224 (0x80116224).
void scriptVmEvaluateGlobalInteger(s32 index) {
    scriptValueGetInteger((ScriptValue *)(((u8 *)&gScriptVmGlobalValues + (index * SCRIPT_VALUE_BYTES))));
}

/// Assign an integer to a global script variable.
/// \param index Valid global script-variable index.
/// \param value Integer to assign.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80116254 (0x80116254).
void scriptVmSetGlobalInteger(s32 index, s32 value) {
    scriptValueSetInteger(&gScriptVmGlobalValues[index], value);
}

/// Run actor script instructions until execution yields, finishes, or encounters an error.
/// \param actor Actor with valid script state and well-formed bytecode whose stack, frame, and variable accesses remain within capacity.
/// \returns Execution state: 1 when yielded, 2 when waiting on an action, 4 when finished, or 5 on a missing context or invalid instruction.
/// \note Original decompilation identifier: \c func_80116284 (0x80116284).
/// \note Instructions continue until a scheduling boundary; execution does not
/// implicitly yield after each instruction. A yielding native command resumes
/// bytecode later, while an action wait advances its handler before resuming.
/// \see actorScriptDispatchRegisteredCommand
/// \see actorUpdateScriptVm
s32 actorRunScriptVm(u8 *actor) {
    ScriptVm *vm;
    u8 *owner;
    u8 op;

    owner = ((RuntimeObjectPrefix *)actor)->state.bytes;
    vm = (ScriptVm *)((ActorScriptState *)owner)->vmAddress;
    if (vm == NULL) {
        messageBarEnqueue(0, 3600.0F, (u8 *)"No PPU");
        goto block_29;
    }
loop_2:
loop_3:
    op = vm->program[vm->instructionOffset];
    vm->instructionOffset++;
    TRACE(("code = %d\n", op));
    switch (op) {
    case 0:
    case 1:
    case 2:
        goto loop_2;
    case SCRIPT_OP_DISCARD: {
        ScriptValue sp98;
        scriptVmPopValue(vm, &sp98);
        goto loop_2;
    }
    case SCRIPT_OP_NEGATE:
    case SCRIPT_OP_BITWISE_NOT:
    case SCRIPT_OP_LOGICAL_NOT:
        scriptVmApplyUnaryOperator(vm, op);
        goto loop_2;
    case SCRIPT_OP_ADD:
    case SCRIPT_OP_SUBTRACT:
    case SCRIPT_OP_MULTIPLY:
    case SCRIPT_OP_DIVIDE:
    case SCRIPT_OP_REMAINDER:
        scriptVmApplyArithmeticOperator(vm, op);
        goto loop_2;
    case SCRIPT_OP_EQUAL:
    case SCRIPT_OP_NOT_EQUAL:
    case SCRIPT_OP_GREATER_THAN:
    case SCRIPT_OP_LESS_THAN:
    case SCRIPT_OP_GREATER_OR_EQUAL:
    case SCRIPT_OP_LESS_OR_EQUAL:
    case SCRIPT_OP_LOGICAL_AND:
    case SCRIPT_OP_LOGICAL_OR:
        scriptVmApplyComparisonOperator(vm, op);
        goto loop_2;
    case SCRIPT_OP_BITWISE_AND:
    case SCRIPT_OP_BITWISE_OR:
    case SCRIPT_OP_SHIFT_RIGHT:
    case SCRIPT_OP_SHIFT_LEFT:
        scriptVmApplyBitwiseOperator(vm, op);
        goto loop_2;
    case SCRIPT_OP_PUSH_LITERAL:
        scriptVmPushValue(vm, (ScriptValue *)(vm->program + vm->instructionOffset));
        vm->instructionOffset = (s32)(vm->instructionOffset + SCRIPT_VALUE_BYTES);
        goto loop_3;
    case SCRIPT_OP_LOAD_VARIABLE:
        scriptVmPushValue(vm, scriptVmReadVariableReference(vm));
        goto loop_2;
    case SCRIPT_OP_STORE_VARIABLE:
        scriptVmPopValue(vm, scriptVmReadVariableReference(vm));
        goto loop_2;
    case SCRIPT_OP_LOAD_ELEMENT: {
        u8 *elements;
        s32 elementIndex;
        elements = (u8 *)scriptVmReadVariableReference(vm);
        elementIndex = scriptVmPopInteger(vm);
        scriptVmPushValue(vm, (ScriptValue *)((u8(*)[SCRIPT_VALUE_BYTES])elements)[elementIndex]);
        goto loop_2;
    }
    case SCRIPT_OP_STORE_ELEMENT: {
        ScriptValue *elements;
        s32 i;
        s32 elementIndex;
        ScriptValue value;
        elements = scriptVmReadVariableReference(vm);
        scriptVmPopValue(vm, &value);
        elementIndex = scriptVmPopInteger(vm);
        for (i = 0; i < SCRIPT_VALUE_BYTES; i++) {
            elements[elementIndex].bytes[i] = value.bytes[i];
        }
        goto loop_2;
    }
    case SCRIPT_OP_FINISH:
        vm->instructionOffset = 0;
        vm->stackDepth = 0;
        vm->savedDepthCount = 0;
        vm->returnCount = 0;
        TRACE(("PPU - process end.\n"));
        return SCRIPT_VM_FINISHED;
    case SCRIPT_OP_JUMP: {
        s32 address;
        TRACE(("%d\n", vm->instructionOffset));
        address = scriptValueGetInteger((ScriptValue *)((vm->program + vm->instructionOffset)));
        vm->instructionOffset = address;
        goto loop_2;
    }
    case SCRIPT_OP_JUMP_IF_ZERO: {
        s32 condition;
        ScriptValue sp6C;
        scriptVmPopValue(vm, &sp6C);
        condition = scriptValueGetInteger(&sp6C);
        if (condition == 0) {
            vm->instructionOffset = scriptValueGetInteger((ScriptValue *)((vm->program + vm->instructionOffset)));
        } else {
            vm->instructionOffset = (s32)(vm->instructionOffset + SCRIPT_VALUE_BYTES);
        }
        goto loop_2;
    }
    case SCRIPT_OP_CALL_NATIVE: {
        s32 commandStatus;
        commandStatus = actorScriptDispatchRegisteredCommand(actor);
        if (commandStatus == SCRIPT_COMMAND_CONTINUE) {
            goto loop_2;
        }
        if (commandStatus == SCRIPT_COMMAND_WAIT_FOR_ACTION) {
            return SCRIPT_VM_WAITING_FOR_ACTION;
        }
        return SCRIPT_VM_YIELDED;
    }
    case SCRIPT_OP_CALL: {
        s32 callTargetOffset;
        callTargetOffset = scriptValueGetInteger((ScriptValue *)((vm->program + vm->instructionOffset)));
        vm->instructionOffset = (s32)(vm->instructionOffset + SCRIPT_VALUE_BYTES);
        scriptVmSaveReturnLocation(vm);
        vm->instructionOffset = callTargetOffset;
        goto loop_2;
    }
    case SCRIPT_OP_RETURN: {
        s32 restoredFrameIndex;
        ScriptValue sp58;
        s32 restoredFrameBase;
        scriptVmPopValue(vm, &sp58);
        restoredFrameIndex = scriptVmPopInteger(vm);
        restoredFrameBase = scriptVmPopInteger(vm);
        scriptVmRestoreStackDepth(vm);
        scriptVmPushValue(vm, &sp58);
        vm->localFrameBases[restoredFrameIndex] = restoredFrameBase;
        scriptVmRestoreReturnLocation(vm);
        goto loop_2;
    }
    case SCRIPT_OP_ENTER_FRAME: {
        s32 argumentCount;
        s32 localCount;
        s32 frameIndex;
        s32 previousFrameBase;
        s32 *localSlot;
        argumentCount = scriptVmReadIntegerOperand(vm);
        localCount = scriptVmReadIntegerOperand(vm);
        frameIndex = scriptVmReadIntegerOperand(vm);
        vm->stackDepth = (vm->stackDepth - argumentCount);
        scriptVmSaveStackDepth(vm);
        _bzero(((u8 *)vm->values) + ((vm->stackDepth + argumentCount) * SCRIPT_VALUE_BYTES),
               (localCount - argumentCount) * SCRIPT_VALUE_BYTES);
        localSlot = &vm->localFrameBases[frameIndex];
        previousFrameBase = *localSlot;
        *localSlot = vm->stackDepth;
        vm->stackDepth = (vm->stackDepth + localCount);
        scriptVmPushInteger(vm, previousFrameBase);
        scriptVmPushInteger(vm, frameIndex);
        goto loop_2;
    }
    default:
    block_29:
        return SCRIPT_VM_ERROR;
    }
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Advance the actor script execution and timers for the current frame.
/// \param actor Actor whose script context is advanced when present.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80116658 (0x80116658).
/// \see actorRunScriptVm
/// \see actorScriptDispatchActionHandler
void actorUpdateScriptVm(u8 *actor) {
    u8 *st;
    ScriptVm *ctx;
    s32 state;

    st = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ctx = (ScriptVm *)((ActorScriptState *)st)->vmAddress;
    if (ctx != NULL) {
        state = ctx->executionState;
        ctx->elapsed += gRuntimeFrameSeconds;
        ctx->totalElapsed += gRuntimeFrameSeconds;
        if (state == SCRIPT_VM_WAITING_FOR_ACTION) {
            ctx->executionState = actorScriptDispatchActionHandler(actor);
            ctx->actionInitializing = 0;
        } else if ((state == SCRIPT_VM_YIELDED) || (state == SCRIPT_VM_READY)) {
            ctx->executionState = actorRunScriptVm(actor);
        } else if (state == SCRIPT_VM_FINISHED) { /* same as default; the ROM tests it */
            ctx->executionState = SCRIPT_VM_READY;
        } else {
            ctx->executionState = SCRIPT_VM_READY;
        }
    }
}

/// Reset all global script variables to their initial values.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80116708 (0x80116708).
void scriptVmResetGlobals(void) {
    _bzero(&gScriptVmGlobalValues, sizeof(gScriptVmGlobalValues));
}
