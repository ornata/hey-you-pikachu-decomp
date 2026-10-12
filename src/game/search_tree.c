/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "search_tree.h"

/// Initialize an empty search tree with a key comparator.
/// \param tree Tree record to initialize.
/// \param compare Callback comparing the target-width key against a node.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80040F20 (0x80040F20).
void searchTreeInitialize(SearchTree *tree, TreeCompare compare) {
    _bzero(tree, 0xC);
    tree->compare = compare;
}

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
s32 searchTreeLocateRoot(TreeNode **rootAddress, s32 minimum, TreeCompare compare, s32 key) {
    TreeNode *root = *rootAddress;
    TreeNode *child;
    TreeNode sentinels[2];
    TreeNode *tails[2];
    TreeNode *next;
    s32 direction;
    s32 childDirection;
    s32 result;
    s32 childResult;

    if (root == NULL) {
        return 1;
    }
    _bzero(sentinels, sizeof(sentinels));
    tails[0] = &sentinels[0];
    tails[1] = &sentinels[1];
    for (;;) {
        if (minimum) {
            result = -1;
            direction = 0;
        } else {
            direction = (result = compare(root, key)) >= 0;
        }
        child = root->links[direction];
        if (result == 0 || child == NULL) {
            break;
        }
        if (minimum) {
            childResult = -1;
            childDirection = 0;
        } else {
            childDirection = (childResult = compare(child, key)) >= 0;
        }
        next = child->links[childDirection];
        if (childResult == 0 || next == NULL) {
            // clang-format off
            root->links[!!direction] = NULL; tails[!direction]->links[!!direction] = root;
            // clang-format on
            tails[!direction] = root;
            root = child;
            result = childResult;
            break;
        }
        if (direction == childDirection) {
            root->links[direction] = child->links[!childDirection];
            child->links[!childDirection] = root;
            // clang-format off
            child->links[!!childDirection] = NULL; tails[!childDirection]->links[!!childDirection] = child;
            // clang-format on
            tails[!childDirection] = child;
        } else {
            // clang-format off
            root->links[!!direction] = NULL; tails[!direction]->links[!!direction] = root;
            // clang-format on
            tails[!direction] = root;
            // clang-format off
            child->links[!!childDirection] = NULL; tails[!childDirection]->links[!!childDirection] = child;
            // clang-format on
            tails[!childDirection] = child;
        }
        root = next;
    }
    tails[0]->links[1] = root->links[0];
    tails[1]->links[0] = root->links[1];
    root->links[0] = sentinels[0].links[1];
    root->links[1] = sentinels[1].links[0];
    *rootAddress = root;
    return result;
}


/// Find a matching entry in a search tree.
/// \param tree Tree to search; its root may change.
/// \param key Key to locate.
/// \returns The matching node address, or zero when absent.
/// \note Original decompilation identifier: \c func_80041184 (0x80041184).
s32 searchTreeFind(u8 *tree, s32 key) {
    if (searchTreeLocateRoot(&((SearchTree *)tree)->root, 0, ((SearchTree *)tree)->compare, key) != 0) {
        return 0;
    }
    return (s32)((SearchTree *)tree)->root;
}

/// Find a matching entry through an independently supplied root and comparator.
/// \param rootAddress Address of the tree root, updated by the search.
/// \param compare Comparator address comparing the key against a node.
/// \param key Key to locate.
/// \returns The matching node address, or zero when absent.
/// \note Original decompilation identifier: \c func_800411C8 (0x800411C8).
TreeNode *searchTreeFindAtRoot(TreeNode **rootAddress, TreeCompare compare, s32 key) {
    if (searchTreeLocateRoot(rootAddress, 0, compare, key) != 0) {
        return 0;
    }
    return *rootAddress;
}

