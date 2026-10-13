/// \file
/// Declares decompression of cartridge archive members.

#ifndef ARCHIVE_COMPRESSION_H
#define ARCHIVE_COMPRESSION_H

#include <ultra64.h>

/// Decode an archive member stored verbatim or compressed with LZ/Huffman codes.
/// Callers must serialize decoding.
/// \param input Complete encoded member, including its eight-byte header.
/// \param output Writable buffer of at least the decoded size in input[4..7].
/// \returns Decoded byte count from the member header. Malformed input and
/// buffer bounds are not validated.
///
/// \note NONPORTABLE: IDO schedules the first refill stores by source line.
/// \note Original decompilation identifier: \c archiveDecompressMember (0x80049F74).
s32 archiveDecompressMember(u8 *input, u8 *output);

#endif
