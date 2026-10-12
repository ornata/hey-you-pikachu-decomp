#include "actor_held_item.h"
#include "player_view_search.h"
#include "item_participation.h"
#include "item_queries.h"
#include "item_search.h"
#include "action_script_playback.h"
#include "camera_pose.h"
#include "action_query.h"
#include "interaction.h"
#include "object_target_search.h"
#include "path_query.h"
#include "scene_region.h"
#include "cave.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "runtime_clock.h"
#include "object_script_query.h"
#include "actor_target.h"
#include "player_control.h"
#include "object_lookup.h"
#include "actor_script.h"
#include "item_types.h"
#include "player_types.h"
#include "scene_types.h"
#include "object_types.h"



/// Find an eligible item of the requested kind near an actor's facing direction, honoring its interaction cooldown.
/// \param actor Actor supplying the reference position, facing, and cooldown.
/// \param kind Item kind to search for.
/// \param distance Inclusive maximum approximate search distance.
/// \returns The qualifying item kind, or zero during the cooldown or when no eligible item is found.
/// \note Original decompilation identifier: \c func_80117210 (0x80117210).
s16 actorFindInteractableItemKind(u8 *actor, s32 kind, f32 distance) {
    u8 *item;
    u8 *state;
    u8 *rec;

    state = ((RuntimeObjectPrefix *)actor)->state.bytes;
    if (runtimeGetElapsedMilliseconds() - ((ActorScriptState *)state)->lastInteractionMilliseconds < 500) {
        return 0;
    }
    item = itemFindNearest((f32 *)(actor + 0x44), distance, ((ActorScriptState *)state)->orientation[0],
                           ((ActorScriptState *)state)->orientation[1], 45.0f, kind, kind, 0, 0);
    if ((item == NULL) || (itemCanBePickedUp(item, 0) == 0)) {
        return 0;
    }
    rec = ((RuntimeObjectPrefix *)item)->state.bytes;
    return ((ItemStatePrefix *)rec)->kind;
}

f32 vector3LengthApprox(f32 *); /* extern */

/// Measure an object's distance from the world origin.
/// \param object Object whose world position is queried.
/// \returns The approximate three-dimensional distance from the world origin.
/// \note Original decompilation identifier: \c func_801172D4 (0x801172D4).
f32 objectGetOriginDistance(u8 *object) {
    f32 pos[3];

    pos[0] = ((RuntimeObjectPrefix *)object)->worldPosition[0];
    pos[1] = ((RuntimeObjectPrefix *)object)->worldPosition[1];
    pos[2] = ((RuntimeObjectPrefix *)object)->worldPosition[2];
    return vector3LengthApprox(pos);
}



/// Measure the distance between an object and an available target.
/// \param object Object providing the reference position.
/// \param handle Handle of the target object.
/// \returns The approximate three-dimensional distance, or -1 when the target is unavailable.
/// \note Original decompilation identifier: \c func_8011730C (0x8011730C).
f32 objectGetDistanceToHandle(u8 *object, s32 handle) {
    u8 *obj = objectResolveAvailableHandle(handle);
    f32 d[3];

    if (obj == NULL) {
        return -1.0f;
    }
    d[0] = ((RuntimeObjectPrefix *)obj)->worldPosition[0] - ((RuntimeObjectPrefix *)object)->worldPosition[0];
    d[1] = ((RuntimeObjectPrefix *)obj)->worldPosition[1] - ((RuntimeObjectPrefix *)object)->worldPosition[1];
    d[2] = ((RuntimeObjectPrefix *)obj)->worldPosition[2] - ((RuntimeObjectPrefix *)object)->worldPosition[2];
    return vector3LengthApprox(d);
}

/// Measure an available target's height relative to an object.
/// \param object Object providing the reference position.
/// \param handle Handle of the target object.
/// \returns The target height minus the object height, or zero when the target is unavailable.
/// \note Original decompilation identifier: \c func_80117388 (0x80117388).
f32 objectGetHeightDifferenceToHandle(u8 *object, s32 handle) {
    u8 *resolvedObject;

    resolvedObject = objectResolveAvailableHandle(handle);
    if (resolvedObject == NULL) {
        return 0.0f;
    }
    return ((RuntimeObjectPrefix *)resolvedObject)->worldPosition[1] -
           ((RuntimeObjectPrefix *)object)->worldPosition[1];
}

/// Measure the horizontal distance between an object and an available target.
/// \param object Object providing the reference position.
/// \param handle Handle of the target object.
/// \returns The approximate horizontal distance, or zero when the target is unavailable.
/// \note Original decompilation identifier: \c func_801173C8 (0x801173C8).
f32 objectGetHorizontalDistanceToHandle(u8 *object, s32 handle) {
    u8 *obj = objectResolveAvailableHandle(handle);
    f32 dir[3];

    if (obj == NULL) {
        return 0.0f;
    }
    dir[0] = ((RuntimeObjectPrefix *)obj)->worldPosition[0] - ((RuntimeObjectPrefix *)object)->worldPosition[0];
    dir[1] = 0.0f;
    dir[2] = ((RuntimeObjectPrefix *)obj)->worldPosition[2] - ((RuntimeObjectPrefix *)object)->worldPosition[2];
    return vector3LengthApprox(dir);
}

/// Measure the horizontal distance to an actor's stored target position.
/// \param actor Actor whose target distance is measured.
/// \returns The approximate horizontal distance to the target position.
/// \note Original decompilation identifier: \c func_80117434 (0x80117434).
f32 actorGetHorizontalTargetDistance(u8 *actor) {
    f32 dir[3];
    u8 *state = ((RuntimeObjectPrefix *)actor)->state.bytes;

    dir[0] = ((ActorScriptState *)state)->targetPosition[0] - ((RuntimeObjectPrefix *)actor)->worldPosition[0];
    dir[1] = 0.0f;
    dir[2] = ((ActorScriptState *)state)->targetPosition[2] - ((RuntimeObjectPrefix *)actor)->worldPosition[2];
    return vector3LengthApprox(dir);
}

/// Get one world-space coordinate of an actor's stored target position.
/// \param actor Actor whose target position is queried.
/// \param axis Nonnegative coordinate axis: zero for X, one for Y, or two for Z.
/// \returns The coordinate, or -65536 when the axis is greater than two.
/// \note Original decompilation identifier: \c func_80117480 (0x80117480).
f32 actorGetTargetCoordinate(u8 *actor, s32 axis) {
    u8 *rec = ((RuntimeObjectPrefix *)actor)->state.bytes;

    if (axis < 3) {
        return ((ActorScriptState *)rec)->targetPosition[axis];
    }
    return -65536.0f;
}

/// Get one coordinate of an object's world position.
/// \param object Object whose world position is queried.
/// \param axis Nonnegative coordinate axis: zero for X, one for Y, or two for Z.
/// \returns The coordinate, or -65536 when the axis is greater than two.
/// \note Original decompilation identifier: \c func_801174B0 (0x801174B0).
f32 objectGetWorldCoordinate(u8 *object, s32 axis) {
    if (axis < 3) {
        return ((f32 *)(object + 0x44))[axis];
    }
    return -65536.0f;
}

/// Get the elapsed time of an object's current action script.
/// \param object Object whose action script is queried.
/// \returns The elapsed script time in seconds.
/// \note Original decompilation identifier: \c func_801174DC (0x801174DC).
f32 actionScriptGetElapsedTime(u8 *object) {
    u8 *objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    return ((ActorScriptState *)objectState)->actionScriptElapsed;
}

