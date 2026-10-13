/// \file
/// Declares target-following converging-particle effect creation.
#ifndef CONVERGING_PARTICLE_H
#define CONVERGING_PARTICLE_H

#include <ultra64.h>

/// Create styled particles that converge around a live target.
/// \param targetId Handle of the target the effect surrounds.
/// \param lifetime Positive effect lifetime in seconds.
/// \param count Positive number of visual elements; drawing and matrix allocation sizes must fit in 32 bits.
/// \param styleId Valid index in the converging-effect style table.
/// \returns Registered effect managed by the object system, or null if the target is unavailable.
///
/// \note Original decompilation identifier: \c func_8001A624 (0x8001A624).
u8 *convergingParticleEffectCreate(s32 targetId, f32 lifetime, s32 count, s32 styleId);

#endif
