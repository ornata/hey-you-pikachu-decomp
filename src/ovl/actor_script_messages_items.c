#include "object_target_search.h"
#include "actor_held_item.h"
#include "actor_target.h"
#include "progress_fields.h"
#include "action_script_playback.h"
#include "actor_script_action.h"
#include "actor_sound.h"
#include "actor_rotation.h"
/// \file
/// Native script commands for messages, items, and microphone configuration.
/// \ingroup ActorScriptVm
///
/// See docs/VOICE_AND_SCRIPT_VM.md for the subsystem walkthrough.

/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "object_script_query.h"
#include "audio_cue.h"
#include "object_lookup.h"
#include "player_types.h"
#include "object_types.h"
#include "actor_script.h"
#include "script_vm_stack.h"

/// Compatibility declaration retaining the extra actor argument in the mailbox-acknowledgement call.

/// Track the player's primary object as the actor's movement destination.
/// \param actor Actor whose tracking target is selected with zero distance allowance; its script VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Portability: The unprototyped VM push retains an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_8011C800 (0x8011C800).
s32 actorScriptTrackPlayerPrimaryObject(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    actorTrackPlayerPrimaryObject(actor);
    ((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->targetDistanceAllowance = 0.0f;
    scriptVmPushInteger(sp1C, 0);
    return 0;
}



/// Acknowledge the actor's pending script message.
/// \param actor Actor whose pending-message flag is cleared and whose script VM receives its previous value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C84C (0x8011C84C).
s32 actorScriptAcknowledgeMessage(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, actorAcknowledgeScriptMessage(actor));
    return 0;
}

/// Push the actor's pending script-message flag without acknowledging it.
/// \param actor Actor whose script VM receives the current pending-message flag.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C884 (0x8011C884).
/// \note Inspection leaves the notification pending. Acknowledge explicitly
/// when the script has handled the message.
/// \see actorScriptAcknowledgeMessage
/// \see actorScriptPushMessageType
s32 actorScriptPushMessagePending(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress, ((ActorScriptState *)actorState)->messagePending);
    return 0;
}

/// Push the type of the actor's stored script message.
/// \param actor Actor whose script VM receives the stored message type; the pending flag is unchanged.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C8B0 (0x8011C8B0).
s32 actorScriptPushMessageType(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress, ((ActorScriptState *)actorState)->message.type);
    return 0;
}

/// Push argument 0 of the actor's stored script message.
/// \param actor Actor whose script VM receives the stored message argument; the pending flag is unchanged.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C8DC (0x8011C8DC).
s32 actorScriptPushMessageArgument0(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress, ((ActorScriptState *)actorState)->message.arg0);
    return 0;
}

/// Push argument 2 of the actor's stored script message.
/// \param actor Actor whose script VM receives the stored message argument; the pending flag is unchanged.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C908 (0x8011C908).
s32 actorScriptPushMessageArgument2(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress, ((ActorScriptState *)actorState)->message.arg2);
    return 0;
}

/// Push argument 3 of the actor's stored script message.
/// \param actor Actor whose script VM receives the stored message argument; the pending flag is unchanged.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C934 (0x8011C934).
s32 actorScriptPushMessageArgument3(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress, ((ActorScriptState *)actorState)->message.arg3);
    return 0;
}

/// Push argument 4 of the actor's stored script message.
/// \param actor Actor whose script VM receives the stored message argument; the pending flag is unchanged.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011C960 (0x8011C960).
s32 actorScriptPushMessageArgument4(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress, ((ActorScriptState *)actorState)->message.arg4);
    return 0;
}


u8 *attachedEffectCreate(u8 *, s32, s32); /* extern */
u32 randomNext(void);               /* extern */
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern u8 *gPikachuActor;

/// Create an animated effect attached to the actor and record related Pikachu reaction progress.
/// \param actor Actor whose script VM supplies a valid effect-definition index followed by a valid animation-track index and receives zero; its previous attached effect is replaced.
/// \returns Zero, allowing script execution to continue.
///
/// \note Portability: The unused local preserves the stack-frame size required by the ROM.
/// \note Original decompilation identifier: \c func_8011C98C (0x8011C98C).
s32 actorScriptCreateAttachedEffect(u8 *actor) {
    s32 vm;
    s32 pad;
    s32 kind;
    s32 value;

    vm = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    kind = scriptVmPopInteger(vm);
    value = scriptVmPopInteger(vm);
    attachedEffectCreate(actor, value, kind);
    if ((actor == gPikachuActor) && ((kind == 0xA6) || (kind == 0xA7) || (kind == 0xAF) || (kind == 0xB3))) {
        if (0.5 < randomNext() * 2.3283064365386963e-10) {
            progressAddValue(0x1A, 1);
        }
    }
    scriptVmPushInteger(vm, 0);
    return 0;
}


/// Find the nearest actor in the current actor's facing cone.
/// \param actor Actor whose script VM supplies minimum kind, maximum kind, exclusive distance limit, then cone angle; kind 38 selects the creature range and angles of 360 or greater allow all directions. Receives the nearest qualifying handle, or zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CA80 (0x8011CA80).
s32 actorScriptFindNearestInFacingCone(u8 *actor) {
    s32 sp_pop;
    s32 vmAddress;
    s32 sp34;
    f32 sp30;
    f32 sp2C;

    vmAddress = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    sp2C = scriptVmPopFloat(vmAddress);
    sp30 = scriptVmPopFloat(vmAddress);
    sp34 = scriptVmPopInteger(vmAddress);
    sp_pop = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, actorFindNearestHandleInFacingCone(actor, sp_pop, sp34, sp30, sp2C));
    return 0;
}


