/// \file
/// Defines corrected ROM asset descriptors and allocation/loading contracts.

#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include <ultra64.h>

/// Serialized metadata for a corrected compressed asset.
/// Range and correction values are normalized against rangeBase by consumers.
/// This record describes metadata, not ownership of decoded or staging storage.
typedef struct AssetDescriptor {
    s32 romAddress;      ///< Cartridge address of encoded data.
    s32 romEnd;          ///< Exclusive cartridge end address retained in the metadata.
    s32 rangeBase;       ///< Common bias for sizes and correction metadata.
    s32 decodedEnd;      ///< Subtract rangeBase to obtain decoded allocation size.
    s32 encodedEnd;      ///< Subtract rangeBase to obtain encoded byte count.
    s32 patchStart;      ///< Subtract rangeBase to obtain correction byte offset.
    s32 patchCountValue; ///< Subtract rangeBase to obtain correction record count.
} AssetDescriptor;

typedef char AssetDescriptorSizeCheck[(sizeof(AssetDescriptor) == 0x1C) ? 1 : -1];
typedef char AssetDescriptorPatchOffsetCheck[(((u32) & ((AssetDescriptor *)0)->patchStart) == 0x14) ? 1 : -1];

/// Decode an asset and apply its supplied corrections.
/// \param destination Writable storage large enough for the decoded asset.
/// \param source Encoded asset and its correction records.
/// \param size Encoded size metadata; unused by this operation.
/// \param outputSize Decoded size metadata; unused by this operation.
/// \param patchOffset Byte offset of the correction records within source.
/// \param count Number of correction records to apply.
/// \returns Decoded asset size in bytes.
/// \note Original decompilation identifier: \c func_8002D010 (0x8002D010).
s32 assetDecompressAndPatch(u8 *destination, u8 *source, s32 size, s32 outputSize, s32 patchOffset, s32 count);

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
void assetLoadAndPatchFromRom(s32 romAddress, u8 *destination, u8 *scratch, s32 encodedSize, s32 outputSize,
                              s32 patchOffset, s32 patchCount);

/// Allocate and load a corrected asset described by a ROM asset record.
/// \param info Asset descriptor containing its ROM address, decoded range,
/// encoded range, and correction range.
/// \returns Loaded asset allocation; release it with assetFree.
/// \note Bounds: staging capacity for nonzero correction counts is not established;
/// the title and eye-catch descriptors used here have zero corrections.
/// \note Original decompilation identifier: \c func_8002D160 (0x8002D160).
void *assetAllocateAndLoad(AssetDescriptor *info);

/// Release an asset allocation.
/// \param asset Address of an allocation returned by assetAllocateAndLoad.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D214 (0x8002D214).
void assetFree(void *asset);

#endif
