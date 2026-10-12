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
void scriptVmReset(u8 *vm) {
    if (vm != NULL) {
        ((ScriptVm *)vm)->executionState = SCRIPT_VM_READY;
        ((ScriptVm *)vm)->instructionOffset = 0;
        ((ScriptVm *)vm)->stackDepth = 0;
        ((ScriptVm *)vm)->elapsed = 0.0f;
        _bzero(((ScriptVm *)vm)->values, ((ScriptVm *)vm)->stackCapacity * sizeof(ScriptValue));
        ((ScriptVm *)vm)->savedDepthCount = 0;
        _bzero(((ScriptVm *)vm)->savedDepths, sizeof(((ScriptVm *)vm)->savedDepths));
        ((ScriptVm *)vm)->returnCount = 0;
        _bzero(((ScriptVm *)vm)->returnOffsets, sizeof(((ScriptVm *)vm)->returnOffsets));
        _bzero(((ScriptVm *)vm)->localFrameBases, sizeof(((ScriptVm *)vm)->localFrameBases));
        ((ScriptVm *)vm)->unknown2B0 = 0;
        ((ScriptVm *)vm)->messageStyle = 0;
        ((ScriptVm *)vm)->actionHandler = 0;
        ((ScriptVm *)vm)->actionInitializing = 0;
        ((ScriptVm *)vm)->messageDuration = 3.5f;
        ((ScriptVm *)vm)->actionTimer = 0.0f;
    }
}

/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;
/// Create a script context with a specified value-stack capacity.
/// \param config Configuration supplying borrowed program, string, and command tables that must outlive the context.
/// \param capacity Positive maximum number of values in the script stack.
/// \returns An owned initialized context to release with scriptVmDestroy.
/// \note Original decompilation identifier: \c func_80115114 (0x80115114).
u8 *scriptVmCreateWithCapacity(ScriptVmConfiguration *config, s32 capacity) {
    u8 *vm;
    vm = heapAllocLocal((HpHdr *)&gGameHeap, sizeof(ScriptVm), 0xB3);
    _bzero(vm, sizeof(ScriptVm));
    ((ScriptVm *)vm)->totalElapsed = 0.0f;
    ((ScriptVm *)vm)->commands = config->commands;
    ((ScriptVm *)vm)->program = config->program;
    ((ScriptVm *)vm)->stringTable = config->stringTable;
    ((ScriptVm *)vm)->stackCapacity = capacity;
    ((ScriptVm *)vm)->values = (ScriptValue *)heapAllocLocal((HpHdr *)&gGameHeap, capacity * sizeof(ScriptValue), 0xC6);
    scriptVmReset(vm);
    return vm;
}

/// Create a script context using its configured stack capacity.
/// \param config Configuration with borrowed program, string, and command tables that must outlive the context; a nonpositive stack capacity selects 200 values.
/// \returns An owned initialized context to release with scriptVmDestroy.
/// \note Original decompilation identifier: \c func_801151B0 (0x801151B0).
u8 *scriptVmCreate(ScriptVmConfiguration *config) {
    s32 configuredCapacity = config->stackCapacity;
    s32 capacity;

    if (configuredCapacity <= 0) {
        TRACE(("WARNING : stack size out of range, %d\n", configuredCapacity));
        capacity = 0xC8;
    } else {
        capacity = configuredCapacity;
    }
    return scriptVmCreateWithCapacity(config, capacity);
}


/// Release a script context and its owned storage.
/// \param vm Script context to release, or null.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801151E4 (0x801151E4).
void scriptVmDestroy(u8 *vm) {
    if (vm != NULL) {
        heapFreeLocal(&gGameHeap, ((ScriptVm *)vm)->values);
        heapFreeLocal(&gGameHeap, vm);
    }
}


#define VM_SP(vm) (((ScriptVm *)(vm))->stackDepth)
#define VM_STACK(vm) (((ScriptVm *)(vm))->values)

