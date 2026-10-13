#include "heap_free.h"
#include "heap_tail.h"
#include "common.h"
#include "heap_types.h"
#include "rom_dma.h"
void archiveConsumeBits(s32 bitCount, u8 **source, s32 *bitWindow, s32 *availableBits);

s32 archiveEncodeMemberStub(s32 unusedSourceAddress, s32 unusedSourceBytes, u8 *unusedDestination);
/// Create archive metadata for a collection of source members.
/// This build produces no encoded member payloads.
/// \param memberCount Nonnegative number of source members.
/// \param sourceAddresses Array of source addresses, one per member.
/// \param sourceByteCounts Array of source byte counts, one per member.
/// \param archiveOutput Writable archive output, beginning with the count, member-end
/// offsets, and decoded byte counts; must have space for all generated output.
/// \returns Total archive byte count, including four-byte payload alignment.
/// \note Original decompilation identifier: \c archiveBuildFromSources (0x80049C90).
s32 archiveBuildFromSources(s32 memberCount, s32 *sourceAddresses, s32 *sourceByteCounts, s32 *archiveOutput) {
    s32 *inputLengths;
    s32 *inputSources;
    s32 *ends;
    s32 offset = memberCount;
    s32 i;
    s32 *sizes;
    *archiveOutput = memberCount;
    offset = (offset * 2 + 1) * sizeof(s32);
    i = 0;
    if (memberCount > 0) {
        sizes = (s32 *)((u8 *)archiveOutput + memberCount * sizeof(s32));
        inputLengths = sourceByteCounts;
        inputSources = sourceAddresses;
        ends = archiveOutput;
        do {
            sizes[1] = *inputLengths;
            offset += archiveEncodeMemberStub(*inputSources, *inputLengths, (u8 *)archiveOutput + offset);
            i++;
            if (offset & 3) {
                offset = (offset | 3) + 1;
            }
            sizes++;
            inputLengths++;
            inputSources++;
            ends++;
            *ends = offset;
        } while (i != memberCount);
    }
    return offset;
}

/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;
void *heapAllocLocal(HpHdr *, u32, u16);
s32 heapGetAllocationSize(u8 *, s32);

/// Reuse an output allocation if large enough, otherwise free and replace it.
/// Existing contents are not copied into a replacement allocation.
/// \param outputBuffer Existing game-heap allocation, or NULL to allocate a new buffer.
/// \param requiredBytes Nonnegative required capacity in bytes.
/// \returns Buffer with the requested capacity, or NULL on allocation failure.
/// A failed replacement has already freed the old buffer.
/// \note Original decompilation identifier: \c archiveResizeOutputBuffer (0x80049D64).
void *archiveResizeOutputBuffer(void *outputBuffer, s32 requiredBytes) {
    /* Keep free and allocation on one line: IDO builds a0 before reloading size. */
    if (outputBuffer == NULL) {
        return heapAllocLocal(&gGameHeap, (u32)requiredBytes, 0x3E);
    }
    if (heapGetAllocationSize((u8 *)&gGameHeap, (s32)outputBuffer) < requiredBytes) {
        // clang-format off
        heapFreeLocal(&gGameHeap, outputBuffer); return heapAllocLocal(&gGameHeap, (u32)requiredBytes, 0x41);
        // clang-format on
    }
    return outputBuffer;
}

/// Read one 32-bit value from cartridge ROM synchronously.
/// \param romAddress Cartridge ROM address of the word.
/// \returns The word's native big-endian value.
/// \note Original decompilation identifier: \c romDmaReadWord (0x80049DFC).
s32 romDmaReadWord(s32 romAddress) {
    s32 ret;
    {
        s32 sp18;
        romDmaReadBlocking(romAddress, (u8 *)&sp18, 4);
        ret = sp18;
    }
    return ret;
}

/// Read an archive's header word synchronously and discard its value.
/// \param archiveAddress Cartridge ROM address of the archive header.
/// \returns Nothing.
/// \note Original decompilation identifier: \c archiveReadHeaderWordDiscard (0x80049E24).
void archiveReadHeaderWordDiscard(s32 archiveAddress) {
    romDmaReadWord(archiveAddress);
}

