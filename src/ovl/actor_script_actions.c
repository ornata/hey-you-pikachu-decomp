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
/* harvested: matched m2c drafts; pending style pass */
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

/// Compatibility declaration retaining an unused third argument from the original call.

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
    if (*itemFlags & 0x800000) {
        return 0;
    }
    itemTransform(((RuntimeObjectPrefix *)liveObject)->handle, 0);
    actorAdjustActionMeter(actor, 10.0f);
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

/// Wait for the actor action to stop and provide a zero script result when it does.
/// \param actor Actor whose action is awaited.
/// \returns One when the wait completes, or two while the action continues.
/// \note Original decompilation identifier: \c func_801167BC (0x801167BC).
s32 actorScriptWaitForActionResult(u8 *actor) {
    u8 *sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.bytes;
    if (actionScriptIsStopped(actor) != 0) {
        scriptVmPushInteger(((ActorScriptState *)sp1C)->vmAddress, 0);
        return SCRIPT_VM_YIELDED;
    }
    return SCRIPT_VM_WAITING_FOR_ACTION;
}


/// Start the action supplied by the script and wait for it to stop.
/// \param actor Actor whose script supplies the action and receives its completion result.
/// \returns One when complete, or two while waiting.
/// \note Original decompilation identifier: \c func_80116800 (0x80116800).
s32 actorScriptPlayAction(u8 *actor) {
    u8 *vmBytes;
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;

    vmBytes = (u8 *)((ActorScriptState *)actorState)->vmAddress;
    if (((ScriptVm *)vmBytes)->actionInitializing != 0) {
        actionScriptStart(actor, scriptVmPopInteger((s32)vmBytes));
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    return actorScriptWaitForActionResult(actor);
}


/// Wait for an action to stop or interrupt it when a script message is pending.
/// \param actor Actor whose action and script mailbox are checked.
/// \returns One when complete, or two while waiting; completion provides the pending-message flag or zero as the script result.
/// \note Original decompilation identifier: \c func_80116858 (0x80116858).
s32 actorScriptWaitForActionOrMessage(u8 *actor) {
    s32 messagePending;
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    messagePending = ((ActorScriptState *)actorState)->messagePending;
    if (messagePending != 0) {
        scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress, messagePending);
        actionScriptStop(actor);
        return SCRIPT_VM_YIELDED;
    }
    return actorScriptWaitForActionResult(actor);
}


/// Start the script-selected action and wait for completion or a pending message.
/// \param actor Actor whose script supplies the action and receives its completion result.
/// \returns One when complete, or two while waiting; completion supplies the pending-message flag or zero as the script result.
/// \note Original decompilation identifier: \c func_801168A8 (0x801168A8).
s32 actorScriptPlayInterruptibleAction(u8 *actor) {
    u8 *vmBytes;
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;

    vmBytes = (u8 *)((ActorScriptState *)actorState)->vmAddress;
    if (((ScriptVm *)vmBytes)->actionInitializing != 0) {
        actionScriptStart(actor, scriptVmPopInteger((s32)vmBytes));
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    return actorScriptWaitForActionOrMessage(actor);
}

/// Start the script-selected held-item throwing action and wait for it to stop.
/// \param actor Actor whose held item is thrown by the selected action.
/// \returns One when complete or no held item is available, otherwise two; the script result is -1 for a missing item or zero after the action stops.
/// \note Original decompilation identifier: \c func_80116900 (0x80116900).
s32 actorScriptThrowHeldItem(u8 *actor) {
    u8 *state = ((RuntimeObjectPrefix *)actor)->state.bytes;
    s32 anim;

    if (((ScriptVm *)((ActorScriptState *)state)->vmAddress)->actionInitializing != 0) {
        anim = scriptVmPopInteger(((ActorScriptState *)state)->vmAddress);
        TRACE("nerveThrowItem - %d\n", ((ActorScriptState *)state)->heldItemHandle);
        if (((ActorScriptState *)state)->heldItemHandle == 0) {
            scriptVmPushInteger(((ActorScriptState *)state)->vmAddress, -1);
            return SCRIPT_VM_YIELDED;
        }
        actionScriptStart(actor, anim);
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    return actorScriptWaitForActionResult(actor);
}


/// Run the script-selected item acquisition action and report the held item kind.
/// \param actor Actor whose script supplies the acquisition action and settings.
/// \returns One when complete, or two while waiting; completion supplies the held item kind, or zero when absent, as the script result.
/// \note Original decompilation identifier: \c func_80116984 (0x80116984).
s32 actorScriptAcquireItem(u8 *actor) {
    s32 value;
    u8 *vmBytes;
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    vmBytes = (u8 *)((ActorScriptState *)actorState)->vmAddress;
    if (((ScriptVm *)vmBytes)->actionInitializing != 0) {
        ((ActorScriptState *)actorState)->searchItemRange = scriptVmPopFloat((s32)vmBytes);
        ((ActorScriptState *)actorState)->searchItemKind =
            scriptVmPopInteger(((ActorScriptState *)actorState)->vmAddress);
        value = scriptVmPopInteger(((ActorScriptState *)actorState)->vmAddress);
        if (((ActorScriptState *)actorState)->heldItemHandle != 0) {
            scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress, actorGetHeldItemKind(actor));
            return SCRIPT_VM_YIELDED;
        }
        actionScriptStart(actor, value);
        goto block_6;
    }
    if (actionScriptIsStoppedAndLastActionEnded(actor) != 0) {
        scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress, actorGetHeldItemKind(actor));
        return SCRIPT_VM_YIELDED;
    }
block_6:
    return SCRIPT_VM_WAITING_FOR_ACTION;
}