/// Push a typed value onto the script stack.
/// \param vm Script context with room for another value.
/// \param value Typed value to copy onto the stack.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115228 (0x80115228).
void scriptVmPushValue(u8 *vm, ScriptValue *value) {
    s32 i;

    for (i = 0; i < 5; i++) {
        VM_STACK(vm)[VM_SP(vm)].bytes[i] = value->bytes[i];
    }
    VM_SP(vm)++;
}

/// Remove the top typed value from the script stack.
/// \param vm Script context with a nonempty value stack.
/// \param value Output receiving the removed typed value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801152DC (0x801152DC).
void scriptVmPopValue(u8 *vm, ScriptValue *value) {
    s32 i;

    VM_SP(vm)--;
    for (i = 0; i < 5; i++) {
        value->bytes[i] = VM_STACK(vm)[VM_SP(vm)].bytes[i];
    }
}

/// Save the script value-stack depth for later restoration.
/// \param vm Script context with fewer than 64 saved stack depths.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011538C (0x8011538C).
void scriptVmSaveStackDepth(u8 *vm) {
    ((ScriptVm *)vm)->savedDepths[((ScriptVm *)vm)->savedDepthCount] = (s32)((ScriptVm *)vm)->stackDepth;
    ((ScriptVm *)vm)->savedDepthCount = (s32)(((ScriptVm *)vm)->savedDepthCount + 1);
}

/// Restore the most recently saved script value-stack depth.
/// \param vm Script context with a saved stack depth to restore.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801153B0 (0x801153B0).
void scriptVmRestoreStackDepth(u8 *vm) {
    ((ScriptVm *)vm)->savedDepthCount--;
    ((ScriptVm *)vm)->stackDepth = ((ScriptVm *)vm)->savedDepths[((ScriptVm *)vm)->savedDepthCount];
}

/// Save the current script instruction location for a later return.
/// \param vm Script context with fewer than 64 saved return locations.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801153D0 (0x801153D0).
void scriptVmSaveReturnLocation(u8 *vm) {
    ((ScriptVm *)vm)->returnOffsets[((ScriptVm *)vm)->returnCount] = (s32)((ScriptVm *)vm)->instructionOffset;
    ((ScriptVm *)vm)->returnCount = (s32)(((ScriptVm *)vm)->returnCount + 1);
}

