/// \file
/// Timed action sequencing beneath actor behavior programs.
/// \ingroup ActorScriptVm
///
/// See docs/VOICE_AND_SCRIPT_VM.md for the subsystem walkthrough.

/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
void actionScriptStart(u8 *, s32);
void actionCancelPendingRequest(u8 *);
void actionRequestLoopExit(u8 *);
s32 actorPlayTrackedSound(s32, s32);
/// Action-script command stream addresses indexed by script identifier.
/// \note Original identifier: \c D_80071D00 (0x80071D00).
extern s32 gActionScriptPrograms[];
s32 actionScriptHasLastActionEnded(u8 *);
s32 actionGetStoredRequestId(u8 *);

s32 actionRequestPlayback(u8 *, s32, s32, s32 *);
/// Request the action specified by the current script command.
/// \param object Object executing the action script.
/// \param state Action state belonging to the object.
/// \param script Script state containing the current command and action request identifiers.
/// \returns 1 when the requested action has started and the script may advance, or 0 while waiting.
/// \note Original decompilation identifier: \c func_80011CB0 (0x80011CB0).
s32 actionScriptCommandStartAction(u8 *object, u8 *state, u8 *script) {
    u8 *command;
    u8 **saved = &command;
    u32 current;
    s32 *args;
    s32 kind;
    command = *(u8 **)(script + 0x10);
    current = actionGetStoredRequestId(object);
    if ((current == 0 || *(u32 *)(script + 0x1C) < current) && *(s16 *)(state + 0x120) == -1) {
        args = (s32 *)(*saved + 8);
        kind = args[1];
        if (kind == 0x12) {
            kind = *(s16 *)(state + 0x13C);
        }
        if (actionRequestPlayback(object, args[0], kind, (s32 *)(script + 0x1C))) {
            current = *(u32 *)(script + 0x1C);
        }
        *(u32 *)(script + 0x18) = *(u32 *)(script + 0x1C);
    }
    if (current != *(u32 *)(script + 0x1C))
        return 0;
    *(u32 *)(script + 0x1C) = 0;
    *(f32 *)(script + 0xC) = 0.0f;
    return 1;
}

