/// \file
/// Creates animated effect chains with target motion and neighbor constraints.

#ifndef LINKED_EFFECT_H
#define LINKED_EFFECT_H

#include <ultra64.h>

/// Initial motion policies for linked effect chains.
typedef enum LinkedEffectKind {
    LINKED_EFFECT_TARGET = 0,        ///< Approach the target with an outward horizontal launch.
    LINKED_EFFECT_RISING_TARGET = 1, ///< Approach the target with an upward launch component.
    LINKED_EFFECT_REBOUND = 2,       ///< Rebound before adopting target-directed motion.
    LINKED_EFFECT_FALLING = 3        ///< Fall before adopting a later motion policy.
} LinkedEffectKind;

/// Create and register a chain of animated effects directed toward a target.
/// \param kind Supported LinkedEffectKind motion policy.
/// \param x Initial world-space X coordinate.
/// \param y Initial world-space Y coordinate.
/// \param z Initial world-space Z coordinate.
/// \param scale Uniform effect size.
/// \param target Object-manager handle of the target, resolved during updates.
/// \param speed Nonnegative maximum target-directed displacement per update.
/// \param damping Fraction of retained velocity used during constrained motion.
/// \param spacing Allowed separation from the next effect.
/// \param previousPull Correction strength toward the previous effect.
/// \param nextPull Correction strength toward the next effect.
/// \param channelA Supported texture-frame animation-program index.
/// \param channelB Supported opacity animation-program index.
/// \param count Positive requested chain length.
/// \returns Registered group object whose lifetime is managed by the object system.
/// \note Original identifier: \c func_8001D3B8 (0x8001D3B8).
u8 *linkedEffectCreateChain(s32 kind, f32 x, f32 y, f32 z, f32 scale, s32 target, f32 speed, f32 damping, f32 spacing,
                            f32 previousPull, f32 nextPull, s32 channelA, s32 channelB, s32 count);

#endif
