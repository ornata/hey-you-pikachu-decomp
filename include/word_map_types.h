/// \file
/// Defines storage for integer-key maps shared by motion and actor registries.

#ifndef WORD_MAP_TYPES_H
#define WORD_MAP_TYPES_H

#include <ultra64.h>

/// Integer lookup key and its nonzero stored word value.
typedef struct WordMapEntry {
    u32 key; ///< Caller-defined lookup key.
    s32 val; ///< Stored word; zero denotes an available entry.
} WordMapEntry;

/// Owned map storage and its occupancy.
typedef struct WordMap {
    WordMapEntry *slots; ///< Owned entry allocation, released by map destruction.
    u32 size;            ///< Entry capacity; must be positive for insertion and lookup.
    u32 count;           ///< Number of occupied entries.
} WordMap;

typedef char WordMapEntrySizeCheck[(sizeof(WordMapEntry) == 8) ? 1 : -1];
typedef char WordMapSizeCheck[(sizeof(WordMap) == 12) ? 1 : -1];

#endif
