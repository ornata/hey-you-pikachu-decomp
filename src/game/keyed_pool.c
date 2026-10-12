#include "common.h"
#include "heap_alloc.h"
#include "keyed_pool.h"

/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;
/// Initialize an empty pool for keyed values.
/// \param pool Pool header to initialize.
/// \param count Positive number of entries the pool can hold.
/// \returns Nothing.
///
/// \note UB: the first predecessor is formed before the array, then replaced below.
/// \note Original decompilation identifier: \c func_80041670 (0x80041670).
void keyedPoolInitialize(KeyedPool *pool, u32 count) {
    KeyedPoolEntry *node;
    u32 i;
    pool->capacity = count;
    pool->nextId = 1;
    pool->entries = (KeyedPoolEntry *)heapAllocLocal((u8 *)&gGameHeap, count * sizeof(KeyedPoolEntry), 0x27);
    i = 0;
    node = pool->entries;
    while (i != count) {
        node->key = 0;
        node->value = 0;
        node->chain = 0;

        node->previous = node - 1;
        node->next = node + 1;
        i++;
        node++;
    }
    node--;
    pool->entries->previous = node;
    node->next = pool->entries;
    pool->free = pool->entries + pool->capacity / 2;
    pool->free->previous->next = NULL;
    pool->free->previous = NULL;
    pool->head = NULL;
    pool->tail = NULL;
    pool->cursor = NULL;
    pool->active = 0;
}

// Preserve the ROM caller's unused third argument; see docs/TYPE_RECOVERY.md.
void heapFreeLocal();

/// Release the entry storage owned by a keyed pool without releasing its stored values.
/// \param pool Pool header whose entry storage is to be released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800417EC (0x800417EC).
void keyedPoolDestroy(KeyedPool *pool) {
    heapFreeLocal(&gGameHeap, pool->entries, pool);
}

/// Append an entry to the pool traversal order.
/// \param pool Pool to operate on.
/// \param entry Entry to append; must be detached from the traversal order.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041818 (0x80041818).
void keyedPoolAppendOrderedEntry(KeyedPool *pool, KeyedPoolEntry *entry) {
    if (pool->tail != NULL) {
        pool->tail->next = entry;
    }
    entry->previous = pool->tail;
    pool->tail = entry;
    entry->next = 0;
    if (pool->head == NULL) {
        pool->head = entry;
    }
}

/// Remove an entry from the pool traversal order.
/// \param pool Pool to operate on.
/// \param entry Entry currently present in the traversal order.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041850 (0x80041850).
void keyedPoolUnlinkOrderedEntry(KeyedPool *pool, KeyedPoolEntry *entry) {
    KeyedPoolEntry *previous;
    KeyedPoolEntry *next;

    previous = entry->previous;
    next = entry->next;
    if (entry == pool->head) {
        pool->head = next;
    } else {
        previous->next = next;
    }
    if (entry == pool->tail) {
        pool->tail = previous;
    } else {
        next->previous = previous;
    }
    if (entry == pool->cursor) {
        pool->cursor = previous;
    }
}

/// Reserve an unused pool entry.
/// \param pool Pool with available entry storage.
/// \param entry Specific unused entry to reserve, or NULL to choose an available entry.
/// \returns Reserved entry.
/// \note Original decompilation identifier: \c func_800418A0 (0x800418A0).
KeyedPoolEntry *keyedPoolAcquireEntry(KeyedPool *pool, KeyedPoolEntry *entry) {
    KeyedPoolEntry *firstFree;
    KeyedPoolEntry *nextFree;

    if (entry != NULL) {
        firstFree = pool->free;
        if (entry == firstFree) {
            pool->free = firstFree->next;
        } else {
            (entry->previous)->next = entry->next;
        }
    } else {
        entry = pool->free;
        pool->free = entry->next;
    }
    nextFree = entry->next;
    if (nextFree != NULL) {
        nextFree->previous = entry->previous;
    }
    entry->chain = 0;
    entry->next = NULL;
    entry->previous = 0;
    pool->active = (s32)(pool->active + 1);
    return entry;
}

/// Return a detached pool entry to the available entries.
/// \param pool Pool to operate on.
/// \param entry Reserved entry that is no longer referenced by the pool.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041910 (0x80041910).
void keyedPoolReleaseEntry(KeyedPool *pool, KeyedPoolEntry *entry) {
    KeyedPoolEntry *firstFree;

    entry->previous = 0;
    entry->value = 0;
    entry->next = pool->free;
    firstFree = pool->free;
    if (firstFree != NULL) {
        firstFree->previous = entry;
    }
    pool->free = entry;
    pool->active = (s32)(pool->active - 1);
}

