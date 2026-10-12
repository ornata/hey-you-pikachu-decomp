/// \file
/// Declares normalized actor-state orientation updates in degrees.
#ifndef ACTOR_ROTATION_H
#define ACTOR_ROTATION_H
#include <ultra64.h>

/// Set one actor-state orientation component.
/// \param actor Live actor with initialized model state.
/// \param angle Finite angle in degrees, normalized to zero inclusive through 360 exclusive.
/// \param axis Component index: zero for pitch, one for yaw, two for roll.
/// \returns Nothing.
/// \pre Angle magnitudes must allow a full-turn adjustment to change their floating-point value.
/// \note Original decompilation identifier: \c func_80015F1C (0x80015F1C).
void actorSetRotationAxisDegrees(u8 *actor, f32 angle, s32 axis);

/// Add an angular displacement to one actor-state orientation component.
/// \param actor Live actor with initialized model state.
/// \param angle Finite angular displacement in degrees; the resulting angle must remain finite.
/// \param axis Component index: zero for pitch, one for yaw, two for roll.
/// \returns Nothing; the resulting component lies in zero inclusive through 360 exclusive.
/// \pre Angle magnitudes must allow a full-turn adjustment to change their floating-point value.
/// \note Original decompilation identifier: \c func_80015F94 (0x80015F94).
void actorAddRotationAxisDegrees(u8 *actor, f32 angle, s32 axis);

/// Update object rotation from the actor orientation state and reference transform.
/// \param actor Actor with initialized orientation state and a current reference transform.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80015EB8 (0x80015EB8).
void actorUpdateObjectRotationFromState(u8 *actor);

/// Rotate the actor on one axis when the current action permits turning.
/// \param actor Actor whose orientation is updated.
/// \param angle Finite angular displacement in degrees.
/// \param axis Orientation component: zero for pitch, one for yaw, or two for roll.
/// \returns Nothing.
/// \pre Updated angles must remain finite and allow full-turn adjustments to change their floating-point values.
/// \note Original decompilation identifier: \c func_80016030 (0x80016030).
void actorAddActionRotationAxisDegrees(u8 *actor, f32 angle, s32 axis);

/// Adjust actor pitch and yaw when the current action permits turning.
/// \param actor Actor whose orientation is updated.
/// \param angles Two finite pitch and yaw displacements in degrees, in that order.
/// \returns Nothing.
/// \pre Updated angles must remain finite and allow full-turn adjustments to change their floating-point values.
/// \note Original decompilation identifier: \c func_800160FC (0x800160FC).
void actorAddActionPitchAndYawDegrees(u8 *actor, f32 *angles);

#endif
