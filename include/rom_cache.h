/// \file
/// Defines bounded cartridge-data caches and their read-lifetime interfaces.

#ifndef ROM_CACHE_H
#define ROM_CACHE_H

#include "heap_types.h"
#include "keyed_pool.h"
#include "rom_dma.h"

/// Cache entry metadata; the data address remains a 32-bit cartridge-runtime handle.
typedef struct CacheEnt {
    s32 flags; ///< Bit 0 marks current use; bit 1 marks a pending read.
    u32 size;  ///< Number of bytes reserved for data.
    s32 key;   ///< Lookup key, also the cartridge address for read-backed entries.
    s32 data;  ///< RAM address of the owned data buffer.
} CacheEnt;

/// Cache-owned heap, keyed index, and outstanding cartridge reads.
typedef struct RomCache {
    s32 storageAddress; ///< RAM address of the owned heap region.
    HpHdr heap;         ///< Allocator for entry metadata and data buffers.
    KeyedPool index;    ///< Keyed index of entry addresses.
    DmaQueue reads;     ///< Requests that must finish before data is consumed or released.
} RomCache;

typedef char CacheEntSizeCheck[(sizeof(CacheEnt) == 0x10) ? 1 : -1];
typedef char RomCacheSizeCheck[(sizeof(RomCache) == 0x5C) ? 1 : -1];
#define CACHE_OFFSET(field) ((u32) & (((RomCache *)0)->field))
typedef char RomCacheIndexOffsetCheck[(CACHE_OFFSET(index) == 0x14) ? 1 : -1];
typedef char RomCacheReadsOffsetCheck[(CACHE_OFFSET(reads) == 0x34) ? 1 : -1];
#undef CACHE_OFFSET

/// Initialize a bounded cache for cartridge data.
/// \param cache Cache state to initialize.
/// \param heapBytes Number of bytes available for cached allocations.
/// \param entryCount Positive maximum number of cached entries.
/// \param queueCapacity Number of ROM read requests the cache can queue.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003F700 (0x8003F700).
void romCacheInitialize(RomCache *cache, s32 heapBytes, s32 entryCount, s32 queueCapacity);

/// Release a cartridge data cache and its outstanding read resources.
/// \param cache Initialized cache to destroy.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003F770 (0x8003F770).
void romCacheDestroy(RomCache *cache);

/// Allocate an unindexed cache entry and its data storage.
/// \param cacheAddress RAM address of the initialized cache.
/// \param byteCount Number of data bytes to reserve.
/// \returns Allocated entry, or NULL if its storage is unavailable.
/// \note Original decompilation identifier: \c func_8003F7B4 (0x8003F7B4).
u8 *romCacheAllocateEntry(s32 cacheAddress, s32 byteCount);

/// Remove an indexed cache entry and release its storage.
/// \param cacheAddress RAM address of the initialized cache.
/// \param entry Indexed entry that is no longer in use or awaiting a read.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003F80C (0x8003F80C).
void romCacheRemoveEntry(s32 cacheAddress, u8 *entry);

/// Acquire cached cartridge data, starting a ROM read when necessary.
/// \param c Initialized cache with enough available or reclaimable storage.
/// \param key Cartridge ROM address of the requested data.
/// \param size Number of bytes required.
/// \param out Receives the RAM address of the data; synchronize pending reads before use.
/// \returns 3 for reusable cached data, or 4 when read completion must be synchronized.
/// \note Original decompilation identifier: \c func_8003F848 (0x8003F848).
s32 romCacheRead(RomCache *c, s32 key, u32 size, s32 *out);

/// Acquire a keyed cache buffer without requesting a cartridge read.
/// \param c Initialized cache with enough available or reclaimable storage.
/// \param key Key identifying the buffer.
/// \param size Requested byte count; an existing allocation must already be large enough.
/// \param out Receives the RAM address of the reserved data buffer.
/// \returns 0 for a new buffer, 1 for an existing unused buffer, or 2 for an existing buffer already in use.
/// \note Original decompilation identifier: \c func_8003F950 (0x8003F950).
s32 romCacheReserveBuffer(RomCache *c, s32 key, u32 size, s32 *out);

/// Acquire a cache entry, reclaiming unused entries when storage is needed.
/// \param cache Initialized cache to allocate from.
/// \param byteCount Number of data bytes to reserve.
/// \returns RAM address of the allocated entry, or zero if storage cannot be reclaimed.
/// \note Original decompilation identifier: \c func_8003FA3C (0x8003FA3C).
s32 romCacheReclaimAndAllocateEntry(RomCache *cache, s32 byteCount);

/// Synchronize cartridge reads and retire recent cache-use state.
/// \param cache Initialized cache to synchronize.
/// \param stateMask State categories to retire: bit 0 for cache use, bit 1 for pending reads.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FAE8 (0x8003FAE8).
void romCacheSynchronizeAndClearState(RomCache *cache, s32 stateMask);

/// Wait for cartridge reads and make their cached data reusable.
/// \param cache Initialized cache to synchronize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FB4C (0x8003FB4C).
void romCacheWaitForReads(RomCache *cache);

/// Wait for cartridge reads and release the current uses of cached entries.
/// \param cache Initialized cache whose current uses have finished.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FB6C (0x8003FB6C).
void romCacheReleaseUses(RomCache *cache);

#endif
