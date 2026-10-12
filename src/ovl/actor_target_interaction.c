/// \file
/// Actor targeting, interaction, and script-message delivery.
/// \ingroup ActorScriptVm
///
/// See docs/VOICE_AND_SCRIPT_VM.md for the subsystem walkthrough.

/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "player_types.h"
#include "item_types.h"
#include "object_types.h"
#include "actor_script.h"

u8 *objectResolveAvailableHandle(s32); /* extern */
/// Resolve the actor stored target identifier to an object.
/// \param actor Actor whose target identifier is resolved.
/// \returns The resolved target object, or null when it cannot be resolved.
/// \note Original decompilation identifier: \c func_80118B30 (0x80118B30).
u8 *actorResolveStoredTarget(u8 *actor) {
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    return objectResolveAvailableHandle(((ActorScriptState *)actorState)->targetHandle);
}

/// Get the actor target when target tracking is enabled.
/// \param actor Actor whose tracking target is inspected.
/// \returns The resolved tracking target, or null when tracking is disabled or the target cannot be resolved.
/// \note Original decompilation identifier: \c func_80118B54 (0x80118B54).
u8 *actorGetTrackingTarget(u8 *actor) {
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;

    if (((ActorScriptState *)actorState)->targetMode == 0) {
        return 0;
    }
    return actorResolveStoredTarget(actor);
}

