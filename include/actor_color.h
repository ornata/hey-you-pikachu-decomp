/// \file
/// Declares immediate and timed actor model environment-color updates.

#ifndef ACTOR_COLOR_H
#define ACTOR_COLOR_H

#include "ultra64.h"

/// Request a timed model environment-color transition.
/// \param actor Actor with initialized model state.
/// \param r Target red component from zero to 255.
/// \param g Target green component from zero to 255.
/// \param b Target blue component from zero to 255.
/// \param a Target alpha component from zero to 255.
/// \param duration Nonnegative duration in seconds; zero applies the target on the next update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015588 (0x80015588).
void actorTransitionModelEnvironmentColor(u8 *actor, u8 r, u8 g, u8 b, u8 a, f32 duration);

/// Set the model environment color immediately.
/// \param actor Actor with initialized model state.
/// \param r Red component from zero to 255.
/// \param g Green component from zero to 255.
/// \param b Blue component from zero to 255.
/// \param a Alpha component from zero to 255.
/// \returns Nothing.
/// \note An existing timed transition remains pending.
/// \note Original decompilation identifier: \c func_800155B8 (0x800155B8).
void actorSetModelEnvironmentColor(u8 *actor, u8 r, u8 g, u8 b, u8 a);

#endif
