#include "runtime_mode.h"
#include "area_names.h"
#include "player_profile.h"
#include "pikachu_actor.h"
#include "math_angles.h"
#include "random.h"
#include "backdrop_view.h"
#include "runtime_control.h"
#include "wisp_effect.h"
#include "effect_emitter.h"
#include "object_transform.h"
#include "scroll_board.h"
#include "script_vm_globals.h"
#include "actor_behavior.h"
#include "player_view_search.h"
#include "profile_counters.h"
#include "item_participation.h"
#include "item_size.h"
#include "item_queries.h"
#include "story_chapter.h"
#include "progress_fields.h"
#include "progress_value.h"
#include "player_camera.h"
#include "actor_script_action.h"
#include "camera_overlay.h"
#include "actor_color.h"
#include "object_distance.h"
#include "object_flags.h"
#include "door.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "item_response.h"
#include "actor_text.h"
#include "voice_recognition.h"
#include "item_text.h"
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
    if (scriptVmPopInteger((ScriptVm *)(sp1C)) != 0) {
        actorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
        ((ActorScriptState *)actorState)->behaviorFlags =
            (u16)(((ActorScriptState *)actorState)->behaviorFlags | ACTOR_INHIBIT_HORIZONTAL_MOVEMENT);
    } else {
        updatedActorState = ((RuntimeObjectPrefix *)actor)->state.bytes;
        ((ActorScriptState *)updatedActorState)->behaviorFlags =
            (u16)(((ActorScriptState *)updatedActorState)->behaviorFlags & ~ACTOR_INHIBIT_HORIZONTAL_MOVEMENT);
    }
    scriptVmPushInteger((ScriptVm *)(sp1C), 0);
    return 0;
}

/// Apply a selected transformation cause to an item.
/// \param actor Actor whose script VM supplies an item handle followed by a supported transformation-cause selector from zero through five; the VM receives the surviving item handle, which may identify a replacement, or zero if the item is absent or removed.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FB7C (0x8011FB7C).
s32 actorScriptTransformItem(u8 *actor) {
    s32 vmAddress;
    s32 unusedVmAddress;
    s32 transformationCause;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    unusedVmAddress = vmAddress;
    transformationCause = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress),
                        itemTransform(scriptVmPopInteger((ScriptVm *)(vmAddress)), transformationCause));
    return 0;
}

#include "actor_interaction.h"
#include "area_services.h"
#include "backdrop_query.h"
#include "player_interaction.h"
#include "render_frame.h"
#include "title_screen_state.h"

/// Enable the elapsed-time camera tint using the script-supplied initial elapsed time.
/// \param actor Actor whose script VM supplies the initial elapsed time in seconds and receives zero; negative time values are ignored.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FBCC (0x8011FBCC).
s32 actorScriptEnableElapsedTimeTint(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    cameraEnableElapsedTimeTint(scriptVmPopFloat((ScriptVm *)(sp1C)));
    scriptVmPushInteger((ScriptVm *)(sp1C), 0);
    return 0;
}

/// Request the transition to collection results for a completed game.
/// \param actor Actor whose script VM receives zero after the transition is requested.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011FC0C (0x8011FC0C).
s32 actorScriptRequestCollectionResults(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    runtimeRequestGameModeChange(GAME_MODE_FADE_TO_COLLECTION);
    scriptVmPushInteger((ScriptVm *)(sp1C), 0);
    return 1;
}

/// Whether dismissing the post-save prompt resets the player profile.
/// \note Original identifier: \c D_800AE50C (0x800AE50C).
extern s32 gRuntimeResetProfileAfterSave;

/// Request the save-prompt transition and select the subsequent profile-reset behavior.
/// The request does not immediately write EEPROM.
/// \param actor Actor whose script VM supplies nonzero to reset the player profile when the post-save prompt is dismissed, or zero to retain it; the VM receives zero.
/// \returns One, yielding script execution.
/// \note Original decompilation identifier: \c func_8011FC48 (0x8011FC48).
s32 actorScriptRequestSavePrompt(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    gRuntimeResetProfileAfterSave = scriptVmPopInteger((ScriptVm *)(vmAddress));
    runtimeRequestGameModeChange(GAME_MODE_FADE_TO_SAVE_PROMPT);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 1;
}

/// Hide an eligible item and suspend its world interaction.
/// \param actor Actor whose script VM supplies an item handle and receives zero; missing items and items in state four are unchanged.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FC90 (0x8011FC90).
s32 actorScriptHideAndSuspendItem(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    itemHideAndSuspend(scriptVmPopInteger((ScriptVm *)(sp1C)));
    scriptVmPushInteger((ScriptVm *)(sp1C), 0);
    return 0;
}

/// Restore an item to its ordinary world state and enable its visibility.
/// \param actor Actor whose script VM supplies an item handle and receives zero; missing items are unchanged.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FCD0 (0x8011FCD0).
s32 actorScriptRestoreItemToWorld(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    itemRestoreToWorld(scriptVmPopInteger((ScriptVm *)(sp1C)));
    scriptVmPushInteger((ScriptVm *)(sp1C), 0);
    return 0;
}