/// Force release of the actor's held item and report its kind.
/// \param actor Actor whose held handle identifies an initialized item when live; its script VM receives the resolved kind or zero when the handle is not live.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CB00 (0x8011CB00).
s32 actorScriptReleaseHeldItem(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, actorReleaseHeldItem(actor));
    return 0;
}

f32 objectGetPlayerDistance(u8 *);         /* extern */

/// Push the actor's approximate distance from the player.
/// \param actor Actor whose script VM receives the approximate distance from its local position to the player position.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CB38 (0x8011CB38).
s32 actorScriptPushPlayerDistance(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, objectGetPlayerDistance(actor));
    return 0;
}

f32 vector3LengthApprox(f32 *); /* extern */

/// Replace a world position on the script stack with its distance from the actor.
/// \param actor Actor whose script VM supplies world X, Y, Z (Z on top) and receives the approximate three-dimensional distance.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CB70 (0x8011CB70).
s32 actorScriptGetPointDistance(u8 *actor) {
    s32 vm = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    f32 d[3];

    d[2] = scriptVmPopFloat(vm) - ((RuntimeObjectPrefix *)actor)->worldPosition[2];
    d[1] = scriptVmPopFloat(vm) - ((RuntimeObjectPrefix *)actor)->worldPosition[1];
    d[0] = scriptVmPopFloat(vm) - ((RuntimeObjectPrefix *)actor)->worldPosition[0];
    scriptVmPushFloat(vm, vector3LengthApprox(d));
    return 0;
}

/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern PlayerControlPrefix gPlayerState;
/// Replace an object handle on the script stack with its distance from the player.
/// \param actor Actor whose script VM supplies a handle and receives the approximate three-dimensional distance, or -1 when the object is unavailable.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CBF8 (0x8011CBF8).
s32 actorScriptGetObjectPlayerDistance(u8 *actor) {
    s32 vm;
    f32 d[3];
    u8 *obj;

    vm = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    obj = objectResolveAvailableHandle(scriptVmPopInteger(vm));
    if (obj == NULL) {
        scriptVmPushFloat(vm, -1.0f);
        return 0;
    }
    d[2] = ((PlayerControlPrefix *)&gPlayerState)->position[0] - ((RuntimeObjectPrefix *)obj)->worldPosition[0];
    d[1] = ((PlayerControlPrefix *)&gPlayerState)->position[1] - ((RuntimeObjectPrefix *)obj)->worldPosition[1];
    d[0] = ((PlayerControlPrefix *)&gPlayerState)->position[2] - ((RuntimeObjectPrefix *)obj)->worldPosition[2];
    scriptVmPushFloat(vm, vector3LengthApprox(d));
    return 0;
}