#include "heap_types.h"
s32 archiveGetMemberDecodedSize(s32 memberIndex, s32 *archiveAddress);
s32 archiveDecompressMember(u8 *input, u8 *output);
/// Load and decompress one archive member from ROM into a game-heap buffer.
/// \param memberIndex Zero-based member index; not bounds-checked.
/// \param archiveAddress Cartridge ROM address of the archive header.
/// \param outputBuffer Existing output allocation to reuse or replace, or NULL.
/// \returns Output buffer containing the decoded member. Allocation failures
/// are not handled by this loader; sufficient heap space is required.
/// \note Original decompilation identifier: \c archiveLoadMember (0x80049E44).
void *archiveLoadMember(s32 memberIndex, s32 archiveAddress, void *outputBuffer) {
    s32 entry;
    s32 length;
    s32 size;
    s32 start;
    u8 *buffer;
    size = romDmaReadWord(archiveAddress);
    if (memberIndex != 0) {
        entry = archiveAddress + memberIndex * sizeof(s32);
        start = romDmaReadWord(entry);
    } else {
        start = (size * 2 + 1) * sizeof(s32);
        entry = archiveAddress + memberIndex * sizeof(s32);
    }
    length = romDmaReadWord(entry + 4) - start;
    buffer = heapAllocTailLocal(&gGameHeap, (u32)length, 0x5F);
    romDmaReadBlocking(archiveAddress + start, buffer, length);
    size = archiveGetMemberDecodedSize(memberIndex, (s32 *)archiveAddress);
    outputBuffer = archiveResizeOutputBuffer(outputBuffer, size);
    archiveDecompressMember(buffer, outputBuffer);
    heapFreeLocal(&gGameHeap, buffer);
    return outputBuffer;
}

s32 romDmaReadWord(s32 romAddress);

/// Read a member's decoded byte count from the ROM archive header.
/// \param memberIndex Zero-based member index; not bounds-checked.
/// \param archiveAddress Cartridge ROM address of the archive header;
/// RAM addresses are unsupported.
/// \returns Decoded member size in bytes.
/// \note Original decompilation identifier: \c archiveGetMemberDecodedSize (0x80049F2C).
s32 archiveGetMemberDecodedSize(s32 memberIndex, s32 *archiveAddress) {
    s32 tmp;
    s32 n = romDmaReadWord((s32)archiveAddress);
    return romDmaReadWord((s32)(archiveAddress + memberIndex + n + (tmp = 1)));
}

