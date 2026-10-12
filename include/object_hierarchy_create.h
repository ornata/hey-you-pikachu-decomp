/// \file
/// Declares creation of complete runtime hierarchies from borrowed object templates.
#ifndef OBJECT_HIERARCHY_CREATE_H
#define OBJECT_HIERARCHY_CREATE_H
#include "object_template.h"

/// Clone and initialize a hierarchy with owned transform buffers.
/// \param definition Non-null acyclic template hierarchy with valid runtime kinds and resources.
/// \returns Owned root; remove active links before releasing it with objectDestroyHierarchy.
/// Template storage is borrowed for this call; model display lists must remain valid for the runtime hierarchy.
u8 *objectCreateHierarchyFromTemplate(ObjectTemplate *definition);
#endif