/// Find the entry immediately following a matching key.
/// \param tree Tree to search; its root and subtree roots may change.
/// \param key Existing key whose successor is requested.
/// \returns The successor node address, or zero if the key or its successor is absent.
/// \note Original decompilation identifier: \c func_80041208 (0x80041208).
s32 searchTreeFindSuccessor(u8 *tree, s32 key) {
    if (searchTreeFind(tree, key) == 0) {
        return 0;
    }
    searchTreeLocateRoot(&((SearchTree *)tree)->root->links[1], 1, 0, 0);
    return (s32)((SearchTree *)tree)->root->links[1];
}

/// Find the minimum entry in a search tree.
/// \param tree Tree to search; its root may change.
/// \returns The minimum node address, or zero for an empty tree.
/// \note Original decompilation identifier: \c func_8004125C (0x8004125C).
s32 searchTreeFindMinimum(u8 *tree) {
    searchTreeLocateRoot(&((SearchTree *)tree)->root, 1, 0, 0);
    return (s32)((SearchTree *)tree)->root;
}

/// Insert a caller-owned entry into a search tree.
/// \param tree Tree receiving the entry.
/// \param node Detached caller-owned node to insert; receives the supplied key
/// and must remain valid until removed from the tree.
/// \param key Key used to order the entry.
/// \returns The inserted node.
/// \note Original decompilation identifier: \c func_80041290 (0x80041290).
TreeNode *searchTreeInsert(SearchTree *tree, TreeNode *node, s32 key) {
    s32 idx;
    s32 other;
    TreeNode *cur;

    node->value = key;
    idx = searchTreeLocateRoot(&tree->root, 0, tree->compare, key) < 1;
    cur = tree->root;
    other = idx == 0;
    if (cur != NULL) {
        node->links[idx] = cur;
        node->links[other] = cur->links[other];
        cur->links[other] = NULL;
    } else {
        node->links[1] = NULL;
        node->links[0] = NULL;
    }
    tree->root = node;
    if (!idx) {
    }
    return node;
}

/// Detach an entry matching a key from a search tree.
/// \param tree Tree from which to remove the entry.
/// \param key Key identifying the entry.
/// \returns The removed node, whose storage remains caller-owned, or NULL when absent.
/// \note Original decompilation identifier: \c func_8004131C (0x8004131C).
u8 *searchTreeRemove(u8 *tree, s32 key) {
    u8 *node;
    u8 *next;
    u8 *rest;

    if (searchTreeLocateRoot(&((SearchTree *)tree)->root, 0, ((SearchTree *)tree)->compare, key) != 0) {
        return NULL;
    }
    node = (u8 *)((SearchTree *)tree)->root;
    next = (u8 *)((TreeNode *)node)->links[1];
    rest = (u8 *)((TreeNode *)node)->links[0];
    if (next != NULL) {
        ((SearchTree *)tree)->root = (TreeNode *)next;
        searchTreeLocateRoot(&((SearchTree *)tree)->root, 1, 0, 0);
        ((SearchTree *)tree)->root->links[0] = (TreeNode *)rest;
    } else {
        ((SearchTree *)tree)->root = (TreeNode *)rest;
    }
    return node;
}

/// Borrowed callback for the active tree traversal; shared traversal state makes nested traversals unsupported.
/// \note Original identifier: \c D_8009D540 (0x8009D540).
extern s32 (*gSearchTreeActiveVisitor)(u8 *, s32);

/// Visit entries in ascending order using the current tree visitor.
/// \param node Subtree root, or NULL.
/// \param depth Depth assigned to the subtree root.
/// \returns 1 when a visitor requests early termination, or 0 after visiting all entries.
/// \note Original decompilation identifier: \c func_800413B4 (0x800413B4).
s32 searchTreeVisitAscendingAtDepth(u8 *node, s32 depth) {
    while (node != NULL) {
        if (searchTreeVisitAscendingAtDepth(*(u8 **)node, depth + 1) != 0) {
            return 1;
        }
        if (gSearchTreeActiveVisitor(node, depth) != 0) {
            return 1;
        }
        node = *(u8 **)(node + 4);
        depth = depth + 1;
    }
    return 0;
}