/// Relocate a live entry to different pool storage while preserving traversal position.
/// \param tab Pool to operate on.
/// \param e Live entry to relocate.
/// \param n Reserved destination entry from the same pool that will replace e.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041944 (0x80041944).
void keyedPoolRelocateEntry(KeyedPool *tab, KeyedPoolEntry *e, KeyedPoolEntry *n) {
    KeyedPoolEntry *p;
    KeyedPoolEntry *a;
    KeyedPoolEntry *b;

    p = &tab->entries[(((~(e->key >> 12) + (e->key >> 8)) ^ ~(e->key >> 4)) + e->key * 0x2E7) % tab->capacity];
    while (p->chain != e) {
        p = p->chain;
    }
    p->chain = n;
    *n = *e;
    a = e->previous;
    if (e == tab->head) {
        tab->head = n;
    } else {
        a->next = n;
    }
    b = e->next;
    if (e == tab->tail) {
        tab->tail = n;
    } else {
        b->previous = n;
    }
    if (e == tab->cursor) {
        tab->cursor = n;
    }
    e->next = NULL;
    e->previous = NULL;
    e->chain = NULL;
}

/// Add a keyed value while preserving existing entries with the same key.
/// \param tab Pool with available entry storage.
/// \param val Nonzero value to store.
/// \param key Lookup key for the value.
/// \returns 1 on success, or 0 on reported insertion failure.
/// \note Original decompilation identifier: \c func_80041A54 (0x80041A54).
s32 keyedPoolInsert(KeyedPool *tab, s32 val, u32 key) {
    u32 h = (((~(key >> 12) + (key >> 8)) ^ ~(key >> 4)) + key * 0x2E7) % tab->capacity;
    KeyedPoolEntry *e = &tab->entries[h];
    KeyedPoolEntry *n;

    if (e->value == 0) {
        keyedPoolAcquireEntry(tab, e);
    } else if (h == (((~(e->key >> 12) + (e->key >> 8)) ^ ~(e->key >> 4)) + e->key * 0x2E7) % tab->capacity) {
        n = (KeyedPoolEntry *)keyedPoolAcquireEntry(tab, NULL);
        if (n == NULL) {
            return 0;
        }
        n->chain = e->chain;
        e->chain = n;
        e = n;
    } else {
        keyedPoolRelocateEntry(tab, e, (KeyedPoolEntry *)keyedPoolAcquireEntry(tab, NULL));
    }
    e->key = key;
    e->value = val;
    keyedPoolAppendOrderedEntry(tab, e);
    return 1;
}

/// Store a value under the next automatically assigned identifier.
/// \param pool Pool with available entry storage.
/// \param value Nonzero value to store.
/// \returns Assigned identifier on success, or 0 on reported insertion failure.
/// \note Original decompilation identifier: \c func_80041BB8 (0x80041BB8).
s32 keyedPoolInsertWithNextId(KeyedPool *pool, s32 value) {
    s32 nextId;
    s32 savedId;

    nextId = pool->nextId;
    savedId = nextId;
    if (keyedPoolInsert(pool, value, nextId) != 0) {
        pool->nextId = pool->nextId + 1;
        return nextId;
    }
    return 0;
}

/// Check whether a pool has unused entry storage.
/// \param pool Pool to operate on.
/// \returns 1 when an entry is available, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80041C04 (0x80041C04).
s32 keyedPoolHasAvailableEntry(KeyedPool *pool) {
    return pool->free != 0;
}

/// Find an entry by its lookup key.
/// \param poolAddress RAM address of the pool header.
/// \param lookupKey Key to find.
/// \param predecessor Receives the preceding searched entry address, or zero if none was visited.
/// \returns A borrowed matching entry, or NULL if the key is absent.
/// Entry addresses may change when the pool is modified.
/// \note Original decompilation identifier: \c func_80041C14 (0x80041C14).
KeyedPoolEntry *keyedPoolFindEntry(s32 poolAddress, s32 lookupKey, s32 *predecessor) {
    KeyedPool *t = (KeyedPool *)poolAddress;
    u32 key = lookupKey;
    KeyedPoolEntry **prev = (KeyedPoolEntry **)predecessor;
    KeyedPoolEntry *e;

    *prev = NULL;
    e = &t->entries[(((~(key >> 12) + (key >> 8)) ^ ~(key >> 4)) + key * 0x2E7) % t->capacity];
    for (; e != NULL && e->value != 0; e = e->chain) {
        if (key == e->key) {
            return e;
        }
        *prev = e;
    }
    return NULL;
}