/// Provide a saved player-profile counter value.
/// \param actor Actor whose script VM supplies a valid profile-counter index from zero through 165 and receives its value from zero through 15.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FD10 (0x8011FD10).
s32 actorScriptGetProfileCounter(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(sp1C), profileGetCounter(scriptVmPopInteger((ScriptVm *)(sp1C))));
    return 0;
}

/// Adjust a saved player-profile counter within its permitted range.
/// \param actor Actor whose script VM supplies a valid profile-counter index from zero through 165 followed by a signed adjustment whose sum with the current value fits a signed 32-bit integer, and receives the resulting value clamped to zero through 15.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FD50 (0x8011FD50).
s32 actorScriptAddProfileCounter(u8 *actor) {
    s32 sp1C;
    s32 sp18;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp18 = scriptVmPopInteger((ScriptVm *)(sp1C));
    scriptVmPushInteger((ScriptVm *)(sp1C), profileAddCounter(scriptVmPopInteger((ScriptVm *)(sp1C)), sp18));
    return 0;
}

/// Provide the player profile name to the script.
/// \param actor Actor whose script VM receives a borrowed reference to the current player-name string.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FDA0 (0x8011FDA0).
s32 actorScriptPushPlayerName(u8 *actor) {
    scriptVmPushStringPointer((ScriptVm *)(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress),
                              (char *)(gPlayerProfileName));
    return 0;
}

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
    sp1C = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushStringPointer((ScriptVm *)(vmAddress),
                              itemGetDisplayName(scriptVmPopInteger((ScriptVm *)(vmAddress)), sp1C));
    return 0;
}

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
    sp1C = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushStringPointer((ScriptVm *)(vmAddress),
                              actorConfigGetDisplayName(scriptVmPopInteger((ScriptVm *)(vmAddress)), sp1C));
    return 0;
}

/// Provide a word name from the selected voice dictionary.
/// \param actor Actor whose script VM supplies a dictionary-local word index and receives a borrowed name string, or an empty string when no dictionary is selected or the index is out of range; an active dictionary must have valid metadata. Only the low 16 bits of the index are significant.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FE70 (0x8011FE70).
s32 actorScriptGetDictionaryWordName(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushStringPointer((ScriptVm *)(vmAddress),
                              voiceDictionaryGetWordName((u16)scriptVmPopInteger((ScriptVm *)(vmAddress))));
    return 0;
}

/// Hide the actor hierarchy and suspend automatic interaction-based visibility updates, or resume those updates.
/// \param actor Actor whose script VM supplies nonzero to hide its hierarchy and suppress automatic visibility updates, or zero to resume updates; resuming does not immediately change visibility. The VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FEB0 (0x8011FEB0).
s32 actorScriptSetInteractionVisibilitySuppressed(u8 *actor) {
    u8 *sp1C;
    s32 sp18;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.bytes;
    sp18 = ((ActorScriptState *)sp1C)->vmAddress;
    if (scriptVmPopInteger((ScriptVm *)(sp18)) != 0) {
        sp1C = sp1C;
        objectSetHierarchyFlags(actor, 2);
        ((ActorScriptState *)sp1C)->behaviorFlags =
            (u16)(((ActorScriptState *)sp1C)->behaviorFlags | ACTOR_INHIBIT_INTERACTION_VISIBILITY);
    } else {
        ((ActorScriptState *)sp1C)->behaviorFlags =
            (u16)(((ActorScriptState *)sp1C)->behaviorFlags & ~ACTOR_INHIBIT_INTERACTION_VISIBILITY);
    }
    scriptVmPushInteger((ScriptVm *)(sp18), 0);
    return 0;
}

/// Provide the full display name of an area.
/// \param actor Actor whose script VM supplies an area index from zero through 22 and receives a borrowed reference to its name string.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FF28 (0x8011FF28).
s32 actorScriptGetAreaName(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushStringPointer((ScriptVm *)(vmAddress), areaGetDisplayName(scriptVmPopInteger((ScriptVm *)(vmAddress))));
    return 0;
}

/// Provide the short display name of an area.
/// \param actor Actor whose script VM supplies an area index from zero through 22 and receives a borrowed short-name string, or a null string reference when no short name is defined.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FF68 (0x8011FF68).
s32 actorScriptGetAreaShortName(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushStringPointer((ScriptVm *)(vmAddress), areaGetRegionName(scriptVmPopInteger((ScriptVm *)(vmAddress))));
    return 0;
}

/// Interior notebook display-frame view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125A7E (0x80125A7E).
extern s16 gItemKindDisplayMaterialFrameEntries;

