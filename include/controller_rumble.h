/// \file
/// Declares queued controller rumble patterns and current playback queries.
/// Requests require an initialized controller thread and take effect asynchronously.

#ifndef CONTROLLER_RUMBLE_H
#define CONTROLLER_RUMBLE_H

#include "ultra64.h"

/// Request that a controller's rumble state be reset.
/// \param port Controller port index, from 0 through 3.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004A0C (0x80004A0C).
void controllerRequestRumbleReset(s32 port);

/// Request a timed pattern of controller rumble pulses.
/// \param port Controller port index, from 0 through 3.
/// \param durationSeconds Total pattern duration in seconds.
/// \param intervalSeconds Time between pulse starts in seconds.
/// \param pulseSeconds Duration of each pulse in seconds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004A3C (0x80004A3C).
void controllerRequestRumblePattern(s32 port, f32 durationSeconds, f32 intervalSeconds, f32 pulseSeconds);

/// Request that a controller's rumble pattern stop.
/// \param port Controller port index, from 0 through 3.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80004A78 (0x80004A78).
void controllerRequestRumbleStop(s32 port);

/// Check whether a controller currently has an active rumble pattern.
/// \param controller Controller port index, from 0 through 3.
/// \returns 1 when a pattern is active, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80004AA8 (0x80004AA8).
s32 controllerIsRumblePatternActive(s32 controller);

/// Check whether a controller port has a running rumble pattern.
/// \param port Controller port index from zero through three.
/// \returns One while a pattern is scheduled to run, or zero otherwise.
/// \note Original decompilation identifier: \c func_80003394 (0x80003394).
s32 controllerIsRumblePatternScheduled(s32 port);

#endif
