/// \file
/// Defines the object-manager allocation and its embedded search-tree entry.

#ifndef MANAGED_OBJECT_TYPES_H
#define MANAGED_OBJECT_TYPES_H

#include "object_types.h"
#include "search_tree_types.h"

/// Object-manager allocation owning its runtime prefix and embedded tree node.
/// Creation initializes entry to the embedded node; registration supplies its key
/// and links. Detach the object from search trees before destroying its storage.
/// A shallow copy borrows the original links, state and resources; it does not
/// become an independently registered object or extend those resources' lifetimes.
typedef struct ManagedObject {
    RuntimeObjectPrefix base; ///< Runtime transform, hierarchy links and kind-specific state.
    TreeNode *entry; ///< Borrowed active search entry, initially pointing to node in this allocation.
    TreeNode node; ///< Embedded tree entry whose lifetime is bounded by this object allocation.
} ManagedObject;

typedef char ManagedObjectSizeCheck[(sizeof(ManagedObject) == 0x68) ? 1 : -1];
typedef char ManagedObjectEntryOffsetCheck[((u32) & ((ManagedObject *)0)->entry == 0x58) ? 1 : -1];
typedef char ManagedObjectNodeOffsetCheck[((u32) & ((ManagedObject *)0)->node == 0x5C) ? 1 : -1];

#endif
