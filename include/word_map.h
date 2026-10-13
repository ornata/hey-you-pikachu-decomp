/// \file
/// Declares ownership and insertion contracts for integer-key word maps.

#ifndef WORD_MAP_H
#define WORD_MAP_H

#include "word_map_types.h"

/// Initialize an empty map of integer keys to nonzero word values.
/// \param map Caller-owned map storage without an unreleased backing allocation.
/// \param capacity Positive entry capacity whose total allocation size fits in signed 32 bits.
/// \returns Nothing.
/// \note Portability: The allocator call passes an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_80048CE0 (0x80048CE0).
void wordMapInitialize(WordMap *map, s32 capacity);

/// Release a word map and reset it to an empty unallocated state.
/// \param map Initialized map whose storage is released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80048D44 (0x80048D44).
void wordMapDestroy(WordMap *map);

/// Add a key and nonzero word value to an initialized map.
/// \param map Initialized map with positive capacity.
/// \param value Nonzero word value to store.
/// \param key Integer lookup key; duplicate keys add entries and do not replace existing values.
/// \returns One when inserted, or zero when no reachable free entry is available.
/// \note Original decompilation identifier: \c func_80048D84 (0x80048D84).
s32 wordMapInsert(WordMap *map, s32 value, u32 key);

#endif
