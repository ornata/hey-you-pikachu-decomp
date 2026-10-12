/// \file
/// Declares the shared heap control record used by memory allocation routines.

#ifndef HEAP_TYPES_H
#define HEAP_TYPES_H

#include <ultra64.h>

struct HpBlk;
/// Control record for an initialized game heap and its circular block list.
typedef struct HpHdr {
    s32 thread;          ///< Thread identifier recorded when the heap is initialized.
    s32 size;            ///< Total size in bytes of the heap region.
    struct HpBlk *first; ///< First block in the circular heap list.
    struct HpBlk *last;  ///< Allocation search position in the heap list.
} HpHdr;

#endif
