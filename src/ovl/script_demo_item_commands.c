#include "player_state.h"
#include "title_screen.h"
#include "item_object.h"
#include "particle_cluster.h"
#include "effect_emitter.h"
#include "item_participation.h"
#include "item_queries.h"
#include "action_continuation.h"
#include "player_friendship.h"
#include "common.h"
#include "object_lookup.h"
#include "actor_script.h"
#include "object_types.h"
#include "player_types.h"
#include "script_vm_stack.h"
#include "scene_types.h"

/// Begin the title-screen demo transition and provide a zero script result.
/// \param object Actor whose script receives a zero result; no script arguments are required.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121340 (0x80121340).
s32 scriptCommandBeginDemoTransition(u8 *object) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    titleScreenBeginDemoTransition();
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Provide the current title-screen demo index as the script result.
/// \param object Actor whose script receives the demo index; no script arguments are required.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121378 (0x80121378).
s32 scriptCommandGetDemoIndex(u8 *object) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(vmAddress), titleScreenGetDemoIndex());
    return 0;
}

#include "area_services.h"
#include "scene_configuration.h"
/// Remove the maze block identified by the script handle argument, with visual effects, and provide a zero script result.
/// \param object Actor whose script supplies the target block handle and receives zero,
/// including when the handle does not identify a live maze block.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_801213B0 (0x801213B0).
s32 scriptCommandRemoveMazeBlock(u8 *object) {
    s32 vmAddress;
    s32 blockHandle;
    f32 edgeX;
    f32 edgeZ;
    f32 center[3];
    f32 edge[3];
    u8 *block;
    vmAddress = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    blockHandle = scriptVmPopInteger((ScriptVm *)(vmAddress));
    block = objectFindLiveByHandle(blockHandle);
    if (block != NULL && ((RuntimeObjectPrefix *)block)->kind == 0x88) {
        center[0] = ((RuntimeObjectPrefix *)block)->worldPosition[0];
        center[1] = ((RuntimeObjectPrefix *)block)->worldPosition[1];
        center[2] = ((RuntimeObjectPrefix *)block)->worldPosition[2];
        center[0] += ((RuntimeObjectPrefix *)block)->radius * gCurrentAreaConfiguration->objectScale * 0.5F;
        center[2] += ((RuntimeObjectPrefix *)block)->radius * gCurrentAreaConfiguration->objectScale * 0.5F;
        edge[0] = center[0];
        edge[1] = center[1];
        edge[2] = center[2];
        edge[0] += 1.0F;
        edge[2] += 1.0F;
        effectEmitterCreateForStyle(EFFECT_EMIT_BLACK_WISPS, &edge[0], 3.0F, 0, 0, 0.0F);
        mazeRemoveBlockObject(block);
        edgeX = center[0];
        edgeZ = center[2];
        edgeX -= 1.0F;
        edgeZ -= 1.0F;
        edge[0] = edgeX;
        edge[2] = edgeZ;
        edge[1] = center[1];
        effectEmitterCreateForStyle(EFFECT_EMIT_BLACK_WISPS, &center[0], 3.0F, 0, 0, 0.0F);
    }
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Animate the item identified by the script arguments and create its requested particle cluster, then provide a zero script result.
/// \param object Actor whose script supplies an item handle, particle count, lifetime
/// in seconds, radial scale, and particle scale, and receives zero. A live target must
/// be an item with complete direction-animation storage; count must be positive and allocations must succeed.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121504 (0x80121504).
s32 scriptCommandCreateItemParticles(u8 *object) {
    s32 itemHandle;
    s32 vmAddress;
    s32 particleCount;
    f32 lifetimeSeconds;
    f32 radialScale;
    f32 particleScale;
    u8 *item;

    vmAddress = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    particleScale = scriptVmPopFloat((ScriptVm *)(vmAddress));
    radialScale = scriptVmPopFloat((ScriptVm *)(vmAddress));
    lifetimeSeconds = scriptVmPopFloat((ScriptVm *)(vmAddress));
    particleCount = scriptVmPopInteger((ScriptVm *)(vmAddress));
    itemHandle = scriptVmPopInteger((ScriptVm *)(vmAddress));
    item = objectResolveAvailableHandle(itemHandle);
    if (item != 0) {
        itemStartDirectionMotion(item, lifetimeSeconds, 100.0F, 1000.0F);
        particleClusterCreate(particleCount, lifetimeSeconds, radialScale, particleScale, item);
    }
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Separate replacement-kind view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125A4C (0x80125A4C).
extern s32 gItemKindReplacementEntries;

/// Provide the replacement kind for the script item-kind argument as the script result.
/// \param object Actor whose script supplies an item kind from zero through 165 and receives its stored replacement selector; this query does not replace an item.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_801215B0 (0x801215B0).
s32 scriptCommandGetItemReplacement(u8 *object) {
    s32 vmAddress;
    s32 itemKind;

    vmAddress = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    itemKind = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress), *(s32 *)((u8 *)&gItemKindReplacementEntries + (itemKind * 0x74)));
    return 0;
}

