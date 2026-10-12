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

#endif