/// Get the time limit of an object's current action script.
/// \param object Object whose action script is queried.
/// \returns The script time limit in seconds; a negative value indicates no limit.
/// \note Original decompilation identifier: \c func_801174E8 (0x801174E8).
f32 actionScriptGetDuration(u8 *object) {
    u8 *objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    return ((ActorScriptState *)objectState)->actionScriptDuration;
}

/// Get the identifier of an object's current action script.
/// \param object Object whose action script is queried.
/// \returns The current script identifier, or -1 when the script is stopped.
/// \note Original decompilation identifier: \c func_801174F4 (0x801174F4).
s32 actionScriptGetId(u8 *object) {
    u8 *objectState = ((RuntimeObjectPrefix *)object)->state.bytes;
    return ((ActorScriptState *)objectState)->actionScriptId;
}

/// Get the collision result of an actor's latest movement update.
/// \param actor Actor whose movement collision status is queried.
/// \returns Movement collision flags, or zero when no collision was reported.
/// \note Original decompilation identifier: \c func_80117500 (0x80117500).
u16 actorGetMovementCollisionFlags(u8 *actor) {
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    return (u16)((ActorScriptState *)actorState)->movementCollisionFlags;
}


/// Find an item near an actor within its facing cone and the requested filters.
/// \param actor Actor supplying the search origin and facing.
/// \param distance Inclusive maximum approximate distance from the search origin.
/// \param cone Maximum absolute horizontal angle in degrees; 180 or greater permits all directions.
/// \param minKind Inclusive minimum item kind; 165 allows the first item kind.
/// \param maxKind Inclusive maximum item kind; 165 allows the last item kind.
/// \param filter Required item state, or six to accept any state.
/// \param weighted Nonzero to prefer items with greater selection priority and reset the selected item's priority.
/// \returns The best qualifying item handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_8011750C (0x8011750C).
s32 actorFindItemHandleInFacingCone(u8 *actor, f32 distance, f32 cone, s32 minKind, s32 maxKind, s32 filter, s32 weighted) {
    u8 *rec = ((RuntimeObjectPrefix *)actor)->state.bytes;
    u8 *item = itemFindNearest((f32 *)(actor + 0x44), distance, ((ActorScriptState *)rec)->orientation[0],
                               ((ActorScriptState *)rec)->orientation[1], cone, minKind, maxKind, filter, weighted);

    if (item == NULL) {
        return 0;
    }
    return ((RuntimeObjectPrefix *)item)->handle;
}

/// Track an available object as an actor's movement destination. Otherwise disable target tracking.
/// \param actor Actor whose destination is changed.
/// \param handle Handle of the target object.
/// \param distanceAllowance Distance allowance used when approaching the destination.
/// \returns The target script category, or zero when the target is unavailable or has no supported category.
/// \note Original decompilation identifier: \c func_80117584 (0x80117584).
s32 actorTrackObjectPosition(u8 *actor, s32 handle, f32 distanceAllowance) {
    u8 *state = ((RuntimeObjectPrefix *)actor)->state.bytes;
    u8 *target = objectResolveAvailableHandle(handle);

    if ((target == NULL) || ((((RuntimeObjectPrefix *)target)->kind == 0x86) &&
                             (((ItemStatePrefix *)((RuntimeObjectPrefix *)target)->state.bytes)->state == 4))) {
        ((ActorScriptState *)state)->targetMode = 0;
        return 0;
    }
    ((ActorScriptState *)state)->targetMode = 1;
    ((ActorScriptState *)state)->targetHandle = handle;
    ((ActorScriptState *)state)->targetDistanceAllowance = distanceAllowance;
    actorSetTargetPosition(actor, ((RuntimeObjectPrefix *)target)->worldPosition);
    return objectGetScriptCategory(actor, handle);
}

/// Choose an available object's current position as an actor's destination and disable ongoing tracking.
/// \param actor Actor whose destination is changed.
/// \param handle Handle of the target object.
/// \param distanceAllowance Distance allowance used when approaching the destination.
/// \returns The target script category, or zero when the target is unavailable or has no supported category.
/// \note Original decompilation identifier: \c func_80117624 (0x80117624).
s32 actorSetDestinationFromObject(u8 *actor, s32 handle, f32 distanceAllowance) {
    u8 *targetObject;
    u8 *resolvedObject;
    u8 *actorState;

    resolvedObject = objectResolveAvailableHandle(handle);
    targetObject = resolvedObject;
    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ((ActorScriptState *)actorState)->targetMode = 0;
    if ((resolvedObject == NULL) ||
        ((((RuntimeObjectPrefix *)resolvedObject)->kind == 0x86) &&
         (((ItemStatePrefix *)((RuntimeObjectPrefix *)resolvedObject)->state.bytes)->state == 4))) {
        return 0;
    }
    ((ActorScriptState *)actorState)->targetDistanceAllowance = distanceAllowance;
    actorSetTargetPosition(actor, ((RuntimeObjectPrefix *)targetObject)->worldPosition);
    return objectGetScriptCategory(actor, handle);
}

/// Track an available object as the source of an actor's escape destination. Otherwise disable target tracking.
/// \param actor Actor whose destination is changed.
/// \param handle Handle of the target object.
/// \returns The target script category, or zero when the target is unavailable or has no supported category.
/// \note Original decompilation identifier: \c func_801176A8 (0x801176A8).
s32 actorTrackEscapeFromObject(u8 *actor, s32 handle) {
    u8 *sp1C;
    u8 *resolvedObject;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.bytes;
    resolvedObject = objectResolveAvailableHandle(handle);
    if ((resolvedObject == NULL) ||
        ((((RuntimeObjectPrefix *)resolvedObject)->kind == 0x86) &&
         (((ItemStatePrefix *)((RuntimeObjectPrefix *)resolvedObject)->state.bytes)->state == 4))) {
        ((ActorScriptState *)sp1C)->targetMode = 0;
        return 0;
    }
    ((ActorScriptState *)sp1C)->targetMode = 2;
    ((ActorScriptState *)sp1C)->targetHandle = handle;
    ((ActorScriptState *)sp1C)->targetDistanceAllowance = 0.0f;
    actorChooseEscapePosition(actor, ((RuntimeObjectPrefix *)resolvedObject)->worldPosition);
    return objectGetScriptCategory(actor, handle);
}

/// Choose an escape destination away from an available object and disable ongoing target tracking.
/// \param actor Actor whose destination is changed.
/// \param handle Handle of the target object.
/// \returns The target script category, or zero when the target is unavailable or has no supported category.
/// \note Original decompilation identifier: \c func_80117744 (0x80117744).
s32 actorChooseEscapePositionFromHandle(u8 *actor, s32 handle) {
    u8 *targetObject;
    u8 *resolvedObject;
    u8 *actorState;

    resolvedObject = objectResolveAvailableHandle(handle);
    targetObject = resolvedObject;
    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ((ActorScriptState *)actorState)->targetMode = 0;
    if ((resolvedObject == NULL) ||
        ((((RuntimeObjectPrefix *)resolvedObject)->kind == 0x86) &&
         (((ItemStatePrefix *)((RuntimeObjectPrefix *)resolvedObject)->state.bytes)->state == 4))) {
        return 0;
    }
    ((ActorScriptState *)actorState)->targetDistanceAllowance = 0.0f;
    actorChooseEscapePosition(actor, ((RuntimeObjectPrefix *)targetObject)->worldPosition);
    return objectGetScriptCategory(actor, handle);
}

