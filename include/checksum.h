/// \file
/// Declares checksum calculation for saved byte buffers.

#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <ultra64.h>

/// Compute the standard CRC-32 checksum of a byte buffer.
/// \param len Number of bytes to checksum; nonpositive lengths produce the empty checksum.
/// \param buf Input byte buffer containing at least len bytes when len is positive.
/// \returns The CRC-32 checksum.
/// \note Original decompilation identifier: \c func_8000DD9C (0x8000DD9C).
u32 checksumCrc32(s32 len, u8 *buf);

#endif
