/// \file
/// Declares release of allocations owned by initialized game heaps.

#ifndef HEAP_FREE_H
#define HEAP_FREE_H

#include "heap_types.h"

/// Release an allocation back to its owning heap.
/// \param heap Initialized heap that supplied the allocation.
/// \param allocation Live payload allocated from heap, or NULL for no operation; the caller must stop using released storage.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80038548 (0x80038548).
void heapFreeLocal(HpHdr *heap, void *allocation);

#endif
