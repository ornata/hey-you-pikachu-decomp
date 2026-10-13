/// \file
/// Declares flickering visual effects that follow a live scene object.
#ifndef FLICKERING_EFFECT_H
#define FLICKERING_EFFECT_H

#include <ultra64.h>

/// Create a flickering effect that follows a live target.
/// \param targetId Handle of the target to follow.
/// \param duration Effect lifetime in seconds.
/// \param count Nonnegative number of visual elements; drawing and matrix allocation sizes must fit in 32
/// bits.
/// \returns Registered effect managed by the object system, or null if the target is unavailable.
/// \note Original decompilation identifier: \c func_8001AEDC (0x8001AEDC).
u8 *flickeringEffectCreate(s32 targetId, f32 duration, s32 count);

#endif