/// Find an entry matching both a key and a value.
/// \param poolAddress RAM address of the pool header.
/// \param val Value that the entry must contain.
/// \param lookupKey Key that the entry must match.
/// \param predecessor Receives the preceding searched entry address, or zero if none was visited.
/// \returns The borrowed matching entry address, or zero if no entry matches.
/// Entry addresses may change when the pool is modified.
/// \note Original decompilation identifier: \c func_80041CD0 (0x80041CD0).
s32 keyedPoolFindEntryWithValue(s32 poolAddress, s32 val, s32 lookupKey, s32 *predecessor) {
    KeyedPool *tab = (KeyedPool *)poolAddress;
    u32 key = lookupKey;
    KeyedPoolEntry **prev = (KeyedPoolEntry **)predecessor;
    KeyedPoolEntry *e;
    s32 v;

    *prev = NULL;
    for (e = &tab->entries[(((~(key >> 12) + (key >> 8)) ^ ~(key >> 4)) + key * 0x2E7) % tab->capacity];
         e != NULL && (v = e->value) != 0; e = e->chain) {
        if (key == e->key && val == v) {
            return (s32)e;
        }
        *prev = e;
    }
    return 0;
}

/// Remove a known entry and release its pool storage.
/// \param poolAddress RAM address of the pool header.
/// \param entryAddress RAM address of the live entry to remove.
/// \param predecessorAddress Address of its lookup predecessor, or zero for the first entry.
/// \returns Value stored in the removed entry.
/// \note Original decompilation identifier: \c func_80041D98 (0x80041D98).
s32 keyedPoolRemoveEntry(s32 poolAddress, s32 entryAddress, s32 predecessorAddress) {
    s32 ret = ((KeyedPoolEntry *)entryAddress)->value;
    u8 *t;

    keyedPoolUnlinkOrderedEntry((KeyedPool *)poolAddress, entryAddress);
    if (predecessorAddress != 0) {
        ((KeyedPoolEntry *)predecessorAddress)->chain = ((KeyedPoolEntry *)entryAddress)->chain;
    } else {
        t = ((KeyedPoolEntry *)entryAddress)->chain;
        if (t != NULL) {
            keyedPoolRelocateEntry((KeyedPool *)poolAddress, (KeyedPoolEntry *)t, (KeyedPoolEntry *)entryAddress);
            entryAddress = (s32)t;
        }
    }
    keyedPoolReleaseEntry((KeyedPool *)poolAddress, entryAddress);
    return ret;
}

/// Remove the first entry matching a lookup key.
/// \param poolAddress RAM address of the pool header.
/// \param key Lookup key to remove.
/// \returns Removed value, or zero if the key is absent.
/// \note Original decompilation identifier: \c func_80041E14 (0x80041E14).
s32 keyedPoolRemoveFirstForKey(s32 poolAddress, s32 key) {
    KeyedPoolEntry *entry;
    {
        s32 predecessor;
        entry = keyedPoolFindEntry(poolAddress, key, &predecessor);
        if (entry == NULL) {
            return 0;
        }
        return keyedPoolRemoveEntry(poolAddress, (s32)entry, predecessor);
    }
}

/// Remove all entries matching a lookup key.
/// \param tab RAM address of the pool header.
/// \param key Lookup key whose entries are to be removed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041E54 (0x80041E54).
void keyedPoolRemoveAllForKey(s32 tab, s32 key) {
    KeyedPoolEntry *e;
    KeyedPoolEntry *prev;
    KeyedPoolEntry *head;
    KeyedPoolEntry *next;

    e = (KeyedPoolEntry *)keyedPoolFindEntry(tab, key, (s32 *)&prev);
    if (e == NULL) {
        return;
    }
    if (prev != NULL) {
        head = NULL;
    } else {
        head = e;
        prev = e;
        e = e->chain;
    }
    while (e != NULL && e->value != 0) {
        next = e->chain;
        if (key != e->key) {
            prev = e;
        } else {
            keyedPoolUnlinkOrderedEntry(tab, e);
            prev->chain = e->chain;
            keyedPoolReleaseEntry(tab, e);
        }
        e = next;
    }
    if (head != NULL) {
        keyedPoolRemoveEntry(tab, (s32)head, 0);
    }
}

/// Remove the first entry matching both a key and a value.
/// \param poolAddress RAM address of the pool header.
/// \param value Value that the entry must contain.
/// \param key Key that the entry must match.
/// \returns Removed value, or zero if no entry matches.
/// \note Original decompilation identifier: \c func_80041F3C (0x80041F3C).
s32 keyedPoolRemoveKeyAndValue(s32 poolAddress, s32 value, s32 key) {
    s32 entryAddress;
    {
        s32 predecessor;
        entryAddress = keyedPoolFindEntryWithValue(poolAddress, value, key, &predecessor);
        if (entryAddress == 0) {
            return 0;
        }
        return keyedPoolRemoveEntry(poolAddress, entryAddress, predecessor);
    }
}

