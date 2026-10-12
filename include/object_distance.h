/// \file
/// Declares approximate world-space distance queries for runtime objects.
#ifndef OBJECT_DISTANCE_H
#define OBJECT_DISTANCE_H
#include "vector3_types.h"

/// Measure approximate world-space distance from an object to a point.
/// \param object Borrowed live object with initialized world position.
/// \param point Borrowed three-component world-space position.
/// \returns Approximate distance between the positions.
/// \note Original decompilation identifier: \c func_80009CA4 (0x80009CA4).
f32 objectGetApproxDistanceToPoint(u8 *object, Vec3f *point);

/// Measure approximate world-space distance between two objects.
/// \param first Borrowed live object with initialized world position.
/// \param second Borrowed live object with initialized world position.
/// \returns Approximate distance between the positions.
/// \note Original decompilation identifier: \c func_80009CF4 (0x80009CF4).
f32 objectGetApproxDistance(u8 *first, u8 *second);

#endif
