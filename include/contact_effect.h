/// \file
/// Declares creation of registered contact effects.

#ifndef CONTACT_EFFECT_H
#define CONTACT_EFFECT_H

#include <ultra64.h>

/// Create and register a contact-reactive effect at a world position.
/// \param x World-space X coordinate.
/// \param y World-space Y coordinate; negative values request ground placement using the corresponding positive search height.
/// \param z World-space Z coordinate.
/// \returns Newly initialized and registered effect.
/// \note Original decompilation identifier: \c func_8001D6C0 (0x8001D6C0).
u8 *contactEffectCreate(f32 x, f32 y, f32 z);

#endif
