/// \file
/// Declares falling-particle creation and its supported motion styles.
#ifndef FALLING_PARTICLE_H
#define FALLING_PARTICLE_H

#include <ultra64.h>

/// Supported falling-particle orientation and lateral-motion styles.
typedef enum FallingParticleMotion {
    FALLING_PARTICLE_BILLBOARD_SPIN = 0, ///< Face the view with animated roll.
    FALLING_PARTICLE_BILLBOARD_SWAY = 1, ///< Face the view with roll and lateral swaying.
    FALLING_PARTICLE_TUMBLING = 2        ///< Turn around two axes independently of the view.
} FallingParticleMotion;

/// Create a falling particle with the selected appearance and motion style.
/// \param kind Valid index in the falling-particle appearance table.
/// \param motionStyle Motion style: zero or one for camera-facing motion, two for tumbling.
/// \param position Readable three-component initial world position, copied during creation.
/// \param velocity Readable three-component initial world velocity, copied during creation.
/// \param lifetime Positive time in seconds before the particle begins disappearing.
/// \returns Registered particle object, managed by the object system.
/// \note Original decompilation identifier: \c func_80019C80 (0x80019C80).
u8 *fallingParticleCreate(s32 kind, s32 motionStyle, f32 *position, f32 *velocity, f32 lifetime);

#endif
