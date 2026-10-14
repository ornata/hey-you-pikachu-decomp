/// \file
/// Declares lifecycle and creation interfaces for terrain-constrained objects that orbit an owner.

#ifndef ORBIT_FOLLOWER_H
#define ORBIT_FOLLOWER_H

#include "object_types.h"

/// Initialize the owned movement state of an orbiting follower.
/// \param object New follower object with no unreleased movement state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80026DF0 (0x80026DF0).
void orbitFollowerInitialize(u8 *object);

/// Advance a follower around its owner within scene terrain constraints.
/// \param obj Initialized follower whose owner handle is resolved for this update.
/// \returns Nothing; requests follower destruction if the owner no longer exists.
/// \note Original decompilation identifier: \c func_80026E54 (0x80026E54).
void orbitFollowerUpdate(RuntimeObjectPrefix *obj);

/// Create an orbiting follower for a creature.
/// \param owner Creature object to follow.
/// \param radius Horizontal collision radius of the follower.
/// \param height Follower height above the ground and owner spawn position.
/// \param ceiling Upper extent used for scene collision checks.
/// \returns The created follower object.
/// \note Original decompilation identifier: \c func_8002728C (0x8002728C).
u8 *orbitFollowerCreate(u8 *owner, f32 radius, f32 height, f32 ceiling);

#endif