/// Track the player's secondary object as the actor's escape source.
/// \param actor Actor whose escape tracking is selected; its script VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CC94 (0x8011CC94).
s32 actorScriptTrackEscapeFromPlayerSecondaryObject(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    actorTrackEscapeFromPlayerSecondaryObject(actor);
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

void actorChooseEscapePositionFromPlayer(u8 *);

/// Choose a fixed escape destination away from the player.
/// \param actor Actor whose escape destination is selected and ongoing tracking disabled; requires a live fallback target when the positions coincide. Its script VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CCCC (0x8011CCCC).
s32 actorScriptChooseEscapeFromPlayer(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    actorChooseEscapePositionFromPlayer(actor);
    scriptVmPushInteger(sp1C, 0);
    return 0;
}


/// Place the actor at its stored target position.
/// \param actor Actor to reposition; its script VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CD04 (0x8011CD04).
s32 actorScriptMoveToTargetPosition(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    actorMoveToTargetPosition(actor);
    scriptVmPushInteger(sp1C, 0);
    return 0;
}


/// Set one axis of the actor's rotation from script arguments.
/// \param actor Actor whose script VM supplies a finite angle in degrees followed by axis zero (pitch), one (yaw), or two (roll), and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CD3C (0x8011CD3C).
s32 actorScriptSetRotationAxis(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    actorSetRotationAxisDegrees(actor, scriptVmPopFloat(vmAddress), sp1C);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}

/// Begin a scripted change of player control scheme and controller assignment.
/// \param actor Actor address whose script VM supplies a control-scheme identifier from zero through twenty-seven and later receives zero.
/// \returns Two, transferring script execution to the action handler.
/// \note Original decompilation identifier: \c func_8011CD94 (0x8011CD94).
s32 actorScriptStartControlSchemeChange(s32 actor) {
    actorBeginScriptActionHandler((u8 *)actor, ACTOR_SCRIPT_ACTION_SELECT_CONTROL_SCHEME);
    return 2;
}

/// Begin the scripted area transition.
/// \param actor Actor whose script action will perform the area transition.
/// \returns Two, transferring script execution to the action handler.
/// \note Original decompilation identifier: \c func_8011CDB8 (0x8011CDB8).
s32 actorScriptStartAreaTransition(s32 actor) {
    actorBeginScriptActionHandler((u8 *)actor, ACTOR_SCRIPT_ACTION_BEGIN_AREA_TRANSITION);
    return 2;
}

void scriptVmSetPrimaryCameraView(s32); /* extern */

/// Set the primary camera position and look-at target from script arguments.
/// \param actor Actor whose script VM supplies position X, Y, Z followed by target X, Y, Z (target Z on top), and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CDDC (0x8011CDDC).
s32 actorScriptSetCameraView(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmSetPrimaryCameraView(sp1C);
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

void playerSaveState(); /* extern */
/// Save the current player state for later restoration.
/// \param actor Actor whose script VM receives zero after the shared player snapshot is replaced.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CE14 (0x8011CE14).
s32 actorScriptSavePlayerState(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    playerSaveState();
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

void playerRestoreState(); /* extern */
/// Restore the saved player state and refresh the player view.
/// \param actor Actor whose script VM receives zero; a player snapshot must have been saved.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CE4C (0x8011CE4C).
s32 actorScriptRestorePlayerState(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    playerRestoreState();
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

void playerSetWorldPosition(u8 *); /* extern */

/// Move the player to a script-specified world position.
/// \param actor Actor whose script VM supplies world X, Y, Z (Z on top) and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CE84 (0x8011CE84).
s32 actorScriptSetPlayerPosition(u8 *actor) {
    s32 vm = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    f32 pos[3];

    pos[2] = scriptVmPopFloat(vm);
    pos[1] = scriptVmPopFloat(vm);
    pos[0] = scriptVmPopFloat(vm);
    playerSetWorldPosition((u8 *)pos);
    scriptVmPushInteger(vm, 0);
    return 0;
}

void playerSetViewYaw(f32); /* extern */

/// Request a player heading angle, subject to player view-control permissions.
/// \param actor Actor whose script VM supplies a yaw angle in degrees and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CEE4 (0x8011CEE4).
s32 actorScriptSetPlayerYaw(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    playerSetViewYaw(scriptVmPopFloat(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}


/// Push the player's current view yaw.
/// \param actor Actor whose script VM receives the view yaw in degrees.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CF24 (0x8011CF24).
s32 actorScriptPushPlayerViewYaw(u8 *actor) {
    scriptVmPushFloat(((ActorScriptObject *)actor)->scriptState->vmAddress,
                      ((PlayerControlPrefix *)&gPlayerState)->viewShake + ((PlayerControlPrefix *)&gPlayerState)->yaw);
    return 0;
}

void playerSetViewPitch(f32); /* extern */

/// Request a player view pitch, subject to player view-control permissions.
/// \param actor Actor whose script VM supplies a pitch angle in degrees, limited to -60 through 60 when applied, and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CF64 (0x8011CF64).
s32 actorScriptSetPlayerPitch(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    playerSetViewPitch(scriptVmPopFloat(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}


/// Push the player's current view pitch.
/// \param actor Actor whose script VM receives the view pitch in degrees.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CFA4 (0x8011CFA4).
s32 actorScriptPushPlayerViewPitch(u8 *actor) {
    scriptVmPushFloat(((ActorScriptObject *)actor)->scriptState->vmAddress,
                      ((PlayerControlPrefix *)&gPlayerState)->pitch +
                          ((PlayerControlPrefix *)&gPlayerState)->pitchOffset);
    return 0;
}


/// Push the kind of item held by the actor.
/// \param actor Actor whose held handle identifies an initialized item when live; its script VM receives the item kind or zero when the handle is not live.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011CFE4 (0x8011CFE4).
s32 actorScriptPushHeldItemKind(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, actorGetHeldItemKind(actor));
    return 0;
}

/// Separate transformation-cause-three response view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125A5A (0x80125A5A).
extern u8 gItemKindThirdResponseEntries[]; /* item records (0x74 bytes); type-three response */

/// Test whether an item kind has a qualifying type-three response.
/// \param actor Actor whose script VM supplies a valid item kind and receives one for response codes 2, 4, 5, or 6, or zero otherwise.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D01C (0x8011D01C).
s32 actorScriptTestItemResponseTypeThree(u8 *actor) {
    s32 vm;
    s32 kind;
    s32 result;

    vm = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    result = 0;
    kind = gItemKindThirdResponseEntries[scriptVmPopInteger(vm) * 0x74];
    if ((kind == 2) || (kind == 4) || (kind == 5) || (kind == 6)) {
        result = 1;
    }
    scriptVmPushInteger(vm, result);
    return 0;
}

/// Recognition command status sampled by the most recent gameplay voice update.
/// \note Original identifier: \c D_800722DC (0x800722DC).
extern s32 gVoiceCommandLastRecognitionStatus;

/// Read the current voice-command recognition status.
/// \param actor Actor whose script VM receives the voice-command status.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D0A4 (0x8011D0A4).
s32 actorScriptPushVoiceCommandStatus(u8 *actor) {
    scriptVmPushInteger(((ActorScriptObject *)actor)->scriptState->vmAddress, gVoiceCommandLastRecognitionStatus);
    return 0;
}




/// Post a script message to another actor without replacing a pending message.
/// \param actor Actor whose script VM supplies a handle identifying initialized actor state when live, message type, and arguments 0, 2, 3, 4 (argument 4 on top); argument 1 is zero. The VM receives one if accepted, or zero for a pending message or missing target.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D0D4 (0x8011D0D4).
s32 actorScriptPostMessage(u8 *actor) {
    s32 vm;
    u8 *obj;
    ScriptMessage msg;
    s32 result;

    vm = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    msg.arg1 = 0;
    msg.arg4 = scriptVmPopInteger(vm);
    msg.arg3 = scriptVmPopInteger(vm);
    msg.arg2 = scriptVmPopInteger(vm);
    msg.arg0 = scriptVmPopInteger(vm);
    msg.type = scriptVmPopInteger(vm);
    obj = objectFindLiveByHandle(scriptVmPopInteger(vm));
    if (obj != NULL) {
        result = actorPostScriptMessage((s32)obj, msg, 0);
    } else {
        result = 0;
    }
    scriptVmPushInteger(vm, result);
    return 0;
}

s32 actorFindInteractableItemKind(u8 *, s32, f32); /* extern */

/// Search for an eligible nearby item of a script-specified kind.
/// \param actor Actor whose script VM supplies item kind followed by maximum distance and receives the qualifying kind, or zero during cooldown or when none qualifies.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D1B0 (0x8011D1B0).
s32 actorScriptFindInteractableItem(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    f32 sp1C;

    vmAddress = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopFloat(vmAddress);
    scriptVmPushInteger(vmAddress, actorFindInteractableItemKind(actor, scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

f32 objectGetOriginDistance(u8 *);         /* extern */

/// Push the actor's approximate distance from the world origin.
/// \param actor Actor whose script VM receives the three-dimensional distance.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D208 (0x8011D208).
s32 actorScriptPushOriginDistance(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, objectGetOriginDistance(actor));
    return 0;
}

f32 objectGetDistanceToHandle(u8 *, s32);    /* extern */

/// Replace a target handle on the script stack with its approximate distance from the actor.
/// \param actor Actor whose script VM supplies a target handle and receives the distance, or -1 for an unavailable target.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D240 (0x8011D240).
s32 actorScriptGetObjectDistance(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, objectGetDistanceToHandle(actor, scriptVmPopInteger(sp1C)));
    return 0;
}

f32 objectGetHeightDifferenceToHandle(u8 *, s32);    /* extern */

/// Replace a target handle on the script stack with its height relative to the actor.
/// \param actor Actor whose script VM supplies a target handle and receives target height minus actor height, or zero for an unavailable target.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D288 (0x8011D288).
s32 actorScriptGetObjectHeightDifference(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, objectGetHeightDifferenceToHandle(actor, scriptVmPopInteger(sp1C)));
    return 0;
}


/// Push the stored target position's height relative to the actor.
/// \param actor Actor whose script VM receives target height minus actor height.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D2D0 (0x8011D2D0).
s32 actorScriptPushTargetHeightDifference(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    scriptVmPushFloat(((ActorScriptState *)actorState)->vmAddress,
                      ((ActorScriptState *)actorState)->targetPosition[1] -
                          ((RuntimeObjectPrefix *)actor)->worldPosition[1]);
    return 0;
}

f32 objectGetHorizontalDistanceToHandle(u8 *, s32);    /* extern */

/// Replace a target handle on the script stack with its horizontal distance from the actor.
/// \param actor Actor whose script VM supplies a target handle and receives the approximate horizontal distance, or zero for an unavailable target.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D310 (0x8011D310).
s32 actorScriptGetObjectHorizontalDistance(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, objectGetHorizontalDistanceToHandle(actor, scriptVmPopInteger(sp1C)));
    return 0;
}

f32 actorGetHorizontalTargetDistance(u8 *);         /* extern */

/// Push the actor's approximate horizontal distance to its stored target position.
/// \param actor Actor whose script VM receives the target distance.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D358 (0x8011D358).
s32 actorScriptPushHorizontalTargetDistance(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, actorGetHorizontalTargetDistance(actor));
    return 0;
}

f32 actorGetTargetCoordinate(u8 *, s32);    /* extern */

/// Replace a coordinate axis on the script stack with a stored target coordinate.
/// \param actor Actor whose script VM supplies a nonnegative axis (zero for X, one for Y, two for Z) and receives the target coordinate, or -65536 for an axis greater than two.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D390 (0x8011D390).
s32 actorScriptGetTargetCoordinate(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, actorGetTargetCoordinate(actor, scriptVmPopInteger(sp1C)));
    return 0;
}

f32 objectGetWorldCoordinate(u8 *, s32);    /* extern */

/// Replace a coordinate axis on the script stack with an actor world coordinate.
/// \param actor Actor whose script VM supplies a nonnegative axis (zero for X, one for Y, two for Z) and receives the world coordinate, or -65536 for an axis greater than two.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D3D8 (0x8011D3D8).
s32 actorScriptGetWorldCoordinate(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, objectGetWorldCoordinate(actor, scriptVmPopInteger(sp1C)));
    return 0;
}


/// Replace a rotation axis on the script stack with the actor's rotation angle.
/// \param actor Actor whose script VM supplies a valid rotation axis (zero, one, or two) and receives its angle in degrees.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D420 (0x8011D420).
s32 actorScriptGetRotationCoordinate(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, ((f32 *)(actor + 0xC))[scriptVmPopInteger(sp1C)]);
    return 0;
}

f32 actionScriptGetElapsedTime(u8 *);         /* extern */

/// Push the elapsed time of the actor's current action script.
/// \param actor Actor whose script VM receives elapsed action time in seconds.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D468 (0x8011D468).
s32 actorScriptPushActionElapsedTime(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, actionScriptGetElapsedTime(actor));
    return 0;
}

f32 actionScriptGetDuration(u8 *);         /* extern */

/// Push the time limit of the actor's current action script.
/// \param actor Actor whose script VM receives the action time limit in seconds; a negative value indicates no limit.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D4A0 (0x8011D4A0).
s32 actorScriptPushActionDuration(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, actionScriptGetDuration(actor));
    return 0;
}

