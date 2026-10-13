/// \file
/// Creates animated effects anchored to an actor's tinted model part.

#ifndef ATTACHED_EFFECT_H
#define ATTACHED_EFFECT_H

#include <ultra64.h>

/// Create and register an effect, requesting destruction of the actor's previous effect.
/// \param owner Initialized actor with valid script state and model part 0x79;
/// the actor and part must remain valid while the effect updates.
/// \param kind Supported effect-definition index selecting template, scale and opacity.
/// \param track Supported texture-frame animation-program index.
/// \returns New registered effect object whose lifetime is managed by the object system.
/// \note Original identifier: \c func_80013130 (0x80013130).
u8 *attachedEffectCreate(u8 *owner, s32 kind, s32 track);

#endif
