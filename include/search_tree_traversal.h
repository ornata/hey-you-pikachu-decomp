/// \file
/// Declares ordered search-tree traversal with a shared borrowed visitor.
#ifndef SEARCH_TREE_TRAVERSAL_H
#define SEARCH_TREE_TRAVERSAL_H
#include "search_tree_types.h"

/// Visit one live caller-owned tree node.
/// \param node Borrowed linked entry, valid throughout the callback.
/// \param depth Depth from the supplied root, starting at zero.
/// \returns Nonzero to stop traversal, or zero to continue.
/// \pre Keep all visited nodes and their referenced values live; do not change tree
/// links, insert or remove nodes, or start another traversal from the callback.
typedef s32 (*TreeVisitor)(TreeNode *node, s32 depth);

/// Visit entries in ascending order.
/// \param root Root of a live acyclic tree, or null.
/// \param visitor Borrowed callback used only during this call.
/// \returns Nothing.
/// \pre Traversals must not overlap or nest; depth must fit signed 32 bits.
/// \note Original decompilation identifier: \c func_80041454 (0x80041454).
void searchTreeVisitAscending(TreeNode *root, TreeVisitor visitor);

/// Visit entries in descending order.
/// \param root Root of a live acyclic tree, or null.
/// \param visitor Borrowed callback used only during this call.
/// \returns Nothing.
/// \pre Traversals must not overlap or nest; depth must fit signed 32 bits.
/// \note Original decompilation identifier: \c func_80041520 (0x80041520).
void searchTreeVisitDescending(TreeNode *root, TreeVisitor visitor);

/// Visit entries with each root before its right and left subtrees.
/// \param root Root of a live acyclic tree, or null.
/// \param visitor Borrowed callback used only during this call.
/// \returns Nothing.
/// \pre Traversals must not overlap or nest; depth must fit signed 32 bits.
/// \note Original decompilation identifier: \c func_80041620 (0x80041620).
void searchTreeVisitRootRightLeft(TreeNode *root, TreeVisitor visitor);

#endif
