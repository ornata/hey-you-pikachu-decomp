/// \file
/// Declares action playback stopping and continuation-chain control.

#ifndef ACTION_CONTINUATION_H
#define ACTION_CONTINUATION_H

#include "ultra64.h"

/// Stop action playback and clear pending action requests.
/// \param object Actor whose action state is accessed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F4D4 (0x8000F4D4).
void actionStopPlayback(u8 *object);

/// Set the continuation action for current playback.
/// \param object Actor whose continuation is changed.
/// \param actionId Continuation action identifier or termination sentinel, representable as a signed 16-bit value; identifiers outside 0 through 329 end continuation.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F6CC (0x8000F6CC).
void actionSetContinuation(u8 *object, s32 actionId);

/// Skip looping continuation actions to reach their nonlooping successor or termination sentinel.
/// \pre The continuation chain must not contain a cycle of looping actions.
/// \param object Actor whose action state is accessed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F714 (0x8000F714).
void actionSkipLoopingContinuations(u8 *object);

#endif
