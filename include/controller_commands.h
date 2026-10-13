/// \file
/// Declares the controller-thread command queue and retained rumble request records.

#ifndef CONTROLLER_COMMANDS_H
#define CONTROLLER_COMMANDS_H

#include "ultra64.h"

/// Caller-owned rumble command retained until the controller thread consumes it.
typedef struct ControllerRumbleRequest {
    s16 type;            ///< Controller message kind; seven identifies rumble commands.
    s32 port;            ///< Controller port from zero through three.
    s32 command;         ///< Zero starts a pattern, one stops it, and two resets rumble.
    f32 durationSeconds; ///< Total pattern duration for a start command.
    f32 intervalSeconds; ///< Interval between pulse starts for a start command.
    f32 pulseSeconds;    ///< Duration of each pulse for a start command.
} ControllerRumbleRequest;

typedef char ControllerRumbleRequestSizeCheck[(sizeof(ControllerRumbleRequest) == 0x18) ? 1 : -1];
typedef char
    ControllerRumbleDurationOffsetCheck[((u32) & ((ControllerRumbleRequest *)0)->durationSeconds == 0xC) ? 1 : -1];

/// Get the controller thread's persistent queue for scheduler events and device commands.
/// \returns A borrowed queue; submitted request storage must remain valid until consumed.
/// \note Original decompilation identifier: \c func_8000287C (0x8000287C).
OSMesgQueue *controllerGetCommandQueue(void);

/// Apply a queued rumble start, stop, or reset request to its controller port.
/// \param request Valid rumble command; start commands supply finite nonnegative durations in seconds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8000317C (0x8000317C).
void controllerDispatchRumbleRequest(ControllerRumbleRequest *request);

#endif
