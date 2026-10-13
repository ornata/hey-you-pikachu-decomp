/// \file
/// Declares animated radial effects that follow a live scene object.
#ifndef ANIMATED_RADIAL_EFFECT_H
#define ANIMATED_RADIAL_EFFECT_H

#include <ultra64.h>

/// Create an animated radial effect that follows a live target.
/// \param targetId Handle of the target to follow.
/// \param duration Effect lifetime in seconds.
/// \param count Nonnegative number of visual elements; drawing and matrix allocation sizes must fit in 32
/// bits.
/// \returns Registered effect managed by the object system, or null if the target is unavailable.
/// \note Original decompilation identifier: \c func_8001B618 (0x8001B618).
u8 *animatedRadialEffectCreate(s32 targetId, f32 duration, s32 count);

#endif
