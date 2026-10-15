#include "runtime_lifecycle.h"
#include "audio_playback_query.h"
#include "object_types.h"
#include "action_request.h"
#include "action_script_stop.h"
#include "action_script_playback.h"
#include "action_script_query.h"
#include "actor_sound.h"
/// \file
/// Timed action sequencing beneath actor behavior programs.
/// \ingroup ActorScriptVm
///
/// See docs/VOICE_AND_SCRIPT_VM.md for the subsystem walkthrough.

#include "common.h"
#include "action_script_types.h"
#include "actor_script.h"
/// Action-script command stream addresses indexed by script identifier.
/// \note Original identifier: \c D_80071D00 (0x80071D00).
extern ActionScriptCommand *gActionScriptPrograms[];

/// Request the action specified by the current script command.
/// \param object Object executing the action script.
/// \param state Action state belonging to the object.
/// \param script Script state containing the current command and action request identifiers.
/// \returns 1 when the requested action has started and the script may advance, or 0 while waiting.
/// \note Original decompilation identifier: \c func_80011CB0 (0x80011CB0).
s32 actionScriptCommandStartAction(u8 *object, ActorScriptState *state, ActionScriptState *script) {
    ActionScriptCommand *command;
    ActionScriptCommand **saved = &command;
    u32 current;
    s32 *args;
    s32 kind;
    command = script->command;
    current = actionGetStoredRequestId(object);
    if ((current == 0 || script->pendingRequestId < current) && state->pendingAction == -1) {
        args = (*saved)->args;
        kind = args[1];
        if (kind == 0x12) {
            kind = state->defaultActionMode;
        }
        if (actionRequestPlayback(object, args[0], kind, (s32 *)&script->pendingRequestId)) {
            current = script->pendingRequestId;
        }
        script->lastRequestId = script->pendingRequestId;
    }
    if (current != script->pendingRequestId) {
        return 0;
    }
    script->pendingRequestId = 0;
    script->commandTime = 0.0F;
    return 1;
}

/// Request an action stop once the command iteration threshold is reached.
/// \param object Object executing the action script.
/// \param state Action state belonging to the object.
/// \param script Script state containing the current command and action request identifiers.
/// \returns 1 when the last script action has ended, or 0 while waiting.
/// \note Original decompilation identifier: \c func_80011D78 (0x80011D78).
s32 actionScriptCommandStopAfterIterations(u8 *object, ActorScriptState *state, ActionScriptState *script) {
    ActionScriptCommand *command = script->command;
    if (state->loopCount >= command->args[0]) {
        actionRequestLoopExit(object);
        actionCancelPendingRequest(object);
    }
    if (actionScriptHasLastActionEnded(object) != 0) {
        return 1;
    }
    return 0;
}

/// Wait for the command iteration threshold or the end of the last script action.
/// \param object Object executing the action script.
/// \param state Action state belonging to the object.
/// \param script Script state containing the current command and action request identifiers.
/// \returns 1 when either condition allows the script to advance, or 0 while waiting.
/// \note Original decompilation identifier: \c func_80011DD0 (0x80011DD0).
s32 actionScriptCommandWaitForIterations(u8 *object, ActorScriptState *state, ActionScriptState *script) {
    ActionScriptCommand *command;

    command = script->command;
    if (actionScriptHasLastActionEnded(object) != 0) {
        return 1;
    }
    if (state->loopCount >= command->args[0]) {
        return 1;
    }
    return 0;
}

/// Play a positive sound identifier specified by the current script command.
/// \param object Object at which to play the sound.
/// \param state Unused action state parameter required by the command callback interface.
/// \param script Script state containing the sound command; receives the sound handle when playback is requested.
/// \returns 1, allowing the script to advance.
/// \note Original decompilation identifier: \c func_80011E28 (0x80011E28).
s32 actionScriptCommandPlaySound(u8 *object, ActorScriptState *state, ActionScriptState *script) {
    ActionScriptCommand *command;
    command = script->command;
    if (script->command->args[0] > 0) {
        script->soundHandle = actorPlayTrackedSound(object, command->args[0]);
    }
    return 1;
}

/// Action-script opcode handlers returning whether execution may advance.
/// \note Original identifier: \c D_8006EE50 (0x8006EE50).
extern ActionScriptCommandHandler gActionScriptCommandHandlers[];
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Advance an object action script for the current frame.
/// \param object Object whose action script is updated.
/// \returns -1 when command execution has ended, or 0 while the script remains active.
/// \note Original decompilation identifier: \c func_80011E6C (0x80011E6C).
/// \note This sequencer has its own command-completion contract; its return
/// values are not the actor VM's continue, yield, and action-wait statuses.
/// \see actorUpdateScriptVm
/// \see actorScriptStartActionAndWait
s32 actionScriptUpdate(u8 *object) {
    /* Preserve the original 0x40-byte frame; two reserved words have no recovered value. */
    s32 reserved[2];
    f32 step;
    f32 duration;
    ActionScriptCommand *command;
    ActorScriptState *state;
    s32 opcode;
    ActionScriptState *script;

    state = ((RuntimeObjectPrefix *)object)->state.actor;
    script = &state->actionScript;
    step = gRuntimeFrameSeconds;
    if (actionScriptIsStoppedAndLastActionEnded(object) == 0) {
        script->elapsed += step;
    }
    duration = script->duration;
    if ((duration >= 0.0F) && (duration <= script->elapsed)) {
        script->soundHandle = 0;
        script->command = NULL;
        return -1;
    }
    command = script->command;
    if (command == NULL) {
        return -1;
    }
    script->commandTime += step;
    while (script->command->time <= script->commandTime) {
        opcode = script->command->opcode;
        if (opcode == ACTION_SCRIPT_END) {
            script->command = NULL;
            return -1;
        }
        if (gActionScriptCommandHandlers[script->command->opcode](object, state, script) != 0) {
            script->command++;
        } else {
            script->commandTime -= step;
            break;
        }
    }
    if ((script->command->opcode == ACTION_SCRIPT_END) && (audioCountPlayback(script->soundHandle) == 0) &&
        (actionScriptHasLastActionEnded(object) != 0)) {
        script->soundHandle = 0;
        script->command = NULL;
        return -1;
    }
    return 0;
}

