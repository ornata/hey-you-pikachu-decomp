/// \file
/// Declares object world-coordinate queries and hierarchy world-position refresh.
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

/// Get one coordinate of an object's world position.
/// \param object Object whose world position is queried.
/// \param axis Nonnegative coordinate axis: zero for X, one for Y, or two for Z.
/// \returns The coordinate, or -65536 when the axis is greater than two.
/// \note Original decompilation identifier: \c func_801174B0 (0x801174B0).
f32 objectGetWorldCoordinate(u8 *object, s32 axis);

/// Get one world-space coordinate of an available object.
/// \param handle Handle of the object to query.
/// \param axis Nonnegative coordinate axis: zero for X, one for Y, or two for Z.
/// \returns Zero when the handle is unavailable; otherwise the coordinate, or -65536 when the axis is greater than two.
/// \note Original decompilation identifier: \c func_80117FD4 (0x80117FD4).
f32 objectGetWorldCoordinateByHandle(s32 handle, s32 axis);

#endif
