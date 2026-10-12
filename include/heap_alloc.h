/// \file
/// Declares allocation from an initialized game heap.

#ifndef HEAP_ALLOC_H
#define HEAP_ALLOC_H

#include "heap_types.h"

/// Allocate storage from a game heap.
/// \param heap Initialized heap supplying the allocation.
/// \param size Requested allocation size in bytes.
/// \param flag Allocation flag recorded in the heap block metadata.
/// \returns Owned allocation, or NULL when no suitable block is available.
void *heapAllocLocal(HpHdr *heap, u32 size, u16 flag);

#endif
