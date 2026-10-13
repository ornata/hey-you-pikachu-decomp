/// \file
/// Declares runtime object hierarchy updates and selected-group drawing.

#ifndef OBJECT_GROUPS_H
#define OBJECT_GROUPS_H

#include "object_types.h"

/// Update an object hierarchy and complete its initial-update state.
/// \param object Non-null hierarchy root to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009308 (0x80009308).
void objectUpdateHierarchy(RuntimeObjectPrefix *object);

/// Update active object hierarchies belonging to the selected runtime group.
/// \param group Object group selector: zero or 0x4000.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009344 (0x80009344).
void objectUpdateGroup(s32 group);

/// Draw registered objects belonging to the selected draw group.
/// \param group Draw group selector: zero or 0x2000.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800096D4 (0x800096D4).
void objectDrawGroup(s32 group);

/// Draw the selected group of objects within a hierarchy.
/// \param object First object whose siblings and descendants are considered, or null.
/// \param group Draw group selector: zero or 0x2000.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009784 (0x80009784).
void objectDrawHierarchyGroup(RuntimeObjectPrefix *object, s32 group);

#endif
