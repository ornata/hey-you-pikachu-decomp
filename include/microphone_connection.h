/// \file
/// Declares querying a controller port for a ready microphone.

#ifndef MICROPHONE_CONNECTION_H
#define MICROPHONE_CONNECTION_H

#include <ultra64.h>

/// Check whether a controller port has a usable Voice Recognition Unit.
/// \param port Controller port index, from 0 through 3.
/// \returns 1 when the port reports a voice device and voice initialization succeeded, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80004944 (0x80004944).
s32 micIsDeviceConnected(s32 port);

#endif
