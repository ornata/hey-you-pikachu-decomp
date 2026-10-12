/// \file
/// Defines keyed value storage shared by lookup and ordered traversal.

#ifndef KEYED_POOL_H
#define KEYED_POOL_H

#include "keyed_pool_types.h"

/// Initialize an empty pool for keyed values.
/// \param pool Pool header to initialize.
/// \param count Positive number of entries the pool can hold.
/// \returns Nothing.
///
/// \note UB: the first predecessor is formed before the array, then replaced below.
/// \note Original decompilation identifier: \c func_80041670 (0x80041670).
void keyedPoolInitialize(KeyedPool *pool, u32 count);

/// Release the entry storage owned by a keyed pool without releasing its stored values.
/// \param pool Pool header whose entry storage is to be released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800417EC (0x800417EC).
void keyedPoolDestroy(KeyedPool *pool);

/// Append an entry to the pool traversal order.
/// \param pool Pool to operate on.
/// \param entry Entry to append; must be detached from the traversal order.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041818 (0x80041818).
void keyedPoolAppendOrderedEntry(KeyedPool *pool, KeyedPoolEntry *entry);

/// Remove an entry from the pool traversal order.
/// \param pool Pool to operate on.
/// \param entry Entry currently present in the traversal order.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041850 (0x80041850).
void keyedPoolUnlinkOrderedEntry(KeyedPool *pool, KeyedPoolEntry *entry);

/// Reserve an unused pool entry.
/// \param pool Pool with available entry storage.
/// \param entry Specific unused entry to reserve, or NULL to choose an available entry.
/// \returns Reserved entry.
/// \note Original decompilation identifier: \c func_800418A0 (0x800418A0).
KeyedPoolEntry *keyedPoolAcquireEntry(KeyedPool *pool, KeyedPoolEntry *entry);

/// Return a detached pool entry to the available entries.
/// \param pool Pool to operate on.
/// \param entry Reserved entry that is no longer referenced by the pool.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041910 (0x80041910).
void keyedPoolReleaseEntry(KeyedPool *pool, KeyedPoolEntry *entry);

/// Relocate a live entry to different pool storage while preserving traversal position.
/// \param tab Pool to operate on.
/// \param e Live entry to relocate.
/// \param n Reserved destination entry from the same pool that will replace e.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041944 (0x80041944).
void keyedPoolRelocateEntry(KeyedPool *tab, KeyedPoolEntry *e, KeyedPoolEntry *n);

/// Add a keyed value while preserving existing entries with the same key.
/// \param tab Pool with available entry storage.
/// \param val Nonzero value to store.
/// \param key Lookup key for the value.
/// \returns 1 on success, or 0 on reported insertion failure.
/// \note Original decompilation identifier: \c func_80041A54 (0x80041A54).
s32 keyedPoolInsert(KeyedPool *tab, s32 val, u32 key);

/// Store a value under the next automatically assigned identifier.
/// \param pool Pool with available entry storage.
/// \param value Nonzero value to store.
/// \returns Assigned identifier on success, or 0 on reported insertion failure.
/// \note Original decompilation identifier: \c func_80041BB8 (0x80041BB8).
s32 keyedPoolInsertWithNextId(KeyedPool *pool, s32 value);

/// Check whether a pool has unused entry storage.
/// \param pool Pool to operate on.
/// \returns 1 when an entry is available, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80041C04 (0x80041C04).
s32 keyedPoolHasAvailableEntry(KeyedPool *pool);

/// Find an entry by its lookup key.
/// \param poolAddress RAM address of the pool header.
/// \param lookupKey Key to find.
/// \param predecessor Receives the preceding searched entry address, or zero if none was visited.
/// \returns A borrowed matching entry, or NULL if the key is absent.
/// Entry addresses may change when the pool is modified.
/// \note Original decompilation identifier: \c func_80041C14 (0x80041C14).
KeyedPoolEntry *keyedPoolFindEntry(s32 poolAddress, s32 lookupKey, s32 *predecessor);

