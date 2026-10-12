/// \file
/// Declares actor spatial-sound playback and tracking.

#ifndef ACTOR_SOUND_H
#define ACTOR_SOUND_H

#include "ultra64.h"

/// Play a spatial sound and track it in the actor's model state.
/// \param actor Live actor with initialized model state.
/// \param kind Sound identifier in the actor's sound collection.
/// \returns Playback handle, or zero if playback could not start.
/// \note A full tracking table may forget an existing playback without stopping it.
/// \note Original decompilation identifier: \c func_80014B60 (0x80014B60).
s32 actorPlayTrackedSound(u8 *actor, s32 kind);

#endif
