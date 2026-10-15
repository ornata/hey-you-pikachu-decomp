/// \file
/// Defines storage widths and permitted ranges for saved progress-field types.

#ifndef PROGRESS_FIELD_TYPES_H
#define PROGRESS_FIELD_TYPES_H

#include "ultra64.h"

/// Encodings used by the progress-field catalog and its value accessors.
/// Catalog entries retain their signed-word storage representation.
typedef enum ProgressFieldEncoding {
    PROGRESS_ENCODING_S32 = 0,     ///< Signed 32-bit value.
    PROGRESS_ENCODING_S16 = 1,     ///< Signed 16-bit value; unused by the retained retail catalog.
    PROGRESS_ENCODING_U16 = 2,     ///< Unsigned 16-bit value.
    PROGRESS_ENCODING_S8 = 3,      ///< Signed 8-bit value.
    PROGRESS_ENCODING_U8 = 4,      ///< Unsigned 8-bit value.
    PROGRESS_ENCODING_PACKED4 = 5, ///< Unsigned four-bit field within a byte.
    PROGRESS_ENCODING_PACKED2 = 6, ///< Unsigned two-bit field within a byte.
    PROGRESS_ENCODING_PACKED1 = 7, ///< Unsigned one-bit field within a byte.
    PROGRESS_ENCODING_COUNT = 8    ///< Number of supported encodings; not an encoding.
} ProgressFieldEncoding;

/// Storage representation and accepted values for one progress-field type.
typedef struct ProgressFieldTypeLimits {
    s32 bitWidth; ///< Number of stored bits per field.
    s32 minimum;  ///< Smallest accepted field value.
    s32 maximum;  ///< Largest accepted field value.
} ProgressFieldTypeLimits;

typedef char ProgressFieldTypeLimitsSizeCheck[(sizeof(ProgressFieldTypeLimits) == 0xC) ? 1 : -1];
typedef char ProgressFieldMinimumOffsetCheck[(((u32) & ((ProgressFieldTypeLimits *)0)->minimum) == 4) ? 1 : -1];
typedef char ProgressFieldMaximumOffsetCheck[(((u32) & ((ProgressFieldTypeLimits *)0)->maximum) == 8) ? 1 : -1];

/// Storage widths and permitted value ranges indexed by progress field type.
/// \note Original identifier: \c D_8009A6F4 (0x8009A6F4).
extern ProgressFieldTypeLimits gProgressFieldTypeLimits[];

#endif
