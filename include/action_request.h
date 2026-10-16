/// \file
/// Declares actor action requests, loop exit and request-identifier queries.

#ifndef ACTION_REQUEST_H
#define ACTION_REQUEST_H

#include "ultra64.h"

/// Request an action, starting immediately when playback is idle or replacing the pending request until current playback ends.
/// \param object Actor receiving the action request.
/// \param id Valid action identifier from zero through 329.
/// \param kind Cue-selection tag supplied by the requesting script, representable as a signed 16-bit value; independent of motion playback mode.
/// \param out Receives the identifier assigned to this request.
/// \returns One when idle playback accepts the request immediately; zero when it is queued.
/// \note Original decompilation identifier: \c func_8000F404 (0x8000F404).
s32 actionRequestPlayback(u8 *object, s32 id, s32 kind, s32 *out);

/// Request that the current action leave its loop when playback reaches its end.
/// \param object Actor whose action state is accessed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F4C4 (0x8000F4C4).
void actionRequestLoopExit(u8 *object);

/// Cancel the pending action request.
/// \param object Actor whose action state is accessed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000F51C (0x8000F51C).
void actionCancelPendingRequest(u8 *object);

/// Get the request identifier associated with active action playback.
/// \param object Actor whose action state is accessed.
/// \returns The current request identifier, or zero when playback is inactive.
/// \note Original decompilation identifier: \c func_8000F69C (0x8000F69C).
s32 actionGetActiveRequestId(u8 *object);

/// Get the most recently selected action request identifier.
/// \param object Actor whose action state is accessed.
/// \returns The stored request identifier, regardless of playback activity.
/// \note Original decompilation identifier: \c func_8000F6C0 (0x8000F6C0).
s32 actionGetStoredRequestId(u8 *object);

#endif