/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern PlayerControlPrefix gPlayerState;

/// Choose an escape destination away from the player and disable ongoing target tracking.
/// \param actor Actor whose escape destination is selected.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801177C8 (0x801177C8).
void actorChooseEscapePositionFromPlayer(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ((ActorScriptState *)actorState)->targetMode = 0;
    ((ActorScriptState *)actorState)->targetDistanceAllowance = 0.0f;
    actorChooseEscapePosition(actor, (f32 *)&gPlayerState);
}


/// Get the handle of an actor's available target.
/// \param object Actor whose target is requested.
/// \returns The target handle, or zero when no target is available.
/// \note Original decompilation identifier: \c func_801177FC (0x801177FC).
s32 actorGetAvailableTargetHandle(u8 *object) {
    u8 *targetObject;

    targetObject = actorResolveStoredTarget(object);
    if (targetObject != NULL) {
        return ((RuntimeObjectPrefix *)targetObject)->handle;
    }
    return 0;
}

/// Get an actor's target tracking mode.
/// \param actor Actor whose tracking mode is requested.
/// \returns The current target tracking mode.
/// \note Original decompilation identifier: \c func_80117828 (0x80117828).
s32 actorGetTargetTrackingMode(u8 *actor) {
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    return ((ActorScriptState *)actorState)->targetMode;
}

/// Set how an actor tracks its target.
/// \param actor Actor whose tracking mode is changed.
/// \param mode Target tracking mode: zero disables tracking, one follows the target, and two moves away from it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80117834 (0x80117834).
void actorSetTargetTrackingMode(u8 *actor, s32 mode) {
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ((ActorScriptState *)actorState)->targetMode = mode;
}

/// Classify an available object for actor scripts.
/// \param actor Unused actor context accepted by the script interface.
/// \param handle Handle of the object to classify.
/// \returns One for items, two for object type 127, three for types 141 through 148, four for creatures, five for type 130, or zero for an unavailable or unsupported object.
/// \note Original decompilation identifier: \c func_80117840 (0x80117840).
s32 objectGetScriptCategory(u8 *actor, s32 handle) {
    u16 objectKind;
    u8 *resolvedObject;

    resolvedObject = objectResolveAvailableHandle(handle);
    if (resolvedObject == NULL) {
        return SCRIPT_CATEGORY_UNAVAILABLE;
    }
    objectKind = ((RuntimeObjectPrefix *)resolvedObject)->kind;
    if (objectKind == 0x86) {
        return SCRIPT_CATEGORY_ITEM;
    }
    if (objectKind == 0x7F) {
        return SCRIPT_CATEGORY_TYPE_127;
    }
    if (((s32)objectKind > 0) && ((s32)objectKind < 0x26)) {
        return SCRIPT_CATEGORY_CREATURE;
    }
    if (((s32)objectKind >= 0x82) && ((s32)objectKind < 0x83)) {
        return SCRIPT_CATEGORY_TYPE_130;
    }
    if (((s32)objectKind >= 0x8D) && ((s32)objectKind < 0x95)) {
        return SCRIPT_CATEGORY_TYPES_141_TO_148;
    }
    return SCRIPT_CATEGORY_UNAVAILABLE;
}

/// Get the kind identifier exposed to actor scripts for an available object.
/// \param handle Handle of the object to query.
/// \returns The item or creature kind, the object type for types 130 and 141 through 148, or zero for an unavailable or unsupported object.
/// \note Original decompilation identifier: \c func_801178EC (0x801178EC).
s32 objectGetScriptKind(s32 handle) {
    s32 objectKind;
    u8 *resolvedObject;

    resolvedObject = objectResolveAvailableHandle(handle);
    if (resolvedObject == NULL) {
        return 0;
    }
    objectKind = ((RuntimeObjectPrefix *)resolvedObject)->kind;
    if (objectKind == 0x86) {
        return itemGetKindByHandle(handle);
    }
    if ((objectKind > 0) && (objectKind < 0x26)) {
        return actorGetKindByHandle(handle);
    }
    if ((objectKind >= 0x82) && (objectKind < 0x83)) {
        return objectKind;
    }
    if ((objectKind >= 0x8D) && (objectKind < 0x95)) {
        return objectKind;
    }
    return 0;
}

/// Get the item kind associated with an available handle.
/// \param handle Handle of the item to query.
/// \returns The item kind, or zero if the handle is unavailable or does not identify an item.
/// \note Original decompilation identifier: \c func_80117998 (0x80117998).
s16 itemGetKindByHandle(s32 handle) {
    u8 *obj = objectResolveAvailableHandle(handle);
    u8 *rec;

    if (obj == NULL) {
        return 0;
    }
    if (((RuntimeObjectPrefix *)obj)->kind == 0x86) {
        rec = ((RuntimeObjectPrefix *)obj)->state.bytes;
        return ((ItemStatePrefix *)rec)->kind;
    }
    return 0;
}

/// Get the creature kind associated with an available handle.
/// \param handle Handle of the actor to query.
/// \returns The creature kind, or zero if the handle is unavailable or does not identify a creature.
/// \note Original decompilation identifier: \c func_801179E8 (0x801179E8).
u16 actorGetKindByHandle(s32 handle) {
    u16 objectKind;
    u8 *resolvedObject;

    resolvedObject = objectResolveAvailableHandle(handle);
    if (resolvedObject == NULL) {
        return 0U;
    }
    objectKind = ((RuntimeObjectPrefix *)resolvedObject)->kind;
    if (((s32)objectKind > 0) && ((s32)objectKind < 0x26)) {
        return objectKind;
    }
    return 0;
}

/// Get the type of an available object.
/// \param handle Handle of the object to query.
/// \returns The object type, or zero when the handle is unavailable.
/// \note Original decompilation identifier: \c func_80117A30 (0x80117A30).
u16 objectGetTypeByHandle(s32 handle) {
    u8 *resolvedObject;

    resolvedObject = objectResolveAvailableHandle(handle);
    if (resolvedObject == NULL) {
        return 0U;
    }
    return ((RuntimeObjectPrefix *)resolvedObject)->kind;
}

/// Get the bounding radius of an available object.
/// \param handle Handle of the object to query.
/// \returns The bounding radius, or zero when the handle is unavailable.
/// \note Original decompilation identifier: \c func_80117A64 (0x80117A64).
f32 objectGetBoundingRadiusByHandle(s32 handle) {
    u8 *resolvedObject;

    resolvedObject = objectResolveAvailableHandle(handle);
    if (resolvedObject == NULL) {
        return 0.0f;
    }
    return ((RuntimeObjectPrefix *)resolvedObject)->radius;
}

/// Set the default mode for an actor's actions that inherit it.
/// \param actor Actor whose default action mode is changed.
/// \param mode Default action mode representable as a signed 16-bit value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80117A94 (0x80117A94).
void actorSetDefaultActionMode(u8 *actor, s32 mode) {
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;

    ((ActorScriptState *)actorState)->defaultActionMode = mode;
}


