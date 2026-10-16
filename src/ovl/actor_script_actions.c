/// \file
/// Applies actor script actions to items, effects, controls, and presentation.

#include "runtime_mode.h"
#include "random_scale.h"
#include "player_control.h"
#include "random.h"
#include "particle_cluster.h"
#include "orange_effect.h"
#include "text_window.h"
#include "item_queries.h"
#include "actor_held_item.h"
#include "item_size.h"
#include "action_request.h"
#include "action_script_stop.h"
#include "action_script_playback.h"
#include "action_script_query.h"
#include "actor_script_action.h"
#include "actor_meters.h"
#include "camera_overlay.h"
#include "actor_color.h"
#include "common.h"
#include "item_response.h"
#include "actor_create.h"
#include "actor_parts.h"
#include "audio_cue.h"
#include "object_lookup.h"
#include "item_types.h"
#include "script_vm.h"
#include "script_vm_stack.h"
#include "actor_script.h"
#include "object_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/// Consume an eligible held item and replenish the actor action resource.
/// \param actor Actor whose held item is consumed.
/// \returns One when an eligible item was processed, or zero when no eligible item is held.
/// \note Original decompilation identifier: \c func_80116730 (0x80116730).
s32 actorConsumeHeldItem(u8 *actor) {
    u8 *liveObject;
    s32 *itemFlags;
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;

    liveObject = objectFindLiveByHandle(((ActorScriptState *)actorState)->heldItemHandle);
    if (liveObject == NULL) {
        return 0;
    }
    itemFlags = ((RuntimeObjectPrefix *)liveObject)->state.words;
    if (*itemFlags & ITEM_STATE_DISABLE_INTERACTION_RESPONSE_AND_CONSUMPTION) {
        return 0;
    }
    itemTransform(((RuntimeObjectPrefix *)liveObject)->handle, 0);
    actorAdjustActionMeter(actor, 10.0F);
    return 1;
}

/// Select a script action handler and request its initialization.
/// \param actor Actor with a script context to update.
/// \param handler Script action-handler selector.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801167A0 (0x801167A0).
void actorBeginScriptActionHandler(u8 *actor, s32 handler) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ((ScriptVm *)((ActorScriptState *)actorState)->vmAddress)->actionInitializing = 1;
    ((ScriptVm *)((ActorScriptState *)actorState)->vmAddress)->actionHandler = handler;
}

/// Wait for timed sequencing to stop and provide a zero script result.
/// \param actor Actor whose sequencer is awaited; motion playback may remain active after sequencing stops.
/// \returns One when sequencing has stopped, or two while command execution continues.
/// \note Original decompilation identifier: \c func_801167BC (0x801167BC).
s32 actorScriptWaitForSequencerStop(u8 *actor) {
    ActorScriptState *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.actor;
    if (actionScriptIsStopped(actor) != 0) {
        scriptVmPushInteger((ScriptVm *)actorState->vmAddress, 0);
        return SCRIPT_VM_YIELDED;
    }
    return SCRIPT_VM_WAITING_FOR_ACTION;
}

/// Start the timed program supplied by the script and wait for sequencing to stop.
/// \param actor Actor whose script supplies a valid timed-program index or ACTION_SCRIPT_NO_PROGRAM and receives zero when sequencing stops.
/// \returns One when complete, or two while waiting.
/// \note Original decompilation identifier: \c func_80116800 (0x80116800).
s32 actorScriptPlayAction(u8 *actor) {
    ScriptVm *vm;
    ActorScriptState *actorState = ((RuntimeObjectPrefix *)actor)->state.actor;

    vm = (ScriptVm *)actorState->vmAddress;
    if (vm->actionInitializing != 0) {
        actionScriptStart(actor, scriptVmPopInteger(vm));
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    return actorScriptWaitForSequencerStop(actor);
}

/// Wait for sequencing to stop or interrupt it when a script message is pending.
/// \param actor Actor whose action and script mailbox are checked.
/// \returns One when complete, or two while waiting; completion provides the pending-message flag or zero as the script result.
/// \note Original decompilation identifier: \c func_80116858 (0x80116858).
s32 actorScriptWaitForActionOrMessage(u8 *actor) {
    s32 messagePending;
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    messagePending = ((ActorScriptState *)actorState)->messagePending;
    if (messagePending != 0) {
        scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)actorState)->vmAddress), messagePending);
        actionScriptStop(actor);
        return SCRIPT_VM_YIELDED;
    }
    return actorScriptWaitForSequencerStop(actor);
}

