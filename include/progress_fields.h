/// \file
/// Declares progress-field limits, bounded updates and current-area availability.

#ifndef PROGRESS_FIELDS_H
#define PROGRESS_FIELDS_H

#include "ultra64.h"

/// Gets the minimum permitted value for a progress field.
/// \param fieldId Valid progress-field ID or area-relative selector that resolves to a field in the current area.
/// \returns The field’s minimum permitted value.
/// \note Original decompilation identifier: \c func_80032BCC (0x80032BCC).
s32 progressGetMinimum(s32 fieldId);

/// Gets the maximum permitted value for a progress field.
/// \param fieldId Valid progress-field ID or area-relative selector that resolves to a field in the current area.
/// \returns The field’s maximum permitted value.
/// \note Original decompilation identifier: \c func_80032BF4 (0x80032BF4).
s32 progressGetMaximum(s32 fieldId);

/// Stores a progress-field value within its permitted range.
/// \param field Valid progress-field ID or area-relative selector that resolves to a field in the current area.
/// \param value Requested field value.
/// \returns The value stored after applying the field’s limits.
/// \note Original decompilation identifier: \c func_80032C44 (0x80032C44).
s32 progressSetValue(s32 field, s32 value);

/// Adjusts a progress field by the requested amount within its permitted range.
/// \param field Valid progress-field ID or area-relative selector that resolves to a field in the current area.
/// \param amount Signed amount to add to the current value.
/// \returns The resulting stored value after applying the field’s limits.
/// \note Original decompilation identifier: \c func_80032C70 (0x80032C70).
s32 progressAddValue(s32 field, s32 amount);

/// Checks whether a progress-field selector identifies a field in the current area.
/// \param field Progress-field ID or area-relative field selector to resolve for the current area.
/// \returns One if the selector resolves to a field, or zero if it resolves to no field.
/// \note Original decompilation identifier: \c func_80032CA4 (0x80032CA4).
s32 progressHasFieldInCurrentArea(s32 field);

#endif