/// Provide whether the script item-kind argument belongs to the requested category as the script result.
/// \param object Actor whose script supplies an item kind and category and receives
/// the membership result.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121604 (0x80121604).
s32 scriptCommandIsItemInCategory(u8 *object) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress),
                        itemKindIsInCategory(scriptVmPopInteger((ScriptVm *)(vmAddress)), sp1C));
    return 0;
}

/// Provide the item class for the script item-kind argument as the script result.
/// \param object Actor whose script supplies an item kind and receives its class.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121654 (0x80121654).
s32 scriptCommandGetItemClass(u8 *object) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(sp1C), itemGetClass(scriptVmPopInteger((ScriptVm *)(sp1C))));
    return 0;
}

/// Provide the availability progress-field selector for the script item-kind argument as the script result.
/// \param object Actor whose script supplies an item kind from zero through 165 and receives its signed progress-field selector, or -1 when none is assigned; this is not the saved flag value.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121694 (0x80121694).
s32 scriptCommandGetItemAvailabilityFlag(u8 *object) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(vmAddress), itemGetAvailabilityFlag(scriptVmPopInteger((ScriptVm *)(vmAddress))));
    return 0;
}

/// Force release of the item identified by the script handle argument and provide its success status as the script result.
/// \param object Actor whose script supplies the item handle and receives the release
/// result, or zero when the handle cannot be resolved.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_801216D4 (0x801216D4).
s32 scriptCommandReleaseItem(u8 *object) {
    s32 vmAddress;
    u8 *resolvedObject;

    vmAddress = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    resolvedObject = objectResolveAvailableHandle(scriptVmPopInteger((ScriptVm *)(vmAddress)));
    if (resolvedObject != 0) {
        scriptVmPushInteger((ScriptVm *)(vmAddress), itemTryRelease(resolvedObject, 1));
    } else {
        scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    }
    return 0;
}

/// Stop the calling actor action playback and pending requests, and provide a zero script result.
/// Eligible animated child playback is also disabled; motion resources and audio are not released.
/// \param object Actor whose action is stopped and whose script receives zero;
/// no script arguments are required.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121738 (0x80121738).
s32 scriptCommandStopAction(u8 *object) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    actionStopPlayback(object);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Apply the script friendship milestone and provide its progress identifier, or -1 if already reached, as the script result.
/// \param object Actor whose script supplies the friendship milestone and receives
/// its progress identifier, or -1 when already reached.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121770 (0x80121770).
s32 scriptCommandAdvanceFriendship(u8 *object) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(sp1C),
                        playerAwardFriendshipMilestone(&gPlayerState, scriptVmPopInteger((ScriptVm *)(sp1C))));
    return 0;
}