s32 actionScriptGetId(u8 *); /* extern */

/// Push the identifier of the actor's current action script.
/// \param actor Actor whose script VM receives the action script identifier, or -1 when stopped.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D4D8 (0x8011D4D8).
s32 actorScriptPushActionId(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, actionScriptGetId(actor));
    return 0;
}

u16 actorGetMovementCollisionFlags(u8 *); /* extern */

/// Push the actor's latest movement collision status.
/// \param actor Actor whose script VM receives the collision flags, or zero when no collision was reported.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D510 (0x8011D510).
s32 actorScriptPushMovementCollisionFlags(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, actorGetMovementCollisionFlags(actor));
    return 0;
}

s32 actorTrackObjectPosition(u8 *arg0, s32 arg1, f32 arg2); /* extern */
/// Separate symbol view of the primary player record's mode and camera-transition state.
/// \note Original identifier: \c D_800EF728 (0x800EF728).
extern s32 gPlayerModeState;

/// Track the player's secondary object as the actor's movement destination.
/// \param actor Actor whose tracking destination is changed with zero distance allowance; its script VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D548 (0x8011D548).
s32 actorScriptTrackPlayerSecondaryObject(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    actorTrackObjectPosition(actor, ((PlayerModeState *)&gPlayerModeState)->objectHandles[1], 0.0f);
    scriptVmPushInteger(sp1C, 0);
    return 0;
}


