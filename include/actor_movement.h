/// \file
/// Declares actor movement-result queries over initialized script state.

#ifndef ACTOR_MOVEMENT_H
#define ACTOR_MOVEMENT_H

#include "ultra64.h"

/// Get the collision result of an actor's latest movement update.
/// \param actor Actor whose movement collision status is queried.
/// \returns Movement collision flags, or zero when no collision was reported.
/// \note Original decompilation identifier: \c func_80117500 (0x80117500).
u16 actorGetMovementCollisionFlags(u8 *actor);

#endif