/// Read the material-frame selector used to represent an item kind in a notebook.
/// \param actor Actor whose script VM supplies an item kind from zero through 165 and receives its signed display-frame selector.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8011FFA8 (0x8011FFA8).
s32 actorScriptGetItemDisplayMaterialFrame(u8 *actor) {
    s32 vmAddress;
    s32 itemKind;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    itemKind = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress),
                        *(s16 *)((u8 *)&gItemKindDisplayMaterialFrameEntries + (itemKind * 0x74)));
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
    idx = scriptVmPopInteger((ScriptVm *)(sp1C));
    scriptVmPushInteger((ScriptVm *)(sp1C), *(s32 *)((u8 *)&gItemKindCollectionValueEntries + (idx * 0x74)));
    return 0;
}

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
    sp20 = scriptVmPopInteger((ScriptVm *)(vmAddress));
    sp24 = scriptVmPopInteger((ScriptVm *)(vmAddress));
    sp_pop = scriptVmPopInteger((ScriptVm *)(vmAddress));
    actorSetAppearanceVariants((u8 *)gPikachuActor, sp_pop, sp24, sp20);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

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
    sp30 = scriptVmPopFloat((ScriptVm *)(vmAddress));
    sp2C = scriptVmPopInteger((ScriptVm *)(vmAddress));
    sp2D = scriptVmPopInteger((ScriptVm *)(vmAddress));
    sp2E = scriptVmPopInteger((ScriptVm *)(vmAddress));
    sp2F = scriptVmPopInteger((ScriptVm *)(vmAddress));
    actorTransitionModelEnvironmentColor(actor, sp2F, sp2E, sp2D, sp2C, sp30);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Create a target-following effect emitter with a selected visual style.
/// \param actor Actor whose script VM supplies an emission style from zero through seven, fallback world X/Y/Z coordinates, target handle, lifetime in seconds, reference handle, and signed horizontal offset, in that order; unavailable targets use the fallback position, nonpositive lifetimes disable expiry, and the VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Following and emission begin on later effect updates; creation does not invoke emission callbacks.
/// \note Original decompilation identifier: \c func_80120150 (0x80120150).
s32 actorScriptCreateEffectEmitter(u8 *actor) {
    s32 style;
    s32 vmAddress;
    f32 position[3];
    s32 targetHandle;
    s32 referenceHandle;
    f32 lifetime;
    f32 offset;
    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    offset = scriptVmPopFloat((ScriptVm *)(vmAddress));
    referenceHandle = scriptVmPopInteger((ScriptVm *)(vmAddress));
    lifetime = scriptVmPopFloat((ScriptVm *)(vmAddress));
    targetHandle = scriptVmPopInteger((ScriptVm *)(vmAddress));
    position[2] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    position[1] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    position[0] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    style = scriptVmPopInteger((ScriptVm *)(vmAddress));
    effectEmitterCreateForStyle(style, position, lifetime, targetHandle, referenceHandle, offset);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Create an animated wisp effect at a world position.
/// \param actor Actor whose script VM supplies a supported wisp style followed by floating-point world X, Y, and Z coordinates, and receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120204 (0x80120204).
s32 actorScriptCreateWispEffect(u8 *actor) {
    s32 style;
    s32 vmAddress;
    f32 x;
    f32 y;
    f32 z;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    z = scriptVmPopFloat((ScriptVm *)(vmAddress));
    y = scriptVmPopFloat((ScriptVm *)(vmAddress));
    x = scriptVmPopFloat((ScriptVm *)(vmAddress));
    style = scriptVmPopInteger((ScriptVm *)(vmAddress));
    wispEffectCreate(style, x, y, z);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
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
    scriptVmPushInteger((ScriptVm *)(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress), gTrackedMessengerHandle);
    return 0;
}

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
    sp1C = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress), progressAddValue(scriptVmPopInteger((ScriptVm *)(vmAddress)), sp1C));
    return 0;
}

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
    sp1C = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress), progressSetValue(scriptVmPopInteger((ScriptVm *)(vmAddress)), sp1C));
    return 0;
}

/// Provide the current saved value of a progress field.
/// \param actor Actor whose script VM supplies a valid progress-field ID or area-relative selector and receives its current value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120348 (0x80120348).
s32 actorScriptGetProgressValue(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(sp1C), progressGetValue(scriptVmPopInteger((ScriptVm *)(sp1C))));
    return 0;
}

/// Provide the maximum permitted value of a progress field.
/// \param actor Actor whose script VM supplies a valid progress-field ID or area-relative selector and receives its maximum permitted value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120388 (0x80120388).
s32 actorScriptGetProgressMaximum(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(sp1C), progressGetMaximum(scriptVmPopInteger((ScriptVm *)(sp1C))));
    return 0;
}

