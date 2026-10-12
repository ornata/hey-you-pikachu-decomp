/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "heap_types.h"

/* Debug trace: compiled out in this build, but each format string is still emitted
 * to .rodata and the dead do/if blocks still shape uopt's flow graph. */
#define TRACE(args)                                                                                                    \
    do {                                                                                                               \
        if (0) {                                                                                                       \
            osSyncPrintf args;                                                                                         \
        }                                                                                                              \
    } while (0)

/* Each heap routine keeps a static pointer to its name for the heap checker; the
 * debug prints that also used the names are compiled out. */
typedef struct HpBlk {
    struct HpBlk *prev;
    struct HpBlk *next;
    u32 size;
    u16 flag;
    u8 magic;
    u8 tag;
} HpBlk;

void _bzero(void *, s32);

/// Initialize a heap in a caller-provided memory region.
/// \param heap Writable heap header to initialize.
/// \param regionAddress RAM address of a 16-byte-aligned writable region that remains valid for the heap lifetime.
/// \param regionSize Region size in bytes, at least sixteen, including heap metadata.
/// \returns RAM address of the initialized region.
/// \note Original decompilation identifier: \c func_80037F00 (0x80037F00).
s32 heapInitializeRegion(HpHdr *heap, s32 regionAddress, s32 regionSize) {
    HpHdr *header = (HpHdr *)heap;
    HpBlk *firstBlock = (HpBlk *)regionAddress;

    _bzero(firstBlock, regionSize);
    firstBlock->next = firstBlock;
    firstBlock->prev = firstBlock;
    firstBlock->size = regionSize - 0x10;
    firstBlock->flag = 0xFFFF;
    firstBlock->magic = 0xA5;
    firstBlock->tag = 0;
    header->thread = osGetThreadId(NULL);
    header->size = regionSize;
    header->first = firstBlock;
    header->last = firstBlock;
    return (s32)firstBlock;
}

/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;
/// Depth-buffer storage; its start also bounds the game heap and is used as asset-loading scratch.
/// \note Original identifier: \c D_8038F800 (0x8038F800).
extern u8 gDepthBuffer[];

/// Initialize the global game heap from a RAM starting address.
/// \param startAddress Aligned RAM address below the global heap limit.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80037F88 (0x80037F88).
void heapInitializeGlobal(s32 startAddress) {
    heapInitializeRegion(&gGameHeap, startAddress, (s32)gDepthBuffer - startAddress);
}

/// Provide an inactive heap diagnostic hook.
/// \param heapAddress Unused diagnostic argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80037FBC (0x80037FBC).
void heapDebugNoOp(heapAddress) s32 heapAddress;
{
}

/// Inspect the heap for the disabled block diagnostic report.
/// \param heap Initialized heap to operate on.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80037FC4 (0x80037FC4).
void heapDebugInspectBlocks(u8 *heap) {
    u8 *p = *(u8 **)(heap + 8);

    do {
        TRACE(("un"));
        p = *(u8 **)(p + 4);
    } while (p != *(u8 **)(heap + 8));
}

/// Run the heap diagnostic hook for a block.
/// \param heapAddress RAM address of the heap header.
/// \param blockAddress RAM address of the block to report.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80037FE0 (0x80037FE0).
void heapDebugReportBlock(s32 heapAddress, s32 blockAddress) {
    heapDebugNoOp(heapAddress, blockAddress + 0x10);
}

/// Check whether a heap block is valid for the requested allocation state.
/// \param heap Initialized heap to operate on.
/// \param label Diagnostic label; unused in this build.
/// \param block Readable block header to validate; heap membership is not checked.
/// \param requiredState Required allocation state when positive; nonpositive accepts any state.
/// \returns 1 when the block passes validation, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80038000 (0x80038000).
s32 heapValidateBlock(u8 *heap, char *label, u8 *block, s32 requiredState) {
    if ((s32)block & 1) {
        return 0;
    }
    if (*(u8 *)(block + 0xE) != 0xA5) {
        heapDebugReportBlock((s32)heap, (s32)block);
        return 0;
    }
    if ((requiredState > 0) && (requiredState != *(u8 *)(block + 0xF))) {
        heapDebugReportBlock((s32)heap, (s32)block);
        return 0;
    }
    return 1;
}

