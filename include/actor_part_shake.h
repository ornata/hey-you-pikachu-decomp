/// \file
/// Declares temporary model-part shake requests and their lifetime requirements.

#ifndef ACTOR_PART_SHAKE_H
#define ACTOR_PART_SHAKE_H

#include "ultra64.h"

/// Start or extend a temporary shake of a model part.
/// \param actor Live actor containing the selected part.
/// \param part Identifier of an existing part with initialized behavior state.
/// \returns Nothing.
/// \pre The selected part and its behavior state remain live throughout the shake.
/// \note Selecting a different part captures its current orientation as the shake reference.
/// \note Original decompilation identifier: \c func_800144D4 (0x800144D4).
void actorStartPartShake(u8 *actor, s32 part);

#endif