/// Restore the most recent script return location when one is available.
/// \param vm Script context whose return location is restored.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801153F4 (0x801153F4).
void scriptVmRestoreReturnLocation(u8 *vm) {
    if (((ScriptVm *)vm)->returnCount > 0) {
        ((ScriptVm *)vm)->returnCount--;
        ((ScriptVm *)vm)->instructionOffset = ((ScriptVm *)vm)->returnOffsets[((ScriptVm *)vm)->returnCount];
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
/// \note UB: K&R definition; scriptVmReadIntegerOperand calls it with a second argument
/// \note Original decompilation identifier: \c func_80115424 (0x80115424).
s32 scriptValueGetInteger(value)
u8 *value;
{
    s32 sp24;
    f32 sp20;
    s32 is_int;

    if ((*value == 0) || (*value == 2)) {
        is_int = 1;
    } else {
        is_int = 0;
    }
    if (is_int != 0) {
        bcopy(value + 1, &sp24, 4);
        return sp24;
    }
    bcopy(value + 1, &sp20, 4);
    return sp20;
}

/// Read a numeric script value as a floating-point number.
/// \param value Numeric script value to read.
/// \returns The floating-point value.
/// \note Original decompilation identifier: \c func_8011549C (0x8011549C).
f32 scriptValueGetFloat(s8 *value) {
    f32 f;
    s32 i;
    s32 isFloat;

    if (*(u8 *)value == 1) {
        isFloat = 1;
    } else {
        isFloat = 0;
    }
    if (isFloat) {
        bcopy(value + 1, &f, 4);
        return f;
    }
    bcopy(value + 1, &i, 4);
    return i;
}

/// Assign a floating-point number to a script value.
/// \param value Script value to replace.
/// \param number Floating-point number to assign.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011550C (0x8011550C).
void scriptValueSetFloat(s8 *value, f32 number) {
    *value = SCRIPT_VALUE_FLOAT;
    bcopy(&number, value + 1, 4);
}

/// Assign an integer to a script value.
/// \param value Script value to replace.
/// \param number Integer to assign.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115544 (0x80115544).
void scriptValueSetInteger(s8 *value, s32 number) {
    *value = SCRIPT_VALUE_INTEGER;
    bcopy(&number, value + 1, 4);
}

/// Assign a string pointer to a script value.
/// \param value Script value to replace.
/// \param text Borrowed string address that must remain valid while the value is used.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115578 (0x80115578).
void scriptValueSetStringPointer(s8 *value, s32 text) {
    *value = SCRIPT_VALUE_STRING_POINTER;
    bcopy(&text, value + 1, 4);
}

s32 stdlibParseUnsignedDecimal(s32); /* extern */

/// Parse a decimal number from a script string value.
/// \param vm Script context used to resolve string-table references.
/// \param value Null-terminated string value or valid string-table reference to parse.
/// \returns The parsed decimal value, or zero when the value is not a string.
/// \note Original decompilation identifier: \c func_801155B0 (0x801155B0).
s32 scriptValueParseDecimal(u8 *vm, ScriptValue *value) {
    s32 sp2C;
    s32 ok;
    s32 sp24;
    s32 is_ptr;
    s32 ret;

    if ((value->data.tag == SCRIPT_VALUE_STRING_INDEX) || (value->data.tag == SCRIPT_VALUE_STRING_POINTER)) {
        ok = 1;
    } else {
        ok = 0;
    }
    if (ok == 0) {
        return 0;
    }
    if (value->data.tag == SCRIPT_VALUE_STRING_POINTER) {
        is_ptr = 1;
    } else {
        is_ptr = 0;
    }
    if (is_ptr != 0) {
        bcopy(&value->bytes[1], &sp24, 4);
        ret = stdlibParseUnsignedDecimal(sp24);
    } else {
        bcopy(&value->bytes[1], &sp2C, 4);
        sp24 = ((s32 *)((ScriptVm *)vm)->stringTable)[sp2C];
        ret = stdlibParseUnsignedDecimal(sp24);
    }
    return ret;
}

/// Compatibility declaration for the retained extra context argument in operand decoding.
s32 scriptValueGetInteger(); /* extern */
/// Read the next script operand as an integer and advance execution past it.
/// \param vm Script context positioned at a valid numeric operand.
/// \returns The operand converted to an integer.
/// \note Original decompilation identifier: \c func_8011566C (0x8011566C).
s32 scriptVmReadIntegerOperand(u8 *vm) {
    s32 operandOffset;
    s32 operandAddress;

    operandOffset = ((ScriptVm *)vm)->instructionOffset;
    operandAddress = (s32)((ScriptVm *)vm)->program + operandOffset;
    ((ScriptVm *)vm)->instructionOffset = operandOffset + 5;
    return scriptValueGetInteger(operandAddress, vm);
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
u8 *scriptVmReadVariableReference(u8 *vm) {
    s32 frame;
    s32 idx;
    u8 *box;

    frame = scriptVmReadIntegerOperand(vm);
    idx = scriptVmReadIntegerOperand(vm);
    if (frame >= 0) {
        idx = ((s32 *)(vm + 0x22C))[frame] + idx;
        box = (u8 *)((ScriptVm *)vm)->values + idx * 5;
    } else if (frame == -1) {
        box = (u8 *)&gScriptVmGlobalValues + idx * 5;
    }
    return box;
}

/// Replace the top script value with the result of a unary operation.
/// \param vm Context with a numeric or valid decimal-string operand on its stack; integer negation requires a representable result.
/// \param op Unary operation: 3 negates, 4 complements, and 17 logically negates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115724 (0x80115724).
void scriptVmApplyUnaryOperator(u8 *vm, u8 op) {
    ScriptValue *box;
    s32 test;
    f32 f;
    s32 i;
    box = VM_STACK(vm) + VM_SP(vm) - 1;
    if (box->data.tag == SCRIPT_VALUE_STRING_INDEX || box->data.tag == SCRIPT_VALUE_STRING_POINTER)
        test = 1;
    else
        test = 0;
    if (test)
        scriptValueSetInteger((s8 *)box, scriptValueParseDecimal(vm, box));
    if (box->data.tag == SCRIPT_VALUE_FLOAT)
        test = 1;
    else
        test = 0;
    if (test) {
        f = scriptValueGetFloat((s8 *)box);
        switch (op) {
        case 3:
            scriptValueSetFloat((s8 *)box, -f);
            break;
        case 0x11:
            scriptValueSetFloat((s8 *)box, !f);
            break;
        case 4:
            scriptValueSetInteger((s8 *)box, 0);
            break;
        }
    } else {
        i = scriptValueGetInteger(box->bytes);
        switch (op) {
        case 3:
            scriptValueSetInteger((s8 *)box, -i);
            break;
        case 0x11:
            scriptValueSetInteger((s8 *)box, !i);
            break;
        case 4:
            scriptValueSetInteger((s8 *)box, ~i);
            break;
        }
    }
}

/// Replace two script operands with their arithmetic result.
/// \param vm Context with a numeric left operand and a numeric or valid decimal-string right operand.
/// \pre Integer results must be representable; integer division and remainder require a nonzero divisor and exclude the minimum integer divided by -1.
/// \param op Arithmetic operation: 5 adds, 6 subtracts, 7 multiplies, 8 divides, and 9 computes integer remainder or floating-point zero.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801158D0 (0x801158D0).
void scriptVmApplyArithmeticOperator(u8 *vm, u8 op) {
    ScriptValue right, left, result;
    f32 fv, fr, fl;
    s32 iv, ir, il;
    scriptVmPopValue(vm, &right);
    if ((right.data.tag == SCRIPT_VALUE_STRING_INDEX || right.data.tag == SCRIPT_VALUE_STRING_POINTER) ? 1 : 0)
        scriptValueSetInteger((s8 *)&right, scriptValueParseDecimal(vm, &right));
    scriptVmPopValue(vm, &left);
    if ((right.data.tag == SCRIPT_VALUE_STRING_INDEX || right.data.tag == SCRIPT_VALUE_STRING_POINTER) ? 1 : 0)
        scriptValueSetInteger((s8 *)&right, scriptValueParseDecimal(vm, &right));
    if ((right.data.tag == SCRIPT_VALUE_FLOAT ? 1 : 0) || (left.data.tag == SCRIPT_VALUE_FLOAT ? 1 : 0)) {
        fr = scriptValueGetFloat((s8 *)&right);
        fl = scriptValueGetFloat((s8 *)&left);
        switch (op) {
        case 5:
            fv = fl + fr;
            break;
        case 6:
            fv = fl - fr;
            break;
        case 7:
            fv = fl * fr;
            break;
        case 8:
            fv = fl / fr;
            break;
        case 9:
            fv = 0.0f;
            break;
        }
        scriptValueSetFloat((s8 *)&result, fv);
    } else {
        ir = scriptValueGetInteger(right.bytes);
        il = scriptValueGetInteger(left.bytes);
        switch (op) {
        case 5:
            iv = il + ir;
            break;
        case 6:
            iv = il - ir;
            break;
        case 7:
            iv = il * ir;
            break;
        case 8:
            iv = il / ir;
            break;
        case 9:
            iv = il % ir;
            break;
        }
        scriptValueSetInteger((s8 *)&result, iv);
    }
    scriptVmPushValue(vm, &result);
}

/// Replace two script operands with their bitwise-operation result.
/// \param vm Context with a numeric left operand and a numeric or valid decimal-string right operand; either floating-point operand produces zero.
/// \pre Integer shift counts must be 0 through 31; left shifts require a nonnegative operand and a representable result.
/// \param op Bitwise operation: 20 AND, 21 OR, 22 right shift, or 23 left shift.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115B38 (0x80115B38).
void scriptVmApplyBitwiseOperator(u8 *vm, u8 op) {
    ScriptValue right, left, result;
    s32 value, rhs, lhs;
    scriptVmPopValue(vm, &right);
    if ((right.data.tag == SCRIPT_VALUE_STRING_INDEX || right.data.tag == SCRIPT_VALUE_STRING_POINTER) ? 1 : 0)
        scriptValueSetInteger((s8 *)&right, scriptValueParseDecimal(vm, &right));
    scriptVmPopValue(vm, &left);
    if ((right.data.tag == SCRIPT_VALUE_STRING_INDEX || right.data.tag == SCRIPT_VALUE_STRING_POINTER) ? 1 : 0)
        scriptValueSetInteger((s8 *)&right, scriptValueParseDecimal(vm, &right));
    if ((right.data.tag == SCRIPT_VALUE_FLOAT ? 1 : 0) || (left.data.tag == SCRIPT_VALUE_FLOAT ? 1 : 0)) {
        scriptValueSetInteger((s8 *)&result, 0);
    } else {
        rhs = scriptValueGetInteger(right.bytes);
        lhs = scriptValueGetInteger(left.bytes);
        switch (op) {
        case 20:
            value = lhs & rhs;
            break;
        case 21:
            value = lhs | rhs;
            break;
        case 22:
            value = lhs >> rhs;
            break;
        case 23:
            value = lhs << rhs;
            break;
        }
        scriptValueSetInteger((s8 *)&result, value);
    }
    scriptVmPushValue(vm, &result);
}

f32 fabsf(f32);
#pragma intrinsic(fabsf)
/// Replace two script operands with an integer comparison or logical result.
/// \param vm Context with a numeric left operand and a numeric or valid decimal-string right operand.
/// \pre Floating-point operands of logical operations must be representable as integers; their fractional parts do not affect truth values.
/// \param op Operation: 11 equality, 12 inequality, 13 greater, 14 less, 15 greater-or-equal, 16 less-or-equal, 18 logical AND, or 19 logical OR.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80115CBC (0x80115CBC).
void scriptVmApplyComparisonOperator(u8 *vm, u8 op) {
    ScriptValue right, left, result;
    s32 value;
    f32 fr, fl;
    s32 ir, il;
    scriptVmPopValue(vm, &right);
    if ((right.data.tag == SCRIPT_VALUE_STRING_INDEX || right.data.tag == SCRIPT_VALUE_STRING_POINTER) ? 1 : 0)
        scriptValueSetInteger((s8 *)&right, scriptValueParseDecimal(vm, &right));
    scriptVmPopValue(vm, &left);
    if ((right.data.tag == SCRIPT_VALUE_STRING_INDEX || right.data.tag == SCRIPT_VALUE_STRING_POINTER) ? 1 : 0)
        scriptValueSetInteger((s8 *)&right, scriptValueParseDecimal(vm, &right));
    value = 0;
    if ((right.data.tag == SCRIPT_VALUE_FLOAT ? 1 : 0) || (left.data.tag == SCRIPT_VALUE_FLOAT ? 1 : 0)) {
        fr = scriptValueGetFloat((s8 *)&right);
        fl = scriptValueGetFloat((s8 *)&left);
        switch (op) {
        case 11:
            if (fabsf(fr - fl) <= fabsf(fl) * 0.001f + 0.001f)
                value = 1;
            break;
        case 12:
            if (!(fabsf(fr - fl) <= fabsf(fl) * 0.001f + 0.001f))
                value = 1;
            break;
        case 13:
            if (fr < fl)
                value = 1;
            break;
        case 14:
            if (fl < fr)
                value = 1;
            break;
        case 15:
            if (fr <= fl)
                value = 1;
            break;
        case 16:
            if (fl <= fr)
                value = 1;
            break;
        case 18:
            value = (s32)fr && (s32)fl;
            break;
        case 19:
            value = (s32)fr || (s32)fl;
            break;
        }
    } else {
        ir = scriptValueGetInteger(right.bytes);
        il = scriptValueGetInteger(left.bytes);
        switch (op) {
        case 11:
            if (il == ir)
                value = 1;
            break;
        case 12:
            if (il != ir)
                value = 1;
            break;
        case 13:
            if (ir < il)
                value = 1;
            break;
        case 14:
            if (il < ir)
                value = 1;
            break;
        case 15:
            if (il >= ir)
                value = 1;
            break;
        case 16:
            if (ir >= il)
                value = 1;
            break;
        case 18:
            value = ir && il;
            break;
        case 19:
            value = ir || il;
            break;
        }
    }
    scriptValueSetInteger((s8 *)&result, value);
    scriptVmPushValue(vm, &result);
}


/// Push an integer onto the script value stack.
/// \param vm Address of a script context with stack capacity available.
/// \param value Integer to push.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011601C (0x8011601C).
void scriptVmPushInteger(s32 vm, s32 value) {
    ScriptValue valueBox;

    scriptValueSetInteger((s8 *)&valueBox, value);
    scriptVmPushValue((u8 *)vm, &valueBox);
}

void scriptValueSetFloat(s8 *arg0, f32 arg1);  /* extern */
/// Push a floating-point number onto the script value stack.
/// \param vm Address of a script context with stack capacity available.
/// \param value Floating-point number to push.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011604C (0x8011604C).
void scriptVmPushFloat(s32 vm, f32 value) {
    ScriptValue valueBox;

    scriptValueSetFloat((s8 *)&valueBox, value);
    scriptVmPushValue((u8 *)vm, &valueBox);
}

/// Push a string pointer onto the script value stack.
/// \param vm Address of a script context with stack capacity available.
/// \param text Borrowed string address that must remain valid while the stacked value is used.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80116084 (0x80116084).
void scriptVmPushStringPointer(s32 vm, s32 text) {
    ScriptValue valueBox;

    scriptValueSetStringPointer((s8 *)&valueBox, text);
    scriptVmPushValue((u8 *)vm, &valueBox);
}

void scriptVmPopValue(u8 *arg0, ScriptValue *arg1); /* extern */
/// Pop a numeric script value and convert it to an integer.
/// \param vm Address of a script context with a numeric value on its stack.
/// \returns The popped integer value, with any floating-point fraction discarded.
/// \note Original decompilation identifier: \c func_801160B4 (0x801160B4).
s32 scriptVmPopInteger(s32 vm) {
    ScriptValue valueBox;

    scriptVmPopValue((u8 *)vm, &valueBox);
    return scriptValueGetInteger((s8 *)&valueBox);
}

f32 scriptValueGetFloat(s8 *arg0);             /* extern */

/// Pop a numeric script value and convert it to a floating-point number.
/// \param vm Address of a script context with a numeric value on its stack.
/// \returns The popped floating-point value.
/// \note Original decompilation identifier: \c func_801160DC (0x801160DC).
f32 scriptVmPopFloat(s32 vm) {
    ScriptValue valueBox;

    scriptVmPopValue((u8 *)vm, &valueBox);
    return scriptValueGetFloat((s8 *)&valueBox);
}

/// Pop a script value and obtain its string representation.
/// \param vm Context with a valid string or numeric value on its stack; numeric text must fit in 32 bytes including its terminator.
/// \returns The borrowed referenced string, or shared temporary text for a numeric value that a later numeric conversion overwrites.
/// \note Original decompilation identifier: \c func_80116104 (0x80116104).
char *scriptVmPopString(u8 *vm) {
    ScriptValue box;
    char *text;
    s32 index;
    s32 test;
    scriptVmPopValue(vm, &box);
    if (box.data.tag == SCRIPT_VALUE_STRING_INDEX || box.data.tag == SCRIPT_VALUE_STRING_POINTER)
        test = 1;
    else
        test = 0;
    if (test) {
        if (box.data.tag == SCRIPT_VALUE_STRING_POINTER)
            test = 1;
        else
            test = 0;
        if (test) {
            bcopy(&box.bytes[1], &text, 4);
            return text;
        }
        bcopy(&box.bytes[1], &index, 4);
        return (*(char ***)(vm + 8))[index];
    }
    if (box.data.tag == SCRIPT_VALUE_INTEGER || box.data.tag == 2)
        test = 1;
    else
        test = 0;
    if (test)
        sprintf((char *)&gScriptVmNumericTextBuffer, "%d", scriptValueGetInteger(box.bytes));
    else
        sprintf((char *)&gScriptVmNumericTextBuffer, "%f", scriptValueGetFloat((s8 *)box.bytes));
    return (char *)&gScriptVmNumericTextBuffer;
}


/// Evaluate a global script variable as an integer.
/// \param index Valid index of a numeric global script variable.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80116224 (0x80116224).
void scriptVmEvaluateGlobalInteger(s32 index) {
    scriptValueGetInteger((s8 *)((u8 *)&gScriptVmGlobalValues + index * 5));
}


/// Assign an integer to a global script variable.
/// \param index Valid global script-variable index.
/// \param value Integer to assign.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80116254 (0x80116254).
void scriptVmSetGlobalInteger(s32 index, s32 value) {
    scriptValueSetInteger((s8 *)((u8 *)&gScriptVmGlobalValues + index * 5), value);
}

s32 messageBarEnqueue(s32, f32, u8 *);
s32 actorScriptDispatchRegisteredCommand(u8 *);
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

    owner = *(u8 **)(actor + 0x54);
    vm = *(ScriptVm **)(owner + 0x1B8);
    if (vm == NULL) {
        messageBarEnqueue(0, 3600.0f, (u8 *)"No PPU");
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
    case 31: {
        ScriptValue sp98;
        scriptVmPopValue((u8 *)vm, &sp98);
        goto loop_2;
    }
    case 3:
    case 4:
    case 17:
        scriptVmApplyUnaryOperator((u8 *)vm, op);
        goto loop_2;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        scriptVmApplyArithmeticOperator((u8 *)vm, op);
        goto loop_2;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 18:
    case 19:
        scriptVmApplyComparisonOperator((u8 *)vm, op);
        goto loop_2;
    case 20:
    case 21:
    case 22:
    case 23:
        scriptVmApplyBitwiseOperator((u8 *)vm, op);
        goto loop_2;
    case 24:
        scriptVmPushValue((u8 *)vm, (ScriptValue *)(vm->program + vm->instructionOffset));
        vm->instructionOffset = (s32)(vm->instructionOffset + 5);
        goto loop_3;
    case 25:
        scriptVmPushValue((u8 *)vm, (ScriptValue *)scriptVmReadVariableReference((u8 *)vm));
        goto loop_2;
    case 10:
        scriptVmPopValue((u8 *)vm, (ScriptValue *)scriptVmReadVariableReference((u8 *)vm));
        goto loop_2;
    case 33: {
        u8 *elements;
        s32 elementIndex;
        elements = scriptVmReadVariableReference((u8 *)vm);
        elementIndex = scriptVmPopInteger((s32)vm);
        scriptVmPushValue((u8 *)vm, (ScriptValue *)((u8(*)[5])elements)[elementIndex]);
        goto loop_2;
    }
    case 34: {
        u8 *elements;
        s32 i;
        s32 elementIndex;
        ScriptValue value;
        elements = scriptVmReadVariableReference((u8 *)vm);
        scriptVmPopValue((u8 *)vm, &value);
        elementIndex = scriptVmPopInteger((s32)vm);
        for (i = 0; i < 5; i++) {
            ((u8(*)[5])elements)[elementIndex][i] = value.bytes[i];
        }
        goto loop_2;
    }
    case 35:
        vm->instructionOffset = 0;
        vm->stackDepth = 0;
        vm->savedDepthCount = 0;
        vm->returnCount = 0;
        TRACE(("PPU - process end.\n"));
        return SCRIPT_VM_FINISHED;
    case 28: {
        s32 address;
        TRACE(("%d\n", vm->instructionOffset));
        address = scriptValueGetInteger((s8 *)(vm->program + vm->instructionOffset));
        vm->instructionOffset = address;
        goto loop_2;
    }
    case 29: {
        s32 condition;
        ScriptValue sp6C;
        scriptVmPopValue((u8 *)vm, &sp6C);
        condition = scriptValueGetInteger((s8 *)sp6C.bytes);
        if (condition == 0) {
            vm->instructionOffset = scriptValueGetInteger((s8 *)(vm->program + vm->instructionOffset));
        } else {
            vm->instructionOffset = (s32)(vm->instructionOffset + 5);
        }
        goto loop_2;
    }
    case 27: {
        s32 commandStatus;
        commandStatus = actorScriptDispatchRegisteredCommand(actor);
        if (commandStatus == 0) {
            goto loop_2;
        }
        if (commandStatus == 2) {
            return SCRIPT_VM_WAITING_FOR_ACTION;
        }
        return SCRIPT_VM_YIELDED;
    }
    case 26: {
        s32 callTargetOffset;
        callTargetOffset = scriptValueGetInteger((s8 *)(vm->program + vm->instructionOffset));
        vm->instructionOffset = (s32)(vm->instructionOffset + 5);
        scriptVmSaveReturnLocation((u8 *)vm);
        vm->instructionOffset = callTargetOffset;
        goto loop_2;
    }
    case 30: {
        s32 restoredFrameIndex;
        ScriptValue sp58;
        s32 restoredFrameBase;
        scriptVmPopValue((u8 *)vm, &sp58);
        restoredFrameIndex = scriptVmPopInteger((s32)vm);
        restoredFrameBase = scriptVmPopInteger((s32)vm);
        scriptVmRestoreStackDepth((u8 *)vm);
        scriptVmPushValue((u8 *)vm, &sp58);
        vm->localFrameBases[restoredFrameIndex] = restoredFrameBase;
        scriptVmRestoreReturnLocation((u8 *)vm);
        goto loop_2;
    }
    case 32: {
        s32 argumentCount;
        s32 localCount;
        s32 frameIndex;
        s32 previousFrameBase;
        s32 *localSlot;
        argumentCount = scriptVmReadIntegerOperand((u8 *)vm);
        localCount = scriptVmReadIntegerOperand((u8 *)vm);
        frameIndex = scriptVmReadIntegerOperand((u8 *)vm);
        vm->stackDepth = (s32)(vm->stackDepth - argumentCount);
        scriptVmSaveStackDepth((u8 *)vm);
        _bzero(((u8 *)vm->values) + ((vm->stackDepth + argumentCount) * 5), (localCount - argumentCount) * 5);
        localSlot = &vm->localFrameBases[frameIndex];
        previousFrameBase = *localSlot;
        *localSlot = vm->stackDepth;
        vm->stackDepth = (s32)(vm->stackDepth + localCount);
        scriptVmPushInteger((s32)vm, previousFrameBase);
        scriptVmPushInteger((s32)vm, frameIndex);
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
s32 actorRunScriptVm(u8 *); /* extern */

/// Advance the actor script execution and timers for the current frame.
/// \param actor Actor whose script context is advanced when present.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80116658 (0x80116658).
/// \see actorRunScriptVm
/// \see actorScriptDispatchActionHandler
void actorUpdateScriptVm(u8 *actor) {
    u8 *st;
    u8 *ctx;
    s32 state;

    st = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ctx = (u8 *)((ActorScriptState *)st)->vmAddress;
    if (ctx != NULL) {
        state = ((ScriptVm *)ctx)->executionState;
        ((ScriptVm *)ctx)->elapsed += gRuntimeFrameSeconds;
        ((ScriptVm *)ctx)->totalElapsed += gRuntimeFrameSeconds;
        if (state == SCRIPT_VM_WAITING_FOR_ACTION) {
            ((ScriptVm *)ctx)->executionState = actorScriptDispatchActionHandler(actor);
            ((ScriptVm *)ctx)->actionInitializing = 0;
        } else if ((state == SCRIPT_VM_YIELDED) || (state == SCRIPT_VM_READY)) {
            ((ScriptVm *)ctx)->executionState = actorRunScriptVm(actor);
        } else if (state == SCRIPT_VM_FINISHED) { /* same as default; the ROM tests it */
            ((ScriptVm *)ctx)->executionState = SCRIPT_VM_READY;
        } else {
            ((ScriptVm *)ctx)->executionState = SCRIPT_VM_READY;
        }
    }
}


/// Reset all global script variables to their initial values.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80116708 (0x80116708).
void scriptVmResetGlobals(void) {
    _bzero(&gScriptVmGlobalValues, 0x9C4);
}