/// Provide the minimum permitted value of a progress field.
/// \param actor Actor whose script VM supplies a valid progress-field ID or area-relative selector and receives its minimum permitted value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801203C8 (0x801203C8).
s32 actorScriptGetProgressMinimum(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(sp1C), progressGetMinimum(scriptVmPopInteger((ScriptVm *)(sp1C))));
    return 0;
}

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
    sp1C = scriptVmPopInteger((ScriptVm *)(vmAddress));
    doorSetAccessState(scriptVmPopInteger((ScriptVm *)(vmAddress)), sp1C);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

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
    sp38 = scriptVmPopFloat((ScriptVm *)(vmAddress));
    sp28 = scriptVmPopInteger((ScriptVm *)(vmAddress));
    sp2C = scriptVmPopInteger((ScriptVm *)(vmAddress));
    sp30 = scriptVmPopInteger((ScriptVm *)(vmAddress));
    sp_pop0 = scriptVmPopInteger((ScriptVm *)(vmAddress));
    cameraFadePrimaryOverlayBlend(sp_pop0, sp30, sp2C, sp28, sp38);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
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
        scriptVmPushInteger((ScriptVm *)(vmAddress), 1);
    } else {
        scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
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
    scriptVmPushInteger((ScriptVm *)(((ActorScriptState *)actorState)->vmAddress),
                        ((ActorScriptState *)actorState)->creationScriptArgument);
    return 0;
}

/// Provide the creation script argument of a selected actor.
/// \param actor Actor whose script VM supplies a handle identifying initialized actor state when live, and receives its creation script argument or minus one when the handle is not live.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_8012056C (0x8012056C).
s32 actorScriptGetCreationScriptArgument(u8 *actor) {
    s32 vmAddress;
    u8 *liveObject;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    liveObject = objectFindLiveByHandle(scriptVmPopInteger((ScriptVm *)(vmAddress)));
    if (liveObject != NULL) {
        scriptVmPushInteger((ScriptVm *)(vmAddress),
                            ((RuntimeObjectPrefix *)liveObject)->state.actor->creationScriptArgument);
    } else {
        scriptVmPushInteger((ScriptVm *)(vmAddress), -1);
    }
    return 0;
}

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
    t = scriptVmPopFloat((ScriptVm *)(vm));
    b[2] = scriptVmPopFloat((ScriptVm *)(vm));
    b[1] = scriptVmPopFloat((ScriptVm *)(vm));
    b[0] = scriptVmPopFloat((ScriptVm *)(vm));
    a[2] = scriptVmPopFloat((ScriptVm *)(vm));
    a[1] = scriptVmPopFloat((ScriptVm *)(vm));
    a[0] = scriptVmPopFloat((ScriptVm *)(vm));
    playerPrepareCameraTransition(a, b, t);
    scriptVmPushInteger((ScriptVm *)(vm), 0);
    return 0;
}

/// Begin a scripted actor-kind transformation with visual and audio effects.
/// \param actor Actor address whose script VM supplies the destination kind and later receives the replacement actor handle.
/// \returns Two, transferring execution to the transformation action handler.
/// \note Original decompilation identifier: \c func_80120664 (0x80120664).
s32 actorScriptStartKindTransformation(u8 *actor) {
    actorBeginScriptActionHandler((u8 *)((s32)actor), ACTOR_SCRIPT_ACTION_TRANSFORM_KIND);
    return 2;
}

/// Measure the distance between two available objects.
/// \param actor Actor whose script VM supplies two object handles and receives their approximate three-dimensional distance, or minus one if either object is unavailable.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120688 (0x80120688).
s32 actorScriptGetAvailableObjectDistance(u8 *actor) {
    s32 vmAddress;
    u8 *firstObject;
    u8 *secondObject;
    s32 firstHandle;
    s32 secondHandle;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    secondHandle = scriptVmPopInteger((ScriptVm *)(vmAddress));
    firstHandle = scriptVmPopInteger((ScriptVm *)(vmAddress));
    firstObject = objectResolveAvailableHandle(firstHandle);
    secondObject = objectResolveAvailableHandle(secondHandle);
    if ((firstObject == NULL) || (secondObject == NULL)) {
        scriptVmPushFloat((ScriptVm *)(vmAddress), -1.0F);
    } else {
        scriptVmPushFloat((ScriptVm *)(vmAddress), objectGetApproxDistance(firstObject, secondObject));
    }
    return 0;
}

/// Provide the currently selected backdrop identifier.
/// \param actor Actor whose script VM receives the active backdrop identifier.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120718 (0x80120718).
s32 actorScriptPushActiveBackdrop(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(vmAddress), backdropGetActiveId());
    return 0;
}

