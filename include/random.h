/// \file
/// Declares the shared pseudorandom generator and bounded gameplay sampling.

#ifndef GAME_RANDOM_H
#define GAME_RANDOM_H

#include "ultra64.h"

/// Seed the global pseudorandom generator from current system state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003AA88 (0x8003AA88).
void randomSeedFromSystem(void);

/// Get a bounded value from the global pseudorandom generator.
/// \param limit Exclusive upper bound; nonpositive values produce zero.
/// \returns Value in [0, limit) for a positive limit, or zero otherwise.
/// \note Original decompilation identifier: \c func_8003AB20 (0x8003AB20).
u32 randomBelow(s32 limit);

/// Advance the global pseudorandom generator.
/// \returns Next unsigned 32-bit pseudorandom value.
/// \note Original decompilation identifier: \c func_8003AB68 (0x8003AB68).
u32 randomNext(void);

#endif