/// Start the script-selected timed program and wait for sequencing to stop or a pending message.
/// \param actor Actor whose script supplies the action and receives its completion result.
/// \returns One when complete, or two while waiting; completion supplies the pending-message flag or zero as the script result.
/// \note Interruption preserves the pending message. The result retains the flag's integer value without Boolean normalization.
/// \note Original decompilation identifier: \c func_801168A8 (0x801168A8).
s32 actorScriptPlayInterruptibleAction(u8 *actor) {
    u8 *vmBytes;
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;

    vmBytes = (u8 *)((ActorScriptState *)actorState)->vmAddress;
    if (((ScriptVm *)vmBytes)->actionInitializing != 0) {
        actionScriptStart(actor, scriptVmPopInteger((ScriptVm *)((s32)vmBytes)));
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    return actorScriptWaitForActionOrMessage(actor);
}

/// Start the script-selected held-item throwing action and wait for it to stop.
/// \param actor Actor whose held item is thrown by the selected action.
/// \returns One when complete or no held item is available, otherwise two; the script result is -1 for a missing item or zero after the action stops.
/// \note A nonzero held handle permits startup without establishing that the item is live. Zero reports sequencing completion, not a successful throw.
/// \note Original decompilation identifier: \c func_80116900 (0x80116900).
s32 actorScriptThrowHeldItem(u8 *actor) {
    u8 *state = ((RuntimeObjectPrefix *)actor)->state.bytes;
    s32 anim;

    if (((ScriptVm *)((ActorScriptState *)state)->vmAddress)->actionInitializing != 0) {
        anim = scriptVmPopInteger((ScriptVm *)(((ActorScriptState *)state)->vmAddress));
        TRACE("nerveThrowItem - %d\n", ((ActorScriptState *)state)->heldItemHandle);
        if (((ActorScriptState *)state)->heldItemHandle == 0) {
            scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)state)->vmAddress), -1);
            return SCRIPT_VM_YIELDED;
        }
        actionScriptStart(actor, anim);
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    return actorScriptWaitForSequencerStop(actor);
}

/// Run the script-selected item acquisition action and report the held item kind.
/// \param actor Actor whose script supplies the program, item-kind filter and search range in push order.
/// \returns One when complete, or two while waiting; completion supplies the held item kind, or zero when absent, as the script result.
/// \note Search settings are retained even when an existing held handle completes the request immediately.
/// Otherwise completion requires both stopped sequencing and an ended or superseded last action.
/// \note Original decompilation identifier: \c func_80116984 (0x80116984).
s32 actorScriptAcquireItem(u8 *actor) {
    s32 value;
    u8 *vmBytes;
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    vmBytes = (u8 *)((ActorScriptState *)actorState)->vmAddress;
    if (((ScriptVm *)vmBytes)->actionInitializing != 0) {
        ((ActorScriptState *)actorState)->searchItemRange = scriptVmPopFloat((ScriptVm *)((s32)vmBytes));
        ((ActorScriptState *)actorState)->searchItemKind =
            scriptVmPopInteger((ScriptVm *)(((ActorScriptState *)actorState)->vmAddress));
        value = scriptVmPopInteger((ScriptVm *)(((ActorScriptState *)actorState)->vmAddress));
        if (((ActorScriptState *)actorState)->heldItemHandle != 0) {
            scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)actorState)->vmAddress), actorGetHeldItemKind(actor));
            return SCRIPT_VM_YIELDED;
        }
        actionScriptStart(actor, value);
        goto block_6;
    }
    if (actionScriptIsStoppedAndLastActionEnded(actor) != 0) {
        scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)actorState)->vmAddress), actorGetHeldItemKind(actor));
        return SCRIPT_VM_YIELDED;
    }
