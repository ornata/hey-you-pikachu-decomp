/// \file
/// Declares integer-key lookup in an initialized word map.

#ifndef WORD_MAP_LOOKUP_H
#define WORD_MAP_LOOKUP_H

#include "word_map_types.h"

/// Look up a word value by its integer key.
/// \param map Initialized map with positive capacity.
/// \param key Integer key to find.
/// \returns The stored word value, or zero when the key is absent.
/// \note Original decompilation identifier: \c func_80048E78 (0x80048E78).
s32 wordMapLookup(WordMap *map, u32 key);

#endif