/// Select how messenger objects respond when contacting the actor.
/// \param actor Actor whose messenger contact mode is changed.
/// \param mode Contact mode consumed by messenger interactions.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118B8C (0x80118B8C).
void actorSetMessengerContactMode(u8 *actor, s32 mode) {
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ((ActorScriptState *)actorState)->messengerContactMode = mode;
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
extern f32 fabsf(f32);
#pragma intrinsic(fabsf)
f32 vector3GetYawDegrees(f32 *);
f32 actorConstrainFacingHeading(u8 *, f32);
f32 mathApproachAngleDegrees(f32, f32, f32, f32, f32);
void actorAddRotationAxisDegrees(u8 *, f32, s32);
f32 mathWrapSignedDegrees(f32);
/// Turn an actor toward its stored target position while respecting facing constraints.
/// \param object Actor whose horizontal facing is updated.
/// \param speed Angular turn-speed control in degrees per second.
/// \returns One when the constrained target heading is reached within one degree; otherwise zero.
/// \note Original decompilation identifier: \c func_80118B98 (0x80118B98).
s32 actorTurnTowardTarget(u8 *object, f32 speed) {
    u8 *state;
    f32 direction[3];
    f32 angle;
    f32 heading;
    state = *(u8 **)(object + 0x54);
    direction[0] = *(f32 *)(state + 0x1EC) - *(f32 *)(object + 0x44);
    direction[1] = 0.0f;
    direction[2] = *(f32 *)(state + 0x1F4) - *(f32 *)(object + 0x4C);
    heading = vector3GetYawDegrees(direction);
    angle = actorConstrainFacingHeading(object, heading);
    actorAddRotationAxisDegrees(
        object,
        mathApproachAngleDegrees(*(f32 *)(state + 0x10), angle, 1.0f, 90.0f, speed * gRuntimeFrameSeconds) -
            *(f32 *)(state + 0x10),
        1);
    if (fabsf(mathWrapSignedDegrees(angle - *(f32 *)(state + 0x10))) < 1.0f)
        return 1;
    return 0;
}

void objectGetPendingLocalMovement(f32 *);
void objectSetPendingLocalMovement(f32 *);
f32 mathSinDegrees(f32);
f32 mathCosDegrees(f32);
/// Turn an actor toward its target with a compensating movement around its turning pivot.
/// \param object Actor whose facing and movement are updated.
/// \param speed Angular turn-speed control in degrees per second.
/// \returns One when the constrained target heading is reached within one degree; otherwise zero.
/// \note Original decompilation identifier: \c func_80118C80 (0x80118C80).
s32 actorPivotTowardTarget(u8 *object, f32 speed) {
    u8 *state;
    f32 direction[3];
    f32 angle;
    f32 turn;
    f32 heading;
    f32 lateral;
    f32 offset;
    f32 sine;
    f32 cosine;
    f32 error;
    f32 vector[3];
    f32 magnitude;
    state = *(u8 **)(object + 0x54);
    direction[0] = *(f32 *)(state + 0x1EC) - *(f32 *)(object + 0x44);
    direction[1] = 0.0f;
    direction[2] = *(f32 *)(state + 0x1F4) - *(f32 *)(object + 0x4C);
    heading = vector3GetYawDegrees(direction);
    angle = actorConstrainFacingHeading(object, heading);
    turn = mathApproachAngleDegrees(*(f32 *)(state + 0x10), angle, 1.0f, 90.0f, speed * gRuntimeFrameSeconds) -
           *(f32 *)(state + 0x10);
    sine = mathSinDegrees(turn);
    cosine = mathCosDegrees(turn);
    offset = (1.0f - cosine) * 90.0f;
    objectGetPendingLocalMovement(vector);
    lateral = -sine * 90.0f;
    vector[0] = lateral * cosine - offset * sine;
    vector[2] = lateral * sine + offset * cosine;
    objectSetPendingLocalMovement(vector);
    actorAddRotationAxisDegrees(object, turn, 1);
    error = mathWrapSignedDegrees(angle - *(f32 *)(state + 0x10));
    magnitude = fabsf(error);
    if (magnitude < 1.0f)
        return 1;
    return 0;
}

s32 actorSetTargetPosition(u8 *, u8 *); /* extern */
s32 actorChooseEscapePosition(u8 *, u8 *); /* extern */

/// Refresh the actor destination for pursuit or escape from its tracked target.
/// \param actor Actor whose tracked target position is refreshed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118DF0 (0x80118DF0).
void actorRefreshTrackingTargetPosition(u8 *actor) {
    u8 *state = ((RuntimeObjectPrefix *)actor)->state.bytes;
    u8 *target;

    if ((((ActorScriptState *)state)->targetMode != 0) && (((ActorScriptState *)state)->targetHandle != 0)) {
        target = actorResolveStoredTarget(actor);
        if (target != NULL) {
            if (((ActorScriptState *)state)->targetMode == 1) {
                actorSetTargetPosition(actor, target + 0x44);
            } else if (((ActorScriptState *)state)->targetMode == 2) {
                actorChooseEscapePosition(actor, target + 0x44);
            }
        }
    }
}

/// Post a message to the actor script mailbox.
/// \param obj Address of the actor receiving the message.
/// \param msg Message value and arguments to deliver.
/// \param force Nonzero to replace a message already pending.
/// \returns One when the message is accepted; zero when a pending message prevents delivery.
///
/// \note NONPORTABLE: obj carries a RAM address in the original 32-bit ABI.
/// \note Original decompilation identifier: \c func_80118E74 (0x80118E74).
/// \note Copies the message on successful delivery; the caller need not retain
/// its storage. Acknowledgement is separate from inspecting the stored fields.
/// \see actorAcknowledgeScriptMessage
/// \see actorScriptPushMessagePending
s32 actorPostScriptMessage(s32 obj, ScriptMessage msg, s32 force) {
    ActorScriptState *state = ((ActorScriptObject *)obj)->scriptState;

    if ((force == 0) && (state->messagePending != 0)) {
        return 0;
    }
    state->messagePending = 1;
    state->message = msg;
    return 1;
}

f32 vector3Length(f32 *);                   /* extern */

/// Attempt to notify an actor script of an item impact and its speed.
/// An already pending message prevents delivery.
/// \param actor Address of the actor receiving the impact message.
/// \param item Item whose identity and motion describe the impact.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118EE8 (0x80118EE8).
void actorPostItemImpactMessage(s32 actor, u8 *item) {
    u8 *state;
    ScriptMessage msg;
    f32 dir[3];

    state = ((RuntimeObjectPrefix *)item)->state.bytes;
    dir[0] = ((ItemStatePrefix *)state)->velocityX;
    dir[1] = -((ItemStatePrefix *)state)->downwardVelocity;
    dir[2] = ((ItemStatePrefix *)state)->velocityZ;
    msg.arg1 = 0;
    msg.type = 0xE;
    msg.arg0 = ((RuntimeObjectPrefix *)item)->handle;
    msg.arg2 = vector3Length(dir);
    msg.arg3 = 0;
    msg.arg4 = ((RuntimeObjectPrefix *)item)->handle;
    actorPostScriptMessage(actor, msg, 0);
}

s32 itemCanBePickedUp(u8 *, s32);
s32 itemTryRelease(u8 *, s32);
u8 *objectFindLiveByHandle(s32);
void itemSetMode(u8 *, s32);
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern u8 *gPikachuActor;

/// Attempt to make an actor hold the requested item, releasing its previous held item on success.
/// \param actor Actor that will hold the item.
/// \param itemId Identifier resolved to the item to hold.
/// \param forceRelease Nonzero to force release from the item current holder.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118FAC (0x80118FAC).
void actorTryHoldItem(u8 *actor, s32 itemId, s32 forceRelease) {
    u8 *state = ((RuntimeObjectPrefix *)actor)->state.bytes;
    u8 *obj;
    u8 *other;
    u8 *rec;

    obj = (u8 *)objectResolveAvailableHandle(itemId);
    if ((obj != NULL) && (itemCanBePickedUp(obj, 0) != 0) && (itemTryRelease(obj, forceRelease) != 0)) {
        other = objectFindLiveByHandle(((ActorScriptState *)state)->heldItemHandle);
        if (other != NULL) {
            itemTryRelease(other, 1);
        }
        rec = ((RuntimeObjectPrefix *)obj)->state.bytes;
        if (actor == gPikachuActor) {
            ((ItemStatePrefix *)rec)->state = 2;
        } else {
            ((ItemStatePrefix *)rec)->state = 3;
        }
        ((ItemStatePrefix *)rec)->owner = ((RuntimeObjectPrefix *)actor)->handle;
        itemSetMode(obj, 4);
        ((ActorScriptState *)state)->heldItemHandle = ((RuntimeObjectPrefix *)obj)->handle;
        ((ItemStatePrefix *)rec)->flags &= 0xE3FFFFFF;
    }
}

/// Query the remaining impact count of a resolved item.
/// \param id Identifier resolved to the item.
/// \returns The item impact count, or 0xFFFF0000 when the identifier does not resolve to an item.
/// \note Original decompilation identifier: \c func_8011908C (0x8011908C).
s32 itemGetImpactCount(s32 id) {
    u8 *resolvedObject;

    resolvedObject = objectResolveAvailableHandle(id);
    if (resolvedObject == NULL) {
        return 0xFFFF0000;
    }
    if (((RuntimeObjectPrefix *)resolvedObject)->kind != 0x86) {
        return 0xFFFF0000;
    }
    return ((ItemStatePrefix *)((RuntimeObjectPrefix *)resolvedObject)->state.bytes)->bounceCount;
}

/// Set the remaining impact count of a resolved item.
/// \param id Identifier resolved to the item.
/// \param count Impact count to assign.
/// \returns One when the item was updated, otherwise zero.
/// \note Original decompilation identifier: \c func_801190DC (0x801190DC).
s32 itemSetImpactCount(s32 id, s32 count) {
    u8 *resolvedObject;

    resolvedObject = objectResolveAvailableHandle(id);
    if (resolvedObject == NULL) {
        return 0;
    }
    if (((RuntimeObjectPrefix *)resolvedObject)->kind != 0x86) {
        return 0;
    }
    ((ItemStatePrefix *)((RuntimeObjectPrefix *)resolvedObject)->state.bytes)->bounceCount = count;
    return 1;
}

/// Set the activation state of an object that supports being switched on and off.
/// \param on Zero to switch off, or nonzero to switch on.
/// \param id Identifier resolved to the object.
/// \returns The supplied on value when supported, or zero when unresolved or unsupported.
/// \note Original decompilation identifier: \c func_80119134 (0x80119134).
s32 objectSetOnState(s32 on, s32 id) {
    u8 *obj = (u8 *)objectResolveAvailableHandle(id);
    u8 *st;
    s32 flags;

    if (obj == NULL) {
        return 0;
    }
    if (((RuntimeObjectPrefix *)obj)->kind == 0x10) {
        st = ((RuntimeObjectPrefix *)obj)->state.bytes;
        if (on) {
            ((ActorScriptState *)st)->behaviorInhibitFlags |= 0x10;
        } else {
            ((ActorScriptState *)st)->behaviorInhibitFlags &= ~0x10;
        }
    } else if (((RuntimeObjectPrefix *)obj)->kind == 0x86) {
        st = ((RuntimeObjectPrefix *)obj)->state.bytes;
        flags = ((ItemStatePrefix *)st)->flags;
        if (!(flags & 0x100)) {
            return 0;
        }
        if (on) {
            ((ItemStatePrefix *)st)->flags = flags | 0x80000000;
        } else {
            ((ItemStatePrefix *)st)->flags = flags & 0x7FFFFFFF;
        }
    } else {
        return 0;
    }
    return on;
}

/// Acknowledge the actor pending script message.
/// \param actor Actor whose mailbox notification is acknowledged.
/// \returns The previous pending-message flag.
/// \note Original decompilation identifier: \c func_80119200 (0x80119200).
/// \note Clears the notification without erasing the stored message fields.
/// \see actorPostScriptMessage
s32 actorAcknowledgeScriptMessage(u8 *actor) {
    s32 previousPending;
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    previousPending = ((ActorScriptState *)actorState)->messagePending;
    ((ActorScriptState *)actorState)->messagePending = 0;
    return previousPending;
}

void actorAppearanceSetVariant(u8 *, s32, s32); /* extern */

/// Select both actor appearance variants and whether they persist across action changes.
/// \param actor Actor whose appearance is changed.
/// \param slot0Variant Model variant for appearance slot zero.
/// \param slot1Variant Model variant for appearance slot one.
/// \param preserve Nonzero to preserve the selected appearance across action changes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119210 (0x80119210).
void actorSetAppearanceVariants(u8 *actor, s32 slot0Variant, s32 slot1Variant, s32 preserve) {
    u8 *state = ((RuntimeObjectPrefix *)actor)->state.bytes;

    actorAppearanceSetVariant(actor, 0, slot0Variant);
    actorAppearanceSetVariant(actor, 1, slot1Variant);
    if (preserve != 0) {
        ((ActorScriptState *)state)->behaviorInhibitFlags |= 0x40;
    } else {
        ((ActorScriptState *)state)->behaviorInhibitFlags &= ~0x40;
    }
}

f32 terrainGetWorldHeightForQuery(s32, f32, f32, f32, u8 **); /* extern */

/// Query ground height at an offset along the actor heading.
/// \param actor Actor providing the position and heading.
/// \param distance Signed horizontal distance along the actor heading.
/// \param heightOffset Vertical offset from the actor position for the ground query.
/// \returns The queried ground height, or zero when no ground node is found.
/// \note Original decompilation identifier: \c func_80119288 (0x80119288).
f32 actorGetGroundHeightAhead(u8 *actor, f32 distance, f32 heightOffset) {
    u8 *state = ((RuntimeObjectPrefix *)actor)->state.bytes;
    f32 pos[3];

    pos[0] = ((RuntimeObjectPrefix *)actor)->worldPosition[0] +
             mathSinDegrees(((ActorScriptState *)state)->orientation[1]) * distance;
    pos[1] = ((RuntimeObjectPrefix *)actor)->worldPosition[1] + heightOffset;
    pos[2] = ((RuntimeObjectPrefix *)actor)->worldPosition[2] +
             mathCosDegrees(((ActorScriptState *)state)->orientation[1]) * distance;
    return terrainGetWorldHeightForQuery((((RuntimeObjectPrefix *)actor)->kind == 0x25) ? 3 : 4, pos[0], pos[1], pos[2], NULL);
}

f32 vector2NormalizeComponentsApprox(f32 *, f32 *); /* extern */
f32 mathUnitDirectionDegrees(f32, f32);         /* extern */
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern PlayerControlPrefix gPlayerState;
/// Separate symbol view of the primary player record's view yaw.
/// \note Original identifier: \c D_800EF6AC (0x800EF6AC).
extern f32 gPlayerYaw;
/// Separate symbol view of the primary player record's horizontal view shake.
/// \note Original identifier: \c D_800EF6B8 (0x800EF6B8).
extern f32 gPlayerViewShake;

/// Query the player heading relative to the direction toward an object.
/// \param object Object whose direction from the player is evaluated.
/// \returns Player heading minus the direction toward the object, wrapped to signed degrees.
/// \note Original decompilation identifier: \c func_80119338 (0x80119338).
f32 objectGetPlayerFacingError(u8 *object) {
    f32 sp24;
    f32 sp20;

    sp24 = ((RuntimeObjectPrefix *)object)->worldPosition[0] - ((PlayerControlPrefix *)&gPlayerState)->position[0];
    sp20 = ((RuntimeObjectPrefix *)object)->worldPosition[2] - ((PlayerControlPrefix *)&gPlayerState)->position[2];
    vector2NormalizeComponentsApprox(&sp24, &sp20);
    return mathWrapSignedDegrees((gPlayerYaw + gPlayerViewShake) - mathUnitDirectionDegrees(sp20, sp24));
}

/// Query an object heading relative to the direction toward the player.
/// \param object Object whose facing is evaluated.
/// \returns Object heading minus the direction toward the player, wrapped to signed degrees.
/// \note Original decompilation identifier: \c func_801193AC (0x801193AC).
f32 objectGetFacingErrorToPlayer(u8 *object) {
    f32 dx;
    f32 dz;

    dx = ((PlayerControlPrefix *)&gPlayerState)->position[0] - ((RuntimeObjectPrefix *)object)->worldPosition[0];
    dz = ((PlayerControlPrefix *)&gPlayerState)->position[2] - ((RuntimeObjectPrefix *)object)->worldPosition[2];
    vector2NormalizeComponentsApprox(&dx, &dz);
    return mathWrapSignedDegrees(((RuntimeObjectPrefix *)object)->rotation[1] - mathUnitDirectionDegrees(dz, dx));
}

/// Query an object heading relative to the direction toward a resolved target.
/// \param object Object whose facing is evaluated.
/// \param targetId Identifier resolved to the target object.
/// \returns Object heading minus the target direction in degrees from 0 inclusive to 360 exclusive, or -1 when unresolved.
/// \note Original decompilation identifier: \c func_8011941C (0x8011941C).
f32 objectGetFacingErrorToTarget(u8 *object, s32 targetId) {
    f32 dir;
    f32 sp28;
    f32 sp24;
    f32 var_f2;
    u8 *resolvedObject;

    resolvedObject = objectResolveAvailableHandle(targetId);
    if (resolvedObject == NULL) {
        return -1.0f;
    }
    sp28 =
        ((RuntimeObjectPrefix *)resolvedObject)->worldPosition[0] - ((RuntimeObjectPrefix *)object)->worldPosition[0];
    sp24 =
        ((RuntimeObjectPrefix *)resolvedObject)->worldPosition[2] - ((RuntimeObjectPrefix *)object)->worldPosition[2];
    vector2NormalizeComponentsApprox(&sp28, &sp24);
    dir = mathUnitDirectionDegrees(sp24, sp28);
    var_f2 = ((RuntimeObjectPrefix *)object)->rotation[1] - dir;
    if (var_f2 < 0.0f) {
        do {
            var_f2 += 360.0f;
        } while (var_f2 < 0.0f);
    }
    if (var_f2 >= 360.0f) {
        do {
            var_f2 -= 360.0f;
        } while (var_f2 >= 360.0f);
    }
    return var_f2;
}

typedef struct {
    s32 w[2];
} Pair32;

void scrollBoardSetHierarchyText(u8 *, s32, s32); /* extern */
/// Two preset text references available to scrolling-board hierarchies.
/// \note Original identifier: \c D_8012A6B0 (0x8012A6B0).
extern Pair32 gScrollBoardPresetTexts;

/// Select preset text for a scrolling board hierarchy.
/// \param handle Live object handle of the board hierarchy.
/// \param preset Text preset index, zero or one.
/// \param style Text style passed to the board.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119508 (0x80119508).
void scrollBoardSelectPresetText(s32 handle, s32 preset, s32 style) {
    Pair32 sp20 = gScrollBoardPresetTexts;

    scrollBoardSetHierarchyText(objectFindLiveByHandle(handle), sp20.w[preset], style);
}
