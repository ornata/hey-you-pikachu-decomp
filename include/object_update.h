/// \file
/// Declares object hierarchy updates, height queries and diagnostic hooks.

#ifndef OBJECT_UPDATE_H
#define OBJECT_UPDATE_H

#include <ultra64.h>

/// Provide the inactive per-object diagnostic report hook.
/// \param objectAddress Address of the object to report; unused in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007CD0 (0x80007CD0).
void objectDebugReportNoOp(u8 *objectAddress);

/// Prepare the object search tree with distributed handle searches for a requested kind.
/// \param kind Object kind for the searches.
/// \param searchCount Requested number of distributed searches; values below two perform only the initial search. The object search tree must be nonempty.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80007DC8 (0x80007DC8).
void objectPrepareDistributedKindSearches(u16 kind, s32 searchCount);

/// Update hierarchy visibility according to the object's interaction eligibility.
/// \param object Object whose eligibility determines visibility for its hierarchy.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80008D48 (0x80008D48).
void objectUpdateHierarchyInteractionVisibility(u8 *object);

/// Measure the object world height relative to the ground beneath it.
/// \param object Object whose world position is queried.
/// \returns Signed height above the ground; uses ground height zero when no surface is found.
/// \note Portability: The terrain call passes X and Z float bit patterns through integer parameters, relying on the original MIPS ABI.
/// \note Original decompilation identifier: \c func_80008DA8 (0x80008DA8).
f32 objectGetHeightAboveGround(u8 *object);

#endif