/// Whether the shared archive decoding workspace is in use.
/// \note Original identifier: \c D_8009D984 (0x8009D984).
extern u8 gArchiveDecoderBusy;
/// Shared Huffman decode table for code-length symbols and match-distance widths.
/// \note Original identifier: \c D_800FB570 (0x800FB570).
extern u8 gArchiveAuxiliaryDecodeTable[];
/// Shared storage for extended Huffman decoding tables.
/// \note Original identifier: \c D_800FB970 (0x800FB970).
extern u8 gArchiveDecodeSubtables[];
/// Shared Huffman decode table for literals and match lengths.
/// \note Original identifier: \c D_800FB770 (0x800FB770).
extern u16 gArchiveLiteralLengthDecodeTable[];
/// Next available extended Huffman table in the shared decoding workspace.
/// \note Original identifier: \c D_800FDB70 (0x800FDB70).
extern u16 *gArchiveNextDecodeSubtable;
s32 archiveReadBits(s32 bitCount, u8 **source, s32 *bitWindow, s32 *availableBits);
void archiveReadCodeLengthTable(s32 width, s32 special, u8 **source, s32 *bitWindow, s32 *availableBits);
void archiveReadLiteralLengthTable(u8 **source, s32 *bitWindow, s32 *availableBits);
/// Decode an archive member stored verbatim or compressed with LZ/Huffman codes.
/// Callers must serialize decoding.
/// \param input Complete encoded member, including its eight-byte header.
/// \param output Writable buffer of at least the decoded size in input[4..7].
/// \returns Decoded byte count from the member header. Malformed input and
/// buffer bounds are not validated.
///
/// \note NONPORTABLE: IDO schedules the first refill stores by source line.
/// \note Original decompilation identifier: \c archiveDecompressMember (0x80049F74).
s32 archiveDecompressMember(u8 *input, u8 *output) {
    u8 *source;
    s32 count;
    u8 *end;
    s32 available;
    s32 bits;
    s32 size;
    s32 blocks;
    s32 i;
    s32 symbol;
    s32 distance;
    u8 *dest;
    dest = output;
    source = input;
    while (gArchiveDecoderBusy) {
    }
    gArchiveDecoderBusy++;
    size = *(s32 *)(input + 4);
    source = input + 8;
    end = output + size;
    if (*(s32 *)(source - 8) < 0) {
        for (i = 0; i < size; i++) {
            *dest++ = *source++;
        }
    } else {
        bits = *source++ << 8;

        // clang-format off
        bits += *source++; available = 7;
        // clang-format on
        while ((s32)dest < (s32)end) {
            blocks = archiveReadBits(8, &source, &bits, &available);
            count = archiveReadBits(8, &source, &bits, &available) + (blocks << 8);
            gArchiveNextDecodeSubtable = (u16 *)gArchiveDecodeSubtables;
            archiveReadCodeLengthTable(5, 3, &source, &bits, &available);
            archiveReadLiteralLengthTable(&source, &bits, &available);
            archiveReadCodeLengthTable(4, 0, &source, &bits, &available);
            while (count > 0) {
                count--;
                symbol = *(u16 *)((u8 *)gArchiveLiteralLengthDecodeTable + ((bits >> available) & 0x1FE));
                while ((s32)((u32)symbol << 16) < 0) {
                    available -= 4;
                    if (available < 0) {
                        bits = *source++ + ((bits & 255) << 8);
                        available += 8;
                    }
                    symbol = *(u16 *)(gArchiveDecodeSubtables + (symbol & 0x7FFF) + ((bits >> available) & 0x1E));
                }
                available -= symbol >> 9;
                if (available < 0) {
                    bits = *source++ + ((bits & 255) << 8);
                    available += 8;
                }
                symbol &= 0x1FF;
                if (symbol < 256) {
                    *dest++ = symbol;
                } else {
                    i = *(u16 *)(gArchiveAuxiliaryDecodeTable + ((bits >> available) & 0x1FE));
                    while ((s32)((u32)i << 16) < 0) {
                        available -= 4;
                        if (available < 0) {
                            bits = *source++ + ((bits & 255) << 8);
                            available += 8;
                        }
                        i = *(u16 *)(gArchiveDecodeSubtables + (i & 0x7FFF) + ((bits >> available) & 0x1E));
                    }
                    available -= i >> 9;
                    if (available < 0) {
                        bits = *source++ + ((bits & 255) << 8);
                        available += 8;
                    }
                    i &= 0x1FF;
                    distance = i != 0;
                    while (i > 1) {
                        distance = (s32)((u32)distance * 2U + (u32)(((bits >> available) & 0x1FF) >> 8));
                        available--;
                        if (available < 0) {
                            bits = *source++ + ((bits & 255) << 8);
                            available += 8;
                        }
                        i--;
                    }
                    for (i = 0; i < symbol - 253; i++) {
                        *dest = dest[-distance - 1];
                        dest++;
                    }
                }
            }
        }
    }
    gArchiveDecoderBusy--;
    return size;
}

/// Run the archive member encoder stub, which emits no bytes in this build.
/// \param unusedSourceAddress Source address, ignored.
/// \param unusedSourceBytes Source size in bytes, ignored.
/// \param unusedDestination Output buffer, left untouched.
/// \returns Always zero.
/// \note Original decompilation identifier: \c archiveEncodeMemberStub (0x8004A488).
s32 archiveEncodeMemberStub(s32 unusedSourceAddress, s32 unusedSourceBytes, u8 *unusedDestination) {
    return 0;
}

/// Advance the archive input by the requested number of bits.
/// \param bitCount Number of bits to consume.
/// \param source Input cursor, updated to the next unread byte.
/// \param bitWindow Buffered input state, updated for subsequent reads.
/// \param availableBits Input bit-position state, updated for subsequent reads.
/// \returns Nothing.
/// \note Original decompilation identifier: \c archiveConsumeBits (0x8004A49C).
void archiveConsumeBits(s32 bitCount, u8 **source, s32 *bitWindow, s32 *availableBits) {
    *availableBits = *availableBits - bitCount;
    if (*availableBits < 0) {
        do {
            *bitWindow = ((*bitWindow & 0xFF) << 8) + **source;
            *source += 1;
            *availableBits = *availableBits + 8;
        } while (*availableBits < 0);
    }
}

