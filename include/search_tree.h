/// \file
/// Declares initialization and callback contracts for caller-owned search trees.

#ifndef SEARCH_TREE_H
#define SEARCH_TREE_H

#include "search_tree_types.h"

/// Initialize an empty search tree with a key comparator.
/// \param tree Tree record to initialize; previous nodes must already be detached.
/// \param compare Callback comparing a target-width key against a node. A negative,
/// zero or positive result places the key before, equal to or after the node.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80040F20 (0x80040F20).
void searchTreeInitialize(SearchTree *tree, TreeCompare compare);

/// Search for a key or the minimum entry in a search tree.
/// \param rootAddress Address of the tree root, updated by the search.
/// \param minimum Nonzero to select the minimum entry instead of searching for key.
/// \param compare Callback called with a node and key; returns
/// negative, zero, or positive when the key is less than, equal to, or greater
/// than the node. Ignored for a minimum search.
/// \param key Search key, ignored when minimum is nonzero.
/// \returns For a key search, zero on a match or the final comparison result otherwise; 1 for an empty tree; -1 for a nonempty minimum search.
///
/// \note NONPORTABLE: paired detach/link statements preserve IDO source-line scheduling.
/// \note Original decompilation identifier: \c func_80040F54 (0x80040F54).
s32 searchTreeLocateRoot(TreeNode **rootAddress, s32 minimum, TreeCompare compare, s32 key);

/// Find a matching entry through an independently supplied root and comparator.
/// \param rootAddress Address of the tree root, updated by the search.
/// \param compare Callback comparing the key against a node.
/// \param key Key to locate.
/// \returns Borrowed matching node, or NULL when absent.
/// \note Original decompilation identifier: \c func_800411C8 (0x800411C8).
TreeNode *searchTreeFindAtRoot(TreeNode **rootAddress, TreeCompare compare, s32 key);

/// Insert a caller-owned node into an initialized search tree.
/// \param tree Tree receiving the entry and retaining its node storage.
/// \param node Detached writable node, valid until removed from the tree.
/// \param key Key interpreted by the tree comparator.
/// \returns The supplied borrowed node, now linked into the tree.
/// \note Original decompilation identifier: \c func_80041290 (0x80041290).
TreeNode *searchTreeInsert(SearchTree *tree, TreeNode *node, s32 key);

#endif
