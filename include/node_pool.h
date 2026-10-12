/// \file
/// Defines the ordered value pool and its traversal interface.

#ifndef NODE_POOL_H
#define NODE_POOL_H

#include <ultra64.h>

/// Entry shared between the pool's available and ordered lists.
typedef struct PoolNode {
    s32 value;                 ///< Caller-defined stored value.
    struct PoolNode *previous; ///< Predecessor in the current list, or NULL.
    struct PoolNode *next;     ///< Successor in the current list, or NULL.
} PoolNode;

/// Fixed-capacity pool of values with a mutable traversal cursor.
typedef struct NodePool {
    s32 count;        ///< Number of allocated entries.
    PoolNode *nodes;  ///< Owned entry storage.
    PoolNode *head;   ///< First ordered entry, or NULL.
    PoolNode *tail;   ///< Last ordered entry, or NULL.
    PoolNode *free;   ///< First available entry, or NULL.
    PoolNode *cursor; ///< Selected ordered entry, or NULL.
} NodePool;

typedef char NodePoolNodeSizeCheck[(sizeof(PoolNode) == 0xC) ? 1 : -1];
typedef char NodePoolSizeCheck[(sizeof(NodePool) == 0x18) ? 1 : -1];
#define NODE_POOL_OFFSET(type, field) ((u32) & (((type *)0)->field))
typedef char NodePoolFreeOffsetCheck[(NODE_POOL_OFFSET(NodePool, free) == 0x10) ? 1 : -1];
typedef char NodePoolCursorOffsetCheck[(NODE_POOL_OFFSET(NodePool, cursor) == 0x14) ? 1 : -1];
#undef NODE_POOL_OFFSET

/// Initialize an empty pool.
/// \param pool Writable pool header.
/// \param count Positive entry capacity.
/// \returns Nothing.
void nodePoolInitialize(NodePool *pool, s32 count);

/// Release a pool's entry storage.
/// \param pool Initialized pool to destroy.
/// \returns Nothing.
void nodePoolDestroy(NodePool *pool);

/// Append a value to the traversal order.
/// \param pool Pool with an available entry.
/// \param value Caller-defined value to store.
/// \returns Nothing.
void nodePoolAppend(NodePool *pool, s32 value);

/// Select the first ordered entry.
/// \param pool Pool whose cursor is reset.
/// \param value Receives the selected value, or zero for an empty pool.
/// \returns One when an entry is selected, otherwise zero.
s32 nodePoolIterateFirst(NodePool *pool, s32 *value);

/// Advance the cursor, starting at the head when no entry is selected.
/// \param pool Pool whose cursor is advanced.
/// \param value Receives the selected value, or zero at the end.
/// \returns One when an entry is selected, otherwise zero.
s32 nodePoolIterateNext(NodePool *pool, s32 *value);

/// Remove the selected entry and select its predecessor.
/// \param pool Pool with a selected entry.
/// \returns The removed value.
s32 nodePoolRemoveCurrent(NodePool *pool);

/// Replace the selected value.
/// \param pool Pool with a selected entry.
/// \param value Replacement value.
/// \returns Nothing.
void nodePoolSetCurrentValue(NodePool *pool, s32 value);

/// Check whether another entry can be appended.
/// \param pool Pool to inspect.
/// \returns One when an entry is available, otherwise zero.
s32 nodePoolHasFreeEntry(NodePool *pool);

#endif