/// Start an object action script without a duration limit.
/// \param object Object that will execute the script.
/// \param scriptId Valid index of the action script to start.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012034 (0x80012034).
void actionScriptStart(u8 *object, s32 scriptId) {
    ActorScriptState *actorState;

    actorState = ((RuntimeObjectPrefix *)object)->state.actor;
    actorState->actionScript.command = gActionScriptPrograms[scriptId];
    actorState->actionScript.lastRequestId = 0;
    actorState->actionScript.pendingRequestId = 0;
    actorState->actionScript.soundHandle = 0;
    actorState->actionScript.index = scriptId;
    actorState->actionScript.commandTime = 0.0F;
    actorState->actionScript.elapsed = 0.0F;
    actorState->actionScript.duration = -1.0F;
}

/// Start an action script with a default mode for actions that inherit it.
/// \param object Object that will execute the script.
/// \param scriptId Valid index of the action script to start.
/// \param mode Default action mode for commands that request the inherited mode.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012078 (0x80012078).
void actionScriptStartWithMode(u8 *object, s32 scriptId, s32 mode) {
    ActorScriptState *actorState;

    actorState = ((RuntimeObjectPrefix *)object)->state.actor;
    actorState->defaultActionMode = mode;
    actionScriptStart(object, scriptId);
}

/// Set the elapsed-time limit of an object action script.
/// \param object Object whose script limit is changed.
/// \param duration Script time limit in seconds; a negative value removes the limit.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001209C (0x8001209C).
void actionScriptSetDuration(u8 *object, f32 duration) {
    ActorScriptState *actorState = ((RuntimeObjectPrefix *)object)->state.actor;
    actorState->actionScript.duration = duration;
}

/// Stop script command execution and request the current action to finish.
/// \param object Object whose action script is stopped.
/// \returns A borrowed pointer to the actor's script identifier, now set to -1; valid while its state lives.
/// \note Original decompilation identifier: \c func_800120AC (0x800120AC).
s32 *actionScriptStop(u8 *object) {
    ActorScriptState *state = ((RuntimeObjectPrefix *)object)->state.actor;
    ActorScriptState *script;

    actionRequestLoopExit(object);
    actionCancelPendingRequest(object);
    state->actionScript.command = NULL;
    script = state;
    state = NULL;
    script->actionScript.index = -1;
    // Debug null check; its body is compiled out, but its block keeps the
    // return add out of the store block.
    if (script == NULL) {
    }
    return &script->actionScript.index;
}

/// Check whether script command execution has stopped.
/// \param object Object whose script is inspected.
/// \returns 1 when no command remains active, or 0 otherwise.
/// \note Original decompilation identifier: \c func_800120F0 (0x800120F0).
s32 actionScriptIsStopped(u8 *object) {
    ActorScriptState *actorState = ((RuntimeObjectPrefix *)object)->state.actor;
    if (actorState->actionScript.command == NULL) {
        return 1;
    }
    return 0;
}

/// Check whether the last action started by the script has ended.
/// \param object Object whose script action is inspected.
/// \returns 1 when a recorded script action has ended or been superseded, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80012114 (0x80012114).
s32 actionScriptHasLastActionEnded(u8 *object) {
    ActorScriptState *actorState;
    u32 currentRequestId;

    actorState = ((RuntimeObjectPrefix *)object)->state.actor;
    if (actorState->actionScript.lastRequestId == 0) {
        return 0;
    }
    currentRequestId = actionGetStoredRequestId(object);
    if ((currentRequestId == 0) || (actorState->actionScript.lastRequestId < currentRequestId)) {
        return 1;
    }
    return 0;
}

/// Check whether command execution has stopped and the last script action has ended.
/// \param object Object whose script is inspected.
/// \returns 1 when both conditions hold, or 0 otherwise.
/// \note Original decompilation identifier: \c func_8001216C (0x8001216C).
s32 actionScriptIsStoppedAndLastActionEnded(u8 *object) {
    if (actionScriptIsStopped(object) == 0) {
        return 0;
    }
    return actionScriptHasLastActionEnded(object);
}

/// Provide the inactive action-script initialization hook for runtime startup.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800121A4 (0x800121A4).
void actionScriptInitializeHook(void) {
}
