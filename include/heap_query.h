/// \file
/// Declares querying the size of an existing game-heap allocation.

#ifndef HEAP_QUERY_H
#define HEAP_QUERY_H

#include <ultra64.h>

/// Get the available payload size of a heap allocation.
/// \param heap Initialized heap to operate on.
/// \param allocationAddress RAM address of a payload with a readable heap header, or zero.
/// \returns Allocation payload size in bytes, or -1 for a null allocation or failed block validation.
/// \note Original decompilation identifier: \c func_800384F0 (0x800384F0).
s32 heapGetAllocationSize(u8 *heap, s32 allocationAddress);

#endif