/// Start an object's action script with a specified time limit.
/// \param object Address of the object that will execute the action script.
/// \param scriptId Valid identifier of the action script to start.
/// \param duration Time limit in seconds; a negative value removes the limit.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80117AA0 (0x80117AA0).
void actionScriptStartWithDurationBits(u8 *object, s32 scriptId, f32 duration) {
    actionScriptStart(object, scriptId);
    actionScriptSetDuration(object, duration);
}

/// Separate symbol view of the primary player record's held item handle.
/// \note Original identifier: \c D_800EF724 (0x800EF724).
extern s32 gPlayerHeldItemHandle;

/// Get the handle of the item held by the player.
/// \returns The stored held-item handle, or zero when no item is recorded.
/// \note Original decompilation identifier: \c func_80117AD0 (0x80117AD0).
s32 playerGetHeldItemHandle(void) {
    return gPlayerHeldItemHandle;
}


/// Consider an object for preferred interaction and test whether it lies in an interaction region.
/// \param object Object to evaluate as an interaction candidate.
/// \returns One when the object qualifies for either interaction region, or zero otherwise.
/// \note Original decompilation identifier: \c func_80117ADC (0x80117ADC).
s32 interactionConsiderObjectInRegion(u8 *object) {
    s32 interactionFlags;

    interactionFlags = interactionConsiderObject(object);
    if ((interactionFlags & 1) || (interactionFlags & 2)) {
        return 1;
    }
    return 0;
}

/// Separate symbol view of the primary player record's controller port.
/// \note Original identifier: \c D_800EF6CC (0x800EF6CC).
extern u8 gPlayerControllerPort[];

/// Select which player controls remain enabled.
/// \param mask Low sixteen input bits to retain; bits 64 and 128 also enable the horizontal and vertical analog axes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80117B14 (0x80117B14).
void playerSetInputKeepMask(s32 mask) {
    *(s16 *)(gPlayerControllerPort + 4) = mask;
}


/// Get the player input blocked by the current control mask.
/// \returns Blocked button bits, including bits 64 and 128 for sufficiently strong blocked analog input.
/// \note Original decompilation identifier: \c func_80117B20 (0x80117B20).
u16 playerGetBlockedInput(void) {
    return *(u16 *)(gPlayerControllerPort + 6) & 0xFFFF;
}


/// Request a player interaction mode targeting an actor.
/// \param actor Actor to use as the interaction target.
/// \param mode Requested mode: zero for off, one for Pikachu mode, or two for follow mode; player permissions can prevent the change.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80117B30 (0x80117B30).
void playerSetActorInteractionMode(u8 *actor, s32 mode) {
    playerSetCompanionControlMode((u8 *)&gPlayerState, mode, 0, ((RuntimeObjectPrefix *)actor)->handle);
}


/// Query the player's current interaction mode.
/// \returns The primary player companion-control selection.
/// \note Original decompilation identifier: \c func_80117B64 (0x80117B64).
s32 playerQueryInteractionMode(void) {
    return playerGetCompanionControlMode((u8 *)&gPlayerState);
}

/// Partial path-object state exposing the borrowed, sentinel-terminated points.
/// The complete allocation and the first three words remain unresolved.
typedef struct PathObjectStatePrefix {
    u8 unknown000[0xC];
    f32 *points; ///< Three-coordinate world points followed by an end marker.
} PathObjectStatePrefix;
typedef char PathObjectPointsOffsetCheck[(((u32) & ((PathObjectStatePrefix *)0)->points) == 0xC) ? 1 : -1];

/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

/// Get the number of points in a path in the current area.
/// \param path Valid path index in the current area.
/// \returns The number of points in the path.
/// \note Original decompilation identifier: \c func_80117B88 (0x80117B88).
s32 areaGetPathPointCount(s32 path) {
    return ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->paths[path].count;
}


