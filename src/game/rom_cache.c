#include "heap_free.h"
#include "common.h"
#include "heap_alloc.h"
#include "rom_cache.h"
s32 heapInitializeRegion(HpHdr *, s32, s32);
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

/// Initialize a bounded cache for cartridge data.
/// \param cache Cache state to initialize.
/// \param heapBytes Number of bytes available for cached allocations.
/// \param entryCount Positive maximum number of cached entries.
/// \param queueCapacity Number of ROM read requests the cache can queue.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003F700 (0x8003F700).
void romCacheInitialize(RomCache *cache, s32 heapBytes, s32 entryCount, s32 queueCapacity) {
    s32 storageAddress;
    storageAddress = (s32)heapAllocLocal(&gGameHeap, heapBytes, 0x15);
    cache->storageAddress = storageAddress;
    heapInitializeRegion(&cache->heap, storageAddress, heapBytes);
    keyedPoolInitialize(&cache->index, entryCount);
    romDmaInitializeQueue(&cache->reads, queueCapacity);
}

/// Release a cartridge data cache and its outstanding read resources.
/// \param cache Initialized cache to destroy.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003F770 (0x8003F770).
void romCacheDestroy(RomCache *cache) {
    romDmaDestroyQueue(&cache->reads);
    keyedPoolDestroy(&cache->index);
    heapFreeLocal(&gGameHeap, (u8 *)cache->storageAddress);
}

/// Allocate an unindexed cache entry and its data storage.
/// \param cacheAddress RAM address of the initialized cache.
/// \param byteCount Number of data bytes to reserve.
/// \returns Allocated entry, or NULL if its storage is unavailable.
/// \note Original decompilation identifier: \c func_8003F7B4 (0x8003F7B4).
u8 *romCacheAllocateEntry(s32 cacheAddress, s32 byteCount) {
    CacheEnt *entry;
    entry = heapAllocLocal(&((RomCache *)cacheAddress)->heap, byteCount + 0x10, 0x32);
    if (entry == NULL) {
        return NULL;
    }
    entry->flags = 0;
    entry->size = byteCount;
    entry->key = 0;
    entry->data = (s32)((u8 *)entry + 0x10);
    return (u8 *)entry;
}

/// Remove an indexed cache entry and release its storage.
/// \param cacheAddress RAM address of the initialized cache.
/// \param entry Indexed entry that is no longer in use or awaiting a read.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003F80C (0x8003F80C).
void romCacheRemoveEntry(s32 cacheAddress, u8 *entry) {
    keyedPoolRemoveFirstForKey(&((RomCache *)cacheAddress)->index, ((CacheEnt *)entry)->key);
    heapFreeLocal(&((RomCache *)cacheAddress)->heap, entry);
}

/// Acquire cached cartridge data, starting a ROM read when necessary.
/// \param c Initialized cache with enough available or reclaimable storage.
/// \param key Cartridge ROM address of the requested data.
/// \param size Number of bytes required.
/// \param out Receives the RAM address of the data; synchronize pending reads before use.
/// \returns 3 for reusable cached data, or 4 when read completion must be synchronized.
/// \note Original decompilation identifier: \c func_8003F848 (0x8003F848).
s32 romCacheRead(RomCache *c, s32 key, u32 size, s32 *out) {
    CacheEnt *e = (CacheEnt *)keyedPoolGetValueAndMoveToEnd(&c->index, key);

    if (e != NULL) {
        if (e->size >= size) {
            e->flags |= 1;
            *out = e->data;
            if (e->flags & 2) {
                return 4;
            }
            return 3;
        }
        romCacheRemoveEntry((s32)c, (u8 *)e);
    }
    e = (CacheEnt *)romCacheReclaimAndAllocateEntry(c, size);
    if (e == NULL) {
        keyedPoolHasAvailableEntry(&c->index);
    }
    e->size = size;
    e->key = key;
    romDmaReadQueued(&c->reads, key, (u8 *)e->data, size);
    keyedPoolInsert(&c->index, (s32)e, key);
    e->flags |= 3;
    *out = e->data;
    return 4;
}

