/// \file
/// Declares numeric and borrowed-string stack operations for actor script contexts.

#ifndef SCRIPT_VM_STACK_H
#define SCRIPT_VM_STACK_H

#include <ultra64.h>

/// Push an integer value onto a script stack.
/// \param vm Context address in the original 32-bit ABI, with room for another value.
/// \param value Integer value to copy.
/// \returns Nothing.
void scriptVmPushInteger(s32 vm, s32 value);

/// Push a floating-point value onto a script stack.
/// \param vm Context address in the original 32-bit ABI, with room for another value.
/// \param value Floating-point value to copy.
/// \returns Nothing.
void scriptVmPushFloat(s32 vm, f32 value);

/// Push a borrowed string reference onto a script stack.
/// \param vm Context address in the original 32-bit ABI, with room for another value.
/// \param text String address in the original 32-bit ABI; storage must outlive use of the value.
/// \returns Nothing.
void scriptVmPushStringPointer(s32 vm, s32 text);

/// Remove a numeric value from a script stack and convert it to an integer.
/// \param vm Context address in the original 32-bit ABI, with a numeric value on top.
/// \returns Integer value, discarding any fractional part of a floating-point value.
s32 scriptVmPopInteger(s32 vm);

/// Remove a numeric value from a script stack and convert it to a floating-point number.
/// \param vm Context address in the original 32-bit ABI, with a numeric value on top.
/// \returns Converted floating-point value.
f32 scriptVmPopFloat(s32 vm);

/// Remove a script value and obtain its string representation.
/// \param vm Context with a valid string or numeric value on top; formatted
/// numeric text must fit in 32 bytes including the terminator.
/// \returns Borrowed string, or shared numeric text overwritten by the next
/// numeric string conversion. Consume or copy numeric text before that conversion.
/// \note Original identifier: \c func_80116104 (0x80116104).
char *scriptVmPopString(u8 *vm);

#endif