/// Reduce a block to a requested size and expose its unused remainder.
/// \param block Block whose payload size may be reduced.
/// \param size Requested retained payload size in bytes.
/// \returns Free remainder block, or NULL if the remainder would be too small.
/// \note Original decompilation identifier: \c func_80038074 (0x80038074).
HpBlk *heapSplitBlock(HpBlk *block, u32 size) {
    static char *name = "_divide";
    HpBlk *next;

    TRACE(("_divide(%p,%d)\n", block, size));
    size = (size + 15) & ~15;
    if (block->size < size + 0x30) {
        TRACE(("_divide: not divide %p(%d/%d)\n", block, size, block->size));
        return NULL;
    }
    next = (HpBlk *)((u32)block + size + 0x10);
    block->next->prev = next;
    next->prev = block;
    next->next = block->next;
    next->size = block->size - ((u32)next - (u32)block);
    next->flag = 0xFFFF;
    next->magic = 0xA5;
    next->tag = 0;
    block->next = next;
    block->size = ((u32)next - (u32)block) - 0x10;
    TRACE(("hp:%p prev:%p next:%p size:%d\ndp:%p prev:%p next:%p size:%d\n\n", block, block->prev, block->next,
           block->size, next, next->prev, next->next, next->size));
    return next;
}

/// Allocate tagged storage from a heap.
/// \param heap Initialized heap to operate on.
/// \param size Requested payload size in bytes.
/// \param flag Allocation label to record for the storage.
/// \returns Allocated payload, or NULL if allocation fails.
/// \note Original decompilation identifier: \c func_800380F4 (0x800380F4).
void *heapAllocLocal(HpHdr *heap, u32 size, u16 flag) {
    static char *name = "heapAllocLocal";
    HpBlk *block = heap->last;

    TRACE(("heapAllocLocal(%p, %d, %d)\n", heap, size, flag));
    while (block->tag != 0 || block->size < size) {
        if (!heapValidateBlock((u8 *)heap, name, (u8 *)block, 0)) {
            return NULL;
        }
        block = block->next;
        if (block == heap->last) {
            TRACE(("heapAllocLocal: Warning: Not enough memory.\n"));
            return NULL;
        }
    }
    heapSplitBlock(block, size);
    block->flag = flag;
    block->tag = 1;
    heap->last = block->next;
    TRACE(("hp:%p owner:%d\n", block, block->tag));
    TRACE(("heapAllocLocal: %p\n", block + 1));
    return block + 1;
}

/// Allocate tagged storage from the upper end of a suitable heap region.
/// \param heap Initialized heap to operate on.
/// \param size Requested payload size in bytes.
/// \param flag Allocation label to record for the storage.
/// \returns Allocated payload, or NULL if allocation fails.
/// \note Original decompilation identifier: \c func_800381D4 (0x800381D4).
void *heapAllocTailLocal(HpHdr *heap, u32 size, u16 flag) {
    static char *name = "heapAllocLocal";
    HpBlk *block = heap->last;
    HpBlk *tail;

    TRACE(("heapAllocTail(%d,%d)\n", size, flag));
    size = ((size + 15) & ~15) + 16;
    while (block->tag != 0 || block->size < size) {
        if (!heapValidateBlock((u8 *)heap, name, (u8 *)block, 0)) {
            return NULL;
        }
        block = block->next;
        if (block == heap->last) {
            TRACE(("heapAllocTailLocal: Warning: Not enough memory.\n"));
            return NULL;
        }
    }
    size = block->size - size;
    tail = heapSplitBlock(block, size);
    if (tail != NULL) {
        block = tail;
    }
    block->flag = flag;
    block->tag = 1;
    TRACE(("hp:%p owner:%d\n", block, block->tag));
    return block + 1;
}

