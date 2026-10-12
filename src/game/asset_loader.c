#include "common.h"
#include "heap_alloc.h"

typedef struct {
    s32 offset;
    s32 value;
} AssetPatch;
s32 archiveDecompressMember(u8 *input, u8 *output);
/// Decode an asset and apply its supplied corrections.
/// \param destination Writable storage large enough for the decoded asset.
/// \param source Encoded asset and its correction records.
/// \param size Encoded size metadata; unused by this operation.
/// \param outputSize Decoded size metadata; unused by this operation.
/// \param patchOffset Byte offset of the correction records within source.
/// \param count Number of correction records to apply.
/// \returns Decoded asset size in bytes.
/// \note Original decompilation identifier: \c func_8002D010 (0x8002D010).
s32 assetDecompressAndPatch(u8 *destination, u8 *source, s32 size, s32 outputSize, s32 patchOffset, s32 count) {
    s32 result;
    s32 i;
    u8 *output = destination;
    result = archiveDecompressMember(source, destination);
    for (i = 0; i != count; i++) {
        AssetPatch *patch = (AssetPatch *)(i * sizeof(AssetPatch) + patchOffset + source);
        s32 offset = patch->offset;
        s32 value = patch->value;
        *(s32 *)(output + offset) = value;
    }
    return result;
}

s32 assetDecompressAndPatch(u8 *, u8 *, s32, s32, s32, s32);
void romDmaReadBlocking(s32 romAddress, s32 destination, s32 byteCount); /* extern */

/// Load a corrected asset from cartridge ROM into caller-provided storage.
/// \param romAddress Cartridge address of the encoded asset.
/// \param destination RAM address with room for the decoded asset.
/// \param scratch RAM address with room for the encoded asset and corrections.
/// \param encodedSize Encoded byte count excluding correction records.
/// \param outputSize Decoded size metadata; does not limit output writes.
/// \param patchOffset Byte offset of the correction records in the encoded asset.
/// \param patchCount Number of correction records accompanying the asset.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D104 (0x8002D104).
void assetLoadAndPatchFromRom(s32 romAddress, s32 destination, s32 scratch, s32 encodedSize, s32 outputSize,
                              s32 patchOffset, s32 patchCount) {
    romDmaReadBlocking(romAddress, scratch, (patchCount * 8) + encodedSize);
    assetDecompressAndPatch((u8 *)destination, (u8 *)scratch, encodedSize, outputSize, patchOffset, patchCount);
}

void *heapAllocTailLocal(void *, s32, s32);
void heapFreeLocal(void *, void *);
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern u8 gGameHeap[];

/// Allocate and load a corrected asset described by a ROM asset record.
/// \param info Asset descriptor containing its ROM address, decoded range,
/// encoded range, and correction range.
/// \returns Loaded asset allocation; release it with assetFree.
/// \note Original decompilation identifier: \c func_8002D160 (0x8002D160).
void *assetAllocateAndLoad(s32 *info) {
    /* Two unrecovered words preserve the original 0x40-byte stack frame. */
    s32 reserved[2];
    void *temporary;
    void *result;
    s32 size;

    size = info[3] - info[2];
    result = heapAllocLocal(gGameHeap, size, 0x5D);
    temporary = heapAllocTailLocal(gGameHeap, info[4] - info[2], 0x63);
    assetLoadAndPatchFromRom(info[0], (s32)result, (s32)temporary, info[4] - info[2], info[3] - info[2],
                             info[5] - info[2], info[6] - info[2]);
    heapFreeLocal(gGameHeap, temporary);
    return result;
}

/// Release an asset allocation.
/// \param asset Address of an allocation returned by assetAllocateAndLoad.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D214 (0x8002D214).
void assetFree(s32 asset) {
    heapFreeLocal(gGameHeap, (void *)asset);
}

/// Provide an inactive asset operation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D23C (0x8002D23C).
void assetNoOp2D23C(void) {
}

/// Provide an inactive asset operation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D244 (0x8002D244).
void assetNoOp2D244(void) {
}

/// Provide an inactive asset operation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D24C (0x8002D24C).
void assetNoOp2D24C(void) {
}

/// Provide an inactive asset operation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D254 (0x8002D254).
void assetNoOp2D254(void) {
}