/// Run the script-selected eating action, consume its held item, and handle any remaining item.
/// \param actor Actor whose script supplies the eating action.
/// \returns One when complete or the item is unavailable, otherwise two while waiting.
/// The script result is -1 for an unavailable or ineligible item, one for a player-picked-up item, or zero otherwise.
/// \note Original decompilation identifier: \c func_80116A4C (0x80116A4C).
s32 actorScriptEatHeldItem(u8 *actor) {
    u8 *state;
    u8 *obj;
    u8 *rec;
    s32 anim;

    state = ((RuntimeObjectPrefix *)actor)->state.bytes;
    if (((ScriptVm *)((ActorScriptState *)state)->vmAddress)->actionInitializing != 0) {
        anim = scriptVmPopInteger(((ActorScriptState *)state)->vmAddress);
        TRACE("nerveGrabItem - %d\n", ((ActorScriptState *)state)->heldItemHandle);
        obj = objectFindLiveByHandle(((ActorScriptState *)state)->heldItemHandle);
        if (obj == NULL) {
            scriptVmPushInteger(((ActorScriptState *)state)->vmAddress, -1);
            return SCRIPT_VM_YIELDED;
        }
        rec = ((RuntimeObjectPrefix *)obj)->state.bytes;
        if (((ItemStatePrefix *)rec)->flags & 0x800000) {
            scriptVmPushInteger(((ActorScriptState *)state)->vmAddress, -1);
            return SCRIPT_VM_YIELDED;
        }
        itemEnableShrinking(obj);
        actionScriptStart(actor, anim);
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    if (actionScriptIsStopped(actor) != 0) {
        obj = objectFindLiveByHandle(((ActorScriptState *)state)->heldItemHandle);
        if (obj == NULL) {
            scriptVmPushInteger(((ActorScriptState *)state)->vmAddress, -1);
        } else if (itemHasPlayerPickupHistory(obj) != 0) {
            scriptVmPushInteger(((ActorScriptState *)state)->vmAddress, 1);
        } else {
            scriptVmPushInteger(((ActorScriptState *)state)->vmAddress, 0);
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

typedef struct ActionCost {
    s32 animation;
    s32 insufficientAnimation;
    f32 cost;
    s32 checkCost;
} ActionCost;
/// Action-meter costs and success or insufficient-meter animation selections indexed by script action.
/// \note Original identifier: \c D_8012A660 (0x8012A660).
extern ActionCost gActorScriptCostedActions[];
/// Perform a script-selected action subject to its resource cost and report its outcome.
/// \param object Actor whose script supplies the action and repetition count.
/// \returns One when complete, or two while waiting; completion supplies the stored action outcome as the script result.
/// \note Original decompilation identifier: \c func_80116BC4 (0x80116BC4).
s32 actorScriptPerformCostedAction(u8 *object) {
    f32 cost;
    s32 action;
    s32 count;
    u8 *vm;
    u8 *state;
    state = *(u8 **)(object + 0x54);
    vm = (u8 *)((ActorScriptState *)state)->vmAddress;
    if (((ScriptVm *)vm)->actionInitializing != 0) {
        count = scriptVmPopInteger((s32)vm);
        action = scriptVmPopInteger(((ActorScriptState *)state)->vmAddress);
        cost = gActorScriptCostedActions[action].cost * count;
        if (gActorScriptCostedActions[action].checkCost != 0 && ((ActorScriptState *)state)->actionResource < cost) {
            ((ActorScriptState *)state)->costedActionResult = 0;
            ((ActorScriptState *)state)->targetedBurstAllowance = 0;
            actionScriptStart(object, gActorScriptCostedActions[action].insufficientAnimation);
        } else {
            actorAdjustActionMeter(object, -cost);
            ((ActorScriptState *)state)->costedActionResult = 3;
            ((ActorScriptState *)state)->targetedBurstAllowance = count;
            actionScriptStart(object, gActorScriptCostedActions[action].animation);
        }
    } else if (actionScriptIsStopped(object) != 0) {
        scriptVmPushInteger(((ActorScriptState *)state)->vmAddress, ((ActorScriptState *)state)->costedActionResult);
        return SCRIPT_VM_YIELDED;
    }
    return SCRIPT_VM_WAITING_FOR_ACTION;
}

void playerSelectControlScheme(s32); /* extern */
/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
extern s32 gRuntimeGameMode;

/// Select the script-requested player control scheme and controller assignment when permitted.
/// \param actor Actor whose script supplies a control-scheme identifier from zero through twenty-seven.
/// \returns One after the request and a zero script result, or two while the current game mode delays the request.
/// \note Original decompilation identifier: \c func_80116CF8 (0x80116CF8).
s32 actorScriptSelectControlScheme(u8 *actor) {
    u8 *sp1C;

    if (gRuntimeGameMode == 3) {
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    sp1C = ((RuntimeObjectPrefix *)actor)->state.bytes;
    playerSelectControlScheme(scriptVmPopInteger(((ActorScriptState *)sp1C)->vmAddress));
    scriptVmPushInteger(((ActorScriptState *)sp1C)->vmAddress, 0);
    return SCRIPT_VM_YIELDED;
}

void hudSetAreaLabelVisible(s32);                                 /* extern */
/// Show the area label and begin the area-transition fade when its delay has elapsed.
/// \param actor Actor whose script context controls the transition.
/// \returns Two while this handler or its selected successor waits for the transition.
/// \note Original decompilation identifier: \c func_80116D58 (0x80116D58).
s32 actorScriptBeginAreaTransition(u8 *actor) {
    u8 *vmBytes;
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;

    vmBytes = (u8 *)((ActorScriptState *)actorState)->vmAddress;
    if (((ScriptVm *)vmBytes)->actionInitializing != 0) {
        hudSetAreaLabelVisible(1);
    } else if (((ScriptVm *)vmBytes)->totalElapsed > 1.5f) {
        cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.6f);
        actorBeginScriptActionHandler(actor, ACTOR_SCRIPT_ACTION_FINISH_AREA_TRANSITION);
    }
    return SCRIPT_VM_WAITING_FOR_ACTION;
}

void audioSetMusicDuckingInhibitFlags(s32); /* extern */
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
    if (gPrimarySecondaryOverlayRemainingSeconds > 0.0f) {
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    hudSetAreaLabelVisible(0);
    audioSetMusicDuckingInhibitFlags(0);
    scriptVmPushInteger(((ActorScriptState *)sp1C)->vmAddress, 0);
    return SCRIPT_VM_YIELDED;
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
u8 *particleClusterCreate(s32, f32, f32, f32, u8 *);
u32 randomNext(void);
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
        actorTransitionModelEnvironmentColor(object, 255, 200, 50, 255, 0.6f);
        random = randomNext();
        linked = actorGetDefaultPart(object);
        randomFraction = random * 2.3283064365386963e-10;
        particleClusterCreate(10, 1.7f, 3.0f, randomFraction * 0.2f + 0.5, linked);
        orangePulseEmitterCreate(((RuntimeObjectPrefix *)object)->handle, 2.0f);
        ((ScriptVm *)vm)->actionTimer = 2.0f;
        audioPlayCue(31);
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    ((ScriptVm *)vm)->actionTimer -= gRuntimeFrameSeconds;
    if (((ScriptVm *)vm)->actionTimer > 0.0f)
        return SCRIPT_VM_WAITING_FOR_ACTION;
    linked = actorReplaceKindPreservingScript(object, scriptVmPopInteger((s32)vm));
    actorSetModelEnvironmentColor(linked, 255, 200, 50, 255);
    actorTransitionModelEnvironmentColor(linked, 0, 0, 0, 0, 0.4f);
    audioPlayCue(30);
    scriptVmPushInteger((s32)vm, ((RuntimeObjectPrefix *)linked)->handle);
    return SCRIPT_VM_YIELDED;
}


/// Request a text window using the script-supplied indicator, style, and view mode.
/// \param actor Actor whose script supplies the window settings.
/// \returns One after the request and a zero script result, or two while the game mode delays the request.
/// \note Original decompilation identifier: \c func_80117014 (0x80117014).
s32 actorScriptOpenTextWindow(u8 *actor) {
    s32 sp2C;
    s32 vmAddress;
    s32 sp24;

    if (gRuntimeGameMode == 3) {
        return SCRIPT_VM_WAITING_FOR_ACTION;
    }
    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp2C = scriptVmPopInteger(vmAddress);
    sp24 = scriptVmPopInteger(vmAddress);
    textWindowOpen(scriptVmPopInteger(vmAddress), sp24, sp2C);
    TRACE("messageSetup(%d,%d,%d)\n");
    scriptVmPushInteger(vmAddress, 0);
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
        scriptVmPushInteger(((ActorScriptState *)state)->vmAddress, 0);
        return SCRIPT_VM_YIELDED;
    case ACTOR_SCRIPT_ACTION_POLL_ACTION_STOPPED:
        return actorScriptPollActionStopped(object);
    case ACTOR_SCRIPT_ACTION_PLAY_ACTION:
        return actorScriptPlayAction(object);
    case ACTOR_SCRIPT_ACTION_EXIT_ACTION_LOOP:
        actionRequestLoopExit(object);
        return actorScriptWaitForActionResult(object);
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
