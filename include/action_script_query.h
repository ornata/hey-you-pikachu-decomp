/// \file
/// Declares completion queries for actor action-script commands and requested actions.

#ifndef ACTION_SCRIPT_QUERY_H
#define ACTION_SCRIPT_QUERY_H

#include "ultra64.h"

/// Check whether script command execution has stopped.
/// \param object Object whose script is inspected.
/// \returns 1 when no command remains active, or 0 otherwise.
/// \note Original decompilation identifier: \c func_800120F0 (0x800120F0).
s32 actionScriptIsStopped(u8 *object);

/// Check whether the last action started by the script has ended.
/// \param object Object whose script action is inspected.
/// \returns 1 when a recorded script action has ended or been superseded, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80012114 (0x80012114).
s32 actionScriptHasLastActionEnded(u8 *object);

/// Check whether command execution has stopped and the last script action has ended.
/// \param object Object whose script is inspected.
/// \returns 1 when both conditions hold, or 0 otherwise.
/// \note Original decompilation identifier: \c func_8001216C (0x8001216C).
s32 actionScriptIsStoppedAndLastActionEnded(u8 *object);

#endif