/// Get one world-space coordinate of a path point in the current area.
/// \param path Valid path index in the current area.
/// \param point Nonnegative point index to query.
/// \param axis Nonnegative coordinate axis: zero for X, one for Y, or two for Z.
/// \returns The requested coordinate, or -65536 when the point or axis exceeds its valid range.
/// \note Original decompilation identifier: \c func_80117BA4 (0x80117BA4).
f32 areaGetPathPointCoordinate(s32 path, s32 point, s32 axis) {
    ScenePathEntry *entry;

    if (axis >= 3) {
        return -65536.0f;
    }
    entry = &((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->paths[path];
    if (point < entry->count) {
        return entry->points[point][axis];
    }
    return -65536.0f;
}




/// Find the nearest actor to a world position within a kind range.
/// \param object Object address to exclude from the search.
/// \param position Address of the three-component world-space search origin.
/// \param minKind Inclusive minimum actor kind; 38 selects the first creature kind.
/// \param maxKind Inclusive maximum actor kind; 38 selects the last creature kind.
/// \param distance Exclusive upper limit on approximate target distance.
/// \returns The nearest qualifying object handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_80117C18 (0x80117C18).
s32 actorFindNearestHandleFromPosition(u8 *object, f32 *position, s32 minKind, s32 maxKind, f32 distance) {
    u8 *spawned = actorFindNearestInRange(object, (Vec3f *)position, distance, ((RuntimeObjectPrefix *)object)->rotation, 360.0f, minKind, maxKind);

    if (spawned == NULL) {
        return 0;
    }
    return ((RuntimeObjectPrefix *)spawned)->handle;
}

/// Find an item near a world position using the requested kind and state filters.
/// \param position Three-component world-space search origin.
/// \param distance Inclusive maximum approximate distance from the search origin.
/// \param minKind Inclusive minimum item kind; 165 allows the first item kind.
/// \param maxKind Inclusive maximum item kind; 165 allows the last item kind.
/// \param filter Required item state, or six to accept any state.
/// \param weighted Nonzero to prefer items with greater selection priority and reset the selected item's priority.
/// \returns The best qualifying item handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_80117C74 (0x80117C74).
s32 itemFindHandleNearPosition(f32 *position, f32 distance, s32 minKind, s32 maxKind, s32 filter, s32 weighted) {
    u8 *item = itemFindNearest(position, distance, 0.0f, 0.0f, 360.0f, minKind, maxKind, filter, weighted);

    if (item == NULL) {
        return 0;
    }
    return ((RuntimeObjectPrefix *)item)->handle;
}



/// Find the nearest object within a facing cone and kind range.
/// \param object Object address supplying the search position and facing, excluded from the search.
/// \param minKind Inclusive minimum object kind.
/// \param maxKind Inclusive maximum object kind.
/// \param distance Exclusive upper limit on approximate target distance.
/// \param cone Maximum absolute horizontal angle in degrees; 360 or greater allows all directions.
/// \returns The nearest qualifying object handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_80117CEC (0x80117CEC).
s32 objectFindNearestHandleInFacingCone(u8 *object, s32 minKind, s32 maxKind, f32 distance, f32 cone) {
    u8 *spawned = objectFindNearestTargetInView(object, (Vec3f *)((RuntimeObjectPrefix *)object)->worldPosition, distance, ((RuntimeObjectPrefix *)object)->rotation, cone, minKind, maxKind);

    if (spawned == NULL) {
        return 0;
    }
    return ((RuntimeObjectPrefix *)spawned)->handle;
}



/// Find the nearest object to a world position within a kind range.
/// \param object Object address to exclude from the search.
/// \param position Address of the three-component world-space search origin.
/// \param minKind Inclusive minimum object kind.
/// \param maxKind Inclusive maximum object kind.
/// \param distance Exclusive upper limit on approximate target distance.
/// \returns The nearest qualifying object handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_80117D4C (0x80117D4C).
s32 objectFindNearestHandleFromPosition(u8 *object, f32 *position, s32 minKind, s32 maxKind, f32 distance) {
    u8 *spawned = objectFindNearestTargetInView(object, (Vec3f *)position, distance, ((RuntimeObjectPrefix *)object)->rotation, 360.0f, minKind, maxKind);

    if (spawned == NULL) {
        return 0;
    }
    return ((RuntimeObjectPrefix *)spawned)->handle;
}



/// Set an available object's uniform scale relative to its area scaling convention.
/// \param handle Handle of the object to resize; unavailable handles have no effect. Area-scaled objects require an initialized area with nonzero scale.
/// \param scale Requested uniform scale; the object must have a nonzero current scale.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80117DA8 (0x80117DA8).
void objectSetUniformScaleByHandle(s32 handle, f32 scale) {
    u8 *obj = objectResolveAvailableHandle(handle);
    f32 s;
    f32 old;
    f32 k;

    if (obj != NULL) {
        if (((RuntimeObjectPrefix *)obj)->flags & 4) {
            k = ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale;
            s = k * scale;
            old = ((RuntimeObjectPrefix *)obj)->scale[0] / k;
        } else {
            s = scale;
            old = ((RuntimeObjectPrefix *)obj)->scale[0];
        }
        ((RuntimeObjectPrefix *)obj)->scale[0] = s;
        ((RuntimeObjectPrefix *)obj)->scale[1] = s;
        ((RuntimeObjectPrefix *)obj)->scale[2] = s;
        ((RuntimeObjectPrefix *)obj)->radius = ((RuntimeObjectPrefix *)obj)->radius * scale / old;
    }
}


/// Get an available object's uniform scale relative to its area scaling convention.
/// \param handle Handle of the object to query.
/// \returns The uniform scale, or zero when the handle is unavailable.
/// \pre Area-scaled objects require an initialized area with nonzero scale.
/// \note Original decompilation identifier: \c func_80117E24 (0x80117E24).
f32 objectGetUniformScaleByHandle(s32 handle) {
    f32 var_f2;
    u8 *resolvedObject;

    resolvedObject = objectResolveAvailableHandle(handle);
    if (resolvedObject == NULL) {
        return 0.0f;
    }
    if (((RuntimeObjectPrefix *)resolvedObject)->flags & 4) {
        var_f2 = ((RuntimeObjectPrefix *)resolvedObject)->scale[0] /
                 ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale;
    } else {
        var_f2 = ((RuntimeObjectPrefix *)resolvedObject)->scale[0];
    }
    return var_f2;
}

/// Get the active rotation speed of a voice-reactive spinner.
/// \param handle Handle of the spinner object to query.
/// \returns Rotation speed in degrees per second, or zero when the handle is unavailable, the object is not a spinner, or its speed is nonpositive.
/// \note Original decompilation identifier: \c func_80117E80 (0x80117E80).
f32 voiceReactiveSpinnerGetSpeedByHandle(s32 handle) {
    u16 objectKind;
    f32 *spinnerState;
    f32 rotationSpeed;
    u8 *resolvedObject;

    resolvedObject = objectResolveAvailableHandle(handle);
    if (resolvedObject == NULL) {
        return 0.0f;
    }
    objectKind = ((RuntimeObjectPrefix *)resolvedObject)->kind;
    if (objectKind == 0x82) {
        spinnerState = ((RuntimeObjectPrefix *)resolvedObject)->state.floats;
        rotationSpeed = *spinnerState;
        if (rotationSpeed > 0.0f) {
            return rotationSpeed;
        }
    }
    return 0.0f;
}



/// Get the compact action status exposed to actor scripts for a creature.
/// \param handle Handle of the creature whose current action is queried.
/// \returns The script action status flags, or zero when the handle is not live or does not identify a creature.
/// \note Original decompilation identifier: \c func_80117EE8 (0x80117EE8).
u16 actorGetScriptActionFlagsByHandle(s32 handle) {
    u8 *obj;
    s32 fl;
    u16 bits;

    bits = 0;
    obj = objectFindLiveByHandle(handle);
    if (obj == NULL) {
        return 0;
    }
    if ((((RuntimeObjectPrefix *)obj)->kind > 0) && (((RuntimeObjectPrefix *)obj)->kind < 0x26)) {
        fl = actionGetAttributes(obj);
        if (fl & 0x200) {
            bits = 0x10;
        }
        if (fl & 0x100) {
            bits += 8;
        }
        if (fl & 0xF0) {
            if (fl & 0x800) {
                bits += 2;
            } else {
                bits += 4;
            }
        }
        if (fl & 0x20000000) {
            bits += 0x20;
        }
        if (fl & 0x400) {
            bits += 1;
        }
        if (fl & 0x800) {
            bits += 0x40;
        }
    }
    return bits;
}

/// Get one world-space coordinate of an available object.
/// \param handle Handle of the object to query.
/// \param axis Nonnegative coordinate axis: zero for X, one for Y, or two for Z.
/// \returns Zero when the handle is unavailable; otherwise the coordinate, or -65536 when the axis is greater than two.
/// \note Original decompilation identifier: \c func_80117FD4 (0x80117FD4).
f32 objectGetWorldCoordinateByHandle(s32 handle, s32 axis) {
    u8 *resolvedObject;

    resolvedObject = objectResolveAvailableHandle(handle);
    if (resolvedObject == NULL) {
        return 0.0f;
    }
    if (axis < 3) {
        return ((f32 *)(resolvedObject + 0x44))[axis];
    }
    return -65536.0f;
}

f32 mathSinDegrees(f32);                /* extern */
f32 mathCosDegrees(f32);                /* extern */
f32 terrainGetWorldHeight(f32, f32, f32, s32); /* extern */

/// Get one coordinate of a ground position offset from an actor's heading.
/// \param actor Actor supplying the reference position and heading.
/// \param distance Horizontal distance from the actor.
/// \param angle Heading offset in degrees.
/// \param axis Coordinate axis: zero for X, one for ground height, or two for Z.
/// \returns The requested coordinate of the offset position.
/// \note Original decompilation identifier: \c func_8011802C (0x8011802C).
f32 actorGetGroundOffsetCoordinate(u8 *actor, f32 distance, f32 angle, s32 axis) {
    u8 *state;
    f32 p[3];
    f32 q[3];

    state = ((RuntimeObjectPrefix *)actor)->state.bytes;
    p[0] = ((RuntimeObjectPrefix *)actor)->worldPosition[0] +
           mathSinDegrees(((ActorScriptState *)state)->orientation[1] + angle) * distance;
    p[1] = ((RuntimeObjectPrefix *)actor)->worldPosition[1];
    p[2] = ((RuntimeObjectPrefix *)actor)->worldPosition[2] +
           mathCosDegrees(((ActorScriptState *)state)->orientation[1] + angle) * distance;
    q[0] = p[0];
    q[1] = terrainGetWorldHeight(p[0], p[1], p[2], 0);
    q[2] = p[2];
    return q[axis];
}

void terrainGetSurfaceAttributes(f32, f32, f32, s32); /* extern */


/// Query the ground material at an offset along an actor's heading.
/// \param actor Actor supplying the reference position and heading.
/// \param distance Horizontal distance along the actor's heading.
/// \param heightOffset Vertical offset from the actor's position.
/// \returns Nothing; the queried material is discarded.
/// \note Original decompilation identifier: \c func_801180E8 (0x801180E8).
void actorProbeGroundMaterialAhead(u8 *actor, f32 distance, f32 heightOffset) {
    u8 *state = ((RuntimeObjectPrefix *)actor)->state.bytes;

    terrainGetSurfaceAttributes(((RuntimeObjectPrefix *)actor)->worldPosition[0] +
                                    mathSinDegrees(((ActorScriptState *)state)->orientation[1]) * distance,
                                ((RuntimeObjectPrefix *)actor)->worldPosition[1] + heightOffset,
                                ((RuntimeObjectPrefix *)actor)->worldPosition[2] +
                                    mathCosDegrees(((ActorScriptState *)state)->orientation[1]) * distance,
                                0);
}

/// Query whether a scene region is highlighted.
/// \param handle Handle of a scene region with initialized region state; other object kinds are unsupported.
/// \returns The highlight flag, or zero when the handle is not live.
/// \note Original decompilation identifier: \c func_8011816C (0x8011816C).
u8 sceneRegionIsHighlightedByHandle(s32 handle) {
    u8 *liveObject;
    u8 *regionState;

    liveObject = objectFindLiveByHandle(handle);
    if (liveObject == NULL) {
        return 0;
    }
    regionState = ((RuntimeObjectPrefix *)liveObject)->state.bytes;
    return ((SceneRegionState *)regionState)->highlighted;
}

/// Get one coordinate of the first point in a path object.
/// \param handle Handle of a path object with at least one initialized point.
/// \param axis Nonnegative coordinate axis: zero for X, one for Y, or two for Z.
/// \returns Zero when the handle is not live; otherwise the first point coordinate, or -65536 for a non-path object or an axis greater than two.
/// \note Original decompilation identifier: \c func_801181A4 (0x801181A4).
f32 pathGetFirstPointCoordinateByHandle(s32 handle, s32 axis) {
    u8 *obj = objectFindLiveByHandle(handle);
    u8 *rec;

    if (obj == NULL) {
        return 0.0f;
    }
    if (((RuntimeObjectPrefix *)obj)->kind == 0xAF) {
        rec = ((RuntimeObjectPrefix *)obj)->state.bytes;
        if (axis < 3) {
            return ((PathObjectStatePrefix *)rec)->points[axis];
        }
    }
    return -65536.0f;
}

typedef struct PathVector {
    f32 v[3];
} PathVector;
f32 vector3Length(f32 *);
/// Get a coordinate of the path destination following a waypoint near an actor.
/// \param object Actor whose world position determines the nearby waypoint.
/// \param handle Handle of a path object with at least one waypoint and a complete three-coordinate end marker whose X coordinate is negative.
/// \param axis Nonnegative coordinate axis: zero for X, one for Y, or two for Z.
/// \returns The coordinate following the first waypoint within three ground-distance units, or an end-marker coordinate when none qualifies; zero for a missing handle, or -65536 for a non-path object or an axis greater than two.
/// \note Portability: The adjacent coordinate-difference statements preserve the IDO scheduling required by the ROM.
/// \note Original decompilation identifier: \c func_80118210 (0x80118210).
f32 actorGetFollowingPathPointCoordinate(u8 *object, s32 handle, s32 axis) {
    PathObjectStatePrefix *pathState;
    s32 offset;
    s32 index;
    PathVector delta;
    PathVector waypoint;
    PathVector position;
    u8 *pathObject;
    pathObject = objectFindLiveByHandle(handle);
    if (pathObject == NULL)
        return 0.0f;
    index = 0;
    offset = 0;
    if (((RuntimeObjectPrefix *)pathObject)->kind == 0xAF) {
        pathState = ((RuntimeObjectPrefix *)pathObject)->state.pointer;
        for (;;) {
            waypoint.v[0] = *(f32 *)((u8 *)pathState->points + offset);
            waypoint.v[1] = *(f32 *)((u8 *)pathState->points + offset + 4);
            waypoint.v[2] = *(f32 *)((u8 *)pathState->points + offset + 8);
            position.v[0] = ((RuntimeObjectPrefix *)object)->worldPosition[0];
            position.v[1] = ((RuntimeObjectPrefix *)object)->worldPosition[1];
            position.v[2] = ((RuntimeObjectPrefix *)object)->worldPosition[2];
            waypoint.v[1] = terrainGetWorldHeight(waypoint.v[0], waypoint.v[1], waypoint.v[2], 0);
            position.v[1] = terrainGetWorldHeight(position.v[0], position.v[1], position.v[2], 0);
            // clang-format off
            delta.v[0] = waypoint.v[0] - position.v[0]; delta.v[1] = waypoint.v[1] - position.v[1]; delta.v[2] = waypoint.v[2] - position.v[2];
            // clang-format on
            offset += 12;
            index++;
            if (*(f32 *)((u8 *)pathState->points + offset) < 0.0f)
                break;
            if (vector3Length(&delta.v[0]) <= 3.0f)
                break;
        }
        if (axis < 3)
            return *(f32 *)((u8 *)pathState->points + index * 12 + axis * 4);
    }
    return -65536.0f;
}


/// Get one coordinate of a cave's first scene anchor.
/// \param handle Handle of a cave object; other object kinds are unsupported.
/// \param axis Nonnegative coordinate axis: zero for X, one for Y, or two for Z.
/// \returns Zero when the handle is not live; otherwise the anchor coordinate, or -65536 when the axis is greater than two.
/// \note Original decompilation identifier: \c func_801183CC (0x801183CC).
f32 caveGetFirstAnchorCoordinate(s32 handle, s32 axis) {
    u8 *liveObject;
    CaveState *v;

    liveObject = objectFindLiveByHandle(handle);
    if (liveObject == NULL) {
        return 0.0f;
    }
    v = ((RuntimeObjectPrefix *)liveObject)->state.pointer;
    if (axis < 3) {
        return v->firstAnchor[axis];
    }
    return -65536.0f;
}

/// Get one coordinate of a cave's second scene anchor.
/// \param handle Handle of a cave object; other object kinds are unsupported.
/// \param axis Nonnegative coordinate axis: zero for X, one for Y, or two for Z.
/// \returns Zero when the handle is not live; otherwise the anchor coordinate, or -65536 when the axis is greater than two.
/// \note Original decompilation identifier: \c func_80118428 (0x80118428).
f32 caveGetSecondAnchorCoordinate(s32 handle, s32 axis) {
    u8 *obj = objectFindLiveByHandle(handle);
    u8 *rec;

    if (obj == NULL) {
        return 0.0f;
    }
    rec = ((RuntimeObjectPrefix *)obj)->state.bytes;
    if (axis < 3) {
        return ((CaveState *)rec)->secondAnchor[axis];
    }
    return -65536.0f;
}


/// Get the landmark associated with a cave.
/// \param handle Handle of a cave with a valid linked landmark; other object kinds are unsupported.
/// \returns The linked landmark handle, or zero when the cave handle is not live.
/// \note Original decompilation identifier: \c func_80118484 (0x80118484).
s32 caveGetLinkedLandmarkHandle(s32 handle) {
    u8 *caveState;
    u8 *liveObject;

    liveObject = objectFindLiveByHandle(handle);
    if (liveObject == NULL) {
        return 0;
    }
    caveState = ((RuntimeObjectPrefix *)liveObject)->state.bytes;
    return ((RuntimeObjectPrefix *)((CaveState *)caveState)->landmark)->handle;
}

/// Find an item in the player viewing cone using the requested filters.
/// \param minKind Inclusive minimum item kind; 165 allows the first item kind.
/// \param maxKind Inclusive maximum item kind; 165 allows the last item kind.
/// \param distance Inclusive maximum approximate distance from the search origin.
/// \param cone Maximum absolute horizontal angle in degrees; 180 or greater permits all directions.
/// \param filter Required item state, or six to accept any state.
/// \param weighted Nonzero to prefer items with greater selection priority and reset the selected item's priority.
/// \returns The best qualifying item handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_801184C0 (0x801184C0).
s32 itemFindHandleFromPlayerView(s32 minKind, s32 maxKind, f32 distance, f32 cone, s32 filter, s32 weighted) {
    u8 *item =
        itemFindNearest((f32 *)&gPlayerState, distance, 0.0f,
                        ((PlayerControlPrefix *)&gPlayerState)->yaw + ((PlayerControlPrefix *)&gPlayerState)->viewShake,
                        cone, minKind, maxKind, filter, weighted);

    if (item == NULL) {
        return 0;
    }
    return ((RuntimeObjectPrefix *)item)->handle;
}

/// Find the nearest actor in the player viewing cone, honoring script actor-kind aliases.
/// \param object Object excluded from the search, or null.
/// \param minKind Inclusive minimum kind; 38 selects the first creature kind.
/// \param maxKind Inclusive maximum kind; 38 selects Pikachu.
/// \param distance Exclusive maximum approximate distance from the search origin.
/// \param cone Maximum horizontal angle in degrees; 360 or greater permits all directions.
/// \returns The nearest qualifying actor handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_80118544 (0x80118544).
s32 actorFindNearestFromPlayerView(u8 *object, s32 minKind, s32 maxKind, f32 distance, f32 cone) {
    f32 *origin;
    u8 *result;
    f32 rotation[3];
    origin = (f32 *)&gPlayerState;
    rotation[1] = origin[6] + origin[3];
    rotation[0] = 0.0f;
    rotation[2] = 0.0f;
    result = actorFindNearestInRange(object, (Vec3f *)origin, distance, rotation, cone, minKind, maxKind);
    if (result == NULL)
        return 0;
    return ((RuntimeObjectPrefix *)result)->handle;
}

/// Find the nearest object in the player viewing cone.
/// \param self Object excluded from the search, or null.
/// \param minKind Inclusive minimum kind.
/// \param maxKind Inclusive maximum kind.
/// \param distance Exclusive maximum approximate distance from the search origin.
/// \param cone Maximum horizontal angle in degrees; 360 or greater permits all directions.
/// \returns The nearest qualifying object handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_801185C4 (0x801185C4).
s32 objectFindNearestFromPlayerView(u8 *self, s32 minKind, s32 maxKind, f32 distance, f32 cone) {
    f32 *origin;
    u8 *item;
    f32 rotation[3];
    origin = (f32 *)&gPlayerState;
    rotation[1] = origin[6] + origin[3];
    rotation[0] = 0.0f;
    rotation[2] = 0.0f;
    item = objectFindNearestTargetInView(self, (Vec3f *)origin, distance, rotation, cone, minKind, maxKind);
    if (item == NULL) {
        return 0;
    }
    return ((RuntimeObjectPrefix *)item)->handle;
}

/// Set the actor's stored target position.
/// \param actor Actor whose target position is changed.
/// \param position Three world-space coordinates to copy; the pointer is not retained.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118644 (0x80118644).
void actorSetTargetPosition(u8 *actor, f32 *position) {
    ((RuntimeObjectPrefix *)actor)->state.actor->targetPosition[0] = position[0];
    ((RuntimeObjectPrefix *)actor)->state.actor->targetPosition[1] = position[1];
    ((RuntimeObjectPrefix *)actor)->state.actor->targetPosition[2] = position[2];
}

f32 vector3Normalize(u8 *);
u32 randomBelow(s32);
/// Choose a target position away from the supplied threat position.
/// \param object Actor whose escape target is changed; its fallback target handle must be live when the positions coincide.
/// \param target Three-component world-space threat position; the pointer is not retained.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011866C (0x8011866C).
void actorChooseEscapePosition(u8 *object, f32 *target) {
    f32 rotated[3];
    f32 direction[3];
    u8 *state;
    f32 distance;
    f32 angle;
    f32 trig;
    state = ((RuntimeObjectPrefix *)object)->state.bytes;
    direction[0] = target[0] - ((RuntimeObjectPrefix *)object)->worldPosition[0];
    direction[1] = target[1] - ((RuntimeObjectPrefix *)object)->worldPosition[1];
    direction[2] = target[2] - ((RuntimeObjectPrefix *)object)->worldPosition[2];
    if (vector3Normalize((u8 *)direction) == 0.0f) {
        actorSetTargetToFallbackObject(object);
        return;
    }
    distance = *(f32 *)gCurrentAreaConfiguration * 4.0f;
    angle = (s32)(randomBelow(60) - 15);
    trig = mathCosDegrees(angle);
    rotated[0] = direction[0] * trig - direction[2] * mathSinDegrees(angle);
    rotated[1] = direction[1];
    trig = mathSinDegrees(angle);
    rotated[2] = direction[0] * trig + direction[2] * mathCosDegrees(angle);
    rotated[0] *= distance;
    rotated[1] *= distance;
    rotated[2] *= distance;
    ((ActorScriptState *)state)->targetPosition[0] = ((RuntimeObjectPrefix *)object)->worldPosition[0] - rotated[0];
    ((ActorScriptState *)state)->targetPosition[1] = ((RuntimeObjectPrefix *)object)->worldPosition[1] - rotated[1];
    ((ActorScriptState *)state)->targetPosition[2] = ((RuntimeObjectPrefix *)object)->worldPosition[2] - rotated[2];
}

/// Measure the distance from the actor's local position to its stored target position.
/// \param actor Actor whose target distance is queried.
/// \returns The approximate three-dimensional distance to the target position.
/// \note Original decompilation identifier: \c func_801187E4 (0x801187E4).
f32 actorGetTargetDistance(u8 *actor) {
    f32 dir[3];
    u8 *state = ((RuntimeObjectPrefix *)actor)->state.bytes;

    dir[0] = ((ActorScriptState *)state)->targetPosition[0] - ((RuntimeObjectPrefix *)actor)->position[0];
    dir[1] = ((ActorScriptState *)state)->targetPosition[1] - ((RuntimeObjectPrefix *)actor)->position[1];
    dir[2] = ((ActorScriptState *)state)->targetPosition[2] - ((RuntimeObjectPrefix *)actor)->position[2];
    return vector3LengthApprox(dir);
}

/// Measure the distance from an object's local position to the player position.
/// \param object Object whose distance is queried.
/// \returns The approximate three-dimensional distance to the player.
/// \note Original decompilation identifier: \c func_80118838 (0x80118838).
f32 objectGetPlayerDistance(u8 *object) {
    f32 dir[3];
    f32 *target = (f32 *)&gPlayerState;
    f32 *pos = (f32 *)(object + 0x18);

    dir[0] = target[0] - pos[0];
    dir[1] = target[1] - pos[1];
    dir[2] = target[2] - pos[2];
    return vector3LengthApprox(dir);
}

/// Set an actor's target position to its fallback object's current position.
/// \param actor Actor with a live fallback object whose position supplies the destination.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118890 (0x80118890).
void actorSetTargetToFallbackObject(u8 *actor) {
    u8 *sp1C;
    u8 *liveObject;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.bytes;
    liveObject = objectFindLiveByHandle(((ActorScriptState *)sp1C)->fallbackObjectHandle);
    ((ActorScriptState *)sp1C)->targetPosition[0] = (f32)((RuntimeObjectPrefix *)liveObject)->worldPosition[0];
    ((ActorScriptState *)sp1C)->targetPosition[1] = (f32)((RuntimeObjectPrefix *)liveObject)->worldPosition[1];
    ((ActorScriptState *)sp1C)->targetPosition[2] = (f32)((RuntimeObjectPrefix *)liveObject)->worldPosition[2];
}

/// Query the kind of item held by an actor.
/// \param actor Actor with initialized state whose held handle identifies an item when live.
/// \returns The held item kind, or zero when no live held item is found.
/// \note Original decompilation identifier: \c func_801188D4 (0x801188D4).
s16 actorGetHeldItemKind(u8 *actor) {
    u8 *liveObject;
    u8 *itemState;
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;

    liveObject = objectFindLiveByHandle(((ActorScriptState *)actorState)->heldItemHandle);
    if (liveObject == NULL) {
        return 0;
    }
    itemState = ((RuntimeObjectPrefix *)liveObject)->state.bytes;
    return ((ItemStatePrefix *)itemState)->kind;
}

/// Check whether an item has been marked as picked up by the player.
/// \param item Initialized item whose pickup history is queried.
/// \returns One when player pickup is recorded, or zero otherwise.
/// \note Original decompilation identifier: \c func_80118910 (0x80118910).
s32 itemHasPlayerPickupHistory(u8 *item) {
    ItemStatePrefix *itemState = (ItemStatePrefix *)((RuntimeObjectPrefix *)item)->state.bytes;
    if (itemState->flags & 0x02000000) {
        return 1;
    }
    return 0;
}


/// Force release of the actor's held item and report its kind.
/// \param actor Actor with initialized state whose held handle identifies an item when live.
/// \returns The resolved item kind, or zero when no live held item is found.
///
/// \note Portability: The unused padding array preserves the stack-frame size required by the ROM.
/// \note Original decompilation identifier: \c func_80118938 (0x80118938).
s16 actorReleaseHeldItem(u8 *actor) {
    s32 pad[2];
    u8 *sp1C;
    u8 *liveObject;
    u8 *actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;

    liveObject = objectFindLiveByHandle(((ActorScriptState *)actorState)->heldItemHandle);
    if (liveObject == NULL) {
        return 0;
    }
    sp1C = ((RuntimeObjectPrefix *)liveObject)->state.bytes;
    itemTryRelease(liveObject, 1);
    return ((ItemStatePrefix *)sp1C)->kind;
}



/// Find the nearest actor in an object facing cone, honoring script actor-kind aliases.
/// \param object Live object providing the world-space search origin and facing, excluded from the search.
/// \param minKind Inclusive minimum kind.
/// \param maxKind Inclusive maximum kind.
/// \param distance Exclusive maximum approximate distance from the search origin.
/// \param cone Maximum horizontal angle in degrees; 360 or greater permits all directions.
/// \returns The nearest qualifying actor handle, or zero when none is found.
/// \note Original decompilation identifier: \c func_80118984 (0x80118984).
s32 actorFindNearestHandleInFacingCone(u8 *object, s32 minKind, s32 maxKind, f32 distance, f32 cone) {
    u8 *spawned = actorFindNearestInRange(object, (Vec3f *)((RuntimeObjectPrefix *)object)->worldPosition, distance, ((RuntimeObjectPrefix *)object)->rotation, cone, minKind, maxKind);

    if (spawned == NULL) {
        return 0;
    }
    return ((RuntimeObjectPrefix *)spawned)->handle;
}

/// Separate symbol view of the primary player record's mode and camera-transition state.
/// \note Original identifier: \c D_800EF728 (0x800EF728).
extern s32 gPlayerModeState[];

/// Select the player's secondary object as an actor's tracked escape source.
/// \param actor Actor whose tracked target and escape mode are changed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801189E4 (0x801189E4).
void actorTrackEscapeFromPlayerSecondaryObject(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ((ActorScriptState *)actorState)->targetHandle = gPlayerModeState[1];
    ((ActorScriptState *)actorState)->targetMode = 2;
}


/// Select the player's primary object as an actor's tracked movement target.
/// \param actor Actor whose tracked target and follow mode are changed.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118A00 (0x80118A00).
void actorTrackPlayerPrimaryObject(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ((ActorScriptState *)actorState)->targetHandle = gPlayerModeState[0];
    ((ActorScriptState *)actorState)->targetMode = 1;
}

f32 scriptVmPopFloat(s32);                   /* extern */

/// Set the primary camera position and look-at target from script arguments.
/// \param vm Script VM containing position X, Y, Z followed by target X, Y, Z, with target Z on top of the stack.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118A1C (0x80118A1C).
void scriptVmSetPrimaryCameraView(s32 vm) {
    f32 b[3];
    f32 a[3];

    a[2] = scriptVmPopFloat(vm);
    a[1] = scriptVmPopFloat(vm);
    a[0] = scriptVmPopFloat(vm);
    b[2] = scriptVmPopFloat(vm);
    b[1] = scriptVmPopFloat(vm);
    b[0] = scriptVmPopFloat(vm);
    cameraSetEyeAndTarget(0, b, a);
}

/// Place the actor at its stored target position.
/// \param actor Actor to reposition.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80118A94 (0x80118A94).
void actorMoveToTargetPosition(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    ((RuntimeObjectPrefix *)actor)->position[0] = (f32)((ActorScriptState *)actorState)->targetPosition[0];
    ((RuntimeObjectPrefix *)actor)->position[1] = (f32)((ActorScriptState *)actorState)->targetPosition[1];
    ((RuntimeObjectPrefix *)actor)->position[2] = (f32)((ActorScriptState *)actorState)->targetPosition[2];
    ((RuntimeObjectPrefix *)actor)->worldPosition[0] = (f32)((ActorScriptState *)actorState)->targetPosition[0];
    ((RuntimeObjectPrefix *)actor)->worldPosition[1] = (f32)((ActorScriptState *)actorState)->targetPosition[1];
    ((RuntimeObjectPrefix *)actor)->worldPosition[2] = (f32)((ActorScriptState *)actorState)->targetPosition[2];
}

/// Resolve a live object handle while excluding unavailable items.
/// \param handle Object handle to resolve, or zero for no object.
/// \returns A borrowed available-object pointer, or null when absent or unavailable.
/// \note Original decompilation identifier: \c func_80118ACC (0x80118ACC).
u8 *objectResolveAvailableHandle(s32 handle) {
    u8 *liveObject;

    if (handle == 0) {
        return NULL;
    }
    liveObject = objectFindLiveByHandle(handle);
    if ((liveObject != NULL) && (((RuntimeObjectPrefix *)liveObject)->kind == 0x86) &&
        (((ItemStatePrefix *)((RuntimeObjectPrefix *)liveObject)->state.bytes)->state == 4)) {
        return NULL;
    }
    return liveObject;
}
