/// \file
/// Defines gameplay microphone queries forwarded to the controller-owned session.

#ifndef MICROPHONE_QUERY_H
#define MICROPHONE_QUERY_H

#include <ultra64.h>

/// Check whether vocabulary upload is currently active.
/// \returns One during an active upload, otherwise zero.
/// \note Original decompilation identifier: \c func_800048A4 (0x800048A4).
s32 micIsDictionaryUploadActive(void);

/// Check readiness for the current microphone trigger.
/// \returns One when the dictionary is settled and any active trigger has microphone input enabled.
/// \note Original decompilation identifier: \c func_800048C4 (0x800048C4).
s32 micIsTriggerReady(void);

/// Read the current microphone trigger state.
/// \returns Nonzero when the trigger is active, otherwise zero.
/// \note Original decompilation identifier: \c func_800048E4 (0x800048E4).
s32 micGetTriggerState(void);

/// Read the dictionary requested by microphone configuration.
/// \returns Requested dictionary identifier.
/// \note Original decompilation identifier: \c func_80004904 (0x80004904).
s32 micGetRequestedDictionary(void);

/// Read the selected recognition-device port.
/// \returns Selected controller port, or minus one when unavailable.
/// \note Original decompilation identifier: \c func_80004924 (0x80004924).
s32 micGetDevicePort(void);

#endif
