/// \file
/// Declares microphone trigger, dictionary-upload and device-state queries.

#ifndef MICROPHONE_SESSION_QUERY_H
#define MICROPHONE_SESSION_QUERY_H

#include "ultra64.h"

/// Check whether a voice dictionary upload is in progress.
/// \returns 1 during an active upload, or 0 otherwise; a requested change that
/// has not started uploading does not make this active.
/// \note Original decompilation identifier: \c func_80004108 (0x80004108).
s32 micSessionIsDictionaryUploadActive(void);

/// Check whether the session is ready for its current microphone trigger state.
/// \returns 1 when the dictionary is settled and an active trigger has microphone input enabled, or 0 otherwise.
/// \note Original decompilation identifier: \c func_8000411C (0x8000411C).
int micSessionIsTriggerReady(void);

/// Get the current microphone trigger state.
/// \returns Nonzero when the trigger is active, or zero when inactive.
/// \note Original decompilation identifier: \c func_80004180 (0x80004180).
s32 micSessionGetTriggerState(void);

/// Get the voice dictionary requested for the session.
/// \returns Requested dictionary identifier.
/// \note Original decompilation identifier: \c func_8000418C (0x8000418C).
s32 micSessionGetRequestedDictionary(void);

/// Get the controller port selected for the Voice Recognition Unit.
/// \returns Selected port index, or -1 when no device has initialized.
/// \note Original decompilation identifier: \c func_80004198 (0x80004198).
s32 micSessionGetDevicePort(void);

/// Check whether a port has a usable Voice Recognition Unit.
/// \param port Controller port index, from 0 through 3.
/// \returns 1 when the port reports a voice device and voice initialization succeeded, or 0 otherwise.
/// \note Original decompilation identifier: \c func_800041A4 (0x800041A4).
s32 micSessionIsDeviceConnected(s32 port);

#endif