/// Combine a free block with preceding free space when possible.
/// \param heap Initialized heap to operate on.
/// \param b Free block whose preceding space may be reclaimed.
/// \returns Resulting free block, which may start before b.
/// \note Original decompilation identifier: \c func_800382BC (0x800382BC).
HpBlk *heapCombinePreviousFree(u8 *heap, HpBlk *b) {
    static char *name = "heapPrevCombine";
    HpBlk *p = b->prev;

    if (p >= b || p->tag != 0) {
        return b;
    }
    if (heapValidateBlock(heap, name, (u8 *)p, 0) == 0) {
        return b;
    }
    if (b != (HpBlk *)((u8 *)p + p->size + 0x10)) {
        TRACE(("%s: Error: Heap chain is broken at %p\n", name, b));
        heapDebugReportBlock((s32)heap, (s32)b);
    }
    if (b == ((HpHdr *)heap)->last) {
        ((HpHdr *)heap)->last = p;
    }
    p->next = b->next;
    p->size = p->size + b->size + 0x10;
    b->next->prev = p;
    b->magic = 0;
    return p;
}

/// Combine a free block with following free space when possible.
/// \param heap Initialized heap to operate on.
/// \param b Free block whose following space may be reclaimed.
/// \returns Resulting free block.
/// \note Original decompilation identifier: \c func_80038394 (0x80038394).
HpBlk *heapCombineNextFree(u8 *heap, HpBlk *b) {
    static char *name = "heapNextCombine";
    HpBlk *n = b->next;

    if (b >= n || n->tag != 0) {
        return b;
    }
    if (heapValidateBlock(heap, name, (u8 *)n, 0) == 0) {
        return b;
    }
    if (n != (HpBlk *)((u8 *)b + b->size + 0x10)) {
        TRACE(("%s: Error: Heap chain is broken at %p\n", name, n));
        heapDebugReportBlock((s32)heap, (s32)b);
    }
    if (n == ((HpHdr *)heap)->last) {
        ((HpHdr *)heap)->last = b;
    }
    b->next = n->next;
    b->size = b->size + n->size + 0x10;
    n->next->prev = b;
    n->magic = 0;
    return b;
}

HpBlk *heapCombineNextFree(u8 *, HpBlk *);

/// Release excess storage from an existing heap allocation.
/// \param heap Initialized heap to operate on.
/// \param allocation Allocated payload to shrink, or NULL for no operation.
/// \param size Requested retained payload size in bytes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003846C (0x8003846C).
void heapShrinkLocal(u8 *heap, void *allocation, s32 size) {
    static char *name = "heapShrinkLocal";
    HpBlk *b;
    HpBlk *n;

    if (allocation == NULL) {
        TRACE(("%s: warning: tried to shrink NULL\n", name));
        return;
    }
    b = (HpBlk *)allocation - 1;
    if (heapValidateBlock(heap, name, (u8 *)b, 1) == 0) {
        return;
    }
    n = heapSplitBlock(b, size);
    if (n == NULL) {
        return;
    }
    heapCombineNextFree(heap, n);
    if ((u32)b < *(u32 *)(heap + 0xC)) {
        *(HpBlk **)(heap + 0xC) = b;
    }
}

/// Get the available payload size of a heap allocation.
/// \param heap Initialized heap to operate on.
/// \param allocationAddress RAM address of a payload with a readable heap header, or zero.
/// \returns Allocation payload size in bytes, or -1 for a null allocation or failed block validation.
/// \note Original decompilation identifier: \c func_800384F0 (0x800384F0).
s32 heapGetAllocationSize(u8 *heap, s32 allocationAddress) {
    static char *name = "heapCheckSize";
    u8 *block;
    int blockAddress;
    u8 *savedBlock;
    s32 *allocationAddressPointer;

    if (allocationAddress == 0) {
        TRACE(("%s: warning: tried to check NULL\n", name));
        return -1;
    }
    allocationAddressPointer = &allocationAddress;
    savedBlock = (block = (u8 *)(blockAddress = (*allocationAddressPointer) - 0x10));
    if (heapValidateBlock(heap, name, block, 1) == 0) {
        return -1;
    }
    return *(s32 *)(block + 8);
}

HpBlk *heapCombinePreviousFree(u8 *, HpBlk *);

