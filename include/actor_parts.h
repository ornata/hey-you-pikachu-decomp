/// \file
/// Provides borrowed actor-part lookup and world-position queries.

#ifndef ACTOR_PARTS_H
#define ACTOR_PARTS_H

#include <ultra64.h>

/// Find an actor part by its identifier.
/// \param actor Actor whose parts are searched.
/// \param partId Identifier of the requested part.
/// \returns Borrowed part pointer, or null if it does not exist.
/// \note Original decompilation identifier: \c func_800163FC (0x800163FC).
u8 *actorFindPart(u8 *actor, s32 partId);

/// Get the world position of a named part, using the default part when the requested part is absent.
/// \param actor Actor containing the requested part and a valid default part.
/// \param partId Identifier of the preferred part.
/// \param position Output array receiving three world-space coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800164AC (0x800164AC).
void actorGetPartWorldPosition(u8 *actor, s32 partId, f32 *position);

/// Get the default reference part configured for the actor type.
/// \param actor Actor whose default reference part is requested.
/// \returns Borrowed default reference part, or null if it does not exist.
/// \note Original decompilation identifier: \c func_8001647C (0x8001647C).
u8 *actorGetDefaultPart(u8 *actor);

/// Get the object handle of a named actor part.
/// \param actor Actor whose part is requested.
/// \param partId Identifier of the requested part.
/// \returns Part object handle, or zero if the part does not exist.
/// \note Original decompilation identifier: \c func_80016448 (0x80016448).
s32 actorGetPartObjectHandle(u8 *actor, s32 partId);

/// Get the world-space attachment position selected by the actor attachment mode.
/// \param actor Actor whose attachment mode and parts determine the position.
/// \param heightOffset Vertical offset applied by the raised attachment mode.
/// \param position Output array receiving three world-space coordinates; unchanged for an unsupported attachment mode.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800164FC (0x800164FC).
void actorGetConfiguredAttachmentPosition(u8 *actor, f32 heightOffset, f32 *position);

/// Allocate an empty part table sized for the actor type.
/// \param actor Actor with initialized model state and registered type receiving a new part table; existing tables are not released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800169B8 (0x800169B8).
void actorAllocatePartTable(u8 *actor);

/// Register matching objects from a hierarchy in an actor part table.
/// \param hierarchy First object in the hierarchy to register, or null.
/// \param actor Actor whose allocated part table borrows matching objects for their lifetimes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016A1C (0x80016A1C).
void actorPopulatePartTable(u8 *hierarchy, u8 *actor);

#endif
