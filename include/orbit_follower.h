/// \file
/// Declares creation of terrain-constrained objects that orbit an owner.

#ifndef ORBIT_FOLLOWER_H
#define ORBIT_FOLLOWER_H

#include <ultra64.h>

/// Create an orbiting follower for a creature.
/// \param owner Creature object to follow.
/// \param radius Horizontal collision radius of the follower.
/// \param height Follower height above the ground and owner spawn position.
/// \param ceiling Upper extent used for scene collision checks.
/// \returns The created follower object.
/// \note Original decompilation identifier: \c func_8002728C (0x8002728C).
u8 *orbitFollowerCreate(u8 *owner, f32 radius, f32 height, f32 ceiling);

#endif
