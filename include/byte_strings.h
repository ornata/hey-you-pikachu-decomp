/// \file
/// Declares target byte-string measurement, comparison and character conversion.

#ifndef BYTE_STRINGS_H
#define BYTE_STRINGS_H

#include <ultra64.h>

/// Check whether two byte regions have identical contents.
/// \param first First region to compare.
/// \param second Second region to compare.
/// \param count Nonnegative number of bytes to compare.
/// \returns Zero when the regions match, or a nonzero value otherwise.
/// \note Original decompilation identifier: \c func_800499D0 (0x800499D0).
s32 memoryCompareBytes(u8 *first, u8 *second, s32 count);

/// Measure a null-terminated string.
/// \param s String to measure, or NULL.
/// \returns The number of bytes before the terminator, or zero for NULL.
/// \note Original decompilation identifier: \c strlen (0x80049AAC).
u32 strlen(const u8 *s);

/// Convert a packed JIS character code to the font character encoding.
/// \param code Packed JIS row and cell bytes; high bits are ignored.
/// \returns The packed Shift-JIS character code.
/// \note Original decompilation identifier: \c func_80049BD0 (0x80049BD0).
s32 fontConvertJisToShiftJis(s32 code);

#endif
