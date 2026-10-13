/// \file
/// Declares resident integer-animation programs for effect and marker opacity.

#ifndef OPACITY_ANIMATION_PROGRAMS_H
#define OPACITY_ANIMATION_PROGRAMS_H

#include "integer_animation_types.h"

/// Borrowed resident opacity key programs indexed by supported resource identifiers.
/// Table entries and key streams outlive animation tracks. The complete table bound
/// remains unresolved; entry zero supplies the default attached-effect opacity.
/// \note Original identifier: \c D_8006B4B0 (0x8006B4B0).
extern ValueKey *gOpacityAnimationPrograms[];

#endif
