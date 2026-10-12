/// \file
/// Declares nearest-target searches over live objects and script actor-kind ranges.
#ifndef OBJECT_TARGET_SEARCH_H
#define OBJECT_TARGET_SEARCH_H
#include "vector3_types.h"

/// Find the nearest qualifying object using shared, nonreentrant query state.
/// \param self Object excluded from the search, or null.
/// \param origin World-space search origin.
/// \param distance Exclusive maximum approximate distance.
/// \param rotation Three finite Euler angles in degrees; unused when cone is at least 360.
/// \param cone Maximum absolute horizontal viewing angle in degrees; at least 360 disables filtering.
/// \param minKind Inclusive minimum runtime kind.
/// \param maxKind Inclusive maximum runtime kind.
/// \returns Borrowed live object, or null when no target qualifies.
u8 *objectFindNearestTargetInView(u8 *self, Vec3f *origin, f32 distance, f32 *rotation, f32 cone, s32 minKind,
                                  s32 maxKind);

/// Find the nearest actor using script actor-kind aliases and shared query state.
/// \param self Object excluded from the search, or null.
/// \param origin World-space search origin.
/// \param distance Exclusive maximum approximate distance.
/// \param rotation Three finite Euler angles in degrees; unused for an unrestricted cone.
/// \param cone Maximum absolute horizontal viewing angle; at least 360 disables filtering.
/// \param minKind Inclusive minimum kind; 38 selects the first creature kind.
/// \param maxKind Inclusive maximum kind; 38 selects Pikachu.
/// \returns Borrowed live actor, or null when no target qualifies.
u8 *actorFindNearestInRange(u8 *self, Vec3f *origin, f32 distance, f32 *rotation, f32 cone, s32 minKind, s32 maxKind);
#endif
