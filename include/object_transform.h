/// \file
/// Declares hierarchy world-position refresh with retained traversal state.
#ifndef OBJECT_TRANSFORM_H
#define OBJECT_TRANSFORM_H
#include <ultra64.h>

/// Refresh world positions while preserving the surrounding traversal state.
/// \param object First live object in an acyclic sibling and descendant hierarchy, or null.
/// \returns The supplied borrowed object pointer.
/// \pre The transform stack has room for the hierarchy depth and the surrounding saved transform.
/// \note Uses shared traversal state and must not overlap another traversal.
/// \note Original decompilation identifier: \c func_80008FAC (0x80008FAC).
u8 *objectRefreshHierarchyWorldPositions(u8 *object);

#endif