/// Track the player's tertiary object as the actor's movement destination.
/// \param actor Actor whose tracking destination is changed with zero distance allowance; its script VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D58C (0x8011D58C).
s32 actorScriptTrackPlayerTertiaryObject(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    actorTrackObjectPosition(actor, ((PlayerModeState *)&gPlayerModeState)->objectHandles[2], 0.0f);
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

void micRequestDictionary(s32); /* extern */

/// Request a speech recognition dictionary and yield script execution.
/// \param actor Actor whose script VM supplies the dictionary identifier and receives zero.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011D5D0 (0x8011D5D0).
/// \note Requests a vocabulary change; registration can complete on later updates.
/// \see micRequestDictionary
/// \see micSessionUpdateDictionary
s32 actorScriptRequestMicDictionary(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    micRequestDictionary(scriptVmPopInteger(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 1;
}

s32 micQueryRequestedDictionaryNoResult();     /* extern */
/// Query the requested voice dictionary.
/// \param actor Actor whose script VM receives an unspecified legacy result from the dictionary query.
/// \returns Zero, allowing script execution to continue.
/// \note Portability/UB: The helper is defined as void but declared as returning an integer here; its consumed result is unspecified.
/// \note Original decompilation identifier: \c func_8011D610 (0x8011D610).
s32 actorScriptQueryRequestedMicDictionary(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, micQueryRequestedDictionaryNoResult());
    return 0;
}


/// Change the current audio cue when needed while preserving its requested playback rate.
/// \param actor Actor whose script VM supplies a valid cue ID or a negative value for silence, nonnegative fade-out seconds, then nonnegative fade-in seconds; receives one when a change is requested or zero when the cue is already selected.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D648 (0x8011D648).
s32 actorScriptChangeCuePreservingRate(u8 *actor) {
    s32 handle;
    f64 a;
    f64 b;
    s32 cur;
    s32 v;
    f64 cur_t;

    handle = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    audioGetCurrentCueState(&cur, &cur_t);
    a = scriptVmPopFloat(handle);
    b = scriptVmPopFloat(handle);
    v = scriptVmPopInteger(handle);
    if (v != cur) {
        audioChangeCurrentCue(v, a, b);
        audioSetCurrentCueRate(cur_t);
        scriptVmPushInteger(handle, 1);
    } else {
        scriptVmPushInteger(handle, 0);
    }
    return 0;
}


/// Transition the current audio cue using script-specified fade durations.
/// \param actor Actor whose script VM supplies a valid cue ID or a negative value for silence, nonnegative fade-out seconds, then nonnegative fade-in seconds; a negative cue stops playback. Receives the new playback identifier, or zero for silence or unavailable capacity.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D6F0 (0x8011D6F0).
s32 actorScriptChangeCue(u8 *actor) {
    s32 handle;
    f64 a;
    f64 b;

    handle = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    a = scriptVmPopFloat(handle);
    b = scriptVmPopFloat(handle);
    scriptVmPushInteger(handle, audioChangeCurrentCue(scriptVmPopInteger(handle), a, b));
    return 0;
}


/// Set the requested playback rate of the current audio cue.
/// \param actor Actor whose script VM supplies a positive playback rate multiplier and receives one if applied or zero if no playback was updated.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D768 (0x8011D768).
s32 actorScriptSetCueRate(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, audioSetCurrentCueRate((f64)scriptVmPopFloat(sp1C)));
    return 0;
}


/// Start an audio cue at full volume.
/// \param actor Actor whose script VM supplies a valid cue ID in the initialized current bank and receives the playback identifier, or zero if no slot is available.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D7A8 (0x8011D7A8).
s32 actorScriptPlayCue(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, audioPlayCue(scriptVmPopInteger(sp1C)));
    return 0;
}


/// Play a spatial sound for the actor and track its playback.
/// \param actor Actor whose script VM supplies an actor sound kind and receives the playback handle, or zero if playback could not start.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D7E8 (0x8011D7E8).
s32 actorScriptPlayTrackedSound(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, actorPlayTrackedSound(actor, scriptVmPopInteger(sp1C)));
    return 0;
}

