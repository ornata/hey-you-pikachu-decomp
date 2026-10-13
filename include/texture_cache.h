/// \file
/// Defines the shared texture cache lifetime and retained cartridge-data storage.

#ifndef TEXTURE_CACHE_H
#define TEXTURE_CACHE_H

#include "rom_cache.h"

/// Shared cache owning loaded texture images, palettes and pending reads.
/// Texture references are borrowed and remain valid only while retained by this cache.
/// \note Original identifier: \c D_800B67B0 (0x800B67B0).
extern RomCache gTextureRomCache;

/// Begin a texture-cache lifetime before requesting texture descriptors.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000C140 (0x8000C140).
void textureCacheInitialize(void);

/// Finish pending reads and release the texture cache and its borrowed texture data.
/// \returns Nothing; outstanding image and palette references become invalid.
/// \note Original decompilation identifier: \c func_8000C174 (0x8000C174).
void textureCacheDestroy(void);

#endif
