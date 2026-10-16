/// \file
/// Native actor-script dispatch and action scheduling commands.
/// \ingroup ActorScriptVm
///
/// See docs/VOICE_AND_SCRIPT_VM.md for the subsystem walkthrough.

#include "random_scale.h"
#include "player_chase.h"
#include "random.h"
#include "backdrop_view.h"
#include "area_transition.h"
#include "runtime_control.h"
#include "text_window.h"
#include "message_bar.h"
#include "actor_target.h"
#include "story_milestones.h"
#include "progress_value.h"
#include "action_continuation.h"
#include "action_script_stop.h"
#include "action_script_playback.h"
#include "action_script_query.h"
#include "actor_script_action.h"
#include "common.h"
#include "object_types.h"
#include "actor_script.h"
#include "script_vm.h"
#include "script_vm_stack.h"

/// Shared 80-byte text used by actor-script text and message commands.
/// \note NONPORTABLE: buffer accesses span separate declarations whose contiguous target layout is retained for matching.
/// \note Original identifier: \c D_80134220 (0x80134220).
s8 gActorScriptSharedTextBuffer;
/// Retained word storage within the shared actor-script text buffer.
/// \note Original identifier: \c D_80134220_0 (0x80134224).
s32 gActorScriptSharedTextWordStorage;
/// Retained trailing storage within the shared actor-script text buffer.
/// \note Original identifier: \c D_80134220_1 (0x80134228).
struct {
    s32 w[18];
} gActorScriptSharedTextTailStorage;

#include "scene_configuration.h"

/// Selected eye-catch group index, or minus one when no interlude is selected.
/// \note Original identifier: \c D_8009B580 (0x8009B580).
extern s32 gEyeCatchSelectedGroup;

