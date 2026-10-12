/// \file
/// Declares ordered search-tree traversal with a shared borrowed visitor.
#ifndef SEARCH_TREE_TRAVERSAL_H
#define SEARCH_TREE_TRAVERSAL_H
#include "search_tree_types.h"

/// Visit one live tree node through its original byte-address view.
/// \param node Borrowed node storage, valid for the callback; do not detach traversal links.
/// \param depth Depth from the supplied root, starting at zero.
/// \returns Nonzero to stop traversal, or zero to continue.
typedef s32 (*TreeVisitor)(u8 *node, s32 depth);

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
