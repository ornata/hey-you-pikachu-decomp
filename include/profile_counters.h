/// \file
/// Declares bounded persistent counters indexed by item kind in the player profile.

#ifndef PROFILE_COUNTERS_H
#define PROFILE_COUNTERS_H

#include "ultra64.h"

/// Profile storage for 166 counters with values from zero through fifteen.
/// \note Original identifier: \c D_800F0168 (0x800F0168).
extern u8 gProfilePackedCounters[0x53];

/// Read a saved profile counter.
/// \param index Profile counter index from zero through 165.
/// \returns The counter value from zero through fifteen.
/// \note Original decompilation identifier: \c func_80114CC8 (0x80114CC8).
u8 profileGetCounter(s32 index);

/// Adjust a saved profile counter within its supported limits.
/// \param idx Profile counter index from zero through 165.
/// \param delta Signed adjustment; its sum with the current counter must fit a signed integer before
/// clamping.
/// \returns The updated counter value, limited to zero through fifteen.
/// \note Original decompilation identifier: \c func_80114D04 (0x80114D04).
u8 profileAddCounter(s32 idx, s32 delta);

#endif
