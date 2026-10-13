/// \file
/// Defines controller-thread microphone session initialization and policy updates.

#ifndef MICROPHONE_SESSION_H
#define MICROPHONE_SESSION_H

#include <ultra64.h>

/// Initialize session defaults and discover an eligible recognition device.
/// \param portMask Low four bits select controller ports eligible to host the device.
/// \returns Nothing; session processing remains disabled until configured.
/// Requires initialized controller status and serial communication.
/// \note Original decompilation identifier: \c func_800043F4 (0x800043F4).
void micSessionInitialize(s32 portMask);

/// Advance enabled trigger, amplifier, dictionary and recognition policy.
/// \returns Nothing; device recovery and dictionary upload may require later updates.
/// Called by the controller thread after controller status and samples are refreshed.
/// \note Original decompilation identifier: \c func_80004204 (0x80004204).
void micSessionUpdate(void);

#endif
