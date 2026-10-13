/// \file
/// Defines initialization of game heap controls over caller-owned memory regions.

#ifndef HEAP_INITIALIZE_H
#define HEAP_INITIALIZE_H

#include "heap_types.h"

/// Initialize a heap over a writable region, replacing its previous contents.
/// \param heap Writable control record outside the supplied region.
/// \param region Sixteen-byte-aligned memory retained for the heap and allocation lifetimes.
/// \param regionSize Positive region size in bytes, at least sixteen, including metadata.
/// \returns The supplied region pointer.
/// \note Original decompilation identifier: \c func_80037F00 (0x80037F00).
void *heapInitializeRegion(HpHdr *heap, void *region, s32 regionSize);

/// Initialize the global heap from a starting pointer up to its fixed RAM limit.
/// \param start Sixteen-byte-aligned writable memory below the depth-buffer boundary;
/// at least sixteen bytes must be available, retained throughout global heap use.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80037F88 (0x80037F88).
void heapInitializeGlobal(void *start);

#endif
