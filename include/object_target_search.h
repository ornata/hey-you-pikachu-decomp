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
/// Find an actor near a supplied position, using script kind aliases.
/// \param object Live object excluded from the search.
/// \param position Three world-space search coordinates.
/// \param minKind Inclusive minimum kind; 38 selects the first creature kind.
/// \param maxKind Inclusive maximum kind; 38 selects Pikachu.
/// \param distance Exclusive maximum approximate distance.
/// \returns Nearest qualifying handle, or zero when none qualifies.
s32 actorFindNearestHandleFromPosition(u8 *object, f32 *position, s32 minKind, s32 maxKind, f32 distance);

/// Find an object within the supplied object's facing cone.
/// \param object Live object supplying position and facing, excluded from the search.
/// \param minKind Inclusive minimum runtime kind.
/// \param maxKind Inclusive maximum runtime kind.
/// \param distance Exclusive maximum approximate distance.
/// \param cone Maximum horizontal angle in degrees; at least 360 disables filtering.
/// \returns Nearest qualifying handle, or zero when none qualifies.
s32 objectFindNearestHandleInFacingCone(u8 *object, s32 minKind, s32 maxKind, f32 distance, f32 cone);

/// Find an object near a supplied position.
/// \param object Live object excluded from the search.
/// \param position Three world-space search coordinates.
/// \param minKind Inclusive minimum runtime kind.
/// \param maxKind Inclusive maximum runtime kind.
/// \param distance Exclusive maximum approximate distance.
/// \returns Nearest qualifying handle, or zero when none qualifies.
s32 objectFindNearestHandleFromPosition(u8 *object, f32 *position, s32 minKind, s32 maxKind, f32 distance);

/// Find the nearest actor in an object facing cone, honoring script actor-kind aliases.
/// \param object Live object providing the world-space search origin and facing, excluded from the search.
/// \param minKind Inclusive minimum kind.
/// \param maxKind Inclusive maximum kind.
/// \param distance Exclusive maximum approximate distance from the search origin.
/// \param cone Maximum horizontal angle in degrees; 360 or greater permits all directions.
/// \returns The nearest qualifying actor handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_80118984 (0x80118984).
s32 actorFindNearestHandleInFacingCone(u8 *object, s32 minKind, s32 maxKind, f32 distance, f32 cone);

#endif
