/// \file
/// Declares target-following particle emitters and their supported visual styles.

#ifndef EFFECT_EMITTER_H
#define EFFECT_EMITTER_H

#include <ultra64.h>

/// Particle behavior selected for a target-following emitter.
typedef enum EffectEmissionStyle {
    EFFECT_EMIT_BLACK_WISPS = 0,
    EFFECT_EMIT_PALE_GREEN_PARTICLES = 1,
    EFFECT_EMIT_YELLOW_PARTICLES = 2,
    EFFECT_EMIT_PINK_PARTICLES = 3,
    EFFECT_EMIT_GRAY_PARTICLES = 4,
    EFFECT_EMIT_WHITE_WISPS = 5,
    EFFECT_EMIT_SCATTERED_PARTICLES = 6,
    EFFECT_EMIT_AUDIBLE_BURST = 7
} EffectEmissionStyle;

/// Create an emitter using the requested particle style.
/// \param kind EffectEmissionStyle value; unsupported values create no emitter.
/// \param position Three fallback world coordinates, copied when the target is unavailable.
/// \param lifetime Seconds until expiry; nonpositive values disable automatic expiry.
/// \param targetHandle Object to follow while available.
/// \param referenceHandle Object defining the offset direction while both objects are available.
/// \param offset Signed horizontal distance from the target, positive away from the reference.
/// \returns Nothing; the object system manages emitter lifetime.
/// \note Original identifier: \c func_8001940C (0x8001940C).
void effectEmitterCreateForStyle(u32 kind, f32 *position, f32 lifetime, s32 targetHandle, s32 referenceHandle, f32 offset);

#endif
