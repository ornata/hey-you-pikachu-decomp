/// \file
/// Declares moving ribbon hierarchies that steer toward a world position.
#ifndef RIBBON_EFFECT_H
#define RIBBON_EFFECT_H

#include <ultra64.h>

/// Create a moving ribbon effect that steers toward a world position.
/// \param count Nonnegative number of intermediate ribbon segments; drawing and matrix allocation sizes must
/// fit in 32 bits.
/// \param duration Effect lifetime in seconds.
/// \param baseWidth Base ribbon width scale.
/// \param widthVariation Amount of variation in the ribbon width.
/// \param widthStyle Width variation style: zero for waves or one for random variation.
/// \param x Initial world X coordinate.
/// \param y Initial world Y coordinate.
/// \param z Initial world Z coordinate.
/// \param pitch Initial motion-node Y-axis rotation in degrees.
/// \param yaw Initial motion-node Z-axis rotation in degrees.
/// \param destination Readable three-component world position toward which the ribbon steers; copied during
/// creation.
/// \returns Registered ribbon hierarchy root, managed by the object system.
/// \note Original decompilation identifier: \c func_80018514 (0x80018514).
u8 *ribbonEffectCreate(s32 count, f32 duration, f32 baseWidth, f32 widthVariation, s32 widthStyle, f32 x, f32 y, f32 z,
                       f32 pitch, f32 yaw, f32 *destination);

#endif
