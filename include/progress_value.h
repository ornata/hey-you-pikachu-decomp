/// \file
/// Declares saved progress-field reads, including current-area selectors.

#ifndef PROGRESS_VALUE_H
#define PROGRESS_VALUE_H

#include "ultra64.h"

/// Gets the saved value of a progress field.
/// \param field Valid progress-field ID or area-relative selector that resolves to a field in the current area.
/// \returns The current field value.
/// \note Original decompilation identifier: \c __osSpSetPc (0x80032C1C).
s32 progressGetValue(u32 field);

#endif
