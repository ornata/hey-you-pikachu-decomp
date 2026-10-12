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

#endif