/// Find an entry matching both a key and a value.
/// \param poolAddress RAM address of the pool header.
/// \param val Value that the entry must contain.
/// \param lookupKey Key that the entry must match.
/// \param predecessor Receives the preceding searched entry address, or zero if none was visited.
/// \returns The borrowed matching entry address, or zero if no entry matches.
/// Entry addresses may change when the pool is modified.
/// \note Original decompilation identifier: \c func_80041CD0 (0x80041CD0).
s32 keyedPoolFindEntryWithValue(s32 poolAddress, s32 val, s32 lookupKey, s32 *predecessor);

/// Remove a known entry and release its pool storage.
/// \param poolAddress RAM address of the pool header.
/// \param entryAddress RAM address of the live entry to remove.
/// \param predecessorAddress Address of its lookup predecessor, or zero for the first entry.
/// \returns Value stored in the removed entry.
/// \note Original decompilation identifier: \c func_80041D98 (0x80041D98).
s32 keyedPoolRemoveEntry(s32 poolAddress, s32 entryAddress, s32 predecessorAddress);

/// Remove the first entry matching a lookup key.
/// \param poolAddress RAM address of the pool header.
/// \param key Lookup key to remove.
/// \returns Removed value, or zero if the key is absent.
/// \note Original decompilation identifier: \c func_80041E14 (0x80041E14).
s32 keyedPoolRemoveFirstForKey(s32 poolAddress, s32 key);

/// Remove all entries matching a lookup key.
/// \param tab RAM address of the pool header.
/// \param key Lookup key whose entries are to be removed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041E54 (0x80041E54).
void keyedPoolRemoveAllForKey(s32 tab, s32 key);

/// Remove the first entry matching both a key and a value.
/// \param poolAddress RAM address of the pool header.
/// \param value Value that the entry must contain.
/// \param key Key that the entry must match.
/// \returns Removed value, or zero if no entry matches.
/// \note Original decompilation identifier: \c func_80041F3C (0x80041F3C).
s32 keyedPoolRemoveKeyAndValue(s32 poolAddress, s32 value, s32 key);

/// Get the value of the first entry matching a key.
/// \param poolAddress RAM address of the pool header.
/// \param key Lookup key to find.
/// \returns Matching value, or zero if the key is absent.
/// \note Original decompilation identifier: \c func_80041F7C (0x80041F7C).
s32 keyedPoolGetValue(s32 poolAddress, s32 key);

/// Get a keyed value and move its entry to the end of the traversal order.
/// \param poolAddress RAM address of the pool header.
/// \param key Lookup key to find.
/// \returns Matching value, or zero if the key is absent.
/// \note Original decompilation identifier: \c func_80041FB0 (0x80041FB0).
s32 keyedPoolGetValueAndMoveToEnd(s32 poolAddress, s32 key);

/// Start traversal at the first ordered entry, replacing the pool shared traversal position.
/// \param pool Pool to operate on.
/// \returns First value, or zero if the pool is empty.
/// \note Original decompilation identifier: \c func_80042004 (0x80042004).
s32 keyedPoolIterateFirst(KeyedPool *pool);

/// Advance to the next ordered entry, starting at the first if no entry is selected.
/// \param it Pool header containing the current traversal position.
/// \returns Next value, or zero after the last entry or when the pool is empty.
/// \note Original decompilation identifier: \c func_80042028 (0x80042028).
s32 keyedPoolIterateNext(KeyedPool *it);

/// Start keyed traversal at the first matching entry, replacing the pool shared traversal position.
/// \param pool Pool to operate on.
/// \param key Lookup key to select.
/// \returns First matching value, or zero if the key is absent.
/// \note Original decompilation identifier: \c func_80042078 (0x80042078).
s32 keyedPoolIterateFirstForKey(KeyedPool *pool, s32 key);

/// Advance keyed traversal to the next entry matching a key.
/// \param l Pool header with a currently selected entry.
/// \param key Lookup key to find after the current entry.
/// \returns Next matching value, or zero if no further entry matches.
/// \note Original decompilation identifier: \c func_800420B4 (0x800420B4).
s32 keyedPoolIterateNextForKey(KeyedPool *l, s32 key);

/// Remove the first entry matching the key and value selected by the traversal position.
/// When duplicate pairs exist, the removed entry may differ from the selected entry.
/// \param pool Pool to operate on.
/// \returns Removed value, or zero when no entry is selected.
/// \note Original decompilation identifier: \c func_800420E8 (0x800420E8).
s32 keyedPoolRemoveCurrent(KeyedPool *pool);

#endif
