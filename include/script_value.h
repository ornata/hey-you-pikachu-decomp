/// \file
/// Reads and assigns packed actor-script numeric values and borrowed strings.

#ifndef SCRIPT_VALUE_H
#define SCRIPT_VALUE_H

#include "script_vm.h"

/// Read a numeric script value as a floating-point number.
/// \param value Valid numeric value to read.
/// \returns Floating-point value, converting integer values when necessary.
/// \note Original identifier: \c func_8011549C (0x8011549C).
f32 scriptValueGetFloat(ScriptValue *value);

/// Assign a floating-point value to a script value.
/// \param value Destination value to replace.
/// \param number Floating-point number to assign.
/// \returns Nothing.
/// \note Original identifier: \c func_8011550C (0x8011550C).
void scriptValueSetFloat(ScriptValue *value, f32 number);

/// Assign an integer to a script value.
/// \param value Destination value to replace.
/// \param number Integer number to assign.
/// \returns Nothing.
/// \note Original identifier: \c func_80115544 (0x80115544).
void scriptValueSetInteger(ScriptValue *value, s32 number);

/// Assign a borrowed string reference to a script value.
/// \param value Destination value to replace.
/// \param text Borrowed null-terminated string; its storage
/// must remain valid while the script value is used.
/// \returns Nothing.
/// \note Original identifier: \c func_80115578 (0x80115578).
void scriptValueSetStringPointer(ScriptValue *value, char *text);

#endif