/// Acquire a keyed cache buffer without requesting a cartridge read.
/// \param c Initialized cache with enough available or reclaimable storage.
/// \param key Key identifying the buffer.
/// \param size Requested byte count; an existing allocation must already be large enough.
/// \param out Receives the RAM address of the reserved data buffer.
/// \returns 0 for a new buffer, 1 for an existing unused buffer, or 2 for an existing buffer already in use.
/// \note Original decompilation identifier: \c func_8003F950 (0x8003F950).
s32 romCacheReserveBuffer(RomCache *c, s32 key, u32 size, s32 *out) {
    CacheEnt *e;
    s32 status;

    e = (CacheEnt *)keyedPoolGetValueAndMoveToEnd(&c->index, key);
    if (e != NULL) {
        if (e->flags & 1) {
            status = 2;
        } else {
            status = 1;
        }
        if (size == e->size && key != e->key) {
        }
    } else {
        status = 0;
        e = (CacheEnt *)romCacheReclaimAndAllocateEntry(c, size);
        if (e == NULL) {
            keyedPoolHasAvailableEntry(&c->index);
        }
        e->size = size;
        e->key = key;
        keyedPoolInsert(&c->index, (s32)e, key);
    }
    e->flags |= 1;
    *out = e->data;
    return status;
}

/// Acquire a cache entry, reclaiming unused entries when storage is needed.
/// \param cache Initialized cache to allocate from.
/// \param byteCount Number of data bytes to reserve.
/// \returns RAM address of the allocated entry, or zero if storage cannot be reclaimed.
/// \note Original decompilation identifier: \c func_8003FA3C (0x8003FA3C).
s32 romCacheReclaimAndAllocateEntry(RomCache *cache, s32 byteCount) {
    KeyedPool *index;
    u8 *v;
    u8 *r;

    index = &cache->index;
    v = (u8 *)keyedPoolIterateFirst(index);
    while (1) {
        if (keyedPoolHasAvailableEntry(index) != 0) {
            r = romCacheAllocateEntry((s32)cache, byteCount);
            if (r != NULL) {
                return (s32)r;
            }
        }
        if (v == NULL || (*(s32 *)v & 3)) {
            return 0;
        }
        romCacheRemoveEntry((s32)cache, v);
        v = (u8 *)keyedPoolIterateNext(index);
    }
}

/// Synchronize cartridge reads and retire recent cache-use state.
/// \param cache Initialized cache to synchronize.
/// \param stateMask State categories to retire: bit 0 for cache use, bit 1 for pending reads.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FAE8 (0x8003FAE8).
void romCacheSynchronizeAndClearState(RomCache *cache, s32 stateMask) {
    s32 *flags;
    s32 previousFlags;
    KeyedPoolEntry *entry;
    romDmaWaitForQueue(&cache->reads);
    entry = cache->index.tail;
    if (entry != NULL) {
    loop_1:
        flags = (s32 *)entry->value;
        previousFlags = *flags;
        if (previousFlags & stateMask) {
            *flags = (u32)previousFlags & ~(u32)stateMask;
            entry = entry->previous;
            if (entry != NULL) {
                goto loop_1;
            }
        }
    }
}

/// Wait for cartridge reads and make their cached data reusable.
/// \param cache Initialized cache to synchronize.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FB4C (0x8003FB4C).
void romCacheWaitForReads(RomCache *cache) {
    romCacheSynchronizeAndClearState(cache, 2);
}

/// Wait for cartridge reads and release the current uses of cached entries.
/// \param cache Initialized cache whose current uses have finished.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003FB6C (0x8003FB6C).
void romCacheReleaseUses(RomCache *cache) {
    romCacheSynchronizeAndClearState(cache, 3);
}