/// Dispatch the next registered command for an actor script.
/// \param actor Actor whose script VM supplies a valid command selector and command arguments.
/// \returns The selected command execution status.
/// \note Original decompilation identifier: \c func_8011BD50 (0x8011BD50).
/// \pre The command selector and stack arguments satisfy the selected command's contract.
/// \see actorRunScriptVm
/// \see actorScriptYield
/// \see actorScriptStartActionAndWait
s32 actorScriptDispatchRegisteredCommand(u8 *actor) {
    ScriptVm *vm = (ScriptVm *)((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    s32 commandIndex = scriptVmReadIntegerOperand(vm);
    ScriptNativeCommand command = vm->commands[commandIndex];

    return command(actor);
}

/// Prepend text to a string, truncating the combined text to fit its destination.
/// \param destination Null-terminated string in a writable buffer of at least capacity bytes.
/// \param capacity Positive destination buffer capacity, including the terminating null byte.
/// \param prefix Null-terminated prefix in storage that does not overlap the destination.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011BD9C (0x8011BD9C).
void stringPrependBounded(u8 *destination, s32 capacity, u8 *prefix) {
    s32 i;
    s32 prefixLength;
    s32 length;
    for (prefixLength = 0; prefix[prefixLength] != 0 && prefixLength < capacity - 1; prefixLength++) {
    }
    for (length = 0; destination[length] != 0 && length < capacity - prefixLength - 1; length++) {
    }
    for (; length >= 0; length--) {
        destination[length + prefixLength] = destination[length];
    }
    for (i = 0; i < prefixLength; i++) {
        destination[i] = prefix[i];
    }
    destination[capacity - 1] = 0;
}

/// Yield actor script execution with a zero script result.
/// \param actor Actor whose script VM receives zero.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011BEB0 (0x8011BEB0).
s32 actorScriptYield(u8 *actor) {
    scriptVmPushInteger((ScriptVm *)(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress), 0);
    return SCRIPT_COMMAND_YIELD;
}

/// Schedule playback of the script-selected action and wait for completion.
/// \param actor Actor address whose script action is started; its VM supplies the action arguments and later receives the completion result.
/// \returns Two, transferring script execution to the action handler.
/// \note Original decompilation identifier: \c func_8011BEDC (0x8011BEDC).
s32 actorScriptStartActionAndWait(u8 *actor) {
    actorBeginScriptActionHandler((u8 *)((s32)actor), ACTOR_SCRIPT_ACTION_PLAY_ACTION);
    return SCRIPT_COMMAND_WAIT_FOR_ACTION;
}

/// Request action-loop exit and wait for the current action result.
/// \param actor Actor address whose current action must leave its loop; its VM later receives the completion result.
/// \returns Two, transferring script execution to the action handler.
/// \note Original decompilation identifier: \c func_8011BF00 (0x8011BF00).
s32 actorScriptStartLoopExitAndWait(u8 *actor) {
    actorBeginScriptActionHandler((u8 *)((s32)actor), ACTOR_SCRIPT_ACTION_EXIT_ACTION_LOOP);
    return SCRIPT_COMMAND_WAIT_FOR_ACTION;
}

/// Request action-loop exit and wait for the current action result through the alternate script command.
/// \param actor Actor address whose current action must leave its loop; its VM later receives the completion result.
/// \returns Two, transferring script execution to the action handler.
/// \note Original decompilation identifier: \c func_8011BF24 (0x8011BF24).
s32 actorScriptStartLoopExitAndWaitAlternate(u8 *actor) {
    actorBeginScriptActionHandler((u8 *)((s32)actor), ACTOR_SCRIPT_ACTION_EXIT_ACTION_LOOP);
    return SCRIPT_COMMAND_WAIT_FOR_ACTION;
}

/// Begin playback of the script-selected action with message interruption enabled.
/// \param actor Actor address whose script action is started; its VM supplies the action arguments and later receives the completion result.
/// \returns Two, transferring script execution to the action handler.
/// \note Original decompilation identifier: \c func_8011BF48 (0x8011BF48).
s32 actorScriptStartInterruptibleAction(u8 *actor) {
    actorBeginScriptActionHandler((u8 *)((s32)actor), ACTOR_SCRIPT_ACTION_PLAY_INTERRUPTIBLE_ACTION);
    return SCRIPT_COMMAND_WAIT_FOR_ACTION;
}

/// Begin the script-selected held-item throwing action.
/// \param actor Actor address whose script action is started; its VM supplies the action arguments and later receives the completion result.
/// \returns Two, transferring script execution to the action handler.
/// \note Original decompilation identifier: \c func_8011BF6C (0x8011BF6C).
s32 actorScriptStartHeldItemThrow(u8 *actor) {
    actorBeginScriptActionHandler((u8 *)((s32)actor), ACTOR_SCRIPT_ACTION_THROW_HELD_ITEM);
    return SCRIPT_COMMAND_WAIT_FOR_ACTION;
}

/// Begin the scripted item-acquisition action.
/// \param actor Actor address whose script action is started; its VM supplies the action arguments and later receives the completion result.
/// \returns Two, transferring script execution to the action handler.
/// \note Original decompilation identifier: \c func_8011BF90 (0x8011BF90).
s32 actorScriptStartItemAcquisition(u8 *actor) {
    actorBeginScriptActionHandler((u8 *)((s32)actor), ACTOR_SCRIPT_ACTION_ACQUIRE_ITEM);
    return SCRIPT_COMMAND_WAIT_FOR_ACTION;
}

/// Select how the actor attachment position is determined.
/// \param actor Actor whose script VM supplies an attachment mode from zero through five and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011BFB4 (0x8011BFB4).
s32 actorScriptSetAttachmentMode(u8 *actor) {
    u8 *actorState;
    s32 vmAddress;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    vmAddress = ((ActorScriptState *)actorState)->vmAddress;
    ((ActorScriptState *)actorState)->attachmentMode = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Begin the scripted held-item eating action.
/// \param actor Actor address whose script VM supplies the eating action and later receives its result.
/// \returns Two, transferring script execution to the action handler.
/// \note Original decompilation identifier: \c func_8011BFF8 (0x8011BFF8).
s32 actorScriptStartEatingHeldItem(u8 *actor) {
    actorBeginScriptActionHandler((u8 *)((s32)actor), ACTOR_SCRIPT_ACTION_EAT_HELD_ITEM);
    return SCRIPT_COMMAND_WAIT_FOR_ACTION;
}

/// Begin an action that uses the actor action resource.
/// \param actor Actor address whose script VM supplies an action selector followed by a repetition count and later receives the action result.
/// \returns Two, transferring script execution to the action handler.
/// \note Original decompilation identifier: \c func_8011C01C (0x8011C01C).
s32 actorScriptStartCostedAction(u8 *actor) {
    actorBeginScriptActionHandler((u8 *)((s32)actor), ACTOR_SCRIPT_ACTION_PERFORM_COSTED_ACTION);
    return SCRIPT_COMMAND_WAIT_FOR_ACTION;
}

/// Build shared script text from string arguments.
/// \param actor Actor whose script supplies a nonnegative string count as an instruction operand and that many stack strings; the combined text follows argument order, is limited to 79 characters, and its VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C040 (0x8011C040).
s32 actorScriptBuildSharedText(u8 *actor) {
    s32 vmAddress;
    s32 remainingStrings;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    gActorScriptSharedTextBuffer = 0;
    remainingStrings = scriptVmReadIntegerOperand((ScriptVm *)(vmAddress)) - 1;
    if (remainingStrings >= 0) {
        do {
            stringPrependBounded((u8 *)&gActorScriptSharedTextBuffer, 0x50,
                                 (u8 *)scriptVmPopString((ScriptVm *)vmAddress));
            remainingStrings -= 1;
        } while (remainingStrings >= 0);
    }
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Build shared script text through the alternate text command.
/// \param actor Actor whose script supplies a nonnegative string count as an instruction operand and that many stack strings; the combined text follows argument order, is limited to 79 characters, and its VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C0C8 (0x8011C0C8).
s32 actorScriptBuildSharedTextAlternate(u8 *actor) {
    s32 vmAddress;
    s32 var_s0;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    gActorScriptSharedTextBuffer = 0;
    var_s0 = scriptVmReadIntegerOperand((ScriptVm *)(vmAddress)) - 1;
    if (var_s0 >= 0) {
        do {
            stringPrependBounded((u8 *)&gActorScriptSharedTextBuffer, 0x50,
                                 (u8 *)scriptVmPopString((ScriptVm *)vmAddress));
            var_s0 -= 1;
        } while (var_s0 >= 0);
    }
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Provide zero as the script result.
/// \param actor Actor whose script VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C150 (0x8011C150).
s32 actorScriptPushZero(u8 *actor) {
    scriptVmPushInteger((ScriptVm *)(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Advance the shared generator and append a random float in the inclusive unit interval.
/// \param actor Actor whose script VM receives a random value from zero through one.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C17C (0x8011C17C).
s32 actorScriptPushRandomUnitFloat(u8 *actor) {
    ScriptVm *vm = (ScriptVm *)((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;

    scriptVmPushFloat(vm, randomNext() * RANDOM_U32_UNIT_SCALE);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Replace a script-supplied limit with a bounded random integer.
/// Nonpositive limits leave the shared generator unchanged.
/// \param actor Actor whose script VM supplies an exclusive upper bound and receives a random nonnegative integer below it, or zero for a nonpositive bound.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C1F0 (0x8011C1F0).
s32 actorScriptRandomBelow(u8 *actor) {
    ScriptVm *vm;

    vm = (ScriptVm *)((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(vm, randomBelow(scriptVmPopInteger(vm)));
    return SCRIPT_COMMAND_CONTINUE;
}

#include "vector_operations.h"
/// Choose a random integer while accounting for recent selections in small ranges.
/// \param actor Actor whose script VM supplies an exclusive upper bound and receives a random nonnegative integer below it, or zero for a nonpositive bound.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C230 (0x8011C230).
s32 actorScriptRandomBelowWithHistory(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(vmAddress), randomBelowWithHistory(scriptVmPopInteger((ScriptVm *)(vmAddress))));
    return SCRIPT_COMMAND_CONTINUE;
}

/// Convert a numeric script value to an integer result.
/// \param actor Actor whose script VM supplies the numeric value and receives its integer conversion.
/// \returns Zero, allowing script execution to continue.
/// \pre Floating-point input must be finite and representable as a signed 32-bit integer after truncation.
/// \note Original decompilation identifier: \c func_8011C270 (0x8011C270).
s32 actorScriptPassThroughInteger(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(sp1C), scriptVmPopInteger((ScriptVm *)(sp1C)));
    return SCRIPT_COMMAND_CONTINUE;
}

/// Provide the supplied floating-point value as the script result.
/// \param actor Actor whose script VM supplies a floating-point value and receives that same value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C2A8 (0x8011C2A8).
s32 actorScriptPassThroughFloat(u8 *actor) {
    u8 *vm = (u8 *)((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;

    scriptVmPushFloat((ScriptVm *)((s32)vm), scriptVmPopFloat((ScriptVm *)((s32)vm)));
    return SCRIPT_COMMAND_CONTINUE;
}

/// Restart the resettable script timer.
/// \param actor Actor whose script timer is reset to zero seconds and whose VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C2E0 (0x8011C2E0).
s32 actorScriptResetTimer(u8 *actor) {
    u8 *vmBytes;

    vmBytes = (u8 *)((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->vmAddress;
    ((ScriptVm *)vmBytes)->elapsed = 0.0F;
    scriptVmPushInteger((ScriptVm *)((s32)vmBytes), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Provide the elapsed time on the resettable script timer.
/// \param actor Actor whose script VM receives the number of elapsed seconds since the timer was reset.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C314 (0x8011C314).
s32 actorScriptPushTimer(u8 *actor) {
    u8 *vmBytes;

    vmBytes = (u8 *)((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->vmAddress;
    scriptVmPushFloat((ScriptVm *)((s32)vmBytes), ((ScriptVm *)vmBytes)->elapsed);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Queue script text for display using the configured message style and duration.
/// \param actor Actor whose script supplies a nonnegative string count as an instruction operand and that many stack strings, whose combined text in argument order must fit within 59 characters; its VM receives zero regardless of whether the message is accepted.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C340 (0x8011C340).
s32 actorScriptEnqueueMessage(u8 *actor) {
    s32 var_s0;
    u8 *vmBytes;

    vmBytes = (u8 *)((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->vmAddress;
    gActorScriptSharedTextBuffer = 0;
    var_s0 = scriptVmReadIntegerOperand((ScriptVm *)(vmBytes)) - 1;
    if (var_s0 >= 0) {
        do {
            stringPrependBounded((u8 *)&gActorScriptSharedTextBuffer, 0x50,
                                 (u8 *)scriptVmPopString((ScriptVm *)vmBytes));
            var_s0 -= 1;
        } while (var_s0 >= 0);
    }
    messageBarEnqueue(((ScriptVm *)vmBytes)->messageStyle, ((ScriptVm *)vmBytes)->messageDuration,
                      (u8 *)&gActorScriptSharedTextBuffer);
    scriptVmPushInteger((ScriptVm *)((s32)vmBytes), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Clear queued messages and end the active message display.
/// \param actor Actor whose script VM receives zero after the messages are cleared.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C3D8 (0x8011C3D8).
s32 actorScriptClearMessages(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    messageBarClear();
    scriptVmPushInteger((ScriptVm *)(sp1C), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Select the font style for subsequent script messages.
/// \param actor Actor whose script VM supplies a valid message font-style index and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C410 (0x8011C410).
s32 actorScriptSetMessageStyle(u8 *actor) {
    u8 *sp1C;

    sp1C = (u8 *)((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->vmAddress;
    ((ScriptVm *)sp1C)->messageStyle = scriptVmPopInteger((ScriptVm *)((s32)sp1C));
    scriptVmPushInteger((ScriptVm *)((s32)sp1C), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Set the display duration for subsequent script messages.
/// \param actor Actor whose script VM supplies a nonnegative floating-point duration in seconds and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C44C (0x8011C44C).
s32 actorScriptSetMessageDuration(u8 *actor) {
    u8 *vmBytes;

    vmBytes = (u8 *)((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->vmAddress;
    ((ScriptVm *)vmBytes)->messageDuration = scriptVmPopFloat((ScriptVm *)((s32)vmBytes));
    scriptVmPushInteger((ScriptVm *)((s32)vmBytes), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Append script text to the text window.
/// \param actor Actor whose script supplies a nonnegative string count as an instruction operand and that many stack strings; the combined text follows argument order, is limited to 79 characters, and its VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C488 (0x8011C488).
s32 actorScriptAppendWindowText(u8 *actor) {
    s32 vmAddress;
    s32 var_s0;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    gActorScriptSharedTextBuffer = 0;
    var_s0 = scriptVmReadIntegerOperand((ScriptVm *)(vmAddress)) - 1;
    if (var_s0 >= 0) {
        do {
            stringPrependBounded((u8 *)&gActorScriptSharedTextBuffer, 0x50,
                                 (u8 *)scriptVmPopString((ScriptVm *)vmAddress));
            var_s0 -= 1;
        } while (var_s0 >= 0);
    }
    textWindowAppendText((u8 *)&gActorScriptSharedTextBuffer);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Schedule a text-window request with the script-supplied settings.
/// \param actor Actor whose script VM supplies the window indicator, font style, and view mode when the request can proceed; the VM receives zero after the request.
/// \returns Two, transferring execution to the text-window action handler.
/// \note Original decompilation identifier: \c func_8011C518 (0x8011C518).
s32 actorScriptStartTextWindow(u8 *actor) {
    actorBeginScriptActionHandler((u8 *)((s32)actor), ACTOR_SCRIPT_ACTION_OPEN_TEXT_WINDOW);
    return SCRIPT_COMMAND_WAIT_FOR_ACTION;
}

/// Display pending text and enter the text-window game mode.
/// \param actor Actor whose script VM receives zero after the mode is requested.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C53C (0x8011C53C).
s32 actorScriptEnterTextWindowMode(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    textWindowEnterMode();
    scriptVmPushInteger((ScriptVm *)(sp1C), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Start an action script for the actor.
/// \param actor Actor whose script VM supplies a valid action-script index and receives zero.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011C574 (0x8011C574).
s32 actorScriptStartActionScript(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    actionScriptStart(actor, scriptVmPopInteger((ScriptVm *)(sp1C)));
    scriptVmPushInteger((ScriptVm *)(sp1C), 0);
    return SCRIPT_COMMAND_YIELD;
}

/// Stop action-script command execution and request the current action to finish.
/// \param actor Actor whose action script is stopped and whose script VM receives zero.
/// \returns Zero, allowing script execution to continue.
///
/// \note Original decompilation identifier: \c func_8011C5BC (0x8011C5BC).
s32 actorScriptStopActionScript(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    actionScriptStop(actor);
    scriptVmPushInteger((ScriptVm *)(sp1C), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Set a fixed target position with no target-distance allowance.
/// \param actor Actor whose script VM supplies the target X, Y, and Z coordinates as floating-point values and receives zero; tracked-target selection is cleared.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C5F4 (0x8011C5F4).
s32 actorScriptSetTargetPosition(u8 *actor) {
    f32 pos[3];
    s32 vm;

    vm = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    pos[2] = scriptVmPopFloat((ScriptVm *)(vm));
    pos[1] = scriptVmPopFloat((ScriptVm *)(vm));
    pos[0] = scriptVmPopFloat((ScriptVm *)(vm));
    actorSetTargetPosition(actor, pos);
    ((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->targetMode = ACTOR_TARGET_FIXED;
    ((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->targetDistanceAllowance = 0.0F;
    scriptVmPushInteger((ScriptVm *)(vm), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Report the remaining distance to approach the target.
/// \param actor Actor whose script VM receives the approximate three-dimensional target distance minus its distance allowance, clamped to zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C674 (0x8011C674).
s32 actorScriptPushTargetApproachDistance(u8 *actor) {
    u8 *vm = (u8 *)((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    f32 d = actorGetTargetDistance(actor) -
            ((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->targetDistanceAllowance;

    if (d < 0.0F) {
        d = 0.0F;
    }
    scriptVmPushFloat((ScriptVm *)((s32)vm), d);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Select the fallback object position as a fixed target with no distance allowance.
/// \param actor Actor with a live fallback object whose current position supplies the target; tracked-target selection is cleared and its script VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C6D8 (0x8011C6D8).
s32 actorScriptSetTargetToFallbackObject(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    actorSetTargetToFallbackObject(actor);
    ((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->targetMode = ACTOR_TARGET_FIXED;
    ((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->targetDistanceAllowance = 0.0F;
    scriptVmPushInteger((ScriptVm *)(sp1C), 0);
    return SCRIPT_COMMAND_CONTINUE;
}

/// Report whether action-script command execution has stopped.
/// \param actor Actor whose script VM receives one when no action-script command remains active, or zero otherwise.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C72C (0x8011C72C).
s32 actorScriptPushActionScriptStopped(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(sp1C), actionScriptIsStopped(actor));
    return SCRIPT_COMMAND_CONTINUE;
}

/// Report whether the last action started by the action script has ended.
/// \param actor Actor whose script VM receives one when a recorded script action has ended or been superseded, or zero otherwise.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C764 (0x8011C764).
s32 actorScriptPushLastScriptActionEnded(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(vmAddress), actionScriptHasLastActionEnded(actor));
    return SCRIPT_COMMAND_CONTINUE;
}

/// Stop the action script and begin a transition to a requested action pose.
/// \param actor Actor whose script VM supplies an action index from zero through 329 followed by a positive transition duration in seconds and receives zero; the motion cache and actor animation resources must be initialized.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011C79C (0x8011C79C).
s32 actorScriptBeginPoseTransition(u8 *actor) {
    s32 vmAddress;
    s32 actionIndex;
    f32 transitionSeconds;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    transitionSeconds = scriptVmPopFloat((ScriptVm *)(vmAddress));
    actionIndex = scriptVmPopInteger((ScriptVm *)(vmAddress));
    actionScriptStop(actor);
    actionBeginPoseTransition(actor, actionIndex, transitionSeconds);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return SCRIPT_COMMAND_YIELD;
}