/// Request an action stop once the command iteration threshold is reached.
/// \param object Object executing the action script.
/// \param state Action state belonging to the object.
/// \param script Script state containing the current command and action request identifiers.
/// \returns 1 when the last script action has ended, or 0 while waiting.
/// \note Original decompilation identifier: \c func_80011D78 (0x80011D78).
s32 actionScriptCommandStopAfterIterations(s32 object, u8 *state, u8 *script) {
    u8 *command = *(u8 **)(script + 0x10);
    if (*(s16 *)(state + 0x11C) >= *(s32 *)(command + 8)) {
        actionRequestLoopExit((u8 *)object);
        actionCancelPendingRequest((u8 *)object);
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
s32 actionScriptCommandWaitForIterations(u8 *object, u8 *state, u8 *script) {
    u8 *sp1C;

    sp1C = *(u8 **)(script + 0x10);
    if (actionScriptHasLastActionEnded(object) != 0) {
        return 1;
    }
    if (*(s16 *)(state + 0x11C) >= *(s32 *)(sp1C + 8)) {
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
s32 actionScriptCommandPlaySound(s32 object, s32 state, u8 *script) {
    u8 *command;
    command = *(u8 **)(script + 0x10);
    if (*(s32 *)(*(u8 **)(script + 0x10) + 8) > 0) {
        *(s32 *)(script + 0x14) = actorPlayTrackedSound(object, *(s32 *)(command + 8));
    }
    return 1;
}

typedef struct ScriptCommand {
    s32 opcode;
    f32 time;
    s32 args[2];
} ScriptCommand;
typedef struct MotionScript {
    s32 index;
    f32 elapsed, duration, commandTime;
    ScriptCommand *command;
    s32 child, previous, current;
} MotionScript;
s32 actionScriptIsStoppedAndLastActionEnded(s32);
s32 audioCountPlayback(s32);
/// Action-script opcode handlers returning whether execution may advance.
/// \note Original identifier: \c D_8006EE50 (0x8006EE50).
extern s32 (*gActionScriptCommandHandlers[])(u8 *, u8 *, MotionScript *);
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
    ScriptCommand *command;
    u8 *state;
    s32 opcode;
    MotionScript *script;

    state = *(u8 **)(object + 0x54);
    script = (MotionScript *)(state + 0x158);
    step = gRuntimeFrameSeconds;
    if (actionScriptIsStoppedAndLastActionEnded((s32)object) == 0) {
        script->elapsed += step;
    }
    duration = script->duration;
    if ((duration >= 0.0f) && (duration <= script->elapsed)) {
        script->child = 0;
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
        if (opcode == 4) {
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
    if ((script->command->opcode == 4) && (audioCountPlayback(script->child) == 0) &&
        (actionScriptHasLastActionEnded(object) != 0)) {
        script->child = 0;
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
    u8 *actorState;

    actorState = *(u8 **)(object + 0x54);
    *(s32 *)(actorState + 0x168) = gActionScriptPrograms[scriptId];
    *(s32 *)(actorState + 0x170) = 0;
    *(s32 *)(actorState + 0x174) = 0;
    *(s32 *)(actorState + 0x16C) = 0;
    *(s32 *)(actorState + 0x158) = scriptId;
    *(f32 *)(actorState + 0x164) = 0.0f;
    *(f32 *)(actorState + 0x15C) = 0.0f;
    *(f32 *)(actorState + 0x160) = -1.0f;
}

/// Start an action script with a default mode for actions that inherit it.
/// \param object Object that will execute the script.
/// \param scriptId Valid index of the action script to start.
/// \param mode Default action mode for commands that request the inherited mode.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012078 (0x80012078).
void actionScriptStartWithMode(u8 *object, s32 scriptId, s32 mode) {
    u8 *v;

    v = *(u8 **)(object + 0x54);
    *(s16 *)(v + 0x13C) = mode;
    actionScriptStart(object, scriptId);
}

/// Set the elapsed-time limit of an object action script.
/// \param object Object whose script limit is changed.
/// \param duration Script time limit in seconds; a negative value removes the limit.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8001209C (0x8001209C).
void actionScriptSetDuration(u8 *object, f32 duration) {
    u8 *actorState = *(u8 **)(object + 0x54);
    *(f32 *)(actorState + 0x160) = duration;
}

/// Stop script command execution and request the current action to finish.
/// \param object Object whose action script is stopped.
/// \returns A borrowed pointer to the object's stopped script state.
/// \note Original decompilation identifier: \c func_800120AC (0x800120AC).
s32 *actionScriptStop(u8 *object) {
    u8 *state = *(u8 **)(object + 0x54);
    u8 *script;

    actionRequestLoopExit(object);
    actionCancelPendingRequest(object);
    *(s32 *)(state + 0x168) = 0;
    script = state;
    state = NULL;
    *(s32 *)(script + 0x158) = -1;
    // Debug null check; its body is compiled out, but its block keeps the
    // return add out of the store block.
    if (script == NULL) {
    }
    return (s32 *)(script + 0x158);
}

/// Check whether script command execution has stopped.
/// \param object Object whose script is inspected.
/// \returns 1 when no command remains active, or 0 otherwise.
/// \note Original decompilation identifier: \c func_800120F0 (0x800120F0).
s32 actionScriptIsStopped(u8 *object) {
    u8 *actorState = *(u8 **)(object + 0x54);
    if (*(s32 *)(actorState + 0x168) == 0) {
        return 1;
    }
    return 0;
}

/// Check whether the last action started by the script has ended.
/// \param object Object whose script action is inspected.
/// \returns 1 when a recorded script action has ended or been superseded, or 0 otherwise.
/// \note Original decompilation identifier: \c func_80012114 (0x80012114).
s32 actionScriptHasLastActionEnded(u8 *object) {
    u8 *actorState;
    u32 currentRequestId;

    actorState = *(u8 **)(object + 0x54);
    if (*(u32 *)(actorState + 0x170) == 0) {
        return 0;
    }
    currentRequestId = actionGetStoredRequestId(object);
    if ((currentRequestId == 0) || (*(u32 *)(actorState + 0x170) < currentRequestId)) {
        return 1;
    }
    return 0;
}

/// Check whether command execution has stopped and the last script action has ended.
/// \param object Object whose script is inspected.
/// \returns 1 when both conditions hold, or 0 otherwise.
/// \note Original decompilation identifier: \c func_8001216C (0x8001216C).
s32 actionScriptIsStoppedAndLastActionEnded(s32 object) {
    if (actionScriptIsStopped((u8 *)object) == 0) {
        return 0;
    }
    return actionScriptHasLastActionEnded(object);
}

/// Provide the inactive action-script initialization hook for runtime startup.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800121A4 (0x800121A4).
void actionScriptInitializeHook(void) {
}