/// Read a field of up to nine bits and advance the archive bitstream.
/// \param bitCount Field width, 1 through 9 bits.
/// \param source Input cursor, updated to the next unread byte.
/// \param bitWindow Rolling input bit window.
/// \param availableBits Current bit-position state, updated after consumption.
/// \returns Unsigned field value in a signed 32-bit result.
/// \note Original decompilation identifier: \c archiveReadBits (0x8004A4EC).
s32 archiveReadBits(s32 bitCount, u8 **source, s32 *bitWindow, s32 *availableBits) {
    s32 sp1C;
    sp1C = (((s32)*bitWindow >> *availableBits) & 0x1FF) >> (9 - bitCount);
    archiveConsumeBits(bitCount, source, bitWindow, availableBits);
    return sp1C;
}

/// Shared code lengths for the Huffman table currently being prepared.
/// \note Original identifier: \c D_800FB170 (0x800FB170).
extern u16 gArchiveSymbolCodeLengths[];
/// Code-space contribution indexed by Huffman code length.
/// \note Original identifier: \c D_8009D960 (0x8009D960).
extern u16 gArchiveCodeSpaceWeights[];
/// Shared Huffman decode table for code-length symbols and match-distance widths.
/// \note Original identifier: \c D_800FB570 (0x800FB570).
extern u8 gArchiveAuxiliaryDecodeTable[];
void archiveBuildDecodeTable(s32 symbols, s32 total, u16 *table);
/// Load an auxiliary Huffman table from the archive bitstream.
/// Callers must serialize access to the shared decoding workspace.
/// \param width Bit width of the code-count and constant-symbol fields.
/// \param special Code-length index after which an optional zero run is read;
/// zero disables this special run.
/// \param source Byte cursor, advanced while decoding.
/// \param bitWindow Rolling input bit window.
/// \param availableBits Current bit-position state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c archiveReadCodeLengthTable (0x8004A52C).
void archiveReadCodeLengthTable(s32 width, s32 special, u8 **source, s32 *bitWindow, s32 *availableBits) {
    s32 count;
    s32 i = 0;
    s32 total = 0;
    s32 length;
    s32 run;
    s32 zeros;

    count = archiveReadBits(width, source, bitWindow, availableBits);
    if (count == 0) {
        count = archiveReadBits(width, source, bitWindow, availableBits);
        for (; i < 256; i++) {
            ((u16 *)gArchiveAuxiliaryDecodeTable)[i] = count;
        }
        return;
    }
    while (i < count) {
        length = archiveReadBits(3, source, bitWindow, availableBits);
        if (length >= 7) {
            run = 1;
            length = (s32)((u32)*bitWindow << (23 - *availableBits));
            while (length < 0) {
                run++;
                length = (s32)((u32)length << 1);
            }
            archiveConsumeBits(run, source, bitWindow, availableBits);
            length = run + 6;
        }
        total += gArchiveCodeSpaceWeights[length];
        gArchiveSymbolCodeLengths[i++] = length;
        if (--special == 0) {
            zeros = archiveReadBits(2, source, bitWindow, availableBits);
            while (zeros > 0) {
                zeros--;
                gArchiveSymbolCodeLengths[i++] = 0;
            }
        }
    }
    archiveBuildDecodeTable(count, total, (u16 *)gArchiveAuxiliaryDecodeTable);
}

/* Expand zero runs and literal code lengths, then build their decode table. */
/// Shared code lengths for the Huffman table currently being prepared.
/// \note Original identifier: \c D_800FB170 (0x800FB170).
extern u16 gArchiveSymbolCodeLengths[];
/// Code-space contribution indexed by Huffman code length.
/// \note Original identifier: \c D_8009D960 (0x8009D960).
extern u16 gArchiveCodeSpaceWeights[];
s32 archiveDecodeSymbol(u8 *table, u8 **source, s32 *bitWindow, s32 *availableBits);

