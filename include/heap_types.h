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

typedef char HpHdrSizeCheck[(sizeof(HpHdr) == 0x10) ? 1 : -1];
typedef char HpHdrFirstOffsetCheck[(((u32)&((HpHdr *)0)->first) == 8) ? 1 : -1];
typedef char HpHdrLastOffsetCheck[(((u32)&((HpHdr *)0)->last) == 0xC) ? 1 : -1];

#endif
