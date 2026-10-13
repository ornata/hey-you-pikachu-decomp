#include "particle_cluster.h"
#include "effect_emitter.h"
#include "item_participation.h"
#include "item_queries.h"
#include "action_continuation.h"
#include "player_friendship.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "object_lookup.h"
#include "actor_script.h"
#include "object_types.h"
#include "player_types.h"
#include "script_vm_stack.h"
#include "scene_types.h"

void titleScreenBeginDemoTransition(void); /* extern */
/// Begin the title-screen demo transition and provide a zero script result.
/// \param object Actor whose script receives a zero result; no script arguments are required.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121340 (0x80121340).
s32 scriptCommandBeginDemoTransition(u8 *object) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    titleScreenBeginDemoTransition();
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

s32 titleScreenGetDemoIndex();     /* extern */
/// Provide the current title-screen demo index as the script result.
/// \param object Actor whose script receives the demo index; no script arguments are required.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121378 (0x80121378).
s32 scriptCommandGetDemoIndex(u8 *object) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, titleScreenGetDemoIndex());
    return 0;
}

void mazeRemoveBlockObject(u8 *);
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
/// Remove the maze block identified by the script handle argument, with visual effects, and provide a zero script result.
/// \param object Actor whose script supplies the target block handle and receives zero,
/// including when the handle does not identify a live maze block.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_801213B0 (0x801213B0).
s32 scriptCommandRemoveMazeBlock(u8 *object) {
    s32 script;
    s32 id;
    f32 x;
    f32 z;
    f32 center[3];
    f32 edge[3];
    u8 *target;
    script = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    id = scriptVmPopInteger(script);
    target = objectFindLiveByHandle(id);
    if (target != NULL && ((RuntimeObjectPrefix *)target)->kind == 0x88) {
        center[0] = ((RuntimeObjectPrefix *)target)->worldPosition[0];
        center[1] = ((RuntimeObjectPrefix *)target)->worldPosition[1];
        center[2] = ((RuntimeObjectPrefix *)target)->worldPosition[2];
        center[0] += ((RuntimeObjectPrefix *)target)->radius * ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale * 0.5f;
        center[2] += ((RuntimeObjectPrefix *)target)->radius * ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale * 0.5f;
        edge[0] = center[0];
        edge[1] = center[1];
        edge[2] = center[2];
        edge[0] += 1.0f;
        edge[2] += 1.0f;
        effectEmitterCreateForStyle(EFFECT_EMIT_BLACK_WISPS, &edge[0], 3.0f, 0, 0, 0.0f);
        mazeRemoveBlockObject(target);
        x = center[0];
        z = center[2];
        x -= 1.0f;
        z -= 1.0f;
        edge[0] = x;
        edge[2] = z;
        edge[1] = center[1];
        effectEmitterCreateForStyle(EFFECT_EMIT_BLACK_WISPS, &center[0], 3.0f, 0, 0, 0.0f);
    }
    scriptVmPushInteger(script, 0);
    return 0;
}

void itemStartDirectionMotion(s32, f32, f32, f32);    /* extern */

/// Animate the item identified by the script arguments and create its requested particle cluster, then provide a zero script result.
/// \param object Actor whose script supplies an item handle, particle count, lifetime
/// in seconds, radial scale, and particle scale, and receives zero.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121504 (0x80121504).
s32 scriptCommandCreateItemParticles(u8 *object) {
    s32 sp_pop;
    s32 vmAddress;
    s32 sp3C;
    f32 sp38;
    f32 sp34;
    f32 sp30;
    u8 *sp2C;

    vmAddress = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    sp30 = scriptVmPopFloat(vmAddress);
    sp34 = scriptVmPopFloat(vmAddress);
    sp38 = scriptVmPopFloat(vmAddress);
    sp3C = scriptVmPopInteger(vmAddress);
    sp_pop = scriptVmPopInteger(vmAddress);
    sp2C = objectResolveAvailableHandle(sp_pop);
    if (sp2C != 0) {
        itemStartDirectionMotion((s32)sp2C, sp38, 100.0f, 1000.0f);
        particleClusterCreate(sp3C, sp38, sp34, sp30, sp2C);
    }
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}

/// Separate replacement-kind view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125A4C (0x80125A4C).
extern s32 gItemKindReplacementEntries;

/// Provide the replacement kind for the script item-kind argument as the script result.
/// \param object Actor whose script supplies a valid item kind and receives its replacement kind.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_801215B0 (0x801215B0).
s32 scriptCommandGetItemReplacement(u8 *object) {
    s32 sp1C;
    s32 idx;

    sp1C = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    idx = scriptVmPopInteger(sp1C);
    scriptVmPushInteger(sp1C, *(s32 *)((u8 *)&gItemKindReplacementEntries + idx * 0x74));
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
    sp1C = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, itemKindIsInCategory(scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}


/// Provide the item class for the script item-kind argument as the script result.
/// \param object Actor whose script supplies an item kind and receives its class.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121654 (0x80121654).
s32 scriptCommandGetItemClass(u8 *object) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, itemGetClass(scriptVmPopInteger(sp1C)));
    return 0;
}


/// Provide the availability progress-field selector for the script item-kind argument as the script result.
/// \param object Actor whose script supplies the item kind and receives the result.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121694 (0x80121694).
s32 scriptCommandGetItemAvailabilityFlag(u8 *object) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, itemGetAvailabilityFlag(scriptVmPopInteger(sp1C)));
    return 0;
}


/// Force release of the item identified by the script handle argument and provide its success status as the script result.
/// \param object Actor whose script supplies the item handle and receives the release
/// result, or zero when the handle cannot be resolved.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_801216D4 (0x801216D4).
s32 scriptCommandReleaseItem(u8 *object) {
    s32 sp1C;
    u8 *resolvedObject;

    sp1C = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    resolvedObject = objectResolveAvailableHandle(scriptVmPopInteger(sp1C));
    if (resolvedObject != 0) {
        scriptVmPushInteger(sp1C, itemTryRelease(resolvedObject, 1));
    } else {
        scriptVmPushInteger(sp1C, 0);
    }
    return 0;
}

/// Stop the calling actor action playback and provide a zero script result.
/// \param object Actor whose action is stopped and whose script receives zero;
/// no script arguments are required.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121738 (0x80121738).
s32 scriptCommandStopAction(u8 *object) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    actionStopPlayback(object);
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern PlayerControlPrefix gPlayerState;

/// Apply the script friendship milestone and provide its progress identifier, or -1 if already reached, as the script result.
/// \param object Actor whose script supplies the friendship milestone and receives
/// its progress identifier, or -1 when already reached.
/// \returns Zero after handling the script command.
/// \note Original decompilation identifier: \c func_80121770 (0x80121770).
s32 scriptCommandAdvanceFriendship(u8 *object) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)object)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, playerAwardFriendshipMilestone(&gPlayerState, scriptVmPopInteger(sp1C)));
    return 0;
}