block_6:
    return SCRIPT_VM_WAITING_FOR_ACTION;
}

/// Run the selected eating action and attempt to consume the current held item.
/// \param actor Actor whose script supplies one eating-program operand and receives
/// -1 when initialization rejects the item or completion finds it missing; otherwise
/// the result is one for player-pickup history or zero without that history.
/// \returns One when finished, or two while the eating or follow-up action waits.
/// \note The result describes the item before the consumption attempt and does not
/// guarantee consumption. A result can already be available during follow-up waiting.
/// \note Original decompilation identifier: \c func_80116A4C (0x80116A4C).
s32 actorScriptEatHeldItem(u8 *actor) {
    u8 *state;
    u8 *obj;
    u8 *rec;
    s32 anim;

    state = ((RuntimeObjectPrefix *)actor)->state.bytes;
    if (((ScriptVm *)((ActorScriptState *)state)->vmAddress)->actionInitializing != 0) {
        anim = scriptVmPopInteger((ScriptVm *)(((ActorScriptState *)state)->vmAddress));
        TRACE("nerveGrabItem - %d\n", ((ActorScriptState *)state)->heldItemHandle);
        obj = objectFindLiveByHandle(((ActorScriptState *)state)->heldItemHandle);
        if (obj == NULL) {
            scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)state)->vmAddress), -1);
            return SCRIPT_VM_YIELDED;
        }
        rec = ((RuntimeObjectPrefix *)obj)->state.bytes;
        if (((ItemStatePrefix *)rec)->flags & 0x800000) {
            scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)state)->vmAddress), -1);
            return SCRIPT_VM_YIELDED;
        }
        itemEnableShrinking(obj);
        actionScriptStart(actor, anim);
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    if (actionScriptIsStopped(actor) != 0) {
        obj = objectFindLiveByHandle(((ActorScriptState *)state)->heldItemHandle);
        if (obj == NULL) {
            scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)state)->vmAddress), -1);
        } else if (itemHasPlayerPickupHistory(obj) != 0) {
            scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)state)->vmAddress), 1);
        } else {
            scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)state)->vmAddress), 0);
        }
        TRACE("nerveEatItem - %d\n", ((ActorScriptState *)state)->heldItemHandle);
        actorConsumeHeldItem(actor);
        if (objectFindLiveByHandle(((ActorScriptState *)state)->heldItemHandle) != NULL) {
            actionScriptStart(actor, 0x4F);
            actorBeginScriptActionHandler(actor, ACTOR_SCRIPT_ACTION_POLL_ACTION_STOPPED);
            return SCRIPT_VM_WAITING_FOR_ACTION;
        }
        return SCRIPT_VM_YIELDED;
    }
    return SCRIPT_VM_WAITING_FOR_ACTION;
}

/// Poll whether the actor action has stopped.
/// \param object Actor whose action is checked.
/// \returns One when stopped, or two while the action continues.
/// \note Original decompilation identifier: \c func_80116B98 (0x80116B98).
s32 actorScriptPollActionStopped(u8 *object) {
    if (actionScriptIsStopped(object) != 0) {
        return SCRIPT_VM_YIELDED;
    }
    return SCRIPT_VM_WAITING_FOR_ACTION;
}

