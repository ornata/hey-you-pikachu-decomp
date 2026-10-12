/// \file
/// Declares flag updates over runtime object sibling and descendant hierarchies.
#ifndef OBJECT_FLAGS_H
#define OBJECT_FLAGS_H
#include <ultra64.h>

/// Enable flags throughout a hierarchy.
/// \param object First object in a live acyclic sibling/descendant hierarchy, or null.
/// \param flags Sixteen-bit object flags to enable; other bits are preserved.
/// \returns Nothing.
void objectSetHierarchyFlags(u8 *object, u16 flags);

/// Disable flags throughout a hierarchy.
/// \param object First object in a live acyclic sibling/descendant hierarchy, or null.
/// \param flags Sixteen-bit object flags to disable; other bits are preserved.
/// \returns Nothing.
void objectClearHierarchyFlags(u8 *object, u16 flags);
#endif
