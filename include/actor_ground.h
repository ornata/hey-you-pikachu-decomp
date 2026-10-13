/// \file
/// Defines terrain-height and material queries relative to an actor's world pose.
/// Requires initialized actor heading and current-area terrain with valid search spacing.

#ifndef ACTOR_GROUND_H
#define ACTOR_GROUND_H

#include <ultra64.h>

/// Read a coordinate of a terrain position offset from an actor's heading.
/// \param actor Borrowed actor supplying world position and initialized script orientation.
/// \param distance Signed horizontal distance from the actor.
/// \param angle Finite heading offset in degrees, within the angle helpers' supported range.
/// \param axis Coordinate selector: zero for X, one for ground height, or two for Z.
/// \returns Selected coordinate; a missing ground surface supplies zero height.
/// \note Original decompilation identifier: \c func_8011802C (0x8011802C).
f32 actorGetGroundOffsetCoordinate(u8 *actor, f32 distance, f32 angle, s32 axis);

/// Query terrain material ahead of an actor.
/// \param actor Borrowed actor supplying world position and initialized script heading.
/// \param distance Signed horizontal distance along the heading.
/// \param heightOffset Vertical offset from the actor used to select the terrain region.
/// \returns Terrain material and attribute flags, including terrainGetSurfaceAttributes sentinel values.
/// \note Original decompilation identifier: \c func_801180E8 (0x801180E8).
s32 actorProbeGroundMaterialAhead(u8 *actor, f32 distance, f32 heightOffset);

/// Query ground height ahead of an actor.
/// \param actor Borrowed actor supplying world position and initialized script heading.
/// \param distance Signed horizontal distance along the heading.
/// \param heightOffset Vertical offset from the actor used to select the terrain region.
/// \returns Ground height, or zero when no surface is found.
/// \note Original decompilation identifier: \c func_80119288 (0x80119288).
f32 actorGetGroundHeightAhead(u8 *actor, f32 distance, f32 heightOffset);

#endif