s32 audioPlaySoundEffect(s32);  /* extern */

/// Start a sound effect from the loaded sound bank.
/// \param actor Actor whose script VM supplies a valid index in the initialized sound bank and receives the playback identifier, or zero if no slot is available.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D830 (0x8011D830).
s32 actorScriptPlaySoundEffect(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, audioPlaySoundEffect(scriptVmPopInteger(sp1C)));
    return 0;
}

s32 audioSetSoundEffectPan(s32, s32); /* extern */

/// Adjust the stereo position of a playing sound effect.
/// \param actor Actor whose script VM supplies a playback identifier followed by a pan value from 0 (left) to 255 (right), and receives an unspecified legacy result.
/// \returns Zero, allowing script execution to continue.
/// \note Portability/UB: The helper is defined as void but declared as returning an integer here; its consumed result is unspecified.
/// \note Original decompilation identifier: \c func_8011D870 (0x8011D870).
s32 actorScriptSetSoundEffectPan(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, audioSetSoundEffectPan(scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

s32 audioSetSoundEffectVolume(s32, s32); /* extern */

/// Adjust the volume of a playing sound effect.
/// \param actor Actor whose script VM supplies a playback identifier followed by a volume in units of 1/128 of the original volume, and receives an unspecified legacy result.
/// \returns Zero, allowing script execution to continue.
/// \note Portability/UB: The helper is defined as void but declared as returning an integer here; its consumed result is unspecified.
/// \note Original decompilation identifier: \c func_8011D8C0 (0x8011D8C0).
s32 actorScriptSetSoundEffectVolume(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, audioSetSoundEffectVolume(scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

f64 sqrt(f64);
#pragma intrinsic(sqrt)

/// Replace a numeric script argument with its square root.
/// \param actor Actor whose script VM supplies a nonnegative floating-point value and receives its square root.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D910 (0x8011D910).
s32 actorScriptSquareRoot(u8 *actor) {
    s32 vm = ((ActorScriptObject *)actor)->scriptState->vmAddress;

    scriptVmPushFloat(vm, sqrt(scriptVmPopFloat(vm)));
    return 0;
}

s32 actorFindItemHandleInFacingCone(u8 *, f32, f32, s32, s32, s32, s32); /* extern */

/// Find an item near the actor within its facing cone and the requested filters.
/// \param actor Actor whose script VM supplies minimum kind, maximum kind, distance, cone angle, item state, and weighted preference (preference on top). Kind 165 removes its bound, state six accepts any state, and nonzero preference enables priority weighting. Receives the selected item handle, or zero when none qualifies.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011D958 (0x8011D958).
s32 actorScriptFindItemInFacingCone(u8 *actor) {
    s32 sp_pop;
    s32 vmAddress;
    s32 sp44;
    f32 sp40;
    f32 sp3C;
    s32 sp38;
    s32 sp34;

    vmAddress = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    sp34 = scriptVmPopInteger(vmAddress);
    sp38 = scriptVmPopInteger(vmAddress);
    sp3C = scriptVmPopFloat(vmAddress);
    sp40 = scriptVmPopFloat(vmAddress);
    sp44 = scriptVmPopInteger(vmAddress);
    sp_pop = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, actorFindItemHandleInFacingCone(actor, sp40, sp3C, sp_pop, sp44, sp38, sp34));
    return 0;
}

/// Set a fixed movement destination and disable ongoing target tracking.
/// \param actor Actor whose script VM supplies world X, Y, Z and a distance allowance (allowance on top), and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DA00 (0x8011DA00).
s32 actorScriptSetDestination(u8 *actor) {
    f32 pos[3];
    f32 speed;
    s32 vm;

    vm = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    speed = scriptVmPopFloat(vm);
    pos[2] = scriptVmPopFloat(vm);
    pos[1] = scriptVmPopFloat(vm);
    pos[0] = scriptVmPopFloat(vm);
    actorSetTargetPosition(actor, pos);
    ((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->targetMode = 0;
    ((ActorScriptState *)((RuntimeObjectPrefix *)actor)->state.bytes)->targetDistanceAllowance = speed;
    scriptVmPushInteger(vm, 0);
    return 0;
}

/// Select a moving object as the actor's tracked destination.
/// \param actor Actor whose script VM supplies a target handle followed by a distance allowance, and receives the target script category or zero for an unavailable or unsupported target.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DA8C (0x8011DA8C).
s32 actorScriptTrackObject(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    f32 sp1C;

    vmAddress = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopFloat(vmAddress);
    scriptVmPushInteger(vmAddress, actorTrackObjectPosition(actor, scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

s32 actorSetDestinationFromObject(u8 *, s32, f32); /* extern */

/// Choose an object's current position as a fixed destination.
/// \param actor Actor whose script VM supplies a target handle followed by a distance allowance, and receives the target script category or zero for an unavailable or unsupported target.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DAE4 (0x8011DAE4).
s32 actorScriptSetDestinationFromObject(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    f32 sp1C;

    vmAddress = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopFloat(vmAddress);
    scriptVmPushInteger(vmAddress, actorSetDestinationFromObject(actor, scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

s32 actorTrackEscapeFromObject(u8 *, s32); /* extern */

/// Select an object as the actor's tracked escape source.
/// \param actor Actor whose script VM supplies a target handle and receives its script category, or zero for an unavailable or unsupported target.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DB3C (0x8011DB3C).
s32 actorScriptTrackEscapeFromObject(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, actorTrackEscapeFromObject(actor, scriptVmPopInteger(sp1C)));
    return 0;
}

s32 actorChooseEscapePositionFromHandle(u8 *, s32); /* extern */

/// Choose a fixed escape destination away from an object.
/// \param actor Actor whose script VM supplies a target handle and receives its script category, or zero for an unavailable or unsupported target.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DB84 (0x8011DB84).
s32 actorScriptChooseEscapeFromObject(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, actorChooseEscapePositionFromHandle(actor, scriptVmPopInteger(sp1C)));
    return 0;
}


/// Push the actor's target distance allowance.
/// \param actor Actor whose script VM receives the distance allowance used when approaching its destination.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DBCC (0x8011DBCC).
s32 actorScriptPushTargetDistanceAllowance(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    scriptVmPushFloat(((ActorScriptState *)actorState)->vmAddress,
                      ((ActorScriptState *)actorState)->targetDistanceAllowance);
    return 0;
}

s32 actorGetAvailableTargetHandle(u8 *); /* extern */

/// Push the handle of the actor's available target.
/// \param actor Actor whose script VM receives its target handle, or zero when unavailable.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DBF8 (0x8011DBF8).
s32 actorScriptPushTargetHandle(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, actorGetAvailableTargetHandle(actor));
    return 0;
}

s32 actorGetTargetTrackingMode(u8 *); /* extern */

/// Push the actor's current target tracking mode.
/// \param actor Actor whose script VM receives its target tracking mode.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DC30 (0x8011DC30).
s32 actorScriptPushTrackingMode(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, actorGetTargetTrackingMode(actor));
    return 0;
}

void actorSetTargetTrackingMode(u8 *, s32); /* extern */

/// Set the actor's target tracking mode from a script argument.
/// \param actor Actor whose script VM supplies a mode (zero disables tracking, one follows, two escapes) and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DC68 (0x8011DC68).
s32 actorScriptSetTrackingMode(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    actorSetTargetTrackingMode(actor, scriptVmPopInteger(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}


/// Replace an object handle on the script stack with its script category.
/// \param actor Actor whose script VM supplies a handle and receives its category, or zero for an unavailable or unsupported object.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DCB0 (0x8011DCB0).
s32 actorScriptGetObjectCategory(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, objectGetScriptCategory(actor, scriptVmPopInteger(sp1C)));
    return 0;
}


/// Replace an object handle on the script stack with its script kind identifier.
/// \param actor Actor whose script VM supplies a handle and receives its kind identifier, or zero for an unavailable or unsupported object.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DCF8 (0x8011DCF8).
s32 actorScriptGetObjectKind(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, objectGetScriptKind(scriptVmPopInteger(sp1C)));
    return 0;
}


/// Replace an object handle on the script stack with its item kind.
/// \param actor Actor whose script VM supplies a handle and receives the item kind, or zero for an unavailable handle or a non-item object.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DD38 (0x8011DD38).
s32 actorScriptGetItemKind(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, itemGetKindByHandle(scriptVmPopInteger(sp1C)));
    return 0;
}


/// Replace an object handle on the script stack with its creature kind.
/// \param actor Actor whose script VM supplies a handle and receives the creature kind, or zero for an unavailable handle or a non-creature object.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DD78 (0x8011DD78).
s32 actorScriptGetCreatureKind(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, actorGetKindByHandle(scriptVmPopInteger(sp1C)));
    return 0;
}


/// Replace an object handle on the script stack with its object type.
/// \param actor Actor whose script VM supplies a handle and receives the object type, or zero for an unavailable handle.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DDB8 (0x8011DDB8).
s32 actorScriptGetObjectType(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, objectGetTypeByHandle(scriptVmPopInteger(sp1C)));
    return 0;
}

f32 objectGetBoundingRadiusByHandle(s32);          /* extern */

/// Replace an object handle on the script stack with its bounding radius.
/// \param actor Actor whose script VM supplies a handle and receives the radius, or zero for an unavailable handle.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DDF8 (0x8011DDF8).
s32 actorScriptGetBoundingRadius(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, objectGetBoundingRadiusByHandle(scriptVmPopInteger(sp1C)));
    return 0;
}

void actorSetDefaultActionMode(u8 *, s32); /* extern */

/// Set the default mode for actions that inherit the actor's mode.
/// \param actor Actor whose script VM supplies a default action mode representable as a signed 16-bit value and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DE38 (0x8011DE38).
s32 actorScriptSetDefaultActionMode(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    actorSetDefaultActionMode(actor, scriptVmPopInteger(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

/// Push the actor's default action mode.
/// \param actor Actor whose script VM receives the default action mode.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DE80 (0x8011DE80).
s32 actorScriptPushDefaultActionMode(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress,
                        ((ActorScriptState *)actorState)->defaultActionMode);
    return 0;
}


/// Start an action script with a default mode and yield actor script execution.
/// \param actor Actor whose script VM supplies a valid action-script identifier followed by a default mode representable as a signed 16-bit value, and receives zero.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011DEAC (0x8011DEAC).
s32 actorScriptStartActionWithMode(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    f32 sp1C;

    vmAddress = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    sp24 = vmAddress;
    sp1C = (f32)scriptVmPopInteger(vmAddress);
    actionScriptStartWithMode(actor, scriptVmPopInteger(vmAddress), (s32)sp1C);
    scriptVmPushInteger(vmAddress, 0);
    return 1;
}


/// Start an action script with a time limit and yield actor script execution.
/// \param actor Actor whose script VM supplies a valid action-script identifier followed by a floating-point duration in seconds, and receives zero; a negative duration removes the limit.
/// \returns One, yielding script execution.
/// \note Portability/UB: The duration helper is defined with an integer bit-pattern parameter but declared with a float here, relying on the original ABI.
/// \note Original decompilation identifier: \c func_8011DF18 (0x8011DF18).
s32 actorScriptStartTimedAction(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    f32 sp1C;

    vmAddress = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopFloat(vmAddress);
    actionScriptStartWithDurationBits(actor, scriptVmPopInteger(vmAddress), sp1C);
    scriptVmPushInteger(vmAddress, 0);
    return 1;
}

s32 playerGetHeldItemHandle();     /* extern */

/// Push the handle of the item held by the player.
/// \param actor Actor whose script VM receives the stored player held-item handle, or zero when none is recorded.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DF70 (0x8011DF70).
s32 actorScriptPushPlayerHeldItem(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, playerGetHeldItemHandle());
    return 0;
}

/// Separate symbol view of the primary player friendship-title level.
/// \note Original identifier: \c D_800EF796 (0x800EF796).
extern u8 gPlayerFriendshipLevel;

/// Read the current friendship level with Pikachu.
/// \param actor Actor whose script VM receives the friendship level used for the friendship title.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DFA8 (0x8011DFA8).
s32 actorScriptPushFriendshipLevel(u8 *actor) {
    scriptVmPushInteger(((ActorScriptObject *)actor)->scriptState->vmAddress, gPlayerFriendshipLevel);
    return 0;
}

/// Player status voice count, incremented when a messenger delivers to Pikachu.
/// \note Original identifier: \c D_800EF79C (0x800EF79C).
extern s32 gPlayerVoiceMessageCount;

/// Read the number of voice messages delivered to Pikachu.
/// \param actor Actor whose script VM receives the delivered voice-message count.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011DFD8 (0x8011DFD8).
s32 actorScriptPushDeliveredVoiceCount(u8 *actor) {
    scriptVmPushInteger(((ActorScriptObject *)actor)->scriptState->vmAddress, gPlayerVoiceMessageCount);
    return 0;
}


/// Read one coordinate of the player position.
/// \param actor Actor whose script VM supplies a position axis (zero for X, one for Y, two for Z) and receives that coordinate.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E008 (0x8011E008).
s32 actorScriptGetPlayerCoordinate(u8 *actor) {
    s32 sp1C;
    s32 idx;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    idx = scriptVmPopInteger(sp1C);
    scriptVmPushFloat(sp1C, *(f32 *)((u8 *)&gPlayerState + idx * 4));
    return 0;
}

f32 objectGetFacingErrorToPlayer(u8 *);         /* extern */

/// Push the actor's facing error relative to the player.
/// \param actor Actor whose script VM receives the signed facing error in degrees.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E04C (0x8011E04C).
s32 actorScriptPushFacingErrorToPlayer(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushFloat(sp1C, objectGetFacingErrorToPlayer(actor));
    return 0;
}

/// Separate symbol view of the primary player profile points balance.
/// \note Original identifier: \c D_800EF790 (0x800EF790).
extern s32 gPlayerProfilePoints;

/// Read the player profile points balance.
/// \param actor Actor whose script VM receives the current points balance.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E084 (0x8011E084).
s32 actorScriptPushPlayerPoints(u8 *actor) {
    scriptVmPushInteger(((ActorScriptObject *)actor)->scriptState->vmAddress, gPlayerProfilePoints);
    return 0;
}

/// Replace the player profile points balance.
/// \param actor Actor whose script VM supplies the new points balance and receives the assigned value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E0B4 (0x8011E0B4).
s32 actorScriptSetPlayerPoints(u8 *actor) {
    s32 id = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    s32 h = scriptVmPopInteger(id);

    scriptVmPushInteger(id, ((PlayerControlPrefix *)&gPlayerState)->points = h);
    return 0;
}

s32 playerAddPoints(s32);  /* extern */

/// Adjust the player profile points balance within its supported range.
/// \param actor Actor whose script VM supplies a signed adjustment whose sum with the current balance fits a signed 32-bit value, and receives the resulting balance clamped to zero through 999999.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E0F8 (0x8011E0F8).
s32 actorScriptAdjustPlayerPoints(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, playerAddPoints(scriptVmPopInteger(sp1C)));
    return 0;
}

/// Separate symbol view of the primary player companion-control target handle.
/// \note Original identifier: \c D_800EF6F0 (0x800EF6F0).
extern s32 gPlayerCompanionTarget;

/// Read the current player movement category.
/// \param actor Actor whose script VM receives the movement category: zero for slow movement, one for medium movement, two for fast movement, or three for falling.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E138 (0x8011E138).
s32 actorScriptPushPlayerMovementClass(u8 *actor) {
    scriptVmPushInteger(((ActorScriptObject *)actor)->scriptState->vmAddress,
                        ((PlayerControlPrefix *)((u8 *)&gPlayerCompanionTarget - 0x50))->movementClass);
    return 0;
}

s32 playerGetChaseStage();     /* extern */
/// Read the current chase difficulty setting.
/// \param actor Actor whose script VM receives the current chase difficulty.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E168 (0x8011E168).
s32 actorScriptPushChaseDifficulty(u8 *actor) {
    s32 sp1C;

    sp1C = ((ActorScriptObject *)actor)->scriptState->vmAddress;
    scriptVmPushInteger(sp1C, playerGetChaseStage());
    return 0;
}
