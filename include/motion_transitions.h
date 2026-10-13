/// \file
/// Declares initialization of actor motion-transition storage.

#ifndef MOTION_TRANSITIONS_H
#define MOTION_TRANSITIONS_H

#include <ultra64.h>

/// Initialize the pose-transition lookup map from explicit transitions and action definitions.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F00C (0x8000F00C).
void actionInitializeTransitionMap(void);

#endif
