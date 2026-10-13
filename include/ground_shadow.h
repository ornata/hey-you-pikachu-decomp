/// \file
/// Provides terrain-conforming shadow objects for actor hierarchies.

#ifndef GROUND_SHADOW_H
#define GROUND_SHADOW_H

#include <ultra64.h>

/// Create a shadow with owned geometry and two frame-specific vertex buffers.
/// \param count Vertices along each grid side; must exceed one and fit the renderer vertex limit.
/// \param spacing Distance between adjacent vertices before object scaling.
/// \returns New object managed by the object system; attach it to its owner before updating or drawing it.
/// \note Original decompilation identifier: \c func_800369D4 (0x800369D4).
u8 *groundShadowCreate(s32 count, f32 spacing);

#endif
