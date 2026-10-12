/// \file
/// Declares copying of null-terminated encoded byte strings.

#ifndef STRING_BYTES_H
#define STRING_BYTES_H

#include <ultra64.h>

/// Copy a null-terminated byte string, including its terminator.
/// \param dst Writable destination with room for the source and terminator.
/// \param src Borrowed source string; source and destination must not overlap.
/// \returns The original destination pointer.
/// \note Original decompilation identifier: \c func_80049A78 (0x80049A78).
u8 *stringCopy(u8 *dst, const u8 *src);

#endif
