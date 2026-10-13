/// \file
/// Declares actor model resource lifetimes and motion updates.

#ifndef ACTOR_MODEL_H
#define ACTOR_MODEL_H

#include <ultra64.h>

/// Update actor behavior, animation, spatial state, and model drawing setup.
/// \param actor Actor with initialized model, action, hierarchy, and collision state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015488 (0x80015488).
void actorUpdateModel(u8 *actor);

/// Allocate and initialize actor model state and its drawing resources.
/// \param actor Actor with a cached model asset receiving newly owned model state; existing state is not released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014FE4 (0x80014FE4).
void actorInitializeModelState(u8 *actor);

/// Advance motion playback for an actor child object.
/// \param object Child object whose motion playback is updated.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800155E0 (0x800155E0).
void actorUpdateChildMotion(u8 *object);

/// Configure an actor animated texture component from texture and animation definitions.
/// \param actor Actor with part 0x78 and allocated texture state; existing drawing buffers are not released.
/// \param parts Up to three valid texture/animation identifier pairs, terminated by a negative texture identifier; not retained.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016884 (0x80016884).
void actorInitializeAnimatedTextures(u8 *actor, s32 *parts);

/// Release actor model resources and remove its collision registration.
/// \param actor Actor with initialized model state; its state pointer becomes invalid after release.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80014DE8 (0x80014DE8).
void actorReleaseModelResources(u8 *actor);

/// Adjust actor movement for terrain, gravity, and ground contact while updating its movement state.
/// \param object Actor with initialized action, model, ground-reference pose, and default part state.
/// \param displacement Three-component movement vector adjusted for terrain and gravity.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80016DE0 (0x80016DE0).
void actorConstrainMovementToTerrain(u8 *object, f32 *displacement);

#endif
