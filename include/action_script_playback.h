/// \file
/// Declares action-script startup, inherited playback modes and elapsed-time limits.

#ifndef ACTION_SCRIPT_PLAYBACK_H
#define ACTION_SCRIPT_PLAYBACK_H

#include "ultra64.h"

/// Start an object action script without a duration limit.
/// \param object Object that will execute the script.
/// \param scriptId Valid index of the action script to start.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012034 (0x80012034).
void actionScriptStart(u8 *object, s32 scriptId);

/// Start an action script with a default mode for actions that inherit it.
/// \param object Object that will execute the script.
/// \param scriptId Valid index of the action script to start.
/// \param mode Default action mode for commands that request the inherited mode.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012078 (0x80012078).
void actionScriptStartWithMode(u8 *object, s32 scriptId, s32 mode);

/// Set the elapsed-time limit of an object action script.
/// \param object Object whose script limit is changed.
/// \param duration Script time limit in seconds; a negative value removes the limit.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001209C (0x8001209C).
void actionScriptSetDuration(u8 *object, f32 duration);

/// Start an object's action script with a specified time limit.
/// \param object Address of the object that will execute the action script.
/// \param scriptId Valid identifier of the action script to start.
/// \param duration Time limit in seconds; a negative value removes the limit.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80117AA0 (0x80117AA0).
void actionScriptStartWithDurationBits(u8 *object, s32 scriptId, f32 duration);

/// Advance an object action script for the current frame.
/// \param object Object whose action script is updated.
/// \returns -1 when command execution has ended, or 0 while the script remains active.
/// \note Original decompilation identifier: \c func_80011E6C (0x80011E6C).
/// \note This sequencer has its own command-completion contract; its return
/// values are not the actor VM's continue, yield, and action-wait statuses.
/// \see actorUpdateScriptVm
/// \see actorScriptStartActionAndWait
s32 actionScriptUpdate(u8 *object);

#endif