/// Program selections and per-repetition resource policy for a costed script action.
typedef struct ActorScriptActionCost {
    s32 programId;                  ///< Timed program selected when the request is accepted.
    s32 insufficientProgramId;      ///< Timed program selected when resource is insufficient.
    f32 repetitionCost;             ///< Action resource charged for each requested repetition.
    s32 requiresSufficientResource; ///< Nonzero rejects requests exceeding the available resource.
} ActorScriptActionCost;

typedef char ActorScriptActionCostSizeCheck[(sizeof(ActorScriptActionCost) == 0x10) ? 1 : -1];
/// Five retained action-cost records indexed by selectors zero through four.
/// Selectors three and four share the first two program pairs but waive the resource check.
/// This neighbor-delimited view does not establish a larger allocation or bounds checking.
/// \note Original identifier: \c D_8012A660 (0x8012A660).
extern ActorScriptActionCost gActorScriptCostedActions[];
/// Perform a script-selected action subject to its resource cost and report its outcome.
/// \param object Actor whose script supplies a valid cost selector followed by a repetition count.
/// The actor and its owning root must have valid action-resource state.
/// \returns One when complete, or two while waiting. The script result is zero after insufficient
/// resource, three after an accepted start, or one when later response handling changes the outcome.
/// \note Startup consumes both operands; polling consumes none. Completion supplies one integer result.
/// A disabled resource check permits the request but still charges the clamped action resource.
/// \note Original decompilation identifier: \c func_80116BC4 (0x80116BC4).
s32 actorScriptPerformCostedAction(u8 *object) {
    f32 cost;
    s32 costSelector;
    s32 repetitions;
    ScriptVm *vm;
    ActorScriptState *state;
    state = ((RuntimeObjectPrefix *)object)->state.actor;
    vm = (ScriptVm *)state->vmAddress;
    if (vm->actionInitializing != 0) {
        repetitions = scriptVmPopInteger(vm);
        costSelector = scriptVmPopInteger((ScriptVm *)state->vmAddress);
        cost = gActorScriptCostedActions[costSelector].repetitionCost * repetitions;
        if (gActorScriptCostedActions[costSelector].requiresSufficientResource != 0 && state->actionResource < cost) {
            state->costedActionResult = 0;
            state->targetedBurstAllowance = 0;
            actionScriptStart(object, gActorScriptCostedActions[costSelector].insufficientProgramId);
        } else {
            actorAdjustActionMeter(object, -cost);
            state->costedActionResult = 3;
            state->targetedBurstAllowance = repetitions;
            actionScriptStart(object, gActorScriptCostedActions[costSelector].programId);
        }
    } else if (actionScriptIsStopped(object) != 0) {
        scriptVmPushInteger((ScriptVm *)(state->vmAddress), state->costedActionResult);
        return SCRIPT_VM_YIELDED;
    }
    return SCRIPT_VM_WAITING_FOR_ACTION;
}

/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
extern s32 gRuntimeGameMode;

/// Select the script-requested player control scheme and controller assignment when permitted.
/// \param actor Actor whose script supplies a control-scheme identifier from zero through twenty-seven.
/// \returns One after the request and a zero script result, or two while the current game mode delays the request.
/// \note Original decompilation identifier: \c func_80116CF8 (0x80116CF8).
s32 actorScriptSelectControlScheme(u8 *actor) {
    u8 *sp1C;

    if (gRuntimeGameMode == GAME_MODE_TEXT_WINDOW) {
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    sp1C = ((RuntimeObjectPrefix *)actor)->state.bytes;
    playerSelectControlScheme(scriptVmPopInteger((ScriptVm *)(((ActorScriptState *)sp1C)->vmAddress)));
    scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)sp1C)->vmAddress), 0);
    return SCRIPT_VM_YIELDED;
}

