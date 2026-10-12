/// \file
/// Declares inherited-scale adjustment of runtime hierarchy visibility bounds.
#ifndef OBJECT_BOUNDS_H
#define OBJECT_BOUNDS_H
#include <ultra64.h>

/// Adjust stored bounds throughout an object hierarchy for inherited scale.
/// \param object First live object in an acyclic sibling and descendant hierarchy, or null.
/// \param parentScale Scale inherited from the parent hierarchy.
/// \returns Nothing.
/// \note Bounds are modified in place; repeated calls compound their scaling.
/// \note Original decompilation identifier: \c func_80008520 (0x80008520).
void objectScaleHierarchyBounds(u8 *object, f32 parentScale);

#endif
