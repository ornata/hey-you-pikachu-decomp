/// \file
/// Declares allocation from the upper end of a free game-heap region.

#ifndef HEAP_TAIL_H
#define HEAP_TAIL_H

#include "heap_types.h"

/// Allocate tagged storage from the upper end of a suitable heap region.
/// \param heap Initialized heap supplying the allocation.
/// \param size Requested payload size in bytes.
/// \param flag Allocation label recorded for the storage.
/// \returns Owned payload, or NULL when no suitable block is available.
/// \note Original decompilation identifier: \c func_800381D4 (0x800381D4).
void *heapAllocTailLocal(HpHdr *heap, u32 size, u16 flag);

#endif
