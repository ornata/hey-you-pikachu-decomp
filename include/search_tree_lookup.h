/// \file
/// Declares node-returning lookup operations for caller-owned search trees.
#ifndef SEARCH_TREE_LOOKUP_H
#define SEARCH_TREE_LOOKUP_H
#include "search_tree_types.h"

/// Find a matching tree entry.
/// \param tree Initialized tree whose root may change during lookup.
/// \param key Key interpreted by the comparator.
/// \returns Matching borrowed node, or null when absent.
/// \note Original decompilation identifier: \c func_80041184 (0x80041184).
TreeNode *searchTreeFind(SearchTree *tree, s32 key);

/// Find the entry following a matching key.
/// \param tree Initialized tree whose root may change during lookup.
/// \param key Existing key whose successor is requested.
/// \returns Borrowed successor, or null when the key or successor is absent.
/// \note Original decompilation identifier: \c func_80041208 (0x80041208).
TreeNode *searchTreeFindSuccessor(SearchTree *tree, s32 key);

/// Find the minimum tree entry.
/// \param tree Initialized tree whose root may change during lookup.
/// \returns Borrowed minimum node, or null for an empty tree.
/// \note Original decompilation identifier: \c func_8004125C (0x8004125C).
TreeNode *searchTreeFindMinimum(SearchTree *tree);

#endif