/// Release an allocation back to its heap.
/// \param heap Initialized heap to operate on.
/// \param allocation Allocated payload to release, or NULL for no operation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80038548 (0x80038548).
void heapFreeLocal(u8 *heap, void *allocation) {
    static char *name = "heapFreeLocal";
    HpBlk *b;

    TRACE(("heapFreeLocal(%p, %p)\n", heap, allocation));
    if (allocation == NULL) {
        TRACE(("%s: warning: tried to free NULL\n", name));
        return;
    }
    b = (HpBlk *)allocation - 1;
    if (heapValidateBlock(heap, name, (u8 *)b, 1) == 0) {
        return;
    }
    b->tag = 0;
    b = heapCombinePreviousFree(heap, b);
    heapCombineNextFree(heap, b);
    if ((u32)b < *(u32 *)(heap + 0xC)) {
        *(HpBlk **)(heap + 0xC) = b;
        TRACE(("free: hp:%p prev:%p next:%p size:%d owner:%d\n", b, b->prev, b->next, b->size, b->tag));
    }
}

s32 heapValidateBlock(u8 *, char *, u8 *, s32); /* extern */

/// Measure total free space and the largest available heap allocation.
/// \param heap Initialized heap to operate on.
/// \param largest Receives the largest free payload size in bytes when validation succeeds.
/// \returns Total free payload bytes, or zero if block validation fails.
/// \note Original decompilation identifier: \c func_800385D0 (0x800385D0).
s32 heapGetFreeSpace(u8 *heap, u32 *largest) {
    static char *name = "heapGetSize";
    char *diagnosticName;
    s32 freeBytes;
    u32 blockSize;
    u32 largestSize;
    u8 *block;

    *largest = 0;
    freeBytes = 0;
    block = (u8 *)((HpHdr *)heap)->last;
    diagnosticName = name;
loop_1:
    if (heapValidateBlock(heap, diagnosticName, block, -1) == 0) {
        return 0;
    }
    if (((HpBlk *)block)->tag == 0) {
        blockSize = ((HpBlk *)block)->size;
        largestSize = *largest;
        freeBytes += blockSize;
        if (blockSize < largestSize) {
            *largest = largestSize;
        } else {
            *largest = blockSize;
        }
    }
    block = (u8 *)((HpBlk *)block)->next;
    if (block == (u8 *)((HpHdr *)heap)->last) {
        return freeBytes;
    }
    goto loop_1;
}

/// Inspect heap free space for the disabled diagnostic report.
/// \param heap Initialized heap to operate on.
/// \param label Diagnostic report label; unused in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80038690 (0x80038690).
void heapDebugReportFreeSpace(u8 *heap, char *label) {
    u32 max;
    s32 total;

    total = heapGetFreeSpace(heap, &max);
    if (heap != (u8 *)&gGameHeap) {
        TRACE(("heapRegion:%p ", heap));
    } else {
        TRACE(("globalHeap "));
    }
    TRACE(("%s total 0x%x  max 0x%x\n", label, total, max));
}

/// Select the region after an allocation for subsequent heap allocation searches.
/// \param heap Initialized heap to operate on.
/// \param allocation Allocated payload to advance past; NULL retains the current selection.
/// \returns RAM payload address at the resulting allocation cursor.
/// \note Original decompilation identifier: \c func_800386B8 (0x800386B8).
s32 heapAdvanceAllocationCursor(u8 *heap, void *allocation) {
    static char *name = "setHeapEntryNext";
    HpBlk *b;

    if (allocation == NULL) {
        TRACE(("%s: warning: tried to set entry to NULL\n", name));
        return *(s32 *)(heap + 0xC) + 0x10;
    }
    b = (HpBlk *)allocation - 1;
    if (heapValidateBlock(heap, name, (u8 *)b, 1) == 0) {
        return *(s32 *)(heap + 0xC) + 0x10;
    }
    *(HpBlk **)(heap + 0xC) = b->next;
    return *(s32 *)(heap + 0xC) + 0x10;
}