/// Get the value of the first entry matching a key.
/// \param poolAddress RAM address of the pool header.
/// \param key Lookup key to find.
/// \returns Matching value, or zero if the key is absent.
/// \note Original decompilation identifier: \c func_80041F7C (0x80041F7C).
s32 keyedPoolGetValue(s32 poolAddress, s32 key) {
    KeyedPoolEntry *entry;
    {
        s32 predecessor;
        entry = keyedPoolFindEntry(poolAddress, key, &predecessor);
    }
    if (entry == NULL) {
        return 0;
    }
    return entry->value;
}

/// Get a keyed value and move its entry to the end of the traversal order.
/// \param poolAddress RAM address of the pool header.
/// \param key Lookup key to find.
/// \returns Matching value, or zero if the key is absent.
/// \note Original decompilation identifier: \c func_80041FB0 (0x80041FB0).
s32 keyedPoolGetValueAndMoveToEnd(s32 poolAddress, s32 key) {
    KeyedPoolEntry *entry;
    {
        s32 predecessor;
        entry = keyedPoolFindEntry(poolAddress, key, &predecessor);
    }
    if (entry == NULL) {
        return 0;
    }
    keyedPoolUnlinkOrderedEntry(poolAddress, entry);
    keyedPoolAppendOrderedEntry((KeyedPool *)poolAddress, entry);
    return entry->value;
}

/// Start traversal at the first ordered entry, replacing the pool shared traversal position.
/// \param pool Pool to operate on.
/// \returns First value, or zero if the pool is empty.
/// \note Original decompilation identifier: \c func_80042004 (0x80042004).
s32 keyedPoolIterateFirst(KeyedPool *pool) {
    KeyedPoolEntry *first;

    first = pool->head;
    pool->cursor = first;
    if (first == NULL) {
        return 0;
    }
    return pool->cursor->value;
}

/// Advance to the next ordered entry, starting at the first if no entry is selected.
/// \param it Pool header containing the current traversal position.
/// \returns Next value, or zero after the last entry or when the pool is empty.
/// \note Original decompilation identifier: \c func_80042028 (0x80042028).
s32 keyedPoolIterateNext(KeyedPool *it) {
    KeyedPoolEntry *n = it->cursor;

    if (n == NULL) {
        return keyedPoolIterateFirst(it);
    }
    it->cursor = n->next;
    n = it->cursor;
    if (n == NULL) {
        return 0;
    }
    return n->value;
}

/// Start keyed traversal at the first matching entry, replacing the pool shared traversal position.
/// \param pool Pool to operate on.
/// \param key Lookup key to select.
/// \returns First matching value, or zero if the key is absent.
/// \note Original decompilation identifier: \c func_80042078 (0x80042078).
s32 keyedPoolIterateFirstForKey(KeyedPool *pool, s32 key) {
    s32 predecessor;
    KeyedPoolEntry *selected;

    selected = keyedPoolFindEntry((s32)pool, key, &predecessor);
    pool->cursor = selected;
    if (selected == NULL) {
        return 0;
    }
    return pool->cursor->value;
}

/// Advance keyed traversal to the next entry matching a key.
/// \param l Pool header with a currently selected entry.
/// \param key Lookup key to find after the current entry.
/// \returns Next matching value, or zero if no further entry matches.
/// \note Original decompilation identifier: \c func_800420B4 (0x800420B4).
s32 keyedPoolIterateNextForKey(KeyedPool *l, s32 key) {
    for (;;) {
        l->cursor = l->cursor->chain;
        if (l->cursor == NULL) {
            return 0;
        }
        if (key == l->cursor->key) {
            return l->cursor->value;
        }
    }
}

/// Remove the first entry matching the key and value selected by the traversal position.
/// When duplicate pairs exist, the removed entry may differ from the selected entry.
/// \param pool Pool to operate on.
/// \returns Removed value, or zero when no entry is selected.
/// \note Original decompilation identifier: \c func_800420E8 (0x800420E8).
s32 keyedPoolRemoveCurrent(KeyedPool *pool) {
    KeyedPoolEntry *selected;
    selected = pool->cursor;
    if (selected == NULL) {
        return 0;
    }
    return keyedPoolRemoveKeyAndValue((s32)pool, selected->value, selected->key);
}