/// Load the archive's literal and match-length Huffman table.
/// Requires an auxiliary Huffman table prepared for this input and exclusive
/// access to the shared decoding workspace.
/// \param source Byte cursor, advanced while decoding.
/// \param bits Rolling input bit window.
/// \param availableBits Current bit-position state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c archiveReadLiteralLengthTable (0x8004A738).
void archiveReadLiteralLengthTable(u8 **source, s32 *bits, s32 *availableBits) {
    s32 decodedCount = 0;
    s32 tableSize = 0;
    s32 symbolCount = archiveReadBits(9, source, bits, availableBits);
    s32 runLength;
    s32 codeLength;
    u16 *output;

    if (symbolCount > 0) {
        do {
            codeLength = archiveDecodeSymbol(gArchiveAuxiliaryDecodeTable, source, bits, availableBits);
            switch (codeLength) {
            case 0:
                runLength = 1;
                codeLength = 0;
                goto emit_run;
            case 1:
                runLength = archiveReadBits(4, source, bits, availableBits) + 3;
                codeLength = 0;
                goto emit_run;
            case 2:
                runLength = archiveReadBits(9, source, bits, availableBits) + 20;
                codeLength = 0;
                goto emit_run;
            }
            /* Other symbols encode one literal code length. */
            runLength = 1;
            codeLength -= 2;
        emit_run:
            if (runLength > 0) {
                output = &gArchiveSymbolCodeLengths[decodedCount];
                do {
                    runLength--;
                    *output = codeLength;
                    decodedCount++;
                    output++;
                } while (runLength > 0);
            }
            tableSize += gArchiveCodeSpaceWeights[codeLength];
        } while (decodedCount < symbolCount);
    }
    archiveBuildDecodeTable(symbolCount, tableSize, gArchiveLiteralLengthDecodeTable);
}

/// Shared storage for extended Huffman decoding tables.
/// \note Original identifier: \c D_800FB970 (0x800FB970).
extern u8 gArchiveDecodeSubtables[];
/// Prepare Huffman decoding for the configured symbol lengths.
/// Callers must serialize access to the shared decoding workspace.
/// \param symbols Number of configured symbol lengths to use.
/// \param total Total code-space weight of the configured symbol lengths.
/// \param table Output table with sufficient capacity for the decoding entries.
/// \returns Nothing.
/// \note Original decompilation identifier: \c archiveBuildDecodeTable (0x8004A8A4).
void archiveBuildDecodeTable(s32 symbols, s32 total, u16 *table) {
    s32 copies;
    s32 length;
    s32 symbol;
    s32 bits;
    s32 i;
    u16 *end;
    u16 *link;
    copies = 128;
    end = table + ((total - 1) >> 8) + 1;
    for (length = 1; length <= 16; length++) {
        for (symbol = 0; symbol < symbols; symbol++) {
            if (length == gArchiveSymbolCodeLengths[symbol]) {
                bits = length < 9 ? length : ((length - 1) & 3) + 5;
                bits = (bits << 9) + symbol;
                for (i = 0; i < copies; i++)
                    *table++ = bits;
            }
        }
        copies >>= 1;
        if (copies == 0 && length < 17) {
            link = table;
            table = gArchiveNextDecodeSubtable;
            while ((s32)link < (s32)end) {
                *link++ = (s32)gArchiveNextDecodeSubtable - (s32)gArchiveDecodeSubtables + 0x8000;
                gArchiveNextDecodeSubtable += 16;
            }
            end = gArchiveNextDecodeSubtable;
            copies = 8;
        }
    }
}

/// Decode one Huffman symbol from the archive bitstream.
/// \param table Prepared Huffman decoding table.
/// \param source Input cursor, updated to the next unread byte.
/// \param bitWindow Rolling input bit window.
/// \param availableBits Current bit-position state, updated after decoding.
/// \returns Decoded nine-bit symbol value.
/// \note Original decompilation identifier: \c archiveDecodeSymbol (0x8004A9F4).
s32 archiveDecodeSymbol(u8 *table, u8 **source, s32 *bitWindow, s32 *availableBits) {
    u16 v = *(u16 *)(table + ((*bitWindow >> *availableBits) & 0x1FE));

    while ((s32)(v << 16) < 0) {
        archiveConsumeBits(4, source, bitWindow, availableBits);
        v = *(u16 *)(gArchiveDecodeSubtables + ((*bitWindow >> *availableBits) & 0x1E) + (v & 0x7FFF));
    }
    archiveConsumeBits(v >> 9, source, bitWindow, availableBits);
    return v & 0x1FF;
}
