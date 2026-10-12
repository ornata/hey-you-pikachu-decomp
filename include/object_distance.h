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

/// Measure an object's distance from the world origin.
/// \param object Object whose world position is queried.
/// \returns The approximate three-dimensional distance from the world origin.
/// \note Original decompilation identifier: \c func_801172D4 (0x801172D4).
f32 objectGetOriginDistance(u8 *object);

/// Measure the distance from an object's local position to the player position.
/// \param object Object whose distance is queried.
/// \returns The approximate three-dimensional distance to the player.
/// \note Original decompilation identifier: \c func_80118838 (0x80118838).
f32 objectGetPlayerDistance(u8 *object);

/// Measure the distance between an object and an available target.
/// \param object Object providing the reference position.
/// \param handle Handle of the target object.
/// \returns The approximate three-dimensional distance, or -1 when the target is unavailable.
/// \note Original decompilation identifier: \c func_8011730C (0x8011730C).
f32 objectGetDistanceToHandle(u8 *object, s32 handle);

/// Measure an available target's height relative to an object.
/// \param object Object providing the reference position.
/// \param handle Handle of the target object.
/// \returns The target height minus the object height, or zero when the target is unavailable.
/// \note Original decompilation identifier: \c func_80117388 (0x80117388).
f32 objectGetHeightDifferenceToHandle(u8 *object, s32 handle);

/// Measure the horizontal distance between an object and an available target.
/// \param object Object providing the reference position.
/// \param handle Handle of the target object.
/// \returns The approximate horizontal distance, or zero when the target is unavailable.
/// \note Original decompilation identifier: \c func_801173C8 (0x801173C8).
f32 objectGetHorizontalDistanceToHandle(u8 *object, s32 handle);

#endif
