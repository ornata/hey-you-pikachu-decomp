/// \file
/// Declares animated wisp effects positioned in world space.
#ifndef WISP_EFFECT_H
#define WISP_EFFECT_H

#include <ultra64.h>

/// Create an animated wisp effect of the selected style at a world position.
/// \param kind Wisp style from zero through nine.
/// \param x World X coordinate.
/// \param y World Y coordinate.
/// \param z World Z coordinate.
/// \returns Registered wisp effect, managed by the object system.
/// \note Original decompilation identifier: \c func_80017918 (0x80017918).
u8 *wispEffectCreate(s32 kind, f32 x, f32 y, f32 z);

#endif