#include "hud.h"
#include "player_interaction.h"
/// Show the area label and begin the area-transition fade when its delay has elapsed.
/// \param actor Actor whose script context controls the transition.
/// \returns Two while this handler or its selected successor waits for the transition.
/// \note The delay uses the VM's total elapsed clock; requesting this action does not restart that clock.
/// \note Original decompilation identifier: \c func_80116D58 (0x80116D58).
s32 actorScriptBeginAreaTransition(u8 *actor) {
    u8 *vmBytes;
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;

    vmBytes = (u8 *)((ActorScriptState *)actorState)->vmAddress;
    if (((ScriptVm *)vmBytes)->actionInitializing != 0) {
        hudSetAreaLabelVisible(1);
    } else if (((ScriptVm *)vmBytes)->totalElapsed > 1.5F) {
        cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.6F);
        actorBeginScriptActionHandler(actor, ACTOR_SCRIPT_ACTION_FINISH_AREA_TRANSITION);
    }
    return SCRIPT_VM_WAITING_FOR_ACTION;
}

/// Remaining duration of the primary camera secondary overlay transition.
/// \note Original identifier: \c D_800ABDCC (0x800ABDCC).
extern f32 gPrimarySecondaryOverlayRemainingSeconds;

/// Wait for the transition fade, then hide the area label and resume normal transition handling.
/// \param actor Actor whose script waits for the transition.
/// \returns One when complete with a zero script result, or two while waiting.
/// \note Original decompilation identifier: \c func_80116DE0 (0x80116DE0).
s32 actorScriptFinishAreaTransition(u8 *actor) {
    u8 *sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.bytes;
    if (((ScriptVm *)((ActorScriptState *)sp1C)->vmAddress)->actionInitializing != 0) {
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    if (gPrimarySecondaryOverlayRemainingSeconds > 0.0F) {
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    hudSetAreaLabelVisible(0);
    audioSetMusicDuckingInhibitFlags(0);
    scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)sp1C)->vmAddress), 0);
    return SCRIPT_VM_YIELDED;
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
/// Transform the actor to the script-selected kind with a timed visual and audio effect.
/// \param object Actor to transform while preserving its script context.
/// \returns One when complete, or two during the effect; completion supplies the replacement actor handle as the script result.
/// \note Original decompilation identifier: \c func_80116E60 (0x80116E60).
s32 actorScriptTransformKind(u8 *object) {
    f64 randomFraction;
    u8 *state;
    u8 *vm;
    u8 *linked;
    u32 random;
    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    vm = (u8 *)((ActorScriptState *)state)->vmAddress;
    if (((ScriptVm *)vm)->actionInitializing != 0) {
        actorTransitionModelEnvironmentColor(object, 255, 200, 50, 255, 0.6F);
        random = randomNext();
        linked = actorGetDefaultPart(object);
        randomFraction = random * RANDOM_U32_UNIT_SCALE;
        particleClusterCreate(10, 1.7F, 3.0F, (randomFraction * 0.2F) + 0.5, linked);
        orangePulseEmitterCreate(((RuntimeObjectPrefix *)object)->handle, 2.0F);
        ((ScriptVm *)vm)->actionTimer = 2.0F;
        audioPlayCue(31);
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    ((ScriptVm *)vm)->actionTimer -= gRuntimeFrameSeconds;
    if (((ScriptVm *)vm)->actionTimer > 0.0F) {
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    linked = actorReplaceKindPreservingScript(object, scriptVmPopInteger((ScriptVm *)((s32)vm)));
    actorSetModelEnvironmentColor(linked, 255, 200, 50, 255);
    actorTransitionModelEnvironmentColor(linked, 0, 0, 0, 0, 0.4F);
    audioPlayCue(30);
    scriptVmPushInteger((ScriptVm *)((s32)vm), ((RuntimeObjectPrefix *)linked)->handle);
    return SCRIPT_VM_YIELDED;
}

/// Request a text window using the script-supplied indicator, style, and view mode.
/// \param actor Actor whose script supplies indicator, style, and scene-view mode in push order.
/// Nonzero scene-view mode permits scene updates while the text window is active.
/// \returns Two without consuming settings while in text-window game mode; otherwise one
/// after consuming all three settings and supplying a zero script result. Zero does not indicate
/// that the window opened: an already-active window can reject the request.
/// \note Original decompilation identifier: \c func_80117014 (0x80117014).
s32 actorScriptOpenTextWindow(u8 *actor) {
    s32 viewMode;
    ScriptVm *vm;
    s32 style;

    if (gRuntimeGameMode == GAME_MODE_TEXT_WINDOW) {
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    vm = (ScriptVm *)((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    viewMode = scriptVmPopInteger(vm);
    style = scriptVmPopInteger(vm);
    textWindowOpen(scriptVmPopInteger(vm), style, viewMode);
    TRACE("messageSetup(%d,%d,%d)\n");
    scriptVmPushInteger(vm, 0);
    return SCRIPT_VM_YIELDED;
}

/// Advance the actor currently selected script action handler.
/// \param object Actor whose script action handler is dispatched.
/// \returns One when the handler completes, two while it waits, or five for an unknown handler.
/// \note Original decompilation identifier: \c func_80117098 (0x80117098).
s32 actorScriptDispatchActionHandler(u8 *object) {
    u8 *state;

    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    switch (((ScriptVm *)((ActorScriptState *)state)->vmAddress)->actionHandler) {
    case ACTOR_SCRIPT_ACTION_DEFAULT_ACTION:
        if (actionScriptIsStopped(object) != 0) {
            actionScriptStart(object, 0x57);
        }
        scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)state)->vmAddress), 0);
        return SCRIPT_VM_YIELDED;
    case ACTOR_SCRIPT_ACTION_POLL_ACTION_STOPPED:
        return actorScriptPollActionStopped(object);
    case ACTOR_SCRIPT_ACTION_PLAY_ACTION:
        return actorScriptPlayAction(object);
    case ACTOR_SCRIPT_ACTION_EXIT_ACTION_LOOP:
        actionRequestLoopExit(object);
        return actorScriptWaitForSequencerStop(object);
    case ACTOR_SCRIPT_ACTION_WAIT_FOR_ACTION_OR_MESSAGE:
        return actorScriptWaitForActionOrMessage(object);
    case ACTOR_SCRIPT_ACTION_PLAY_INTERRUPTIBLE_ACTION:
        return actorScriptPlayInterruptibleAction(object);
    case ACTOR_SCRIPT_ACTION_THROW_HELD_ITEM:
        return actorScriptThrowHeldItem(object);
    case ACTOR_SCRIPT_ACTION_ACQUIRE_ITEM:
        return actorScriptAcquireItem(object);
    case ACTOR_SCRIPT_ACTION_EAT_HELD_ITEM:
        return actorScriptEatHeldItem(object);
    case ACTOR_SCRIPT_ACTION_PERFORM_COSTED_ACTION:
        return actorScriptPerformCostedAction(object);
    case ACTOR_SCRIPT_ACTION_SELECT_CONTROL_SCHEME:
        return actorScriptSelectControlScheme(object);
    case ACTOR_SCRIPT_ACTION_BEGIN_AREA_TRANSITION:
        return actorScriptBeginAreaTransition(object);
    case ACTOR_SCRIPT_ACTION_FINISH_AREA_TRANSITION:
        return actorScriptFinishAreaTransition(object);
    case ACTOR_SCRIPT_ACTION_OPEN_TEXT_WINDOW:
        return actorScriptOpenTextWindow(object);
    case ACTOR_SCRIPT_ACTION_TRANSFORM_KIND:
        return actorScriptTransformKind(object);
    default:
        TRACE("ERROR: Unknown nerve routine. Nerve number %d\n",
              ((ScriptVm *)((ActorScriptState *)state)->vmAddress)->actionHandler);
        return SCRIPT_VM_ERROR;
    }
}
