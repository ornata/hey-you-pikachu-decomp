/// \file
/// Declares resident integer-animation programs for object and UI texture frames.

#ifndef TEXTURE_ANIMATION_PROGRAMS_H
#define TEXTURE_ANIMATION_PROGRAMS_H

#include "integer_animation_types.h"

/// Borrowed resident key programs indexed by supported texture-animation identifiers.
/// Entries and their key streams outlive animation tracks; the full table bound
/// remains unresolved, so callers must use identifiers established by their resources.
/// \note Original identifier: \c D_8006B090 (0x8006B090).
extern ValueKey *gTextureFrameAnimationPrograms[];

#endif
