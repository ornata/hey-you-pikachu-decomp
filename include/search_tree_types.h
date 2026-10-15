/// \file
/// Defines caller-owned search-tree nodes and the root/comparator view.

#ifndef SEARCH_TREE_TYPES_H
#define SEARCH_TREE_TYPES_H

#include <ultra64.h>

/// A caller-owned entry linked to lesser and greater keys.
/// Node storage must remain valid until the entry is detached from its tree.
typedef struct TreeNode {
    struct TreeNode *links[2]; ///< Lesser-key and greater-key subtree roots.
    s32 value;                 ///< Key or payload interpreted by the tree comparator.
} TreeNode;

/// Compare a search key against a live tree entry.
/// \param node Borrowed entry whose stored value supplies the existing key.
/// \param key Target-width key or address interpreted by this tree's comparator.
/// \returns Negative if key precedes the entry, zero if equal, or positive if it follows.
/// \pre Compared values must retain a consistent ordering while their nodes are linked.
/// The callback must not change tree links or invalidate either compared value.
typedef s32 (*TreeCompare)(TreeNode *node, s32 key);

/// Search-tree root and retained comparison callback; two words remain unidentified.
/// Search operations may replace the root while preserving caller-owned nodes.
typedef struct SearchTree {
    s32 unknown0;        ///< Cleared on initialization; no established later role.
    TreeNode *root;      ///< Borrowed root of caller-owned linked entries, or null.
    s32 unknown8;        ///< Cleared on initialization; no established later role.
    TreeCompare compare; ///< Retained ordering callback, valid for this tree's lifetime.
} SearchTree;

typedef char TreeNodeSizeCheck[(sizeof(TreeNode) == 0xC) ? 1 : -1];
typedef char TreeNodeLinksOffsetCheck[((u32) & ((TreeNode *)0)->links == 0) ? 1 : -1];
typedef char TreeNodeValueOffsetCheck[((u32) & ((TreeNode *)0)->value == 8) ? 1 : -1];
typedef char SearchTreeSizeCheck[(sizeof(SearchTree) == 0x10) ? 1 : -1];
typedef char SearchTreeRootOffsetCheck[((u32) & ((SearchTree *)0)->root == 4) ? 1 : -1];
typedef char SearchTreeCompareOffsetCheck[((u32) & ((SearchTree *)0)->compare == 0xC) ? 1 : -1];

#endif
