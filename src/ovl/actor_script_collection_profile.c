/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "object_lookup.h"
#include "scene_types.h"
#include "actor_script.h"
#include "object_types.h"
#include "script_vm_stack.h"

/// Enable or disable suppression of actor horizontal movement.
/// \param actor Actor whose script VM supplies a nonzero value to suppress horizontal movement or zero to allow it, and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FB10 (0x8011FB10).
s32 actorScriptSetHorizontalMovementSuppressed(u8 *actor) {
    s32 sp1C;
    u8 *actorState;
    u8 *updatedActorState;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    if (scriptVmPopInteger(sp1C) != 0) {
        actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
        ((ActorScriptState *)actorState)->behaviorInhibitFlags =
            (u16)(((ActorScriptState *)actorState)->behaviorInhibitFlags | 4);
    } else {
        updatedActorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
        ((ActorScriptState *)updatedActorState)->behaviorInhibitFlags =
            (u16)(((ActorScriptState *)updatedActorState)->behaviorInhibitFlags & 0xFFFB);
    }
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

s32 itemTransform(s32, s32); /* extern */
/// Apply a selected transformation cause to an item.
/// \param actor Actor whose script VM supplies an item handle followed by a supported transformation-cause selector from zero through five; the VM receives the surviving item handle, which may identify a replacement, or zero if the item is absent or removed.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FB7C (0x8011FB7C).
s32 actorScriptTransformItem(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, itemTransform(scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

void cameraEnableElapsedTimeTint(f32); /* extern */

/// Enable the elapsed-time camera tint using the script-supplied initial elapsed time.
/// \param actor Actor whose script VM supplies the initial elapsed time in seconds and receives zero; negative time values are ignored.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FBCC (0x8011FBCC).
s32 actorScriptEnableElapsedTimeTint(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    cameraEnableElapsedTimeTint(scriptVmPopFloat(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

void runtimeRequestGameModeChange(s32); /* extern */
/// Request the transition to collection results for a completed game.
/// \param actor Actor whose script VM receives zero after the transition is requested.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011FC0C (0x8011FC0C).
s32 actorScriptRequestCollectionResults(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    runtimeRequestGameModeChange(0xC);
    scriptVmPushInteger(sp1C, 0);
    return 1;
}

/// Whether dismissing the post-save prompt resets the player profile.
/// \note Original identifier: \c D_800AE50C (0x800AE50C).
extern s32 gRuntimeResetProfileAfterSave;

/// Request the save-prompt transition and select the subsequent profile-reset behavior.
/// \param actor Actor whose script VM supplies nonzero to reset the player profile when the post-save prompt is dismissed, or zero to retain it; the VM receives zero.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011FC48 (0x8011FC48).
s32 actorScriptRequestSavePrompt(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    gRuntimeResetProfileAfterSave = scriptVmPopInteger(sp1C);
    runtimeRequestGameModeChange(7);
    scriptVmPushInteger(sp1C, 0);
    return 1;
}

void itemHideAndSuspend(s32); /* extern */
/// Hide an eligible item and suspend its world interaction.
/// \param actor Actor whose script VM supplies an item handle and receives zero; missing items and items in state four are unchanged.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FC90 (0x8011FC90).
s32 actorScriptHideAndSuspendItem(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    itemHideAndSuspend(scriptVmPopInteger(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

void itemRestoreToWorld(s32); /* extern */
/// Restore an item to its ordinary world state and enable its visibility.
/// \param actor Actor whose script VM supplies an item handle and receives zero; missing items are unchanged.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FCD0 (0x8011FCD0).
s32 actorScriptRestoreItemToWorld(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    itemRestoreToWorld(scriptVmPopInteger(sp1C));
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

s32 profileGetCounter(s32);  /* extern */
/// Provide a saved player-profile counter value.
/// \param actor Actor whose script VM supplies a valid profile-counter index from zero through 165 and receives its value from zero through 15.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FD10 (0x8011FD10).
s32 actorScriptGetProfileCounter(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, profileGetCounter(scriptVmPopInteger(sp1C)));
    return 0;
}

s32 profileAddCounter(s32, s32); /* extern */
/// Adjust a saved player-profile counter within its permitted range.
/// \param actor Actor whose script VM supplies a valid profile-counter index from zero through 165 followed by a signed adjustment whose sum with the current value fits a signed 32-bit integer, and receives the resulting value clamped to zero through 15.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FD50 (0x8011FD50).
s32 actorScriptAddProfileCounter(u8 *actor) {
    s32 sp1C;
    s32 sp18;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp18 = scriptVmPopInteger(sp1C);
    scriptVmPushInteger(sp1C, profileAddCounter(scriptVmPopInteger(sp1C), sp18));
    return 0;
}

/// Player profile name string borrowed by actor scripts.
/// \note Original identifier: \c D_800EF780 (0x800EF780).
extern char gPlayerProfileName[];

/// Provide the player profile name to the script.
/// \param actor Actor whose script VM receives a borrowed reference to the current player-name string.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FDA0 (0x8011FDA0).
s32 actorScriptPushPlayerName(u8 *actor) {
    scriptVmPushStringPointer(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress, (s32)gPlayerProfileName);
    return 0;
}

s32 itemGetDisplayName(s32, s32); /* extern */
/// Provide the display name for an item kind.
/// \param actor Actor whose script VM supplies a valid item kind followed by a name selector of zero or one and receives a borrowed reference to the selected name string; zero prefers the alternate name when present, while one selects the default name.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FDD0 (0x8011FDD0).
s32 actorScriptGetItemDisplayName(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    scriptVmPushStringPointer(vmAddress, itemGetDisplayName(scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

s32 actorConfigGetDisplayName(s32, s32); /* extern */
/// Provide the display name for an actor kind.
/// \param actor Actor whose script VM supplies a valid actor kind followed by a default-name selector and receives a borrowed reference to the selected name string; nonzero selects the default name, while zero prefers an available alternate name.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FE20 (0x8011FE20).
s32 actorScriptGetActorDisplayName(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    scriptVmPushStringPointer(vmAddress, actorConfigGetDisplayName(scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

s32 voiceDictionaryGetWordName(s32);  /* extern */
/// Provide a word name from the selected voice dictionary.
/// \param actor Actor whose script VM supplies a dictionary-local word index and receives a borrowed name string, or an empty string for an invalid index or dictionary; only the low 16 bits of the index are significant.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FE70 (0x8011FE70).
s32 actorScriptGetDictionaryWordName(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushStringPointer(sp1C, voiceDictionaryGetWordName(scriptVmPopInteger(sp1C) & 0xFFFF));
    return 0;
}

void objectSetHierarchyFlags(u8 *, u16); /* extern */
/// Hide the actor hierarchy and suspend automatic interaction-based visibility updates, or resume those updates.
/// \param actor Actor whose script VM supplies nonzero to hide its hierarchy and suppress automatic visibility updates, or zero to resume updates; resuming does not immediately change visibility. The VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FEB0 (0x8011FEB0).
s32 actorScriptSetInteractionVisibilitySuppressed(u8 *actor) {
    u8 *sp1C;
    s32 sp18;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.bytes;
    sp18 = ((ActorScriptState *)sp1C)->vmAddress;
    if (scriptVmPopInteger(sp18) != 0) {
        sp1C = sp1C;
        objectSetHierarchyFlags(actor, 2);
        ((ActorScriptState *)sp1C)->behaviorInhibitFlags =
            (u16)(((ActorScriptState *)sp1C)->behaviorInhibitFlags | 0x20);
    } else {
        ((ActorScriptState *)sp1C)->behaviorInhibitFlags =
            (u16)(((ActorScriptState *)sp1C)->behaviorInhibitFlags & 0xFFDF);
    }
    scriptVmPushInteger(sp18, 0);
    return 0;
}

char *areaGetDisplayName(s32); /* extern */
/// Provide the full display name of an area.
/// \param actor Actor whose script VM supplies an area index from zero through 22 and receives a borrowed reference to its name string.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FF28 (0x8011FF28).
s32 actorScriptGetAreaName(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushStringPointer(sp1C, (s32)areaGetDisplayName(scriptVmPopInteger(sp1C)));
    return 0;
}

char *areaGetRegionName(s32); /* extern */
/// Provide the short display name of an area.
/// \param actor Actor whose script VM supplies an area index from zero through 22 and receives a borrowed short-name string, or a null string reference when no short name is defined.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FF68 (0x8011FF68).
s32 actorScriptGetAreaShortName(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushStringPointer(sp1C, (s32)areaGetRegionName(scriptVmPopInteger(sp1C)));
    return 0;
}

/// Interior opaque catalog-tag view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125A7E (0x80125A7E).
extern s16 gItemKindCatalogTagEntries;

/// Provide the catalog tag associated with an item kind.
/// \param actor Actor whose script VM supplies an item kind from zero through 165 and receives its opaque catalog tag.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FFA8 (0x8011FFA8).
s32 actorScriptGetItemCatalogTag(u8 *actor) {
    s32 sp1C;
    s32 idx;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    idx = scriptVmPopInteger(sp1C);
    scriptVmPushInteger(sp1C, *(s16 *)((u8 *)&gItemKindCatalogTagEntries + idx * 0x74));
    return 0;
}

/// Interior collection-value view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125AAC (0x80125AAC).
extern s32 gItemKindCollectionValueEntries;

/// Provide the collection score assigned to an item kind.
/// \param actor Actor whose script VM supplies an item kind from zero through 165 and receives its collection value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FFFC (0x8011FFFC).
s32 actorScriptGetItemCollectionValue(u8 *actor) {
    s32 sp1C;
    s32 idx;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    idx = scriptVmPopInteger(sp1C);
    scriptVmPushInteger(sp1C, *(s32 *)((u8 *)&gItemKindCollectionValueEntries + idx * 0x74));
    return 0;
}

void actorSetAppearanceVariants(s32, s32, s32, s32); /* extern */
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern s32 gPikachuActor;

/// Select Pikachu appearance variants and whether they persist across action changes.
/// \param actor Actor whose script VM supplies appearance-slot zero and one variants followed by a nonzero value to preserve them across action changes, or zero to allow changes; the VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120050 (0x80120050).
s32 actorScriptSetPikachuAppearanceVariants(u8 *actor) {
    s32 sp_pop;
    s32 vmAddress;
    s32 sp24;
    s32 sp20;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp20 = scriptVmPopInteger(vmAddress);
    sp24 = scriptVmPopInteger(vmAddress);
    sp_pop = scriptVmPopInteger(vmAddress);
    actorSetAppearanceVariants(gPikachuActor, sp_pop, sp24, sp20);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}

void actorTransitionModelEnvironmentColor(u8 *, u8, u8, u8, u8, f32); /* extern */

/// Request a timed transition to a new actor model environment color.
/// \param actor Actor whose script VM supplies red, green, blue, and alpha components from zero through 255 followed by a nonnegative floating-point duration in seconds, and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801200BC (0x801200BC).
s32 actorScriptTransitionEnvironmentColor(u8 *actor) {
    s32 vmAddress;
    f32 sp30;
    u8 sp2F;
    u8 sp2E;
    u8 sp2D;
    u8 sp2C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp30 = scriptVmPopFloat(vmAddress);
    sp2C = scriptVmPopInteger(vmAddress);
    sp2D = scriptVmPopInteger(vmAddress);
    sp2E = scriptVmPopInteger(vmAddress);
    sp2F = scriptVmPopInteger(vmAddress);
    actorTransitionModelEnvironmentColor(actor, sp2F, sp2E, sp2D, sp2C, sp30);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}

typedef struct ScriptVector {
    f32 x, y, z;
} ScriptVector;
void effectEmitterCreateForStyle(u32, f32 *, f32, s32, s32, f32);
/// Create a target-following effect emitter with a selected visual style.
/// \param actor Actor whose script VM supplies an emission style from zero through seven, fallback world X/Y/Z coordinates, target handle, lifetime in seconds, reference handle, and signed horizontal offset, in that order; unavailable targets use the fallback position, nonpositive lifetimes disable expiry, and the VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120150 (0x80120150).
s32 actorScriptCreateEffectEmitter(u8 *actor) {
    s32 kind;
    s32 handle;
    ScriptVector position;
    s32 target;
    s32 id;
    f32 amount;
    f32 scale;
    handle = *(s32 *)(*(u8 **)(actor + 0x54) + 0x1B8);
    scale = scriptVmPopFloat(handle);
    id = scriptVmPopInteger(handle);
    amount = scriptVmPopFloat(handle);
    target = scriptVmPopInteger(handle);
    position.z = scriptVmPopFloat(handle);
    position.y = scriptVmPopFloat(handle);
    position.x = scriptVmPopFloat(handle);
    kind = scriptVmPopInteger(handle);
    effectEmitterCreateForStyle(kind, &position.x, amount, target, id, scale);
    scriptVmPushInteger(handle, 0);
    return 0;
}

u8 *wispEffectCreate(s32, f32, f32, f32); /* extern */

/// Create an animated wisp effect at a world position.
/// \param actor Actor whose script VM supplies a supported wisp style followed by floating-point world X, Y, and Z coordinates, and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120204 (0x80120204).
s32 actorScriptCreateWispEffect(u8 *actor) {
    s32 sp_pop;
    s32 vmAddress;
    f32 sp2C;
    f32 sp28;
    f32 sp24;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = scriptVmPopFloat(vmAddress);
    sp28 = scriptVmPopFloat(vmAddress);
    sp2C = scriptVmPopFloat(vmAddress);
    sp_pop = scriptVmPopInteger(vmAddress);
    wispEffectCreate(sp_pop, sp2C, sp28, sp24);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}

/// Handle of the most recently launched messenger, or zero when no messenger is tracked.
/// \note Original identifier: \c gzip_data_0000 (0x80099960).
extern s32 gTrackedMessengerHandle;

/// Provide the handle of the current messenger object.
/// \param actor Actor whose script VM receives the current messenger handle, or zero when none is registered.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120278 (0x80120278).
s32 actorScriptPushMessengerHandle(u8 *actor) {
    scriptVmPushInteger(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress, gTrackedMessengerHandle);
    return 0;
}

s32 progressAddValue(s32, s32); /* extern */
/// Adjust a saved progress value within its permitted range.
/// \param actor Actor whose script VM supplies a valid progress-field ID or area-relative selector followed by a signed amount whose sum with the current value fits a signed 32-bit integer, and receives the resulting stored value after applying the field limits.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801202A8 (0x801202A8).
s32 actorScriptAddProgressValue(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, progressAddValue(scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

s32 progressSetValue(s32, s32); /* extern */
/// Set a saved progress value within its permitted range.
/// \param actor Actor whose script VM supplies a valid progress-field ID or area-relative selector followed by the requested value, and receives the stored value after applying the field limits.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801202F8 (0x801202F8).
s32 actorScriptSetProgressValue(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, progressSetValue(scriptVmPopInteger(vmAddress), sp1C));
    return 0;
}

s32 progressGetValue(s32);    /* extern */
/// Provide the current saved value of a progress field.
/// \param actor Actor whose script VM supplies a valid progress-field ID or area-relative selector and receives its current value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120348 (0x80120348).
s32 actorScriptGetProgressValue(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, progressGetValue(scriptVmPopInteger(sp1C)));
    return 0;
}

s32 progressGetMaximum(s32);  /* extern */
/// Provide the maximum permitted value of a progress field.
/// \param actor Actor whose script VM supplies a valid progress-field ID or area-relative selector and receives its maximum permitted value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120388 (0x80120388).
s32 actorScriptGetProgressMaximum(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, progressGetMaximum(scriptVmPopInteger(sp1C)));
    return 0;
}

s32 progressGetMinimum(s32);  /* extern */
/// Provide the minimum permitted value of a progress field.
/// \param actor Actor whose script VM supplies a valid progress-field ID or area-relative selector and receives its minimum permitted value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801203C8 (0x801203C8).
s32 actorScriptGetProgressMinimum(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, progressGetMinimum(scriptVmPopInteger(sp1C)));
    return 0;
}

void doorSetAccessState(s32, s32); /* extern */
/// Change the passage-access state of a registered door.
/// \param actor Actor whose script VM supplies a valid created-door index followed by a supported passage-access state, and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120408 (0x80120408).
s32 actorScriptSetDoorAccessState(u8 *actor) {
    s32 vmAddress;
    s32 sp24;
    s32 sp1C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp1C = scriptVmPopInteger(vmAddress);
    doorSetAccessState(scriptVmPopInteger(vmAddress), sp1C);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}

void cameraFadePrimaryOverlayBlend(s32, s32, s32, s32, f32); /* extern */

/// Request a color transition for camera slot zero’s additional overlay blend.
/// \param actor Actor whose script VM supplies red, green, blue, alpha, then a nonnegative duration in seconds and receives zero; color channels are limited to zero through 255.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120458 (0x80120458).
s32 actorScriptFadePrimaryOverlayBlend(u8 *actor) {
    s32 sp_pop0;
    f32 sp38;
    s32 vmAddress;
    s32 sp30;
    s32 sp2C;
    s32 sp28;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp38 = scriptVmPopFloat(vmAddress);
    sp28 = scriptVmPopInteger(vmAddress);
    sp2C = scriptVmPopInteger(vmAddress);
    sp30 = scriptVmPopInteger(vmAddress);
    sp_pop0 = scriptVmPopInteger(vmAddress);
    cameraFadePrimaryOverlayBlend(sp_pop0, sp30, sp2C, sp28, sp38);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}

/// Remaining duration of the primary camera additional overlay blend.
/// \note Original identifier: \c D_800ABDC0 (0x800ABDC0).
extern f32 gPrimaryOverlayBlendRemainingSeconds;

/// Report whether camera slot zero’s additional overlay blend transition has completed.
/// \param actor Actor whose script VM receives one when the blend transition time is zero, or zero otherwise.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801204E0 (0x801204E0).
s32 actorScriptIsPrimaryOverlayBlendComplete(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    if ((f64)gPrimaryOverlayBlendRemainingSeconds == 0.0) {
        scriptVmPushInteger(vmAddress, 1);
    } else {
        scriptVmPushInteger(vmAddress, 0);
    }
    return 0;
}

/// Provide the script argument assigned when this actor was created.
/// \param actor Actor whose script VM receives its creation script argument.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120540 (0x80120540).
s32 actorScriptPushCreationScriptArgument(u8 *actor) {
    u8 *actorState;

    actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
    scriptVmPushInteger(((ActorScriptState *)actorState)->vmAddress,
                        ((ActorScriptState *)actorState)->creationScriptArgument);
    return 0;
}

/// Provide the creation script argument of a selected actor.
/// \param actor Actor whose script VM supplies a handle identifying initialized actor state when live, and receives its creation script argument or minus one when the handle is not live.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8012056C (0x8012056C).
s32 actorScriptGetCreationScriptArgument(u8 *actor) {
    u8 *sp1C;
    u8 *liveObject;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    liveObject = objectFindLiveByHandle(scriptVmPopInteger(sp1C));
    if (liveObject != NULL) {
        scriptVmPushInteger((s32)sp1C, ((RuntimeObjectPrefix *)liveObject)->state.actor->creationScriptArgument);
    } else {
        scriptVmPushInteger((s32)sp1C, -1);
    }
    return 0;
}

void playerPrepareCameraTransition(f32 *, f32 *, f32); /* extern */

/// Configure a smooth primary-camera transition from the current view.
/// \param actor Actor whose script VM supplies destination eye X/Y/Z, look-at target X/Y/Z, and transition duration in seconds as floating-point values, in that order; the VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801205CC (0x801205CC).
s32 actorScriptConfigureCameraTransition(u8 *actor) {
    s32 vm;
    f32 a[3];
    f32 b[3];
    f32 t;

    vm = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    t = scriptVmPopFloat(vm);
    b[2] = scriptVmPopFloat(vm);
    b[1] = scriptVmPopFloat(vm);
    b[0] = scriptVmPopFloat(vm);
    a[2] = scriptVmPopFloat(vm);
    a[1] = scriptVmPopFloat(vm);
    a[0] = scriptVmPopFloat(vm);
    playerPrepareCameraTransition(a, b, t);
    scriptVmPushInteger(vm, 0);
    return 0;
}

void actorBeginScriptActionHandler(u8 *arg0, s32 arg1); /* extern */
/// Begin a scripted actor-kind transformation with visual and audio effects.
/// \param actor Actor address whose script VM supplies the destination kind and later receives the replacement actor handle.
/// \returns Two, transferring execution to the transformation action handler.
/// \note Original decompilation identifier: \c func_80120664 (0x80120664).
s32 actorScriptStartKindTransformation(s32 actor) {
    actorBeginScriptActionHandler((u8 *)actor, 0xE);
    return 2;
}

f32 objectGetApproxDistance(u8 *, u8 *);   /* extern */

/// Measure the distance between two available objects.
/// \param actor Actor whose script VM supplies two object handles and receives their approximate three-dimensional distance, or minus one if either object is unavailable.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120688 (0x80120688).
s32 actorScriptGetAvailableObjectDistance(u8 *actor) {
    s32 vm;
    u8 *objB;
    u8 *objA;
    s32 idB;
    s32 idA;

    vm = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    idA = scriptVmPopInteger(vm);
    idB = scriptVmPopInteger(vm);
    objB = objectResolveAvailableHandle(idB);
    objA = objectResolveAvailableHandle(idA);
    if ((objB == NULL) || (objA == NULL)) {
        scriptVmPushFloat(vm, -1.0f);
    } else {
        scriptVmPushFloat(vm, objectGetApproxDistance(objB, objA));
    }
    return 0;
}

s32 backdropGetActiveId();     /* extern */
/// Provide the currently selected backdrop identifier.
/// \param actor Actor whose script VM receives the active backdrop identifier.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120718 (0x80120718).
s32 actorScriptPushActiveBackdrop(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, backdropGetActiveId());
    return 0;
}

void backdropSelectForView(s32, s32, s32); /* extern */
/// Select and load a backdrop for the primary view.
/// \param actor Actor whose script VM supplies a valid backdrop descriptor index and receives zero; the primary view must have an allocated backdrop buffer.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120750 (0x80120750).
s32 actorScriptSelectBackdrop(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    backdropSelectForView(0, scriptVmPopInteger(sp1C), 0);
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

/// Provide the current scene near clipping distance.
/// \param actor Actor whose script VM receives the near clipping distance as a floating-point value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120798 (0x80120798).
s32 actorScriptPushNearClipDistance(u8 *actor) {
    s32 handle = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;

    scriptVmPushFloat(handle, ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->nearClip);
    return 0;
}


/// Provide the current scene far clipping distance.
/// \param actor Actor whose script VM receives the far clipping distance as a floating-point value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801207CC (0x801207CC).
s32 actorScriptPushFarClipDistance(u8 *actor) {
    s32 handle = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;

    scriptVmPushFloat(handle, ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->farClip);
    return 0;
}


/// Change the current scene near clipping distance.
/// \param actor Actor whose script VM supplies a floating-point near clipping distance and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120800 (0x80120800).
s32 actorScriptSetNearClipDistance(u8 *actor) {
    s32 sp18;
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp18 = vmAddress;
    ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->nearClip = scriptVmPopFloat(vmAddress);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}


/// Change the current scene far clipping distance.
/// \param actor Actor whose script VM supplies a floating-point far clipping distance and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120844 (0x80120844).
s32 actorScriptSetFarClipDistance(u8 *actor) {
    s32 sp18;
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp18 = vmAddress;
    ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->farClip = scriptVmPopFloat(vmAddress);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}


/// Provide the current scene fog start value.
/// \param actor Actor whose script VM receives the fog start value as a floating-point number.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120888 (0x80120888).
s32 actorScriptPushFogStart(u8 *actor) {
    scriptVmPushFloat(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress,
                      (f32)((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->fogStart);
    return 0;
}


/// Change the current scene fog start value.
/// \param actor Actor whose script VM supplies a floating-point fog start value within the signed-integer range and receives zero; fractional values are truncated toward zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801208CC (0x801208CC).
s32 actorScriptSetFogStart(u8 *actor) {
    s32 sp18;
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp18 = vmAddress;
    ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->fogStart = (s32)scriptVmPopFloat(vmAddress);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}


/// Provide the current scene fog end value.
/// \param actor Actor whose script VM receives the fog end value as a floating-point number.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120918 (0x80120918).
s32 actorScriptPushFogEnd(u8 *actor) {
    scriptVmPushFloat(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress,
                      (f32)((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->fogEnd);
    return 0;
}


/// Change the current scene fog end value.
/// \param actor Actor whose script VM supplies a floating-point fog end value within the signed-integer range and receives zero; fractional values are truncated toward zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8012095C (0x8012095C).
s32 actorScriptSetFogEnd(u8 *actor) {
    s32 sp18;
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp18 = vmAddress;
    ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->fogEnd = (s32)scriptVmPopFloat(vmAddress);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}

s32 playerPickUpItem(s32);  /* extern */
/// Try to place an item in the player hand, replacing the previously held item on success.
/// \param actor Actor whose script VM supplies an item handle and receives one on successful pickup, or zero if the item is unavailable or cannot be picked up.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801209A8 (0x801209A8).
s32 actorScriptPlayerTryHoldItem(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, playerPickUpItem(scriptVmPopInteger(sp1C)));
    return 0;
}

u8 *scrollBoardCreateHierarchy(s32, s32, u8, u8, u8, u8, s32, s32);
void objectSetUniformTransform(u8 *, u8 *, u8 *, f32);
void scrollBoardSetHierarchySpeed(u8 *, f32);
void scrollBoardSelectPresetText(s32, s32, s32);
/// Create a scrolling text board with preset text, appearance, and transform settings.
/// \param actor Actor whose script VM supplies, in order, a text preset index of zero or one, text style (zero through two, or another value for the default), positive width in twelve-pixel columns, positive height in sixteen-pixel lines, position X/Y/Z, rotation X/Y/Z in degrees, red/green/blue/alpha components from zero through 255, top-left and bottom-right outline palette indices, uniform scale relative to the scene scale, and scroll speed in pixels per second. Outline indices must be zero through 15, or minus one to disable the corresponding outline; the VM receives the new board hierarchy handle, to release through hierarchy destruction.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801209E8 (0x801209E8).
s32 actorScriptCreateScrollBoard(u8 *actor) {
    s32 handle;
    u8 *created;
    s32 argA;
    s32 argB;
    s32 kind;
    s32 mode;
    ScriptVector position;
    ScriptVector velocity;
    s32 colorA;
    s32 colorB;
    s32 colorC;
    s32 colorD;
    s32 flagA;
    s32 flagB;
    f32 scale;
    f32 duration;
    handle = *(s32 *)(*(u8 **)(actor + 0x54) + 0x1B8);
    duration = scriptVmPopFloat(handle);
    scale = scriptVmPopFloat(handle);
    flagB = scriptVmPopInteger(handle);
    flagA = scriptVmPopInteger(handle);
    colorD = scriptVmPopInteger(handle);
    colorC = scriptVmPopInteger(handle);
    colorB = scriptVmPopInteger(handle);
    colorA = scriptVmPopInteger(handle);
    velocity.z = scriptVmPopFloat(handle);
    velocity.y = scriptVmPopFloat(handle);
    velocity.x = scriptVmPopFloat(handle);
    position.z = scriptVmPopFloat(handle);
    position.y = scriptVmPopFloat(handle);
    position.x = scriptVmPopFloat(handle);
    mode = scriptVmPopInteger(handle);
    kind = scriptVmPopInteger(handle);
    argB = scriptVmPopInteger(handle);
    argA = scriptVmPopInteger(handle);
    created = scrollBoardCreateHierarchy(kind, mode, colorA, colorB, colorC, colorD, flagA, flagB);
    scrollBoardSelectPresetText(*(s32 *)(created + 0x38), argA, argB);
    scrollBoardSetHierarchySpeed(created, duration);
    objectSetUniformTransform(created, (u8 *)&position, (u8 *)&velocity,
                              *(f32 *)(gCurrentAreaConfiguration + 0xC) * scale);
    scriptVmPushInteger(handle, *(s32 *)(created + 0x38));
    return 0;
}


/// Select preset text for a scrolling-board hierarchy.
/// \param actor Actor whose script VM supplies a live scrolling-board hierarchy handle, a text preset index of zero or one, and a text style, in that order; the VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120B78 (0x80120B78).
s32 actorScriptSelectScrollBoardText(u8 *actor) {
    s32 sp_pop;
    s32 vmAddress;
    s32 sp24;
    s32 sp20;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp20 = scriptVmPopInteger(vmAddress);
    sp24 = scriptVmPopInteger(vmAddress);
    sp_pop = scriptVmPopInteger(vmAddress);
    scrollBoardSelectPresetText(sp_pop, sp24, sp20);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}


/// Change the scrolling speed of a scrolling-board hierarchy.
/// \param actor Actor whose script VM supplies a live scrolling-board hierarchy handle followed by a floating-point speed in pixels per second, and receives zero; positive speeds advance, negative speeds reverse, and zero stops scrolling.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120BDC (0x80120BDC).
s32 actorScriptSetScrollBoardSpeed(u8 *actor) {
    s32 vmAddress;
    s32 sp_pop0;
    s32 sp24;
    f32 sp18;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp24 = vmAddress;
    sp18 = scriptVmPopFloat(vmAddress);
    sp_pop0 = scriptVmPopInteger(vmAddress);
    scrollBoardSetHierarchySpeed((u8 *)objectFindLiveByHandle(sp_pop0), sp18);
    scriptVmPushInteger(vmAddress, 0);
    return 0;
}

/// Set a live object position, orientation, and uniform scale.
/// \param actor Actor whose script VM supplies a live object handle, position X/Y/Z, rotation X/Y/Z in degrees, and a uniform scale relative to the scene scale, in that order; the VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120C34 (0x80120C34).
s32 actorScriptSetObjectTransform(u8 *actor) {
    s32 id;
    u8 *target;
    s32 handle;
    ScriptVector position;
    ScriptVector velocity;
    f32 scale;
    handle = *(s32 *)(*(u8 **)(actor + 0x54) + 0x1B8);
    scale = scriptVmPopFloat(handle);
    velocity.z = scriptVmPopFloat(handle);
    velocity.y = scriptVmPopFloat(handle);
    velocity.x = scriptVmPopFloat(handle);
    position.z = scriptVmPopFloat(handle);
    position.y = scriptVmPopFloat(handle);
    position.x = scriptVmPopFloat(handle);
    id = scriptVmPopInteger(handle);
    target = (u8 *)objectFindLiveByHandle(id);
    objectSetUniformTransform(target, (u8 *)&position, (u8 *)&velocity,
                              *(f32 *)(gCurrentAreaConfiguration + 0xC) * scale);
    scriptVmPushInteger(handle, 0);
    return 0;
}

f32 mathSinDegrees(f32);          /* extern */

/// Provide the sine of a script-supplied angle.
/// \param actor Actor whose script VM supplies an angle in degrees satisfying mathSinDegrees input limits and receives its approximate sine.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120CF8 (0x80120CF8).
s32 actorScriptSinDegrees(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat(sp1C, mathSinDegrees(scriptVmPopFloat(sp1C)));
    return 0;
}

f32 mathCosDegrees(f32);          /* extern */

/// Provide the cosine of a script-supplied angle.
/// \param actor Actor whose script VM supplies an angle in degrees satisfying mathCosDegrees input limits and receives its approximate cosine.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120D38 (0x80120D38).
s32 actorScriptCosDegrees(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat(sp1C, mathCosDegrees(scriptVmPopFloat(sp1C)));
    return 0;
}

/// Bedroom name-catalog category codes indexed by catalog entry.
/// \note Original identifier: \c D_8015516C (0x8015516C).
extern u8 gBedroomPokemonNameCategories[];

/// Provide the category associated with a Pokémon name catalog entry.
/// \param actor Actor whose script VM supplies a catalog index from zero through 152 and receives its category code; requires the bedroom scene data.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120D78 (0x80120D78).
s32 actorScriptGetPokemonNameCategory(u8 *actor) {
    s32 sp1C;
    s32 idx;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    idx = scriptVmPopInteger(sp1C);
    scriptVmPushInteger(sp1C, *(u8 *)((u8 *)&gBedroomPokemonNameCategories + idx * 1));
    return 0;
}

/// Bedroom name-catalog string references indexed by catalog entry.
/// \note Original identifier: \c D_80155208 (0x80155208).
extern char *gBedroomPokemonNames[];

/// Provide a selected name from the bedroom Pokémon name catalog.
/// \param actor Actor whose script VM supplies a catalog index from zero through 152 and receives a borrowed name string; requires the bedroom scene data.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120DB8 (0x80120DB8).
s32 actorScriptGetPokemonName(u8 *actor) {
    s32 sp1C;
    s32 idx;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    idx = scriptVmPopInteger(sp1C);
    scriptVmPushStringPointer(sp1C, *(s32 *)((u8 *)&gBedroomPokemonNames + idx * 4));
    return 0;
}

u32 randomBelow(s32);
/// Ring history of up to 100 previously selected bedroom name-catalog indices.
/// \note Original identifier: \c D_8015546C (0x8015546C).
extern u8 gBedroomPokemonNameHistory[];
/// Next insertion index in the bedroom name-selection history.
/// \note Original identifier: \c D_801554D0 (0x801554D0).
extern s32 gBedroomPokemonNameHistoryHead;
/// Oldest retained index in the bedroom name-selection history.
/// \note Original identifier: \c D_801554D4 (0x801554D4).
extern s32 gBedroomPokemonNameHistoryTail;
/// Number of retained bedroom name selections, limited to 100.
/// \note Original identifier: \c D_801554D8 (0x801554D8).
extern s32 gBedroomPokemonNameHistoryCount;
/// Choose a random Pokémon name catalog index outside the retained history of up to 100 choices.
/// \param actor Actor whose script VM receives the selected catalog index from zero through 152; requires initialized bedroom scene data and name-selection history.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120DFC (0x80120DFC).
s32 actorScriptChoosePokemonName(u8 *actor) {
    s32 handle;
    s32 head;
    s32 tail;
    s32 count;
    s32 candidate;
    s32 index;
    s32 i;
    handle = *(s32 *)(*(u8 **)(actor + 0x54) + 0x1B8);
    head = gBedroomPokemonNameHistoryHead;
    tail = gBedroomPokemonNameHistoryTail;
    count = gBedroomPokemonNameHistoryCount;
    do {
        candidate = randomBelow(153);
        index = tail;
        for (i = 0; i < count; i++) {
            if (candidate == gBedroomPokemonNameHistory[index++]) {
                candidate = -1;
                break;
            }
            if (index == 100)
                index = 0;
        }
    } while (candidate == -1);
    gBedroomPokemonNameHistory[head++] = candidate;
    if (head == 100)
        head = 0;
    if (count < 100)
        count++;
    else {
        tail++;
        if (tail == 100)
            tail = 0;
    }
    gBedroomPokemonNameHistoryHead = head;
    gBedroomPokemonNameHistoryTail = tail;
    gBedroomPokemonNameHistoryCount = count;
    scriptVmPushInteger(handle, candidate);
    return 0;
}

s32 itemFindHandleFromPlayerView(s32, s32, f32, f32, s32, s32); /* extern */

/// Find an item within the player viewing cone using script-supplied filters.
/// \param actor Actor whose script VM supplies inclusive minimum and maximum item kinds, inclusive maximum approximate distance, maximum absolute horizontal angle in degrees, required item state, and a priority-selection flag, in that order; kind 165 selects the corresponding range endpoint, state six accepts any state, and a nonzero flag favors selection priority and resets the selected priority. The VM receives the selected handle or zero when none qualifies.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120F34 (0x80120F34).
s32 actorScriptFindItemFromPlayerView(u8 *actor) {
    s32 sp_pop;
    s32 vmAddress;
    s32 sp3C;
    f32 sp38;
    f32 sp34;
    s32 sp30;
    s32 sp2C;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp2C = scriptVmPopInteger(vmAddress);
    sp30 = scriptVmPopInteger(vmAddress);
    sp34 = scriptVmPopFloat(vmAddress);
    sp38 = scriptVmPopFloat(vmAddress);
    sp3C = scriptVmPopInteger(vmAddress);
    sp_pop = scriptVmPopInteger(vmAddress);
    scriptVmPushInteger(vmAddress, itemFindHandleFromPlayerView(sp_pop, sp3C, sp38, sp34, sp30, sp2C));
    return 0;
}

s32 actorFindNearestFromPlayerView(u8 *, s32, s32, f32, f32); /* extern */

/// Find the nearest other actor within the player viewing cone.
/// \param actor Actor excluded from the search whose script VM supplies inclusive minimum and maximum kinds, exclusive maximum approximate distance, and maximum horizontal angle in degrees; script kind aliases are honored and angles of at least 360 allow every direction. The VM receives the nearest qualifying handle or zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120FD0 (0x80120FD0).
s32 actorScriptFindActorFromPlayerView(u8 *actor) {
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
    scriptVmPushInteger(vmAddress, actorFindNearestFromPlayerView(actor, sp_pop, sp34, sp30, sp2C));
    return 0;
}

s32 objectFindNearestFromPlayerView(u8 *, s32, s32, f32, f32); /* extern */

/// Find the nearest other object within the player viewing cone.
/// \param actor Actor excluded from the search whose script VM supplies inclusive minimum and maximum kinds, exclusive maximum approximate distance, and maximum horizontal angle in degrees; angles of at least 360 allow every direction. The VM receives the nearest qualifying handle or zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121050 (0x80121050).
s32 actorScriptFindObjectFromPlayerView(u8 *actor) {
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
    scriptVmPushInteger(vmAddress, objectFindNearestFromPlayerView(actor, sp_pop, sp34, sp30, sp2C));
    return 0;
}

f32 objectGetPlayerFacingError(u8 *);         /* extern */

/// Report the player heading relative to the direction toward the actor.
/// \param actor Actor whose script VM receives the player heading minus the direction toward this actor, wrapped to signed degrees.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801210D0 (0x801210D0).
s32 actorScriptPushPlayerFacingError(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat(sp1C, objectGetPlayerFacingError(actor));
    return 0;
}

f32 objectGetFacingErrorToTarget(u8 *, s32);    /* extern */

/// Report the actor heading relative to the direction toward a target.
/// \param actor Actor whose script VM supplies a target identifier and receives its heading minus the target direction in degrees from zero inclusive to 360 exclusive, or minus one when the target is unavailable.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121108 (0x80121108).
s32 actorScriptGetTargetFacingError(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat(sp1C, objectGetFacingErrorToTarget(actor, scriptVmPopInteger(sp1C)));
    return 0;
}

void itemSetScale(s32, f32); /* extern */

/// Set an available item size relative to its kind and scene scale.
/// \param actor Actor whose script VM supplies a handle identifying an initialized item when available, followed by a floating-point relative scale, and receives zero; unavailable handles are unchanged.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121150 (0x80121150).
s32 actorScriptSetItemScale(u8 *actor) {
    s32 sp1C;
    f32 sp18;
    u8 *resolvedObject;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp18 = scriptVmPopFloat(sp1C);
    resolvedObject = objectResolveAvailableHandle(scriptVmPopInteger(sp1C));
    if (resolvedObject != 0) {
        itemSetScale((s32)resolvedObject, sp18);
    }
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

/// Dictionary lookup index retained by translation, including its legacy distance-based fallback.
/// \note Original identifier: \c D_800722E0 (0x800722E0).
extern s32 gVoiceCommandResultIndex;

/// Provide the legacy script voice-result query supported by this build.
/// \param actor Actor whose script VM receives zero when no recognition result is available, or an unspecified value otherwise.
/// \returns Zero, allowing script execution to continue.
/// \note Portability/UB: A nonnegative recognition-result index reads an uninitialized local value; this preserves the original build behavior.
/// \note Original decompilation identifier: \c func_801211AC (0x801211AC).
s32 actorScriptPushLegacyVoiceResult(u8 *actor) {
    s32 id;
    s32 mode;
    u16 buf[16];

    mode = gVoiceCommandResultIndex;
    id = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(id, (mode < 0) ? 0 : buf[0]);
    return 0;
}

/// Provide the current story chapter.
/// \param actor Actor whose script VM receives the current chapter number.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801211F0 (0x801211F0).
s32 actorScriptPushCurrentChapter(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, progressGetCurrentChapter());
    return 0;
}

s32 progressUpdateChapterAfterVisit();     /* extern */
/// Update the story chapter and unlock milestones according to completed visit objectives.
/// \param actor Actor whose script VM receives the resulting chapter number, which may remain unchanged or return to an earlier chapter.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121228 (0x80121228).
s32 actorScriptUpdateChapterAfterVisit(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, progressUpdateChapterAfterVisit());
    return 0;
}

s32 itemGetAvailableSlots();     /* extern */
/// Provide the remaining item-object capacity.
/// \param actor Actor whose script VM receives the number of available item slots.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121260 (0x80121260).
s32 actorScriptPushAvailableItemSlots(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, itemGetAvailableSlots());
    return 0;
}

void scriptVmResetGlobals(); /* extern */

/// Reset all global script variables to their initial values.
/// \param actor Actor whose script VM receives zero after the shared global variables are reset.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121298 (0x80121298).
s32 actorScriptResetGlobalVariables(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmResetGlobals();
    scriptVmPushInteger(sp1C, 0);
    return 0;
}

s32 titleScreenGetState();     /* extern */
/// Provide the current startup and title-screen state.
/// \param actor Actor whose script VM receives the current title-screen state identifier.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801212D0 (0x801212D0).
s32 actorScriptPushTitleScreenState(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger(sp1C, titleScreenGetState());
    return 0;
}

void titleScreenRequestMenu(); /* extern */
/// Request the main menu when the title screen is awaiting Start.
/// \param actor Actor whose script VM receives zero after the request; other title-screen states are unaffected.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121308 (0x80121308).
s32 actorScriptRequestTitleMenu(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    titleScreenRequestMenu();
    scriptVmPushInteger(sp1C, 0);
    return 0;
}
