/// \file
/// Declares handle lookup and availability filtering for runtime objects.

#ifndef OBJECT_LOOKUP_H
#define OBJECT_LOOKUP_H

#include <ultra64.h>

/// Find a live runtime object by handle.
/// \param handle Object-manager handle identifying the requested object.
/// \returns Borrowed object, or null when absent or marked unavailable or pending destruction.
/// The result remains valid only until the object is destroyed; retaining it does not retain ownership.
u8 *objectFindLiveByHandle(s32 handle);

/// Resolve a gameplay object while excluding unavailable items.
/// \param handle Object-manager handle, or zero for no object.
/// \returns Borrowed live object, or null for an absent object or an item in state four.
/// The result remains valid only until the object is destroyed. Other object kinds are not item-filtered.
u8 *objectResolveAvailableHandle(s32 handle);

#endif
