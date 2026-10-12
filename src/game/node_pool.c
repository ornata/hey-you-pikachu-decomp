#include "common.h"
#include "heap_alloc.h"
#include "node_pool.h"
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern u8 gGameHeap[];

/// Initialize an empty ordered value pool.
/// \param pool Pool to initialize.
/// \param count Positive number of entries the pool can hold.
/// \returns Nothing.
///
/// \note UB: the first predecessor is formed before the array, then cleared below.
/// \note Original decompilation identifier: \c func_8001BE70 (0x8001BE70).
void nodePoolInitialize(NodePool *pool, s32 count) {
    PoolNode *node;
    s32 i;
    pool->count = count;
    pool->nodes = (PoolNode *)heapAllocLocal(gGameHeap, count * sizeof(PoolNode), 0x13);
    i = 0;
    node = pool->nodes;
    while (i != count) {
        node->value = 0;

        node->previous = node - 1;
        node->next = node + 1;
        i++;
        node++;
    }
    node--;
    node->next = NULL;
    pool->nodes->previous = NULL;
    pool->head = NULL;
    pool->tail = NULL;
    pool->cursor = NULL;
    pool->free = pool->nodes;
}

// The callee consumes two arguments. Keep this call unprototyped because the
// matching caller also passes its pool in a2; see docs/TYPE_RECOVERY.md.
void heapFreeLocal();

/// Release the storage owned by a value pool.
/// \param pool Initialized pool to destroy.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001BF8C (0x8001BF8C).
void nodePoolDestroy(NodePool *pool) {
    heapFreeLocal(&gGameHeap, pool->nodes, pool);
}

/// Append a value to the end of a pool's ordered entries.
/// \param pool Initialized pool with at least one free entry.
/// \param value Value to append.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001BFB8 (0x8001BFB8).
void nodePoolAppend(NodePool *pool, s32 value) {
    PoolNode *previousTail;
    PoolNode *entry;
    PoolNode *nextFree;

    entry = pool->free;
    nextFree = entry->next;
    if (nextFree != NULL) {
        nextFree->previous = 0;
    }
    previousTail = pool->tail;
    pool->free = nextFree;
    entry->next = NULL;
    entry->value = value;
    entry->previous = previousTail;
    if (previousTail != NULL) {
        previousTail->next = entry;
    }
    pool->tail = entry;
    if (pool->head == NULL) {
        pool->head = entry;
    }
}

/// Select the first value in a pool for iteration.
/// \param pool Pool whose iteration cursor is reset.
/// \param value Receives the first value, or zero when the pool is empty.
/// \returns 1 when an entry is selected, or 0 when the pool is empty.
/// \note Original decompilation identifier: \c func_8001C008 (0x8001C008).
s32 nodePoolIterateFirst(NodePool *pool, s32 *value) {
    PoolNode *first;

    first = pool->head;
    pool->cursor = first;
    if (first == NULL) {
        *value = 0;
        return 0;
    }
    *value = first->value;
    return 1;
}

/// Select the next value, starting at the first entry when no cursor is set.
/// \param pool Pool whose iteration cursor is advanced.
/// \param value Receives the selected value, or zero when no entry is available.
/// \returns 1 when an entry is selected, or 0 at the end of iteration or for an empty pool.
/// \note Original decompilation identifier: \c func_8001C034 (0x8001C034).
s32 nodePoolIterateNext(NodePool *pool, s32 *value) {
    PoolNode *selected;
    PoolNode *first;
    PoolNode *next;

    selected = pool->cursor;
    if (selected == NULL) {
        first = pool->head;
        pool->cursor = first;
        if (first == NULL) {
            *value = 0;
            return 0;
        }
        *value = first->value;
        return 1;
    }
    next = selected->next;
    pool->cursor = next;
    if (next == NULL) {
        *value = 0;
        return 0;
    }
    *value = next->value;
    return 1;
}

/// Remove the selected entry and move the iteration cursor to its predecessor.
/// \param pool Pool with a currently selected entry.
/// \returns The removed entry's value.
/// \note Original decompilation identifier: \c func_8001C094 (0x8001C094).
s32 nodePoolRemoveCurrent(NodePool *pool) {
    PoolNode *next;
    PoolNode *firstFree;
    PoolNode *previous;
    PoolNode *selected;
    s32 value;

    selected = pool->cursor;
    previous = selected->previous;
    next = selected->next;
    if (previous != NULL) {
        previous->next = next;
    } else {
        pool->head = next;
    }
    if (next != NULL) {
        next->previous = previous;
    } else {
        pool->tail = previous;
    }
    pool->cursor = previous;
    value = selected->value;
    firstFree = pool->free;
    selected->previous = NULL;
    selected->next = firstFree;
    if (firstFree != NULL) {
        firstFree->previous = selected;
    }
    pool->free = selected;
    return value;
}

/// Replace the value of the currently selected entry.
/// \param pool Pool with a currently selected entry.
/// \param value Replacement value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001C0F0 (0x8001C0F0).
void nodePoolSetCurrentValue(NodePool *pool, s32 value) {
    PoolNode *selected;

    selected = pool->cursor;
    selected->value = value;
}

/// Check whether a value pool can accept another entry.
/// \param pool Pool to inspect.
/// \returns 1 when an entry is free, or 0 when the pool is full.
/// \note Original decompilation identifier: \c func_8001C0FC (0x8001C0FC).
s32 nodePoolHasFreeEntry(NodePool *pool) {
    return pool->free != 0;
}
