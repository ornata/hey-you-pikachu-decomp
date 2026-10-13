/// \file
/// Declares game program initialization and update entry points used by runtime dispatch.

#ifndef RUNTIME_LIFECYCLE_H
#define RUNTIME_LIFECYCLE_H

#include <ultra64.h>

/// Provide the inactive action-script initialization hook for runtime startup.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800121A4 (0x800121A4).
void actionScriptInitializeHook(void);

/// Initialize HUD animations and the area label and restore the default visibility settings.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80023098 (0x80023098).
void hudInitialize(void);

/// Load the current area’s rendering assets for use by its scene.
/// \returns Nothing; requires a loaded area configuration and no existing area asset allocation to replace.
/// \note Original address: 0x800006E4.
void renderLoadAreaAssets(void);

/// Initialize the runtime object system and its supporting registries.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80009808 (0x80009808).
void objectSystemInitialize(void);

/// Prepare the current area’s card presentation and owned collection result text.
/// Requires a supported area, initialized area data, and no unreleased presentation text images.
/// The selected collection title must fit its destination and contain at most 79 bytes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002AA08 (0x8002AA08).
void presentationInitializeForCurrentArea(void);

/// Create the toolbox display, empty-slot markers, row titles, and stored item objects.
/// \returns Nothing; requires the saved toolbox contents and scene resources.
/// \note Original decompilation identifier: \c func_8002FB64 (0x8002FB64).
void toolboxCreateDisplayObjects(void);

/// Initialize the current area's maze, prepare its objects, and spawn its configured items.
/// \returns Nothing; requires a loaded area configuration and no existing maze to replace.
/// \note Original decompilation identifier: \c func_8010E4C0 (0x8010E4C0).
/// \note The retained vector spans three adjacent linker symbols; its byte layout is target-specific
/// and the indexed access relies on the original target compiler aliasing behavior.
void mazeInitialize(void);

/// Invoke the current area's optional maze update callback.
/// \returns Nothing; has no effect when the callback is absent.
///
/// \note Requires a loaded current-area configuration; a configured callback must be callable with no
/// arguments.
/// \note Original decompilation identifier: \c func_8010E53C (0x8010E53C).
void mazeRunAreaUpdateCallback(void);

/// Make the selected destination and its loaded configuration the current area.
/// \returns Nothing; requires the destination area's overlay to be loaded.
/// \note Original decompilation identifier: \c func_8010E60C (0x8010E60C).
void areaActivateDestination(void);

/// Initialize the primary player at the selected area spawn with its control objects and initial camera and
/// audio settings.
/// \returns Nothing.
///
/// \note Requires a loaded area configuration, initialized maze and object system, and a valid selected
/// spawn. Existing player-control hierarchies must be released before reinitialization.
/// \note Original decompilation identifier: \c func_80111F40 (0x80111F40).
void playerInitializeForArea(void);

/// Create the active Pikachu actor at the selected area spawn with the current scenario script.
/// \returns Nothing.
///
/// \note Requires a loaded area configuration, valid spawn and scenario indices, and initialized actor
/// resources. Release any previous active Pikachu before replacing it. Portability: actor and script
/// addresses use 32-bit integer storage.
/// \note Original decompilation identifier: \c func_80112CA0 (0x80112CA0).
void pikachuSpawnForArea(void);

/// Initialize an empty item registry for the current area.
/// \returns Nothing.
///
/// \note Release all previous registry entries before reinitializing; existing item search nodes must no
/// longer be linked.
/// \note Original decompilation identifier: \c func_80113C78 (0x80113C78).
void itemInitializeRegistry(void);

/// Prepare system-menu state, owned models and text, and entries available for the current game progress.
/// Requires initialized area, object, and text systems, fresh static menu lists, and no unreleased menu resources.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011BAC8 (0x8011BAC8).
void systemMenuInitialize(void);

/// Advance the current card presentation and return to gameplay when it finishes.
/// Requires initialized area and actor state and a chase stage from zero through two.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80028B84 (0x80028B84).
void presentationUpdateCardMode(void);

/// Advance the collection score and chapter result presentation, then leave the results mode when it finishes.
/// Requires initialized actor, area, and inventory state; the awarded points must fit in signed 32 bits.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80029CD4 (0x80029CD4).
void presentationUpdateCollectionResultsMode(void);

/// Update the whole-map menu, its actor, and transitions into or out of destination selection.
/// \pre Requires an active actor.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002E6C4 (0x8002E6C4).
void wholeMapUpdate(void);

/// Handle toolbox navigation, item descriptions, transfers, and closing requests.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_80031398 (0x80031398).
void toolboxHandleInput(void);

/// Update the toolbox presentation and its current display animation.
/// \returns Nothing; requires initialized toolbox display objects.
/// \note Original decompilation identifier: \c func_80031ECC (0x80031ECC).
void toolboxUpdateDisplay(void);

/// Advance startup checks, title presentation, save-file menus, and transitions into gameplay or a demonstration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800357C4 (0x800357C4).
void titleScreenUpdate(void);

/// Load and initialize the selected eye-catch interlude.
/// Requires a selected group from zero through five and no unreleased interlude.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80036AF4 (0x80036AF4).
void eyeCatchBegin(void);

/// Advance the active eye-catch stage and report when the interlude completes.
/// Requires an interlude prepared by eyeCatchBegin; stop updating after completion.
/// \returns One when the interlude completes, otherwise zero.
/// \note Original decompilation identifier: \c func_800378F8 (0x800378F8).
s32 eyeCatchUpdate(void);

/// Advance the active Pikachu script and service area events at its world position; has no effect when no
/// Pikachu actor is selected.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80112F10 (0x80112F10).
void pikachuUpdateScriptAndTriggers(void);

/// Handle system-menu navigation, item activation, and requests to close the menu.
/// Requires initialized menu and player state, valid menu selection, and history capacity for any submenu.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011A0D8 (0x8011A0D8).
void systemMenuHandleInput(void);

/// Advance the system-menu animation and transitions for the current phase.
/// Requires initialized menu models, player, camera, and area state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011A778 (0x8011A778).
void systemMenuUpdateAnimation(void);

#endif
