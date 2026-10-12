/// \file
/// Declares queries and forced release for items retained by actor script state.

#ifndef ACTOR_HELD_ITEM_H
#define ACTOR_HELD_ITEM_H

#include "ultra64.h"

/// Query the kind of item held by an actor.
/// \param actor Actor with initialized state whose held handle identifies an item when live.
/// \returns The held item kind, or zero when no live held item is found.
/// \note Original decompilation identifier: \c func_801188D4 (0x801188D4).
s16 actorGetHeldItemKind(u8 *actor);

/// Force release of the actor's held item and report its kind.
/// \param actor Actor with initialized state whose held handle identifies an item when live.
/// \returns The resolved item kind, or zero when no live held item is found.
///
/// \note Portability: The unused padding array preserves the stack-frame size required by the ROM.
/// \note Original decompilation identifier: \c func_80118938 (0x80118938).
s16 actorReleaseHeldItem(u8 *actor);

#endif