s32 searchTreeVisitAscendingAtDepth(u8 *, s32);

/// Visit entries in ascending order.
/// \param root Tree root, or NULL.
/// \param visitor Callback receiving each node and its depth from root; a nonzero result stops traversal. Traversals must not be nested.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041454 (0x80041454).
void searchTreeVisitAscending(u8 *root, s32 (*visitor)(u8 *, s32)) {
    gSearchTreeActiveVisitor = visitor;
    searchTreeVisitAscendingAtDepth(root, 0);
    gSearchTreeActiveVisitor = 0;
}


/// Visit entries in descending order using the current tree visitor.
/// \param node Subtree root, or NULL.
/// \param depth Depth assigned to the subtree root.
/// \returns 1 when a visitor requests early termination, or 0 after visiting all entries.
/// \note Original decompilation identifier: \c func_80041480 (0x80041480).
s32 searchTreeVisitDescendingAtDepth(u8 *node, s32 depth) {
    while (node != NULL) {
        if (searchTreeVisitDescendingAtDepth(*(u8 **)(node + 4), depth + 1) != 0) {
            return 1;
        }
        if (gSearchTreeActiveVisitor(node, depth) != 0) {
            return 1;
        }
        node = *(u8 **)node;
        depth = depth + 1;
    }
    return 0;
}

s32 searchTreeVisitDescendingAtDepth(u8 *, s32);

/// Visit entries in descending order.
/// \param root Tree root, or NULL.
/// \param visitor Callback receiving each node and its depth from root; a nonzero result stops traversal. Traversals must not be nested.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041520 (0x80041520).
void searchTreeVisitDescending(u8 *root, s32 (*visitor)(u8 *, s32)) {
    gSearchTreeActiveVisitor = visitor;
    searchTreeVisitDescendingAtDepth(root, 0);
    gSearchTreeActiveVisitor = 0;
}

/// Visit each root before its right and left subtrees using the current tree visitor.
/// \param node Subtree root, or NULL.
/// \param depth Depth assigned to the subtree root.
/// \returns 1 when a visitor requests early termination, or 0 after visiting all entries.
/// \note Original decompilation identifier: \c func_8004154C (0x8004154C).
s32 searchTreeVisitRootRightLeftAtDepth(u8 *node, s32 depth) {
    s32 i;
    while (node != NULL) {
        if (gSearchTreeActiveVisitor(node, depth))
            return 1;
        if (searchTreeVisitRootRightLeftAtDepth(*(u8 **)(node + 4), depth + 1))
            return 1;
        node = *(u8 **)node;
        depth++;
        if (node != NULL) {
            /* Retail code retains this empty depth-counting loop. */
            for (i = 0; i < depth; i++) {
            }
        }
    }
    return 0;
}

s32 searchTreeVisitRootRightLeftAtDepth(u8 *, s32);

/// Visit each root before its right and left subtrees.
/// \param root Tree root, or NULL.
/// \param visitor Callback receiving each node and its depth from root; a nonzero result stops traversal. Traversals must not be nested.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041620 (0x80041620).
void searchTreeVisitRootRightLeft(u8 *root, s32 (*visitor)(u8 *, s32)) {
    gSearchTreeActiveVisitor = visitor;
    searchTreeVisitRootRightLeftAtDepth(root, 0);
    gSearchTreeActiveVisitor = 0;
}

/// Provide an unused search-tree hook with no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8004164C (0x8004164C).
void searchTreeNoOp(void) {
}

/// Provide an alternate unused search-tree hook with no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80041654 (0x80041654).
void searchTreeAlternateNoOp(void) {
}

/// Provide a visitor that always allows traversal to continue.
/// \param node Unused node argument.
/// \param depth Unused depth argument.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_8004165C (0x8004165C).
s32 searchTreeVisitorContinue(s32 node, s32 depth) {
    return 0;
}
