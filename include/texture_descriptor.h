/// \file
/// Defines texture archive descriptors and their cache-resolved drawing view.

#ifndef TEXTURE_DESCRIPTOR_H
#define TEXTURE_DESCRIPTOR_H

#include "ultra64.h"

/// Texture-table entry or resolved drawing descriptor, with the same target layout.
/// Address words are archive-relative offsets in tables and target RAM addresses after resolution.
typedef struct TextureDescriptor {
    s32 image; ///< Image archive offset before resolution, borrowed image address afterward.
    s32 palette; ///< Palette archive offset before resolution, borrowed palette address afterward; zero without a palette.
    u16 width;  ///< Image width in pixels.
    u16 height; ///< Image height in pixels.
    u8 size;    ///< RDP pixel-size selector.
    u8 format;  ///< RDP texture-format selector.
    u8 reserved0E[2];
} TextureDescriptor;

typedef char TextureDescriptorSizeCheck[(sizeof(TextureDescriptor) == 0x10) ? 1 : -1];
typedef char TextureDescriptorWidthOffsetCheck[((u32) & ((TextureDescriptor *)0)->width == 8) ? 1 : -1];
typedef char TextureDescriptorFormatOffsetCheck[((u32) & ((TextureDescriptor *)0)->format == 0xD) ? 1 : -1];

/// Get a texture descriptor with RAM addresses for its image and palette.
/// \param table Texture-table index.
/// \param index Valid texture index within the selected table.
/// \param descriptor Receives the resolved descriptor; pending cache reads must
/// complete before its image or palette is used.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_8000C198 (0x8000C198).
void textureGetDescriptor(s32 table, s32 index, TextureDescriptor *descriptor);

#endif
