/// \file
/// Declares expanding orange effects and their timed target-following emitters.
#ifndef ORANGE_EFFECT_H
#define ORANGE_EFFECT_H

#include <ultra64.h>

/// Create an expanding orange visual effect that follows a target while it exists.
/// \param targetHandle Object handle identifying the target to follow.
/// \param lifetime Effect lifetime in seconds.
/// \returns Registered effect managed by the object system; an unavailable target does not prevent creation.
/// \note Original decompilation identifier: \c func_8001B9EC (0x8001B9EC).
u8 *expandingOrangeEffectCreate(s32 targetHandle, f32 lifetime);

/// Create an emitter that repeatedly produces expanding orange effects at a target.
/// \param targetHandle Object handle identifying the target.
/// \param lifetime Emitter lifetime in seconds.
/// \returns Registered emitter managed by the object system; an unavailable target does not prevent creation.
/// \note Original decompilation identifier: \c func_8001BE0C (0x8001BE0C).
u8 *orangePulseEmitterCreate(s32 targetHandle, f32 lifetime);

#endif
