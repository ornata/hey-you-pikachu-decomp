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

/// Compare a search key with a node; negative, zero and positive select lesser,
/// matching and greater keys respectively.
typedef s32 (*TreeCompare)(TreeNode *, s32);

/// Search-tree root and comparison callback; unused words remain unidentified.
/// Search operations may replace the root while preserving caller-owned nodes.
typedef struct SearchTree {
    s32 unknown0;
    TreeNode *root;
    s32 unknown8;
    TreeCompare compare;
} SearchTree;

typedef char TreeNodeSizeCheck[(sizeof(TreeNode) == 0xC) ? 1 : -1];
typedef char SearchTreeSizeCheck[(sizeof(SearchTree) == 0x10) ? 1 : -1];
typedef char SearchTreeRootOffsetCheck[((u32) & ((SearchTree *)0)->root == 4) ? 1 : -1];
typedef char SearchTreeCompareOffsetCheck[((u32) & ((SearchTree *)0)->compare == 0xC) ? 1 : -1];

#endif