/// Select and load a backdrop for the primary view.
/// \param actor Actor whose script VM supplies a retail backdrop descriptor index from one through 23 and receives zero; the primary view must have an allocated buffer of at least 112000 bytes. Descriptor zero has no loadable source.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120750 (0x80120750).
s32 actorScriptSelectBackdrop(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    backdropSelectForView(0, scriptVmPopInteger((ScriptVm *)(vmAddress)), 0);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

#include "scene_configuration.h"

/// Provide the current scene near clipping distance.
/// \param actor Actor whose script VM receives the near clipping distance as a floating-point value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120798 (0x80120798).
s32 actorScriptPushNearClipDistance(u8 *actor) {
    s32 vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;

    scriptVmPushFloat((ScriptVm *)(vmAddress), gCurrentAreaConfiguration->nearClip);
    return 0;
}

/// Provide the current scene far clipping distance.
/// \param actor Actor whose script VM receives the far clipping distance as a floating-point value.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801207CC (0x801207CC).
s32 actorScriptPushFarClipDistance(u8 *actor) {
    s32 vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;

    scriptVmPushFloat((ScriptVm *)(vmAddress), gCurrentAreaConfiguration->farClip);
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
    gCurrentAreaConfiguration->nearClip = scriptVmPopFloat((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
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
    gCurrentAreaConfiguration->farClip = scriptVmPopFloat((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Provide the current scene fog start value.
/// \param actor Actor whose script VM receives the fog start value as a floating-point number.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120888 (0x80120888).
s32 actorScriptPushFogStart(u8 *actor) {
    scriptVmPushFloat((ScriptVm *)(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress),
                      (f32)gCurrentAreaConfiguration->fogStart);
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
    gCurrentAreaConfiguration->fogStart = (s32)scriptVmPopFloat((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Provide the current scene fog end value.
/// \param actor Actor whose script VM receives the fog end value as a floating-point number.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120918 (0x80120918).
s32 actorScriptPushFogEnd(u8 *actor) {
    scriptVmPushFloat((ScriptVm *)(((RuntimeObjectPrefix *)actor)->state.actor->vmAddress),
                      (f32)gCurrentAreaConfiguration->fogEnd);
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
    gCurrentAreaConfiguration->fogEnd = (s32)scriptVmPopFloat((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Try to place an item in the player hand, replacing the previously held item on success.
/// \param actor Actor whose script VM supplies an item handle and receives one on successful pickup, or zero if the item is unavailable or cannot be picked up.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801209A8 (0x801209A8).
s32 actorScriptPlayerTryHoldItem(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(sp1C), playerPickUpItem(scriptVmPopInteger((ScriptVm *)(sp1C))));
    return 0;
}

/// Create a scrolling text board with preset text, appearance, and transform settings.
/// \param actor Actor whose script VM supplies, in order, a text preset index of zero or one, text style (zero through two, or another value for the default), positive width in twelve-pixel columns, positive height in sixteen-pixel lines, position X/Y/Z, rotation X/Y/Z in degrees, red/green/blue/alpha components from zero through 255, top-left and bottom-right outline palette indices, uniform scale relative to the scene scale, and scroll speed in pixels per second. Outline indices must be zero through 15, or minus one to disable the corresponding outline; the VM receives the new board hierarchy handle, to release through hierarchy destruction.
/// \returns Zero, allowing script execution to continue.
/// \pre Columns are one through 85. Preset zero supports one through 132 visible lines; preset one supports one.
/// \note Preset one has only one valid text entry; later scrolling into its remaining entries is unsupported.
/// \note Original decompilation identifier: \c func_801209E8 (0x801209E8).
s32 actorScriptCreateScrollBoard(u8 *actor) {
    s32 vmAddress;
    u8 *hierarchyRoot;
    s32 preset;
    s32 style;
    s32 columns;
    s32 lines;
    f32 position[3];
    f32 rotation[3];
    s32 red;
    s32 green;
    s32 blue;
    s32 alpha;
    s32 leadingOutline;
    s32 trailingOutline;
    f32 scale;
    f32 scrollSpeed;
    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scrollSpeed = scriptVmPopFloat((ScriptVm *)(vmAddress));
    scale = scriptVmPopFloat((ScriptVm *)(vmAddress));
    trailingOutline = scriptVmPopInteger((ScriptVm *)(vmAddress));
    leadingOutline = scriptVmPopInteger((ScriptVm *)(vmAddress));
    alpha = scriptVmPopInteger((ScriptVm *)(vmAddress));
    blue = scriptVmPopInteger((ScriptVm *)(vmAddress));
    green = scriptVmPopInteger((ScriptVm *)(vmAddress));
    red = scriptVmPopInteger((ScriptVm *)(vmAddress));
    rotation[2] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    rotation[1] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    rotation[0] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    position[2] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    position[1] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    position[0] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    lines = scriptVmPopInteger((ScriptVm *)(vmAddress));
    columns = scriptVmPopInteger((ScriptVm *)(vmAddress));
    style = scriptVmPopInteger((ScriptVm *)(vmAddress));
    preset = scriptVmPopInteger((ScriptVm *)(vmAddress));
    hierarchyRoot =
        scrollBoardCreateHierarchy(columns, lines, red, green, blue, alpha, leadingOutline, trailingOutline);
    scrollBoardSelectPresetText(((RuntimeObjectPrefix *)hierarchyRoot)->handle, preset, style);
    scrollBoardSetHierarchySpeed(hierarchyRoot, scrollSpeed);
    objectSetUniformTransform(hierarchyRoot, position, rotation, gCurrentAreaConfiguration->objectScale * scale);
    scriptVmPushInteger((ScriptVm *)(vmAddress), ((RuntimeObjectPrefix *)hierarchyRoot)->handle);
    return 0;
}

/// Select preset text for a scrolling-board hierarchy.
/// \param actor Actor whose script VM supplies a live scrolling-board hierarchy handle, a text preset index of zero or one, and a text style, in that order; the VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \pre The board has one through 132 visible lines for preset zero, or one for preset one.
/// \note Text selection preserves speed and borrows the preset list. Preset one later entries are not valid strings.
/// \note Original decompilation identifier: \c func_80120B78 (0x80120B78).
s32 actorScriptSelectScrollBoardText(u8 *actor) {
    s32 hierarchyHandle;
    s32 vmAddress;
    s32 presetIndex;
    s32 textStyle;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    textStyle = scriptVmPopInteger((ScriptVm *)(vmAddress));
    presetIndex = scriptVmPopInteger((ScriptVm *)(vmAddress));
    hierarchyHandle = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scrollBoardSelectPresetText(hierarchyHandle, presetIndex, textStyle);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Change the scrolling speed of a scrolling-board hierarchy.
/// \param actor Actor whose script VM supplies a live scrolling-board hierarchy handle followed by a floating-point speed in pixels per second, and receives zero; positive speeds advance, negative speeds reverse, and zero stops scrolling.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120BDC (0x80120BDC).
s32 actorScriptSetScrollBoardSpeed(u8 *actor) {
    s32 vmAddress;
    s32 hierarchyHandle;
    s32 unusedVmAddress;
    f32 pixelsPerSecond;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    unusedVmAddress = vmAddress;
    pixelsPerSecond = scriptVmPopFloat((ScriptVm *)(vmAddress));
    hierarchyHandle = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scrollBoardSetHierarchySpeed(objectFindLiveByHandle(hierarchyHandle), pixelsPerSecond);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Set a live object position, orientation, and uniform scale.
/// Dependent bounds and descendant transforms are not refreshed by this request.
/// \param actor Actor whose script VM supplies a live object handle, position X/Y/Z, rotation X/Y/Z in degrees, and a uniform scale relative to the scene scale, in that order; the VM receives zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120C34 (0x80120C34).
s32 actorScriptSetObjectTransform(u8 *actor) {
    s32 targetHandle;
    u8 *target;
    s32 vmAddress;
    f32 position[3];
    f32 rotation[3];
    f32 scale;
    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scale = scriptVmPopFloat((ScriptVm *)(vmAddress));
    rotation[2] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    rotation[1] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    rotation[0] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    position[2] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    position[1] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    position[0] = scriptVmPopFloat((ScriptVm *)(vmAddress));
    targetHandle = scriptVmPopInteger((ScriptVm *)(vmAddress));
    target = objectFindLiveByHandle(targetHandle);
    objectSetUniformTransform(target, position, rotation, gCurrentAreaConfiguration->objectScale * scale);
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Provide the sine of a script-supplied angle.
/// \param actor Actor whose script VM supplies an angle in degrees satisfying mathSinDegrees input limits and receives its approximate sine.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120CF8 (0x80120CF8).
s32 actorScriptSinDegrees(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat((ScriptVm *)(sp1C), mathSinDegrees(scriptVmPopFloat((ScriptVm *)(sp1C))));
    return 0;
}

/// Provide the cosine of a script-supplied angle.
/// \param actor Actor whose script VM supplies an angle in degrees satisfying mathCosDegrees input limits and receives its approximate cosine.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120D38 (0x80120D38).
s32 actorScriptCosDegrees(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat((ScriptVm *)(sp1C), mathCosDegrees(scriptVmPopFloat((ScriptVm *)(sp1C))));
    return 0;
}

/// Extents of the bedroom name activity's catalog and recent-selection history.
enum { BEDROOM_POKEMON_NAME_COUNT = 153, BEDROOM_POKEMON_NAME_HISTORY_CAPACITY = 100 };

/// Voice dictionary groups indexed by bedroom name-catalog entry.
/// \note Original identifier: \c D_8015516C (0x8015516C).
extern u8 gBedroomPokemonNameDictionaryGroups[BEDROOM_POKEMON_NAME_COUNT];

/// Provide the voice dictionary group associated with a Pokémon name catalog entry.
/// \param actor Actor whose script VM supplies a catalog index from zero through 152 and receives its voice dictionary group; requires the bedroom scene data.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120D78 (0x80120D78).
s32 actorScriptGetPokemonNameDictionaryGroup(u8 *actor) {
    s32 vmAddress;
    s32 catalogIndex;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    catalogIndex = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress),
                        *(u8 *)((u8 *)&gBedroomPokemonNameDictionaryGroups + (catalogIndex * 1)));
    return 0;
}

/// Bedroom name-catalog string references indexed by catalog entry.
/// \note Original identifier: \c D_80155208 (0x80155208).
extern char *gBedroomPokemonNames[BEDROOM_POKEMON_NAME_COUNT];

/// Provide a selected name from the bedroom Pokémon name catalog.
/// \param actor Actor whose script VM supplies a catalog index from zero through 152 and receives a borrowed name string valid while the bedroom scene data remains loaded.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120DB8 (0x80120DB8).
s32 actorScriptGetPokemonName(u8 *actor) {
    s32 vmAddress;
    s32 catalogIndex;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    catalogIndex = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushStringPointer((ScriptVm *)(vmAddress),
                              (char *)(*(s32 *)((u8 *)&gBedroomPokemonNames + (catalogIndex * 4))));
    return 0;
}

/// Ring history of up to 100 previously selected bedroom name-catalog indices.
/// \note Original identifier: \c D_8015546C (0x8015546C).
extern u8 gBedroomPokemonNameHistory[BEDROOM_POKEMON_NAME_HISTORY_CAPACITY];
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
/// \param actor Actor whose script VM receives the selected catalog index from zero through 152; requires initialized bedroom scene data and name-selection history, and the generator must eventually select an index outside that history.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120DFC (0x80120DFC).
s32 actorScriptChoosePokemonName(u8 *actor) {
    s32 vmAddress;
    s32 head;
    s32 tail;
    s32 count;
    s32 candidate;
    s32 index;
    s32 checkedHistoryCount;
    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    head = gBedroomPokemonNameHistoryHead;
    tail = gBedroomPokemonNameHistoryTail;
    count = gBedroomPokemonNameHistoryCount;
    do {
        candidate = randomBelow(BEDROOM_POKEMON_NAME_COUNT);
        index = tail;
        for (checkedHistoryCount = 0; checkedHistoryCount < count; checkedHistoryCount++) {
            if (candidate == gBedroomPokemonNameHistory[index++]) {
                candidate = -1;
                break;
            }
            if (index == BEDROOM_POKEMON_NAME_HISTORY_CAPACITY) {
                index = 0;
            }
        }
    } while (candidate == -1);
    gBedroomPokemonNameHistory[head++] = candidate;
    if (head == BEDROOM_POKEMON_NAME_HISTORY_CAPACITY) {
        head = 0;
    }
    if (count < BEDROOM_POKEMON_NAME_HISTORY_CAPACITY) {
        count++;
    } else {
        tail++;
        if (tail == BEDROOM_POKEMON_NAME_HISTORY_CAPACITY) {
            tail = 0;
        }
    }
    gBedroomPokemonNameHistoryHead = head;
    gBedroomPokemonNameHistoryTail = tail;
    gBedroomPokemonNameHistoryCount = count;
    scriptVmPushInteger((ScriptVm *)(vmAddress), candidate);
    return 0;
}

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
    sp2C = scriptVmPopInteger((ScriptVm *)(vmAddress));
    sp30 = scriptVmPopInteger((ScriptVm *)(vmAddress));
    sp34 = scriptVmPopFloat((ScriptVm *)(vmAddress));
    sp38 = scriptVmPopFloat((ScriptVm *)(vmAddress));
    sp3C = scriptVmPopInteger((ScriptVm *)(vmAddress));
    sp_pop = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress), itemFindHandleFromPlayerView(sp_pop, sp3C, sp38, sp34, sp30, sp2C));
    return 0;
}

/// Find the nearest other object within the player viewing cone using actor-kind aliases.
/// \param actor Actor excluded from the search whose script VM supplies inclusive minimum and maximum kinds, exclusive maximum approximate distance, and maximum horizontal angle in degrees; kind 38 selects the corresponding actor-range endpoint, while other ranges can include nonactors; angles of at least 360 allow every direction. The VM receives the nearest qualifying handle or zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80120FD0 (0x80120FD0).
s32 actorScriptFindActorFromPlayerView(u8 *actor) {
    s32 minimumKind;
    s32 vmAddress;
    s32 maximumKind;
    f32 maximumDistance;
    f32 maximumAngleDegrees;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    maximumAngleDegrees = scriptVmPopFloat((ScriptVm *)(vmAddress));
    maximumDistance = scriptVmPopFloat((ScriptVm *)(vmAddress));
    maximumKind = scriptVmPopInteger((ScriptVm *)(vmAddress));
    minimumKind = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress), actorFindNearestFromPlayerView(actor, minimumKind, maximumKind,
                                                                                maximumDistance, maximumAngleDegrees));
    return 0;
}

/// Find the nearest other object within the player viewing cone.
/// \param actor Actor excluded from the search whose script VM supplies inclusive minimum and maximum kinds, exclusive maximum approximate distance, and maximum horizontal angle in degrees; angles of at least 360 allow every direction. The VM receives the nearest qualifying handle or zero.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121050 (0x80121050).
s32 actorScriptFindObjectFromPlayerView(u8 *actor) {
    s32 minimumKind;
    s32 vmAddress;
    s32 maximumKind;
    f32 maximumDistance;
    f32 maximumAngleDegrees;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    maximumAngleDegrees = scriptVmPopFloat((ScriptVm *)(vmAddress));
    maximumDistance = scriptVmPopFloat((ScriptVm *)(vmAddress));
    maximumKind = scriptVmPopInteger((ScriptVm *)(vmAddress));
    minimumKind = scriptVmPopInteger((ScriptVm *)(vmAddress));
    scriptVmPushInteger((ScriptVm *)(vmAddress), objectFindNearestFromPlayerView(actor, minimumKind, maximumKind,
                                                                                 maximumDistance, maximumAngleDegrees));
    return 0;
}

/// Report the player heading relative to the direction toward the actor.
/// \param actor Actor whose script VM receives the player heading, including horizontal view shake, minus the direction toward this actor in degrees greater than -180 and at most 180.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801210D0 (0x801210D0).
s32 actorScriptPushPlayerFacingError(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat((ScriptVm *)(vmAddress), objectGetPlayerFacingError(actor));
    return 0;
}

/// Report the actor heading relative to the direction toward a target.
/// \param actor Actor whose script VM supplies a target identifier and receives its heading minus the target direction in degrees from zero inclusive to 360 exclusive, or minus one when the target is unavailable.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121108 (0x80121108).
s32 actorScriptGetTargetFacingError(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushFloat((ScriptVm *)(vmAddress),
                      objectGetFacingErrorToTarget(actor, scriptVmPopInteger((ScriptVm *)(vmAddress))));
    return 0;
}

/// Set an available item size relative to its kind and scene scale.
/// \param actor Actor whose script VM supplies a handle identifying an initialized item when available, followed by a floating-point relative scale, and receives zero; unavailable handles are unchanged.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121150 (0x80121150).
s32 actorScriptSetItemScale(u8 *actor) {
    s32 sp1C;
    f32 sp18;
    u8 *resolvedObject;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    sp18 = scriptVmPopFloat((ScriptVm *)(sp1C));
    resolvedObject = objectResolveAvailableHandle(scriptVmPopInteger((ScriptVm *)(sp1C)));
    if (resolvedObject != 0) {
        itemSetScale(resolvedObject, sp18);
    }
    scriptVmPushInteger((ScriptVm *)(sp1C), 0);
    return 0;
}

/// Dictionary lookup index retained by translation, including its legacy distance-based fallback.
/// \note Original identifier: \c D_800722E0 (0x800722E0).
extern s32 gVoiceCommandResultIndex;

/// Provide the legacy script voice-result query supported by this build.
/// \param actor Actor whose script VM receives zero when no recognition result is available, or an unspecified value otherwise.
/// \returns Zero, allowing script execution to continue.
/// \note Portability/UB: A nonnegative recognition-result index uses an uninitialized local halfword. The matched ROM reads native-stack storage before testing the index, including on the zero-result branch. This preserves target code generation and does not define a portable C result.
/// \note Original decompilation identifier: \c func_801211AC (0x801211AC).
s32 actorScriptPushLegacyVoiceResult(u8 *actor) {
    s32 vmAddress;
    s32 recognitionResultIndex;
    u16 uninitializedStackWords[16];

    recognitionResultIndex = gVoiceCommandResultIndex;
    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(vmAddress), (recognitionResultIndex < 0) ? 0 : uninitializedStackWords[0]);
    return 0;
}

/// Provide the current story chapter.
/// \param actor Actor whose script VM receives the current chapter number.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801211F0 (0x801211F0).
s32 actorScriptPushCurrentChapter(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(sp1C), progressGetCurrentChapter());
    return 0;
}

/// Update the story chapter and unlock milestones according to completed visit objectives.
/// \param actor Actor whose script VM receives the resulting chapter number, which may remain unchanged or return to an earlier chapter.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121228 (0x80121228).
s32 actorScriptUpdateChapterAfterVisit(u8 *actor) {
    s32 sp1C;

    sp1C = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(sp1C), progressUpdateChapterAfterVisit());
    return 0;
}

/// Provide the nominal remaining item-object capacity.
/// \param actor Actor whose script VM receives the nominal spare item count, which does not guarantee allocation.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121260 (0x80121260).
s32 actorScriptPushAvailableItemSlots(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(vmAddress), itemGetAvailableSlots());
    return 0;
}

/// Reset all global script variables to their initial values.
/// \param actor Actor whose script VM receives zero after the shared global variables are reset.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121298 (0x80121298).
s32 actorScriptResetGlobalVariables(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmResetGlobals();
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}

/// Provide the current startup and title-screen state.
/// \param actor Actor whose script VM receives the current title-screen state identifier.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_801212D0 (0x801212D0).
s32 actorScriptPushTitleScreenState(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    scriptVmPushInteger((ScriptVm *)(vmAddress), titleScreenGetState());
    return 0;
}

/// Request the main menu when the title screen is awaiting Start.
/// \param actor Actor whose script VM receives zero after the request; other title-screen states are unaffected.
/// \returns Zero, allowing script execution to continue.
/// \note Original decompilation identifier: \c func_80121308 (0x80121308).
s32 actorScriptRequestTitleMenu(u8 *actor) {
    s32 vmAddress;

    vmAddress = ((RuntimeObjectPrefix *)actor)->state.actor->vmAddress;
    titleScreenRequestMenu();
    scriptVmPushInteger((ScriptVm *)(vmAddress), 0);
    return 0;
}
