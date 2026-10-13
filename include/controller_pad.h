/// \file
/// Declares cached controller samples, port availability and hardware polling.

#ifndef CONTROLLER_PAD_H
#define CONTROLLER_PAD_H

#include "ultra64.h"

/// Poll connected controllers and refresh the cached pad samples.
/// Blocks until the serial-interface read completes. If no controllers are
/// connected, leaves the cached samples unchanged.
/// Takes no parameters.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerPollPads (0x80003A40).
void controllerPollPads(void);

/// Clear all four cached controller samples without reading the hardware.
/// Takes no parameters.
/// \returns Nothing.
/// \note Original decompilation identifier: \c controllerClearPadCache (0x80003A98).
void controllerClearPadCache(void);

/// Copy a controller's latest cached sample without reading the hardware.
/// \param pad Output sample; must point to writable storage. Cleared when the
/// port is invalid or disconnected; an errored connected sample is still copied.
/// \param port Controller port index, normally 0 through 3.
/// \returns 1 if the port is connected and its cached sample has no error,
/// 0 otherwise.
/// \note Original decompilation identifier: \c controllerReadCachedPad (0x80003AC0).
s32 controllerReadCachedPad(OSContPad *pad, s32 port);

/// Get the cached device status for a controller port.
/// \param port Controller port index from zero through three.
/// \returns Borrowed pointer to the port’s persistent cached status.
/// \note Original address: 0x80002810.
OSContStatus *controllerGetPortStatus(s32 port);

/// Get the cached availability mask for enabled controller ports.
/// \returns A bit mask with bit zero through three identifying available ports.
/// \note Original address: 0x80002ADC.
u8 controllerGetConnectedPortMask(void);

/// Check cached availability for a controller port.
/// \param port Controller port index; indices outside zero through three are unavailable.
/// \returns One when the enabled port is available, or zero otherwise.
/// \note Original address: 0x80002AE8.
s32 controllerIsPortConnected(s32 port);

#endif
