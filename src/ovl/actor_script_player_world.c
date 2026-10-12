#include "common.h"
#include "player_types.h"
#include "item_types.h"
#include "script_vm.h"
#include "script_vm_stack.h"
#include "actor_script.h"
#include "object_types.h"

/// Unprototyped compatibility declaration for the retained extra argument in actorScriptPushSelfHandle.
void actorSetMessengerContactMode(u8 *, s32); /* extern */

/// Select how messenger objects respond to contact with the actor.
/// \param actor Actor whose script VM supplies the messenger contact mode and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E1A0 (0x8011E1A0).
s32 actorScriptSetMessengerContactMode(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    actorSetMessengerContactMode(actor, scriptVmPopInteger(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

/// Read the actor messenger contact mode.
/// \param actor Actor whose script VM receives its messenger contact mode.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E1E8 (0x8011E1E8).
s32 actorScriptPushMessengerContactMode(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress, ((ActorScriptState *)actorState)->messengerContactMode);
    return 0;
}

s32 interactionConsiderObjectInRegion(u8 *);

/// Consider the actor for preferred interaction and test interaction-region eligibility.
/// \param actor Actor whose script VM receives one when it qualifies for either interaction region, or zero otherwise.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E214 (0x8011E214).
s32 actorScriptConsiderInteractionRegion(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, interactionConsiderObjectInRegion(actor));
    return 0;
}


/// Read whether actor visibility is enabled.
/// \param actor Actor whose script VM receives one when it is marked visible, or zero when hidden.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E24C (0x8011E24C).
s32 actorScriptPushVisibilityEnabled(u8 *actor) {
    s32 handle = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;

    scriptVmPushInteger(handle, (((RuntimeObjectPrefix *)actor)->flags & 2) ? 0 : 1);
    return 0;
}

void playerSetInputKeepMask(s32); /* extern */

/// Select which player controls remain enabled and yield script execution.
/// \param actor Actor whose script VM supplies the input bits to retain, with bits 64 and 128 enabling the horizontal and vertical analog axes, and receives zero.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011E290 (0x8011E290).
s32 actorScriptSetPlayerInputKeepMask(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    playerSetInputKeepMask(scriptVmPopInteger(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 1;
}

u16 playerGetBlockedInput(void); /* extern */
/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
extern s32 gRuntimeGameMode;

/// Read player input blocked by the current control mask.
/// \param actor Actor whose script VM receives blocked button bits, including bits 64 and 128 for strong blocked analog input, or zero in game mode 3.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E2D0 (0x8011E2D0).
s32 actorScriptPushBlockedPlayerInput(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    if (gRuntimeGameMode == 3) {
        scriptVmPushInteger(sp1C, 0);
    } else {
        scriptVmPushInteger(sp1C, playerGetBlockedInput());
    }
    return 0;
}

void playerSetActorInteractionMode(u8 *, s32); /* extern */

/// Request a player interaction mode targeting the actor.
/// \param actor Actor whose script VM supplies zero for off, one for Pikachu mode, or two for follow mode, and receives zero; player permissions may prevent the change.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E32C (0x8011E32C).
s32 actorScriptSetPlayerInteractionMode(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    playerSetActorInteractionMode(actor, scriptVmPopInteger(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

s32 playerQueryInteractionMode();     /* extern */

/// Query the current player interaction mode.
/// \param actor Actor whose script VM receives an unspecified legacy result from the interaction-mode query.
/// \returns Zero, allowing script execution to continue.
/// \note Portability/UB: The interaction-mode helper is defined with a void return but declared as returning an integer here; its consumed result is unspecified.
/// \note Original decompilation identifier: \c func_8011E374 (0x8011E374).
s32 actorScriptQueryPlayerInteractionMode(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, playerQueryInteractionMode());
    return 0;
}

s32 playerIsPitchControlEnabled(u8 *); /* extern */
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern PlayerControlPrefix gPlayerState;

/// Read whether the player permits pitch control.
/// \param actor Actor whose script VM receives one when player pitch control is enabled, or zero otherwise.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E3AC (0x8011E3AC).
s32 actorScriptPushPlayerPitchControlEnabled(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, playerIsPitchControlEnabled(&gPlayerState));
    return 0;
}

void hudSetOverlayMode(s32); /* extern */
/// Select the alternate HUD overlay, disabling microphone input when enabled.
/// \param actor Actor whose script VM supplies nonzero to enable the alternate overlay or zero for normal drawing, and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E3EC (0x8011E3EC).
s32 actorScriptSetHudOverlayMode(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    hudSetOverlayMode(scriptVmPopInteger(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

void hudSetAuxiliaryIconVisible(s32); /* extern */
/// Control visibility of the auxiliary HUD icon.
/// \param actor Actor whose script VM supplies nonzero to show the icon or zero to hide it, and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E42C (0x8011E42C).
s32 actorScriptSetHudAuxiliaryIconVisible(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    hudSetAuxiliaryIconVisible(scriptVmPopInteger(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

void hudSetFeatureMask(s32); /* extern */
/// Select which HUD indicators are enabled.
/// \param actor Actor whose script VM supplies a feature mask (bit zero for action, bit one for movement, bit two for microphone indicators) and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E46C (0x8011E46C).
s32 actorScriptSetHudFeatureMask(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    hudSetFeatureMask(scriptVmPopInteger(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

void hudSetGuideStyle(); /* extern */
/// Select the HUD guide style with selection feedback when a different guide is enabled.
/// \param actor Actor whose script VM supplies a guide style from one through four, or zero to disable the guide, and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E4AC (0x8011E4AC).
s32 actorScriptSetHudGuideStyle(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    hudSetGuideStyle(scriptVmPopInteger(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

u8 *objectFindLiveByHandle(s32);  /* extern */
/// Control ordinary release of the held item and its HUD guide.
/// \param actor Actor whose script VM supplies nonzero to permit ordinary held-item release and select guide style four, or zero to prevent ordinary release and disable the guide, and receives zero; no live held item disables the guide, and any live held-item handle must identify initialized item state.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E4EC (0x8011E4EC).
s32 actorScriptSetHeldItemReleaseAndGuide(u8 *actor) {
    s32 sp1C;
    s32 sp18;
    s32 *itemFlags;
    u8 *liveObject;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp18 = scriptVmPopInteger(sp1C);
    liveObject = objectFindLiveByHandle(((RuntimeObjectPrefix *)actor)->state.actor->heldItemHandle);
    if (liveObject != NULL) {
        itemFlags = (s32 *)((RuntimeObjectPrefix *)liveObject)->state.bytes;
        if (sp18 != 0) {
            *itemFlags |= 0x40000000;
            hudSetGuideStyle(4);
        } else {
            *itemFlags &= 0xBFFFFFFF;
            hudSetGuideStyle(0);
        }
    } else {
        hudSetGuideStyle(0);
    }
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

s32 objectSetOnState(s32, s32); /* extern */

/// Set object activation and select the corresponding HUD guide.
/// \param actor Actor whose script VM supplies an activation value (zero for off, nonzero for on) followed by an object handle, and receives the supplied activation value when supported or zero otherwise; a nonzero result selects guide style two, and zero disables the guide.
/// \returns Zero, allowing script execution to continue.
/// \note Portability: The unprototyped HUD guide call passes an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_8011E598 (0x8011E598).
s32 actorScriptSetObjectActivationAndGuide(u8 *actor) {
    s32 sp24;
    s32 guideStyle;
    s32 sp1C;
    s32 sp18;

    sp24 = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp1C = scriptVmPopInteger(sp24);
    sp18 = objectSetOnState(scriptVmPopInteger(sp24), sp1C);
    if (sp18 != 0) {
        guideStyle = 2;
    } else {
        guideStyle = 0;
    }
    hudSetGuideStyle(guideStyle, sp18);
    scriptVmPushInteger(sp24, sp18);
    return 0;
}

s32 interactionGetCandidateId();     /* extern */
/// Read the preferred object from the current interaction scan.
/// \param actor Actor whose script VM receives the candidate object handle, or zero when none is selected.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E604 (0x8011E604).
s32 actorScriptPushInteractionCandidate(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, interactionGetCandidateId());
    return 0;
}

s32 interactionGetMarkerMode();     /* extern */
/// Read the active interaction marker mode.
/// \param actor Actor whose script VM receives the marker mode, or zero when inactive.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E63C (0x8011E63C).
s32 actorScriptPushInteractionMarkerMode(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, interactionGetMarkerMode());
    return 0;
}

/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern u8 *gPikachuActor;

/// Read the handle of the active Pikachu actor.
/// \param actor Actor whose script VM receives the Pikachu handle; requires an active Pikachu actor.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E674 (0x8011E674).
s32 actorScriptPushPikachuHandle(u8 *actor) {
    s32 handle = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;

    scriptVmPushInteger(handle, ((RuntimeObjectPrefix *)gPikachuActor)->handle);
    return 0;
}

/// Read the executing actor handle.
/// \param actor Actor whose script VM receives its own object handle.
/// \returns Zero, allowing script execution to continue.
/// \note Portability: The unprototyped push call passes an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_8011E6A8 (0x8011E6A8).
s32 actorScriptPushSelfHandle(u8 *actor) {
    scriptVmPushInteger(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress, ((RuntimeObjectPrefix *)actor)->handle);
    return 0;
}

s32 areaGetPathPointCount(s32);  /* extern */

/// Read the number of points in a current-area path.
/// \param actor Actor whose script VM supplies a valid current-area path index and receives its point count.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E6D8 (0x8011E6D8).
s32 actorScriptGetAreaPathPointCount(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, areaGetPathPointCount(scriptVmPopInteger(sp1C)));
    return 0;
}

f32 areaGetPathPointCoordinate(s32 arg0, s32 arg1, s32 arg2); /* extern */
/// Read one world-space coordinate of a current-area path point.
/// \param actor Actor whose script VM supplies a valid path index, a nonnegative point index, then a nonnegative axis (zero for X, one for Y, two for Z), and receives the coordinate or -65536 if the point or axis exceeds its range.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E718 (0x8011E718).
s32 actorScriptGetAreaPathPointCoordinate(u8 *actor) {
    s32 sp_pop;
    s32 vmAddress;
    s32 sp24;
    s32 sp20;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp20 = scriptVmPopInteger(vmAddress);
    sp24 = scriptVmPopInteger(vmAddress);
    sp_pop = scriptVmPopInteger(vmAddress);
    scriptVmPushFloat(vmAddress, areaGetPathPointCoordinate(sp_pop, sp24, sp20));
    return 0;
}

s32 actorFindNearestHandleFromPosition(u8 *, f32 *, s32, s32, f32); /* extern */

/// Find the nearest creature to a world position, excluding the executing actor.
/// \param actor Actor whose script VM supplies X, Y, Z, inclusive minimum and maximum creature kinds (38 selects the respective range endpoint), then an exclusive maximum approximate distance, and receives the nearest handle or zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E77C (0x8011E77C).
s32 actorScriptFindCreatureNearPosition(u8 *actor) {
    s32 vm;
    f32 x;
    f32 y;
    f32 z;
    s32 a;
    s32 b;
    f32 speed;
    f32 pos[3];

    vm = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    speed = scriptVmPopFloat(vm);
    b = scriptVmPopInteger(vm);
    a = scriptVmPopInteger(vm);
    z = scriptVmPopFloat(vm);
    y = scriptVmPopFloat(vm);
    x = scriptVmPopFloat(vm);
    pos[0] = x;
    pos[1] = y;
    pos[2] = z;
    scriptVmPushInteger(vm, actorFindNearestHandleFromPosition(actor, pos, a, b, speed));
    return 0;
}

s32 itemFindHandleNearPosition(f32 *, f32, s32, s32, s32, s32); /* extern */

/// Find an item near a world position using kind, state, and priority filters.
/// \param actor Actor whose script VM supplies X, Y, Z, inclusive minimum and maximum item kinds (165 selects the respective range endpoint), inclusive maximum approximate distance, required state (six accepts any), then a nonzero priority-weighting flag, and receives the selected handle or zero; weighted selection resets the selected item priority.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E828 (0x8011E828).
s32 actorScriptFindItemNearPosition(u8 *actor) {
    s32 vm;
    f32 pos[3];
    s32 d;
    s32 c;
    f32 speed;
    s32 b;
    s32 a;

    vm = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    a = scriptVmPopInteger(vm);
    b = scriptVmPopInteger(vm);
    speed = scriptVmPopFloat(vm);
    c = scriptVmPopInteger(vm);
    d = scriptVmPopInteger(vm);
    pos[2] = scriptVmPopFloat(vm);
    pos[1] = scriptVmPopFloat(vm);
    pos[0] = scriptVmPopFloat(vm);
    scriptVmPushInteger(vm, itemFindHandleNearPosition(pos, speed, d, c, b, a));
    return 0;
}

s32 objectFindNearestHandleInFacingCone(u8 *, s32, s32, f32, f32); /* extern */

/// Find the nearest object in a facing cone, excluding the executing actor.
/// \param actor Actor whose script VM supplies inclusive minimum and maximum kinds, exclusive maximum approximate distance, then maximum absolute horizontal angle in degrees (360 or greater permits all directions), and receives the nearest handle or zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E8E0 (0x8011E8E0).
s32 actorScriptFindObjectInFacingCone(u8 *actor) {
    s32 sp_pop;
    s32 vmAddress;
    s32 sp34;
    f32 sp30;
    f32 sp2C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp2C = scriptVmPopFloat(vmAddress);
    sp30 = scriptVmPopFloat(vmAddress);
    sp34 = scriptVmPopInteger(vmAddress);
    sp_pop = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, objectFindNearestHandleInFacingCone(actor, sp_pop, sp34, sp30, sp2C));
    return 0;
}

s32 objectFindNearestHandleFromPosition(u8 *, f32 *, s32, s32, f32); /* extern */

/// Find the nearest object to a world position, excluding the executing actor.
/// \param actor Actor whose script VM supplies X, Y, Z, inclusive minimum and maximum kinds, then an exclusive maximum approximate distance, and receives the nearest handle or zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011E960 (0x8011E960).
s32 actorScriptFindObjectNearPosition(u8 *actor) {
    s32 vm;
    f32 x;
    f32 y;
    f32 z;
    s32 a;
    s32 b;
    f32 speed;
    f32 pos[3];

    vm = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    speed = scriptVmPopFloat(vm);
    b = scriptVmPopInteger(vm);
    a = scriptVmPopInteger(vm);
    z = scriptVmPopFloat(vm);
    y = scriptVmPopFloat(vm);
    x = scriptVmPopFloat(vm);
    pos[0] = x;
    pos[1] = y;
    pos[2] = z;
    scriptVmPushInteger(vm, objectFindNearestHandleFromPosition(actor, pos, a, b, speed));
    return 0;
}

void objectSetUniformScaleByHandle(s32, f32); /* extern */

/// Resize an available object using its area-relative uniform scale.
/// \param actor Actor whose script VM supplies an object handle followed by the requested scale and receives zero; unavailable handles have no effect, and available objects must have nonzero current scale, and area-scaled objects require initialized area data with nonzero scale.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EA0C (0x8011EA0C).
s32 actorScriptSetObjectUniformScale(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    f32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopFloat(vmAddress);
    objectSetUniformScaleByHandle(scriptVmPopInteger(vmAddress), sp1C);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}

f32 objectGetUniformScaleByHandle(s32);       /* extern */

/// Read an available object uniform scale relative to its area scaling convention.
/// \param actor Actor whose script VM supplies an object handle and receives its uniform scale, or zero when unavailable; area-scaled objects require initialized area data with nonzero scale.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EA5C (0x8011EA5C).
s32 actorScriptGetObjectUniformScale(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat(sp1C, objectGetUniformScaleByHandle(scriptVmPopInteger(sp1C)));
    return 0;
}

f32 voiceReactiveSpinnerGetSpeedByHandle(s32 arg0);  /* extern */
/// Provide the active rotation speed of a voice-reactive spinner to a script.
/// \param actor Actor whose script VM supplies an object handle and receives its spinner speed in degrees per second, or zero for an unavailable handle, a non-spinner object, or a nonpositive speed.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EA9C (0x8011EA9C).
s32 actorScriptGetVoiceReactiveSpinnerSpeed(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat(sp1C, voiceReactiveSpinnerGetSpeedByHandle(scriptVmPopInteger(sp1C)));
    return 0;
}

s32 actorGetScriptActionFlagsByHandle(s32);  /* extern */

/// Read the compact action status exposed to scripts for a creature.
/// \param actor Actor whose script VM supplies a creature handle and receives its action status flags, or zero for a missing handle or a non-creature object.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EADC (0x8011EADC).
s32 actorScriptGetCreatureActionFlags(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, actorGetScriptActionFlagsByHandle(scriptVmPopInteger(sp1C)));
    return 0;
}

f32 objectGetWorldCoordinateByHandle(s32, s32);  /* extern */

/// Read one world-space coordinate of an available object.
/// \param actor Actor whose script VM supplies an object handle followed by a nonnegative axis (zero for X, one for Y, two for Z), and receives the coordinate, zero for an unavailable handle, or -65536 for an axis greater than two.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EB1C (0x8011EB1C).
s32 actorScriptGetObjectWorldCoordinate(u8 *actor) {
    s32 sp24;
    s32 sp20;
    s32 sp1C;

    sp24 = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp1C = scriptVmPopInteger(sp24);
    sp20 = scriptVmPopInteger(sp24);
    scriptVmPushFloat(sp24, objectGetWorldCoordinateByHandle(sp20, sp1C));
    return 0;
}

u8 sceneRegionIsHighlightedByHandle(s32);   /* extern */

/// Provide the highlight state of a scene region to an actor script.
/// \param actor Actor whose script VM supplies a scene-region handle and receives its highlight flag, or zero when the handle is not live; a live handle must identify initialized region state.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EB6C (0x8011EB6C).
s32 actorScriptIsSceneRegionHighlighted(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, sceneRegionIsHighlightedByHandle(scriptVmPopInteger(sp1C)));
    return 0;
}

f32 pathGetFirstPointCoordinateByHandle(s32, s32);  /* extern */

/// Read one coordinate of the first point in a path object.
/// \param actor Actor whose script VM supplies a path handle with at least one initialized point followed by a nonnegative axis (zero for X, one for Y, two for Z), and receives the coordinate, zero for a missing handle, or -65536 for a non-path object or an axis greater than two.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EBAC (0x8011EBAC).
s32 actorScriptGetFirstPathPointCoordinate(u8 *actor) {
    s32 sp24;
    s32 sp20;
    s32 sp1C;

    sp24 = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp1C = scriptVmPopInteger(sp24);
    sp20 = scriptVmPopInteger(sp24);
    scriptVmPushFloat(sp24, pathGetFirstPointCoordinateByHandle(sp20, sp1C));
    return 0;
}

f32 actorGetFollowingPathPointCoordinate(u8 *arg0, s32 arg1, s32 arg2); /* extern */

/// Read a coordinate of the path destination following a waypoint near the actor.
/// \param actor Actor whose script VM supplies a path handle with at least one waypoint and a complete three-coordinate end marker whose X is negative followed by a nonnegative axis (zero for X, one for Y, two for Z), and receives the coordinate following the first waypoint within three ground-distance units, or an end-marker coordinate when none qualifies; missing handles yield zero, and non-path objects or axes greater than two yield -65536.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EBFC (0x8011EBFC).
s32 actorScriptGetFollowingPathPointCoordinate(u8 *actor) {
    s32 sp24;
    s32 sp20;
    s32 sp1C;

    sp24 = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp1C = scriptVmPopInteger(sp24);
    sp20 = scriptVmPopInteger(sp24);
    scriptVmPushFloat(sp24, actorGetFollowingPathPointCoordinate(actor, sp20, sp1C));
    return 0;
}

f32 caveGetFirstAnchorCoordinate(s32, s32);  /* extern */

/// Read one coordinate of a cave first scene anchor.
/// \param actor Actor whose script VM supplies a cave handle followed by a nonnegative axis (zero for X, one for Y, two for Z), and receives the coordinate, zero for a missing handle, or -65536 for an axis greater than two; other object kinds are unsupported.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EC54 (0x8011EC54).
s32 actorScriptGetCaveFirstAnchorCoordinate(u8 *actor) {
    s32 vm;
    s32 a;
    s32 b;

    vm = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    b = scriptVmPopInteger(vm);
    a = scriptVmPopInteger(vm);
    scriptVmPushFloat(vm, caveGetFirstAnchorCoordinate(a, b));
    return 0;
}

f32 caveGetSecondAnchorCoordinate(s32, s32);  /* extern */

/// Read one coordinate of a cave second scene anchor.
/// \param actor Actor whose script VM supplies a cave handle followed by a nonnegative axis (zero for X, one for Y, two for Z), and receives the coordinate, zero for a missing handle, or -65536 for an axis greater than two; other object kinds are unsupported.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011ECA4 (0x8011ECA4).
s32 actorScriptGetCaveSecondAnchorCoordinate(u8 *actor) {
    s32 vm;
    s32 a;
    s32 b;

    vm = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    b = scriptVmPopInteger(vm);
    a = scriptVmPopInteger(vm);
    scriptVmPushFloat(vm, caveGetSecondAnchorCoordinate(a, b));
    return 0;
}

s32 caveGetLinkedLandmarkHandle(s32);  /* extern */

/// Read the landmark associated with a cave.
/// \param actor Actor whose script VM supplies a cave handle and receives the linked landmark handle, or zero for a missing cave handle; a live cave must have a valid linked landmark, and other object kinds are unsupported.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011ECF4 (0x8011ECF4).
s32 actorScriptGetCaveLandmarkHandle(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, caveGetLinkedLandmarkHandle(scriptVmPopInteger(sp1C)));
    return 0;
}

f32 actorGetGroundOffsetCoordinate(u8 *arg0, f32 arg1, f32 arg2, s32 arg3); /* extern */
/// Read a coordinate of a ground position offset from the actor.
/// \param actor Actor whose script VM supplies horizontal distance, finite heading offset in degrees, then a valid axis (zero for X, one for ground height, two for Z), and receives that coordinate.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011ED34 (0x8011ED34).
s32 actorScriptGetGroundOffsetCoordinate(u8 *actor) {
    f32 sp_pop;
    s32 vmAddress;
    f32 sp24;
    s32 sp20;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp20 = scriptVmPopInteger(vmAddress);
    sp24 = scriptVmPopFloat(vmAddress);
    sp_pop = scriptVmPopFloat(vmAddress);
    scriptVmPushFloat(vmAddress, actorGetGroundOffsetCoordinate(actor, sp_pop, sp24, sp20));
    return 0;
}

s32 terrainGetSurfaceAttributes(f32, f32, f32, u8 **); /* extern */

/// Read ground material attributes below a world position.
/// \param actor Actor whose script VM supplies X, Y, then Z and receives the terrain material-attribute value, or 0x77 when no terrain node is found.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EDA0 (0x8011EDA0).
s32 actorScriptGetGroundMaterialAtPosition(u8 *actor) {
    f32 sp_pop;
    s32 vmAddress;
    f32 sp24;
    f32 sp20;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp20 = scriptVmPopFloat(vmAddress);
    sp24 = scriptVmPopFloat(vmAddress);
    sp_pop = scriptVmPopFloat(vmAddress);
    scriptVmPushInteger(vmAddress, terrainGetSurfaceAttributes(sp_pop, sp24, sp20, 0));
    return 0;
}

s32 actorProbeGroundMaterialAhead(u8 *, f32, f32); /* extern */

/// Probe ground material along the actor heading.
/// \param actor Actor whose script VM supplies signed horizontal distance and receives an unspecified legacy result; requires initialized actor heading; the probe uses the actor height.
/// \returns Zero, allowing script execution to continue.
/// \note Portability/UB: The ground-material helper is defined with a void return but declared as returning an integer here; its consumed result is unspecified.
/// \note Original decompilation identifier: \c func_8011EE08 (0x8011EE08).
s32 actorScriptProbeGroundMaterialAhead(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, actorProbeGroundMaterialAhead(actor, scriptVmPopFloat(sp1C), 0.0f));
    return 0;
}


/// Probe ground material ahead with a vertical query offset.
/// \param actor Actor whose script VM supplies signed horizontal distance followed by vertical height offset and receives an unspecified legacy result; requires initialized actor heading.
/// \returns Zero, allowing script execution to continue.
/// \note Portability/UB: The ground-material helper is defined with a void return but declared as returning an integer here; its consumed result is unspecified.
/// \note Original decompilation identifier: \c func_8011EE54 (0x8011EE54).
s32 actorScriptProbeGroundMaterialAtHeightOffset(u8 *actor) {
    s32 sp1C;
    f32 sp20;
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp1C = vmAddress;
    sp20 = scriptVmPopFloat(vmAddress);
    scriptVmPushInteger(vmAddress, actorProbeGroundMaterialAhead(actor, scriptVmPopFloat(vmAddress), sp20));
    return 0;
}

f32 actorGetGroundHeightAhead(u8 *arg0, f32 arg1, f32 arg2); /* extern */

/// Read ground height along the actor heading.
/// \param actor Actor whose script VM supplies signed horizontal distance and receives the ground height, or zero when no ground node is found; the query starts at the actor height.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EEAC (0x8011EEAC).
s32 actorScriptGetGroundHeightAhead(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat(sp1C, actorGetGroundHeightAhead(actor, scriptVmPopFloat(sp1C), 0.0f));
    return 0;
}

f32 actorGetGroundHeightAhead(u8 *, f32, f32); /* extern */

/// Read ground height ahead with a vertical query offset.
/// \param actor Actor whose script VM supplies signed horizontal distance followed by vertical height offset and receives the ground height, or zero when no ground node is found.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EEF8 (0x8011EEF8).
s32 actorScriptGetGroundHeightAtHeightOffset(u8 *actor) {
    s32 vm;
    f32 a;
    f32 b;

    vm = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    b = scriptVmPopFloat(vm);
    a = scriptVmPopFloat(vm);
    scriptVmPushFloat(vm, actorGetGroundHeightAhead(actor, a, b));
    return 0;
}


/// Read the current game mode.
/// \param actor Actor whose script VM receives the current game-mode identifier.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011EF50 (0x8011EF50).
s32 actorScriptPushGameMode(u8 *actor) {
    scriptVmPushInteger(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress, gRuntimeGameMode);
    return 0;
}

/// Processed button and stick samples for all controller ports; scalar declarations view the primary held-button word.
/// \note Original identifier: \c D_800AD540 (0x800AD540).
extern s32 gControllerInputState;

/// Read held buttons from the first controller and yield script execution.
/// \param actor Actor whose script VM receives the first controller held-button mask.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011EF80 (0x8011EF80).
s32 actorScriptPushHeldControllerButtons(u8 *actor) {
    scriptVmPushInteger(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress, gControllerInputState);
    return 1;
}

void areaRequestTransition(s32, s32); /* extern */
/// Request an area transition and yield script execution.
/// \param actor Actor whose script VM supplies an area identifier followed by a valid destination spawn index and receives zero; area identifiers wrap through the 23 supported areas.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011EFB0 (0x8011EFB0).
s32 actorScriptRequestAreaTransition(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    areaRequestTransition(scriptVmPopInteger(vmAddress), sp1C);
    scriptVmPushInteger(vmAddress, 0);
    return 1;
}

/// Identifier of the currently loaded area.
/// \note Original identifier: \c D_800AE4F8 (0x800AE4F8).
extern s32 gRuntimeCurrentArea;

/// Read the current area identifier.
/// \param actor Actor whose script VM receives the current area identifier.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F000 (0x8011F000).
s32 actorScriptPushCurrentArea(u8 *actor) {
    scriptVmPushInteger(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress, gRuntimeCurrentArea);
    return 0;
}

void micRequestRecognitionOverride(s32); /* extern */
void micRequestRecognitionFollowsTrigger(s32); /* extern */
/// Whether gameplay exposes microphone interaction and its HUD indicator.
/// \note Original identifier: \c D_800722D0 (0x800722D0).
extern s32 gGameplayMicrophoneAvailable;
/// Whether gameplay permits recognized voice commands to launch messenger effects.
/// \note Original identifier: \c D_800722D4 (0x800722D4).
extern s32 gGameplayVoiceCommandsAllowed;

/// Configure microphone recognition and gameplay voice availability, then yield script execution.
/// \param actor Actor whose script VM supplies four boolean options in order: recognition override, recognition follows trigger, microphone availability, and voice-command permission; it receives zero.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011F030 (0x8011F030).
s32 actorScriptConfigureMicrophone(u8 *actor) {
    s32 sp_pop;
    s32 vmAddress;
    s32 sp2C;
    s32 sp28;
    s32 sp24;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = scriptVmPopInteger(vmAddress);
    sp28 = scriptVmPopInteger(vmAddress);
    sp2C = scriptVmPopInteger(vmAddress);
    sp_pop = scriptVmPopInteger(vmAddress);
    micRequestRecognitionOverride(sp_pop);
    micRequestRecognitionFollowsTrigger(sp2C);
    if (sp28 != 0) {
        gGameplayMicrophoneAvailable = 1;
    } else {
        gGameplayMicrophoneAvailable = 0;
    }
    if (sp24 != 0) {
        gGameplayVoiceCommandsAllowed = 1;
    } else {
        gGameplayVoiceCommandsAllowed = 0;
    }
    scriptVmPushInteger(vmAddress, 0);
    return 1;
}

f32 actorAdjustSecondaryMeter(u8 *, f32); /* extern */

/// Adjust the active Pikachu secondary gameplay meter.
/// \param actor Actor whose script VM supplies a signed adjustment and receives the updated meter value clamped to zero through 100; requires an active Pikachu actor.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F0E4 (0x8011F0E4).
s32 actorScriptAdjustPikachuSecondaryMeter(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat(sp1C, actorAdjustSecondaryMeter(gPikachuActor, scriptVmPopFloat(sp1C)));
    return 0;
}


/// Set the active Pikachu secondary gameplay meter.
/// \param actor Actor whose script VM supplies a meter value and receives the assigned value clamped to zero through 100; requires an active Pikachu actor.
/// \returns Zero, allowing script execution to continue.
///
/// \note Portability: The adjacent clamp and store preserve the original IDO 5.3 instruction scheduling.
/// \note Original decompilation identifier: \c func_8011F12C (0x8011F12C).
s32 actorScriptSetPikachuSecondaryMeter(u8 *actor) {
    s32 sp1C;
    f32 v;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    v = scriptVmPopFloat(sp1C);

    // clang-format off
    if (v < 0.0f) { v = 0.0f; } else if (v > 100.0f) { v = 100.0f; } ((RuntimeObjectPrefix *)gPikachuActor)->state.actor->secondaryMeter = v;
    // clang-format on
    scriptVmPushFloat(sp1C, ((RuntimeObjectPrefix *)gPikachuActor)->state.actor->secondaryMeter);
    return 0;
}


/// Read the active Pikachu secondary gameplay meter.
/// \param actor Actor whose script VM receives the current secondary meter value; requires an active Pikachu actor.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F1BC (0x8011F1BC).
s32 actorScriptPushPikachuSecondaryMeter(u8 *actor) {
    scriptVmPushFloat(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress,
                      ((RuntimeObjectPrefix *)gPikachuActor)->state.actor->secondaryMeter);
    return 0;
}

f32 actorAdjustActionMeter(u8 *, f32); /* extern */

/// Adjust the active Pikachu action resource and request its HUD display when appropriate.
/// \param actor Actor whose script VM supplies a signed adjustment and receives the updated action resource clamped to zero through 100; requires an active Pikachu actor.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F1F4 (0x8011F1F4).
s32 actorScriptAdjustPikachuActionResource(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat(sp1C, actorAdjustActionMeter(gPikachuActor, scriptVmPopFloat(sp1C)));
    return 0;
}


/// Read the active Pikachu action resource.
/// \param actor Actor whose script VM receives the current action resource value; requires an active Pikachu actor.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F23C (0x8011F23C).
s32 actorScriptPushPikachuActionResource(u8 *actor) {
    scriptVmPushFloat(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress,
                      ((RuntimeObjectPrefix *)gPikachuActor)->state.actor->actionResource);
    return 0;
}

u8 *itemCreate(f32, f32, f32, f32, f32, s32); /* extern */

/// Create an item at a requested position and orientation.
/// \param actor Actor whose script VM supplies X, Y, Z, pitch, yaw in degrees, then a valid item kind, and receives the created item handle; negative Y requests ground placement using its magnitude as the query height, with item size added to the ground height.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F274 (0x8011F274).
s32 actorScriptCreateItem(u8 *actor) {
    f32 sp_pop;
    s32 vmAddress;
    f32 sp3C;
    f32 sp38;
    f32 sp34;
    f32 sp30;
    s32 sp2C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp2C = scriptVmPopInteger(vmAddress);
    sp30 = scriptVmPopFloat(vmAddress);
    sp34 = scriptVmPopFloat(vmAddress);
    sp38 = scriptVmPopFloat(vmAddress);
    sp3C = scriptVmPopFloat(vmAddress);
    sp_pop = scriptVmPopFloat(vmAddress);
    scriptVmPushInteger(vmAddress, ((RuntimeObjectPrefix *)itemCreate(sp_pop, sp3C, sp38, sp34, sp30, sp2C))->handle);
    return 0;
}

void itemSetMode(u8 *, s32);            /* extern */

/// Create an item at a requested position and orientation without autonomous movement.
/// \param actor Actor whose script VM supplies X, Y, Z, pitch, yaw in degrees, then a valid item kind, and receives the created item handle; negative Y requests ground placement using its magnitude as the query height, with item size added to the ground height.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F310 (0x8011F310).
s32 actorScriptCreateStationaryItem(u8 *actor) {
    f32 sp_pop;
    s32 vmAddress;
    f32 sp3C;
    f32 sp38;
    f32 sp34;
    f32 sp30;
    s32 sp2C;
    u8 *sp28;
    u8 *createdItem;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp2C = scriptVmPopInteger(vmAddress);
    sp30 = scriptVmPopFloat(vmAddress);
    sp34 = scriptVmPopFloat(vmAddress);
    sp38 = scriptVmPopFloat(vmAddress);
    sp3C = scriptVmPopFloat(vmAddress);
    sp_pop = scriptVmPopFloat(vmAddress);
    createdItem = itemCreate(sp_pop, sp3C, sp38, sp34, sp30, sp2C);
    sp28 = createdItem;
    itemSetMode(createdItem, 2);
    scriptVmPushInteger(vmAddress, ((RuntimeObjectPrefix *)sp28)->handle);
    return 0;
}

void itemDestroy(s32); /* extern */
u8 *objectResolveAvailableHandle();    /* extern */
/// Remove an available item from the scene.
/// \param actor Actor whose script VM supplies an item handle and receives zero; unavailable handles have no effect, and other object kinds are unsupported.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F3C0 (0x8011F3C0).
s32 actorScriptDestroyItem(u8 *actor) {
    s32 sp1C;
    s32 resolvedObject;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    resolvedObject = objectResolveAvailableHandle(scriptVmPopInteger(sp1C));
    if (resolvedObject != 0) {
        itemDestroy(resolvedObject);
    }
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

s16 actorReleaseHeldItem(u8 *); /* extern */
/// Move an available object to a requested position and release the actor held item.
/// \param actor Actor whose script VM supplies X, Y, Z, then an object handle and receives zero; unavailable handles leave both the target and held item unchanged.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F410 (0x8011F410).
s32 actorScriptMoveObjectAndReleaseHeldItem(u8 *actor) {
    s32 vmAddress;
    f32 sp30;
    f32 sp2C;
    f32 sp28;
    s32 sp24;
    u8 *sp20;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = scriptVmPopInteger(vmAddress);
    sp28 = scriptVmPopFloat(vmAddress);
    sp2C = scriptVmPopFloat(vmAddress);
    sp30 = scriptVmPopFloat(vmAddress);
    sp20 = objectResolveAvailableHandle(sp24);
    if (sp20 != NULL) {
        actorReleaseHeldItem(actor);
        ((RuntimeObjectPrefix *)sp20)->position[1] = sp2C;
        ((RuntimeObjectPrefix *)sp20)->worldPosition[1] = sp2C;
        ((RuntimeObjectPrefix *)sp20)->position[0] = sp30;
        ((RuntimeObjectPrefix *)sp20)->worldPosition[0] = sp30;
        ((RuntimeObjectPrefix *)sp20)->position[2] = sp28;
        ((RuntimeObjectPrefix *)sp20)->worldPosition[2] = sp28;
    }
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}


/// Replace an available item with a fresh item of a requested kind at the same position and orientation.
/// \param actor Actor whose script VM supplies an item handle followed by a valid replacement kind, and receives the new item handle or zero for an unavailable original; other object kinds are unsupported.
/// \returns Zero, allowing script execution to continue.
///
/// \note Portability: The resolved object address is carried in an s32, requiring 32-bit pointers.
/// \note Original decompilation identifier: \c func_8011F4B8 (0x8011F4B8).
s32 actorScriptReplaceItemKind(u8 *actor) {
    s32 vm;
    u8 *item;
    s32 kind;
    f32 x;
    f32 y;
    f32 z;
    f32 dir;
    f32 speed;
    u8 *obj;

    vm = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    kind = scriptVmPopInteger(vm);
    obj = (u8 *)objectResolveAvailableHandle(scriptVmPopInteger(vm));
    if (obj == NULL) {
        scriptVmPushInteger(vm, 0);
    } else {
        x = ((RuntimeObjectPrefix *)obj)->worldPosition[0];
        y = ((RuntimeObjectPrefix *)obj)->worldPosition[1];
        z = ((RuntimeObjectPrefix *)obj)->worldPosition[2];
        dir = ((RuntimeObjectPrefix *)obj)->rotation[0];
        speed = ((RuntimeObjectPrefix *)obj)->rotation[1];
        itemDestroy((s32)obj);
        item = itemCreate(x, y, z, dir, speed, kind);
        scriptVmPushInteger(vm, ((RuntimeObjectPrefix *)item)->handle);
    }
    return 0;
}

/// Read the actor stored held-item handle.
/// \param actor Actor whose script VM receives its held-item handle, or zero when none is recorded.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F574 (0x8011F574).
s32 actorScriptPushHeldItemHandle(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress, ((ActorScriptState *)actorState)->heldItemHandle);
    return 0;
}

s32 actorGetHeldItemKind(u8 *);                   /* extern */
void actorTryHoldItem(u8 *, s32, s32); /* extern */

/// Attempt to hold a requested item, forcing release from its current holder.
/// \param actor Actor whose script VM supplies an item handle and receives its resulting held-item kind, or zero when it has no available held item; the previous held item is released only if the new item can be taken.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F5A0 (0x8011F5A0).
s32 actorScriptTryHoldItem(u8 *actor) {
    s32 sp18;
    s32 vmAddress;
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;

    vmAddress = ((ActorScriptState *)actorState)->vmAddress;
    sp18 = vmAddress;
    actorTryHoldItem(actor, scriptVmPopInteger(vmAddress), 1);
    scriptVmPushInteger(vmAddress, actorGetHeldItemKind(actor));
    return 0;
}


/// Read the current state of a live item.
/// \param actor Actor whose script VM supplies an item handle and receives the item state, or -1 for a missing handle or a non-item object.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F5F4 (0x8011F5F4).
s32 actorScriptGetItemState(u8 *actor) {
    u8 *sp1C;
    u8 *liveObject;

    sp1C = (u8 *)((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    liveObject = objectFindLiveByHandle(scriptVmPopInteger(sp1C));
    if (liveObject != NULL) {
        if ((u16)((RuntimeObjectPrefix *)liveObject)->kind == 0x86) {
            scriptVmPushInteger((s32)sp1C,
                                (s32)((ItemStatePrefix *)((RuntimeObjectPrefix *)liveObject)->state.bytes)->state);
        } else {
            scriptVmPushInteger((s32)sp1C, -1);
        }
    } else {
        scriptVmPushInteger((s32)sp1C, -1);
    }
    return 0;
}

void itemStartMaterialAnimation(s32, s32); /* extern */
/// Select an animation for an available item.
/// \param actor Actor whose script VM supplies an item handle followed by a valid animation identifier and receives zero; unavailable handles have no effect, and other object kinds are unsupported.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F678 (0x8011F678).
s32 actorScriptSetItemAnimation(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;
    s32 resolvedObject;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    resolvedObject = objectResolveAvailableHandle(scriptVmPopInteger(vmAddress));
    if (resolvedObject != 0) {
        itemStartMaterialAnimation(resolvedObject, sp1C);
    }
    scriptVmPushInteger(sp24, 0);
    return 0;
}

void itemSetMaterialFrame(s32, s32); /* extern */
/// Select a fixed material frame for an available item and stop its material animation.
/// \param actor Actor whose script VM supplies an item handle followed by a valid material texture frame identifier that fits in signed 16 bits and receives zero; unavailable handles have no effect, and other object kinds are unsupported.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F6D4 (0x8011F6D4).
s32 actorScriptSetItemMaterialFrame(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;
    s32 resolvedObject;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    resolvedObject = objectResolveAvailableHandle(scriptVmPopInteger(vmAddress));
    if (resolvedObject != 0) {
        itemSetMaterialFrame(resolvedObject, sp1C);
    }
    scriptVmPushInteger(sp24, 0);
    return 0;
}

void itemSetDrawMode(s32, s32); /* extern */
/// Select the draw mode of a live item.
/// \param actor Actor whose script VM supplies an item handle followed by a draw mode from zero through six and receives zero; missing handles have no effect, and other object kinds are unsupported.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F730 (0x8011F730).
s32 actorScriptSetItemDrawMode(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    itemSetDrawMode(scriptVmPopInteger(vmAddress), sp1C);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}

void itemSetTint(s32, u8, u8, u8, s32); /* extern */
/// Set the RGBA tint of a live item.
/// \param actor Actor whose script VM supplies an item handle followed by red, green, blue, and alpha channel values from zero through 255, and receives zero; missing handles have no effect, and other object kinds are unsupported.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F780 (0x8011F780).
s32 actorScriptSetItemTint(u8 *actor) {
    s32 sp_pop;
    s32 vmAddress;
    u8 sp2F;
    u8 sp2E;
    u8 sp2D;
    u8 sp2C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp2C = scriptVmPopInteger(vmAddress);
    sp2D = scriptVmPopInteger(vmAddress);
    sp2E = scriptVmPopInteger(vmAddress);
    sp2F = scriptVmPopInteger(vmAddress);
    sp_pop = scriptVmPopInteger(vmAddress);
    itemSetTint(sp_pop, sp2F, sp2E, sp2D, sp2C);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}

void toolboxStoreBonusObject(s32); /* extern */
/// Replace the toolbox bonus item, removing any previously stored bonus item.
/// \param actor Actor whose script VM supplies a valid available item handle distinct from the previously stored bonus item and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F808 (0x8011F808).
s32 actorScriptStoreBonusItem(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    toolboxStoreBonusObject(objectResolveAvailableHandle(scriptVmPopInteger(sp1C)));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

void toolboxStorePurchasedObject(s32); /* extern */
/// Replace the toolbox purchase item, removing any previously stored purchase item; the replacement must be distinct from the previous item.
/// \param actor Actor whose script VM supplies an item handle and receives zero; unavailable handles have no effect, and other object kinds are unsupported.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F850 (0x8011F850).
s32 actorScriptStorePurchasedItem(u8 *actor) {
    s32 sp1C;
    s32 resolvedObject;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    resolvedObject = objectResolveAvailableHandle(scriptVmPopInteger(sp1C));
    if (resolvedObject != 0) {
        toolboxStorePurchasedObject(resolvedObject);
    }
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

s32 toolboxRecordSupplyItem(s32);  /* extern */
/// Record acquisition of a supply item in persistent progress.
/// \param actor Actor whose script VM supplies a valid item kind and receives one when it has a supply progress flag to record, or zero otherwise.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F8A0 (0x8011F8A0).
s32 actorScriptRecordSupplyItem(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, toolboxRecordSupplyItem(scriptVmPopInteger(sp1C)));
    return 0;
}

s32 itemGetImpactCount(s32);  /* extern */

/// Read the remaining impact count of an available item.
/// \param actor Actor whose script VM supplies an item handle and receives its remaining impact count, or -65536 when the handle is unavailable or identifies a non-item object.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F8E0 (0x8011F8E0).
s32 actorScriptGetItemImpactCount(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, itemGetImpactCount(scriptVmPopInteger(sp1C)));
    return 0;
}

s32 itemSetImpactCount(s32, s32); /* extern */

/// Set the remaining impact count of an available item.
/// \param actor Actor whose script VM supplies an item handle followed by the new count and receives one if updated, or zero when the handle is unavailable or identifies a non-item object.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F920 (0x8011F920).
s32 actorScriptSetItemImpactCount(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, itemSetImpactCount(scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

/// Read elapsed time for the current actor script.
/// \param actor Actor whose script VM receives its elapsed script time in seconds.
/// \returns Zero, allowing script execution to continue.
///
/// \note Portability: The VM pointer is carried in an s32, requiring 32-bit pointers.
/// \note Original decompilation identifier: \c func_8011F970 (0x8011F970).
s32 actorScriptPushElapsedTime(u8 *actor) {
    u8 *vmBytes;

    vmBytes = (u8 *)((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat((s32)vmBytes, ((ScriptVm *)vmBytes)->totalElapsed);
    return 0;
}

f32 terrainGetWorldHeight(f32, f32, f32, s32); /* extern */

/// Read ground height at a world position.
/// \param actor Actor whose script VM supplies X, Y, then Z and receives the ground height, or zero when no terrain node is found.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011F99C (0x8011F99C).
s32 actorScriptGetGroundHeightAtPosition(u8 *actor) {
    s32 vm;
    f32 x;
    f32 y;
    f32 z;

    vm = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    z = scriptVmPopFloat(vm);
    y = scriptVmPopFloat(vm);
    x = scriptVmPopFloat(vm);
    scriptVmPushFloat(vm, terrainGetWorldHeight(x, y, z, 0));
    return 0;
}

s32 itemGetResponse(s32, s32); /* extern */
/// Read the enabled response of a live item.
/// \param actor Actor whose script VM supplies a response type followed by an item handle and receives its configured response code, or zero for a missing handle, suppressed responses, or a response type outside one through five; other object kinds are unsupported.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FA04 (0x8011FA04).
s32 actorScriptGetLiveItemResponse(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, itemGetResponse(scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

s32 itemGetKindResponse(s32, s32); /* extern */
/// Read a response configured for an item kind.
/// \param actor Actor whose script VM supplies a response type followed by a valid item kind and receives its response code, or zero for a response type outside one through five.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FA54 (0x8011FA54).
s32 actorScriptGetItemKindResponse(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, itemGetKindResponse(scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

/// Control suppression of target-facing motion on actor child parts.
/// \param actor Actor whose script VM supplies nonzero to suppress target-facing motion or zero to permit it when the current action allows, and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Portability: The unprototyped pop call passes an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_8011FAA4 (0x8011FAA4).
s32 actorScriptSetTargetFacingSuppressed(u8 *actor) {
    s32 sp1C;
    u8 *actorState;
    u8 *updatedActorState;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    if (scriptVmPopInteger(sp1C) != 0) {
        actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
        ((ActorScriptState *)actorState)->behaviorInhibitFlags =
            (u16)(((ActorScriptState *)actorState)->behaviorInhibitFlags | 1);
    } else {
        updatedActorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
        ((ActorScriptState *)updatedActorState)->behaviorInhibitFlags =
            (u16)(((ActorScriptState *)updatedActorState)->behaviorInhibitFlags & 0xFFFE);
    }
    scriptVmPushInteger(sp1C, 0);
    return 0;
}
