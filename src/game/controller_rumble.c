#include "controller_rumble.h"
#include "controller_commands.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"



/// Index of the most recently submitted controller rumble request slot.
/// \note Original identifier: \c D_80063500 (0x80063500).
extern u32 gControllerRumbleRequestIndex;
/// Eight retained request slots submitted asynchronously to the controller thread.
/// \note Original identifier: \c D_800AD480 (0x800AD480).
extern ControllerRumbleRequest gControllerRumbleRequests[8];
void controllerQueueRumbleCommand(s32, s32, f32, f32, f32);

/// Submit a controller rumble request for asynchronous processing.
/// \param port Controller port index, from 0 through 3.
/// \param command Operation: 0 starts a pattern, 1 stops it, and 2 resets rumble.
/// \param durationSeconds Total pattern duration in seconds; used for command 0.
/// \param intervalSeconds Time between pulse starts in seconds; used for command 0.
/// \param pulseSeconds Duration of each pulse in seconds; used for command 0.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004970 (0x80004970).
void controllerQueueRumbleCommand(s32 port, s32 command, f32 durationSeconds, f32 intervalSeconds, f32 pulseSeconds) {
    ControllerRumbleRequest *slot;

    gControllerRumbleRequestIndex += 1;
    if (gControllerRumbleRequestIndex >= 8) {
        gControllerRumbleRequestIndex = 0;
    }
    slot = &gControllerRumbleRequests[gControllerRumbleRequestIndex];
    slot->type = 7;
    slot->pulseSeconds = pulseSeconds;
    slot->intervalSeconds = intervalSeconds;
    slot->durationSeconds = durationSeconds;
    slot->command = command;
    slot->port = port;
    osSendMesg(controllerGetCommandQueue(), slot, 1);
}

/// Request that a controller's rumble state be reset.
/// \param port Controller port index, from 0 through 3.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004A0C (0x80004A0C).
void controllerRequestRumbleReset(s32 port) {
    controllerQueueRumbleCommand(port, 2, 0.0f, 0.0f, 0.0f);
}

/// Request a timed pattern of controller rumble pulses.
/// \param port Controller port index, from 0 through 3.
/// \param durationSeconds Total pattern duration in seconds.
/// \param intervalSeconds Time between pulse starts in seconds.
/// \param pulseSeconds Duration of each pulse in seconds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004A3C (0x80004A3C).
void controllerRequestRumblePattern(s32 port, f32 durationSeconds, f32 intervalSeconds, f32 pulseSeconds) {
    controllerQueueRumbleCommand(port, 0, durationSeconds, intervalSeconds, pulseSeconds);
}

/// Request that a controller's rumble pattern stop.
/// \param port Controller port index, from 0 through 3.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004A78 (0x80004A78).
void controllerRequestRumbleStop(s32 port) {
    controllerQueueRumbleCommand(port, 1, 0.0f, 0.0f, 0.0f);
}


/// Check whether a controller currently has an active rumble pattern.
/// \param controller Controller port index, from 0 through 3.
/// \returns 1 when a pattern is active, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80004AA8 (0x80004AA8).
s32 controllerIsRumblePatternActive(s32 controller) {
    return controllerIsRumblePatternScheduled(controller);
}
