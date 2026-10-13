/// \file
/// Defines startup and readiness of scheduler-driven controller and device services.

#ifndef CONTROLLER_SERVICE_H
#define CONTROLLER_SERVICE_H

#include "scheduler_types.h"

/// Start controller, microphone, EEPROM and rumble services and wait for readiness.
/// \param scheduler Initialized scheduler retained for frame-event delivery.
/// \param channelCount Number of controller channels to enable, from one through four.
/// \param microphonePortMask Low four bits select ports eligible to host the microphone.
/// \returns Nothing; device initialization and shared command-queue setup have completed.
/// Call once during runtime startup; the scheduler must outlive the service thread.
/// \note Original decompilation identifier: \c func_80002764 (0x80002764).
void controllerStartThread(GameScheduler *scheduler, s32 channelCount, s32 microphonePortMask);

#endif