/// Choose a free region meeting the requested size for subsequent heap allocations.
/// If none meets the request, choose the largest free region; if none is free, retain the cursor.
/// \param heap Initialized heap to operate on.
/// \param desiredSize Desired free payload size in bytes; zero retains the current selection.
/// \returns Payload address at the selected cursor, or NULL if block validation fails.
/// The returned address does not reserve storage.
/// \note Original decompilation identifier: \c func_80038728 (0x80038728).
u8 *heapSelectAllocationRegion(u8 *heap, u32 desiredSize) {
    static char *name = "setHeapEntryMax";
    char *diagnosticName;
    u32 blockSize;
    u32 largestSize;
    u8 *block;
    u8 *selectedBlock;
    block = *(u8 **)(heap + 0xC);
    largestSize = 0;
    selectedBlock = block;
    if (desiredSize != 0) {
        diagnosticName = name;
        do {
            if (heapValidateBlock(heap, diagnosticName, block, -1) == 0) {
                return NULL;
            }
            if (block[0xF] == 0) {
                blockSize = *(u32 *)(block + 8);
                if (largestSize < blockSize) {
                    largestSize = blockSize;
                    selectedBlock = block;
                }
            }
            block = *(u8 **)(block + 4);
        } while ((block != *(u8 **)(heap + 0xC)) && (largestSize < desiredSize));
    }
    *(u8 **)(heap + 0xC) = selectedBlock;
    return selectedBlock + 0x10;
}

/// Append a segment of the heap usage visualization.
/// \param gdl Display-list cursor, advanced past the generated commands.
/// \param width Usable bar width in pixels.
/// \param owned Nonzero for allocated space; zero for free space.
/// \param x0 Current segment left edge in pixels.
/// \param x1 Receives the segment right edge in pixels.
/// \param y Vertical bar position in pixels.
/// \param size Cumulative heap byte offset at the segment right edge.
/// \param total Nonzero total heap region size in bytes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800387F8 (0x800387F8).
void heapDrawUsageSegment(Gfx **gdl, s32 width, s32 owned, s32 *x0, s32 *x1, s32 y, s32 size, u32 total) {
    *x1 = (u32)(width * size) / total + 20;
    gDPPipeSync((*gdl)++);
    if (owned) {
        gDPSetPrimColor((*gdl)++, 0, 0, 255, 0, 0, 255);
    } else {
        gDPSetPrimColor((*gdl)++, 0, 0, 0, 255, 0, 255);
    }
    gDPFillRectangle((*gdl)++, *x0, y, *x1, y + (owned ? 3 : 1));
    TRACE(("use:%d sz:%d (%d-%d)\n", owned, size, *x0, *x1));
}

/// Append a visualization of heap usage and the allocation cursor.
/// \param heap Heap whose usage is to be displayed.
/// \param gdl Display-list cursor, advanced past the generated commands.
/// \param width Available screen width in pixels.
/// \param y Vertical bar position in pixels.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80038900 (0x80038900).
void heapDrawUsage(HpHdr *heap, Gfx **gdl, s32 width, s32 y) {
    HpBlk *block;
    s32 tag = 0;
    s32 x0 = 20;
    s32 x1;
    s32 used = 0;
    HpBlk *first;

    gDPPipeSync((*gdl)++);
    gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
    gDPSetCombineMode((*gdl)++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetRenderMode((*gdl)++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    width -= 40;
    x1 = (u32)((heap->last - heap->first) * sizeof(HpBlk) * width) / (u32)heap->size + 20;
    gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 255);
    gDPFillRectangle((*gdl)++, x1, y - 1, x1 + 1, y + 4);
    block = heap->first;
    do {
        if (tag != block->tag) {
            TRACE(("use:%d size:%d\n", tag, used));
            heapDrawUsageSegment(gdl, width, tag, &x0, &x1, y, used, heap->size);
            tag = block->tag;
            x0 = x1;
        }
        used += block->size + sizeof(HpBlk);
        block = block->next;
        first = heap->first;
    } while (block != first);
    heapDrawUsageSegment(gdl, width, tag, &x0, &x1, y, used, heap->size);
}
