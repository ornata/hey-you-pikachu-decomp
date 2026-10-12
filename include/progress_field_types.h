/// \file
/// Defines storage widths and permitted ranges for saved progress-field types.

#ifndef PROGRESS_FIELD_TYPES_H
#define PROGRESS_FIELD_TYPES_H

#include "ultra64.h"

/// Storage representation and accepted values for one progress-field type.
typedef struct ProgressFieldTypeLimits {
    s32 bitWidth; ///< Number of stored bits per field.
    s32 minimum; ///< Smallest accepted field value.
    s32 maximum; ///< Largest accepted field value.
} ProgressFieldTypeLimits;

typedef char ProgressFieldTypeLimitsSizeCheck[(sizeof(ProgressFieldTypeLimits) == 0xC) ? 1 : -1];
typedef char ProgressFieldMinimumOffsetCheck[(((u32)&((ProgressFieldTypeLimits *)0)->minimum) == 4) ? 1 : -1];
typedef char ProgressFieldMaximumOffsetCheck[(((u32)&((ProgressFieldTypeLimits *)0)->maximum) == 8) ? 1 : -1];

/// Storage widths and permitted value ranges indexed by progress field type.
/// \note Original identifier: \c D_8009A6F4 (0x8009A6F4).
extern ProgressFieldTypeLimits gProgressFieldTypeLimits[];

#endif
