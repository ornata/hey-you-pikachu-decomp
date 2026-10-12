/// \file
/// Declares runtime object creation, hierarchy ownership and active registration.
#ifndef OBJECT_LIFECYCLE_H
#define OBJECT_LIFECYCLE_H
#include <ultra64.h>

/// Create an object from the default template.
/// \returns Newly owned object; release with objectDestroy after removing active hierarchy links.
u8 *objectCreateDefault(void);

/// Designate an object as its hierarchy's owning root.
/// \param object Non-null live object with a valid acyclic hierarchy.
/// \returns The supplied object.
u8 *objectMakeHierarchyRoot(u8 *object);

/// Register a hierarchy for active runtime processing.
/// \param object Live hierarchy with initialized kinds and roots, not already registered.
/// \returns The supplied hierarchy root.
u8 *objectRegisterActiveHierarchy(u8 *object);

/// Destroy an object and release its owned resources and registrations.
/// \param object Live object with a valid runtime kind; its storage becomes invalid.
/// The caller must remove active hierarchy and sibling links retaining it.
/// \returns Nothing.
void objectDestroy(u8 *object);
#endif
