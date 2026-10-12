/// \file
/// Defines keyed-pool storage layouts shared by lookup and traversal clients.

#ifndef KEYED_POOL_TYPES_H
#define KEYED_POOL_TYPES_H

#include <ultra64.h>

/// Key/value entry with independent traversal and hash-chain links.
typedef struct KeyedPoolEntry {
    u32 key;                         ///< Lookup key; duplicate keys are permitted.
    s32 value;                       ///< Caller-defined nonzero value; zero denotes unused storage.
    struct KeyedPoolEntry *previous; ///< Previous ordered or available entry.
    struct KeyedPoolEntry *next;     ///< Next ordered or available entry.
    struct KeyedPoolEntry *chain;    ///< Next entry in the lookup chain.
} KeyedPoolEntry;

/// Fixed-capacity keyed pool with a shared traversal cursor.
typedef struct KeyedPool {
    KeyedPoolEntry *entries; ///< Owned entry storage, including hash buckets.
    KeyedPoolEntry *free;    ///< First available entry, or NULL.
    KeyedPoolEntry *head;    ///< First ordered live entry, or NULL.
    KeyedPoolEntry *tail;    ///< Last ordered live entry, or NULL.
    KeyedPoolEntry *cursor;  ///< Selected entry, or NULL.
    u32 capacity;            ///< Number of entry slots and hash buckets.
    s32 nextId;              ///< Next automatically assigned key.
    s32 active;              ///< Number of reserved entries.
} KeyedPool;

typedef char KeyedPoolEntrySizeCheck[(sizeof(KeyedPoolEntry) == 0x14) ? 1 : -1];
typedef char KeyedPoolSizeCheck[(sizeof(KeyedPool) == 0x20) ? 1 : -1];
#define KEYED_POOL_OFFSET(type, field) ((u32) & (((type *)0)->field))
typedef char KeyedPoolChainOffsetCheck[(KEYED_POOL_OFFSET(KeyedPoolEntry, chain) == 0x10) ? 1 : -1];
typedef char KeyedPoolCapacityOffsetCheck[(KEYED_POOL_OFFSET(KeyedPool, capacity) == 0x14) ? 1 : -1];
#undef KEYED_POOL_OFFSET

#endif
