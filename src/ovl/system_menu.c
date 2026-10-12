#include "camera_interface.h"
#include "action_script_stop.h"
#include "action_script_query.h"
#include "object_flags.h"
#include "object_hierarchy_create.h"
#include "object_lifecycle.h"
#include "heap_diagnostics.h"
#include "player_friendship.h"
#include "backdrop_draw.h"
#include "system_menu.h"
#include "common.h"
#include "runtime_clock.h"
#include "text_image_create.h"
#include "audio_cue.h"
#include "text_draw.h"
#include "scene_types.h"
#include "heap_alloc.h"
#include "player_types.h"

#define TRACE                                                                                                          \
    if (0)                                                                                                             \
    osSyncPrintf

/* .bss 0x801340D0..0x80134220, in address order (IDO lays globals out in declaration order) */
/// Audio cue retained on menu entry for restoration.
/// \note Original identifier: \c D_801340D0 (0x801340D0).
s32 gSystemMenuSavedAudioCue;
s32 D_801340D0_0;
/// Audio playback rate retained on menu entry for restoration.
/// \note Original identifier: \c D_801340D8 (0x801340D8).
f64 gSystemMenuSavedAudioRate;
/// Tracked sound-effect preview stopped when the preview changes or the menu exits.
/// \note Original identifier: \c D_801340E0 (0x801340E0).
s32 gSystemMenuSoundPreviewHandle;
s32 D_801340E0_0;
struct MenuList;
struct MenuItem;
/// Owned menu navigation state with borrowed menu-tree links and retained model handles.
typedef struct MenuState {
    struct MenuList *menu;       ///< Borrowed current menu list.
    struct MenuList *history[3]; ///< Borrowed parent lists, retained while navigating.
    struct MenuItem *selected;   ///< Borrowed item whose details and row are displayed.
    s32 depth;
    s32 phase;
    s32 model;
    s32 secondary;
    s32 tertiary;
    u64 started;
} MenuState;
/// Owned menu navigation state with borrowed menu-tree links and retained model handles.
/// \note Original identifier: \c D_801340E8 (0x801340E8).
MenuState gSystemMenuState;

/// Menu title, item chain and retained selection indices.
typedef struct MenuList {
    char *title;                           ///< Borrowed display title, retained while the list is displayed.
    struct MenuItem *head;                 ///< Borrowed acyclic item chain.
    void (*action)(struct MenuList *menu); ///< Optional list-specific drawing and input callback.
    s32 selectedIndex;                     ///< Index used to resolve the selected menu item.
    s32 confirmedIndex;       ///< Confirmed setting row, or a negative value when confirmation is not tracked.
    s32 firstVisibleIndex;    ///< First item represented by the visible item rows.
    s32 count;                ///< Number of entries in the item chain.
    s32 suppressConfirmSound; ///< Nonzero when the list supplies its own selection feedback.
} MenuList;
/// Menu entry with optional selection action, detail callback and submenu.
typedef struct MenuItem {
    char *label;                 ///< Borrowed row label, retained while the item is displayed.
    void (*action)(s32 value);   ///< Optional selection callback receiving value.
    s32 value;                   ///< Argument supplied to the selection callback.
    char *text;                  ///< Optional borrowed detail message.
    void (*appendDetails)(void); ///< Optional callback appending details for the selected item.
    struct MenuItem *next;       ///< Next item in the containing list.
    MenuList *submenu;           ///< Optional borrowed list entered on selection.
} MenuItem;
typedef char MenuStateSizeCheck[(sizeof(MenuState) == 0x30) ? 1 : -1];
typedef char MenuListSizeCheck[(sizeof(MenuList) == 0x20) ? 1 : -1];
typedef char MenuItemSizeCheck[(sizeof(MenuItem) == 0x1C) ? 1 : -1];
#define MENU_OFFSET_CHECK(Name, Type, Field, Offset)                                                                   \
    typedef char Name[(((u32) & ((Type *)0)->Field) == (Offset)) ? 1 : -1]
MENU_OFFSET_CHECK(MenuStateSelectionOffsetCheck, MenuState, selected, 0x10);
MENU_OFFSET_CHECK(MenuStateTimeOffsetCheck, MenuState, started, 0x28);
MENU_OFFSET_CHECK(MenuListHeadOffsetCheck, MenuList, head, 4);
MENU_OFFSET_CHECK(MenuListVisibleOffsetCheck, MenuList, firstVisibleIndex, 0x14);
MENU_OFFSET_CHECK(MenuItemNextOffsetCheck, MenuItem, next, 0x14);
MENU_OFFSET_CHECK(MenuItemSubmenuOffsetCheck, MenuItem, submenu, 0x18);
#undef MENU_OFFSET_CHECK

/// Interior view at offset 0xA0 of a rendering view, borrowed by the menu.
/// This pair is not the complete view and cannot be allocated independently.
typedef struct MenuBackdropView {
    f32 scroll;      ///< Accumulated horizontal backdrop scroll.
    s32 useBackdrop; ///< Nonzero to draw the selected backdrop instead of the fill background.
} MenuBackdropView;
typedef char MenuBackdropViewSizeCheck[(sizeof(MenuBackdropView) == 8) ? 1 : -1];
typedef char MenuBackdropFlagOffsetCheck[(((u32) & ((MenuBackdropView *)0)->useBackdrop) == 4) ? 1 : -1];

/* 0x38-byte model records: unit scale, zero transform, segment address. */
typedef struct MenuModel {
    f32 scale[3];
    f32 transform[6];
    f32 unk24;
    s32 unk28;
    struct MenuModel *next;
    u32 segment;
    s32 unk34;
} MenuModel;

/* .data 0x8012A8D0..0x8012ACE0: the menu-model chain and the root menu tree.
 * Defined ahead of the code so their labels lead the unit's .rodata. */
void systemMenuAppendDaysDetails(void);
void systemMenuAppendPlayerStatusDetails(void);
void systemMenuUpdateVolumeSettings(MenuList *menu);
void systemMenuCancelSelection(s32 arg0);
void systemMenuGoHome(s32 arg0);
void systemMenuOpenWorldMap(s32 arg0);
void systemMenuSetAudioOutputMode(s32 arg0);
void systemMenuSetVerticalLookMode(s32 arg0);
void systemMenuAppendSaveStatusDetails(void);
void systemMenuRequestSaveTransfer(s32 arg0);
void systemMenuSyncLookSettingSelection(s32 arg0);
void systemMenuSyncSoundSettingSelection(s32 arg0);
/// Linked secondary model template in the menu transition hierarchy.
/// \note Original identifier: \c D_8012A908 (0x8012A908).
extern MenuModel gSystemMenuSecondaryModelTemplate;
/// Hierarchy template for the menu opening and closing model.
/// \note Original identifier: \c D_8012A8D0 (0x8012A8D0).
MenuModel gSystemMenuTransitionModelTemplate = {{1.0f, 1.0f, 1.0f},
                                                {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
                                                200.0f,
                                                0,
                                                &gSystemMenuSecondaryModelTemplate,
                                                0x0100AF28,
                                                0};
/// Linked secondary model template in the menu transition hierarchy.
/// \note Original identifier: \c D_8012A908 (0x8012A908).
MenuModel gSystemMenuSecondaryModelTemplate = {
    {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 200.0f, 0, NULL, 0x0100B2D8, 0};
/// Model template drawn while the menu accepts input or waits to close.
/// \note Original identifier: \c D_8012A940 (0x8012A940).
MenuModel gSystemMenuInteractiveModelTemplate = {
    {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 92.0f, 0, NULL, 0x0100BEE0, 0};
/// Randomly selected text row used by the menu closing effect.
/// \note Original identifier: \c D_8012A978 (0x8012A978).
s32 gSystemMenuClosingEffectRow = 0;
/// Persistent SaveControl menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012A97C (0x8012A97C).
MenuItem gSystemMenuSaveControlEntry = {
    "saveFCB", systemMenuRequestSaveTransfer, 5, "Save to EEPROM", systemMenuAppendSaveStatusDetails, NULL, NULL};
/// Persistent LoadControl menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012A998 (0x8012A998).
MenuItem gSystemMenuLoadControlEntry = {"loadFCB",
                                        systemMenuRequestSaveTransfer,
                                        4,
                                        "Load from EEPROM",
                                        systemMenuAppendSaveStatusDetails,
                                        &gSystemMenuSaveControlEntry,
                                        NULL};
/// Persistent SaveFileTwo menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012A9B4 (0x8012A9B4).
MenuItem gSystemMenuSaveFileTwoEntry = {"save 2",
                                        systemMenuRequestSaveTransfer,
                                        3,
                                        "Save to EEPROM\nfile 2.",
                                        systemMenuAppendSaveStatusDetails,
                                        &gSystemMenuLoadControlEntry,
                                        NULL};
/// Persistent SaveFileOne menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012A9D0 (0x8012A9D0).
MenuItem gSystemMenuSaveFileOneEntry = {"save 1",
                                        systemMenuRequestSaveTransfer,
                                        2,
                                        "Save to EEPROM\nfile 1.",
                                        systemMenuAppendSaveStatusDetails,
                                        &gSystemMenuSaveFileTwoEntry,
                                        NULL};
/// Persistent LoadFileTwo menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012A9EC (0x8012A9EC).
MenuItem gSystemMenuLoadFileTwoEntry = {"load 2",
                                        systemMenuRequestSaveTransfer,
                                        1,
                                        "Load from EEPROM\nfile 2.",
                                        systemMenuAppendSaveStatusDetails,
                                        &gSystemMenuSaveFileOneEntry,
                                        NULL};
/// Persistent LoadFileOne menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AA08 (0x8012AA08).
MenuItem gSystemMenuLoadFileOneEntry = {"load 1",
                                        systemMenuRequestSaveTransfer,
                                        0,
                                        "Load from EEPROM\nfile 1.",
                                        systemMenuAppendSaveStatusDetails,
                                        &gSystemMenuLoadFileTwoEntry,
                                        NULL};
/// Persistent Save menu list and retained selection state.
/// \note Original identifier: \c D_8012AA24 (0x8012AA24).
MenuList gSystemMenuSaveList = {"Save", &gSystemMenuLoadFileOneEntry, NULL, 0, -1, 0, 0, 0};
/// Persistent DebugSave menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AA44 (0x8012AA44).
MenuItem gSystemMenuDebugSaveEntry = {"Data Save", NULL, 0, "Debug", NULL, NULL, &gSystemMenuSaveList};
/// Persistent SoundEffectsVolume menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AA60 (0x8012AA60).
MenuItem gSystemMenuSoundEffectsVolumeEntry = {"Sounds", NULL, 0, NULL, NULL, NULL, NULL};
/// Persistent MusicVolume menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AA7C (0x8012AA7C).
MenuItem gSystemMenuMusicVolumeEntry = {"Songs", NULL, 0, NULL, NULL, &gSystemMenuSoundEffectsVolumeEntry, NULL};
/// Persistent Volume menu list and retained selection state.
/// \note Original identifier: \c D_8012AA98 (0x8012AA98).
MenuList gSystemMenuVolumeList = {"Volume", &gSystemMenuMusicVolumeEntry, systemMenuUpdateVolumeSettings, 0, -1, 0, 0,
                                  0};
/// Persistent Mono menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AAB8 (0x8012AAB8).
MenuItem gSystemMenuMonoEntry = {"Mono", systemMenuSetAudioOutputMode, 1, "Set the sound\n to Mono.", NULL, NULL, NULL};
/// Persistent Stereo menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AAD4 (0x8012AAD4).
MenuItem gSystemMenuStereoEntry = {"Stereo", systemMenuSetAudioOutputMode, 2,   "Set the sound\n to Stereo.",
                                   NULL,     &gSystemMenuMonoEntry,        NULL};
/// Persistent Sound menu list and retained selection state.
/// \note Original identifier: \c D_8012AAF0 (0x8012AAF0).
MenuList gSystemMenuSoundList = {"Sound", &gSystemMenuStereoEntry, NULL, 0, 0, 0, 0, 0};
/// Persistent NormalLook menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AB10 (0x8012AB10).
MenuItem gSystemMenuNormalLookEntry = {
    "Normal", systemMenuSetVerticalLookMode,
    1,        "In the\n View Mode,\nlook up when\n tilting the\n Control Stick\n forward.",
    NULL,     NULL,
    NULL};
/// Persistent ReversedLook menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AB2C (0x8012AB2C).
MenuItem gSystemMenuReversedLookEntry = {
    "Reverse", systemMenuSetVerticalLookMode,
    0,         "In the\n View Mode,\nlook down when\n tilting the\n Control Stick\n forward.",
    NULL,      &gSystemMenuNormalLookEntry,
    NULL};
/// Persistent Control menu list and retained selection state.
/// \note Original identifier: \c D_8012AB48 (0x8012AB48).
MenuList gSystemMenuControlList = {"Control", &gSystemMenuReversedLookEntry, NULL, 0, 0, 0, 0, 0};
/// Persistent ControlSettings menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AB68 (0x8012AB68).
MenuItem gSystemMenuControlSettingsEntry = {"Control@a",
                                            systemMenuSyncLookSettingSelection,
                                            0,
                                            "Change the\n View Mode\n control\n setting.\nPress @W@X\n#036to open.",
                                            NULL,
                                            NULL,
                                            &gSystemMenuControlList};
/// Persistent VolumeSettings menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AB84 (0x8012AB84).
MenuItem gSystemMenuVolumeSettingsEntry = {"Volume@a",
                                           NULL,
                                           0,
                                           "Change the\n volume\n settings.\n\nPress @W@X\n#036to open.",
                                           NULL,
                                           &gSystemMenuControlSettingsEntry,
                                           &gSystemMenuVolumeList};
/// Persistent SoundSettings menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012ABA0 (0x8012ABA0).
MenuItem gSystemMenuSoundSettingsEntry = {"Sound@a",
                                          systemMenuSyncSoundSettingSelection,
                                          0,
                                          "Set to \n Stereo or\n Mono.\n\nPress @W@X\n#036to open.",
                                          NULL,
                                          &gSystemMenuVolumeSettingsEntry,
                                          &gSystemMenuSoundList};
/// Persistent Settings menu list and retained selection state.
/// \note Original identifier: \c D_8012ABBC (0x8012ABBC).
MenuList gSystemMenuSettingsList = {"Settings", &gSystemMenuSoundSettingsEntry, NULL, 0, -1, 0, 0, 0};
/// Persistent CancelGoHome menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012ABDC (0x8012ABDC).
MenuItem gSystemMenuCancelGoHomeEntry = {"Never mind", systemMenuCancelSelection, 0, NULL, NULL, NULL, NULL};
/// Persistent ConfirmGoHome menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012ABF8 (0x8012ABF8).
MenuItem gSystemMenuConfirmGoHomeEntry = {"Go home", systemMenuGoHome, 0, NULL, NULL, &gSystemMenuCancelGoHomeEntry,
                                          NULL};
/// Persistent GoHomeConfirmation menu list and retained selection state.
/// \note Original identifier: \c D_8012AC14 (0x8012AC14).
MenuList gSystemMenuGoHomeConfirmationList = {"Go home?", &gSystemMenuConfirmGoHomeEntry, NULL, 1, -1, 0, 0, 0};
/// Persistent GoHome menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AC34 (0x8012AC34).
MenuItem gSystemMenuGoHomeEntry = {"Go home@a",
                                   NULL,
                                   0,
                                   "Choose to\ngo home.\n\n\nPress @W@X\n#036to open.",
                                   NULL,
                                   NULL,
                                   &gSystemMenuGoHomeConfirmationList};
/// Persistent WorldMap menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AC50 (0x8012AC50).
MenuItem gSystemMenuWorldMapEntry = {"world map", systemMenuOpenWorldMap, 0, "Debug", NULL, NULL, NULL};
/// Persistent Settings menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AC6C (0x8012AC6C).
MenuItem gSystemMenuSettingsEntry = {"Settings@a",
                                     NULL,
                                     0,
                                     "Change the\n game settings.\n\n\nPress @W@X\n#036to open.",
                                     NULL,
                                     NULL,
                                     &gSystemMenuSettingsList};
/// Persistent Days menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AC88 (0x8012AC88).
MenuItem gSystemMenuDaysEntry = {"Days", NULL, 0, NULL, systemMenuAppendDaysDetails, &gSystemMenuSettingsEntry, NULL};
/// Persistent PlayerStatus menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012ACA4 (0x8012ACA4).
MenuItem gSystemMenuPlayerStatusEntry = {
    "Status", NULL, 0, NULL, systemMenuAppendPlayerStatusDetails, &gSystemMenuDaysEntry, NULL};
/// Persistent Root menu list and retained selection state.
/// \note Original identifier: \c D_8012ACC0 (0x8012ACC0).
MenuList gSystemMenuRootList = {"Menu", &gSystemMenuPlayerStatusEntry, NULL, 0, -1, 0, 0, 0};
/// Base position used to place the system-menu model.
/// \note Original identifier: \c D_8012ACE0 (0x8012ACE0).
f32 gSystemMenuModelPosition[3] = {0.0f, 0.0f, -13.0f};
/// RGBA color drawn behind the system menu.
/// \note Original identifier: \c D_8012ACEC (0x8012ACEC).
u8 gSystemMenuBackdropColor[4] = {0, 0, 0, 0x80};
/// Interior view of the current menu navigation history depth.
/// \note Original identifier: \c D_801340FC (0x801340FC).
extern s32 gSystemMenuHistoryDepth;
/// Interior view of the menu opening, interaction and closing phase.
/// \note Original identifier: \c D_80134100 (0x80134100).
extern s32 gSystemMenuPhase;
/// Interior view of the owned model used during menu opening and closing.
/// \note Original identifier: \c D_80134104 (0x80134104).
extern s32 gSystemMenuTransitionModel;
/// Interior view of the linked secondary menu transition model.
/// \note Original identifier: \c D_80134108 (0x80134108).
extern s32 gSystemMenuSecondaryModel;
/// Interior view of the owned model drawn during menu interaction.
/// \note Original identifier: \c D_8013410C (0x8013410C).
extern s32 gSystemMenuInteractiveModel;
/// Interior view of the menu phase start time as a full 64-bit timestamp.
/// \note Original identifier: \c D_80134110 (0x80134110).
extern s32 gSystemMenuPhaseStartMilliseconds;
/// Interior view of the low word of the menu phase start timestamp.
/// \note Original identifier: \c D_80134114 (0x80134114).
extern s32 gSystemMenuPhaseStartMillisecondsLow;
struct {
    s32 w[12];
    /// Camera-derived transform used to position system-menu models.
    /// \note Original identifier: \c D_80134118 (0x80134118).
} gSystemMenuInterfaceTransform;
/// Owned text image marking the currently selected menu row.
/// \note Original identifier: \c D_80134148 (0x80134148).
s32 gSystemMenuSelectedRowMarkerImage;
/// Owned text image marking the currently configured setting row.
/// \note Original identifier: \c D_8013414C (0x8013414C).
s32 gSystemMenuSettingRowMarkerImage;
/// Double-buffered title and item-row text images.
/// \note Original identifier: \c D_80134150 (0x80134150).
u8 *gSystemMenuRowImages[6][2];
/// Separate view of the five double-buffered item rows following the title row.
/// \note Original identifier: \c D_80134158 (0x80134158).
extern u8 *gSystemMenuItemRowImages[5][2];
/// Double-buffered text images for six selected-item detail rows.
/// \note Original identifier: \c D_80134180 (0x80134180).
u8 *gSystemMenuDetailImages[6][2];
/// Selected render buffer for title and item-row images.
/// \note Original identifier: \c D_801341B0 (0x801341B0).
s32 gSystemMenuRowImageBuffer;
/// Selected render buffer for selected-item detail images.
/// \note Original identifier: \c D_801341B4 (0x801341B4).
s32 gSystemMenuDetailImageBuffer;
/// Shared bounded text accumulator for selected-item details.
/// \note Original identifier: \c D_801341B8 (0x801341B8).
s8 gSystemMenuDetailText[0x60];
/// Number of bytes currently stored in the selected-item detail accumulator.
/// \note Original identifier: \c D_80134218 (0x80134218).
s32 gSystemMenuDetailTextLength;
s32 D_80134218_0;

f64 fabs(f64);
#pragma intrinsic(fabs)
s32 audioStopPlayback(s32, f64);      /* extern */

/// Stop the menu sound preview and restore the audio cue and playback rate saved on menu entry.
/// Requires a saved menu-entry audio snapshot and any preview handle to remain valid.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119570 (0x80119570).
void systemMenuRestoreAudio(void) {
    s32 sp34;
    f64 sp28;

    if (gSystemMenuSoundPreviewHandle != 0) {
        audioStopPlayback(gSystemMenuSoundPreviewHandle, 0.0);
    }
    audioGetCurrentCueState(&sp34, &sp28);
    if ((sp34 != gSystemMenuSavedAudioCue) || (fabs(sp28 - gSystemMenuSavedAudioRate) > 0.001f)) {
        audioChangeCurrentCue(gSystemMenuSavedAudioCue, 1.0, 1.0);
        audioSetCurrentCueRate(gSystemMenuSavedAudioRate);
    }
}

/// Owned menu navigation state with borrowed menu-tree links and retained model handles.
/// \note Original identifier: \c D_801340E8 (0x801340E8).
extern MenuState gSystemMenuState;

/// Begin a system-menu phase with a fresh elapsed-time origin.
/// \param phase Menu phase: zero opens, one accepts input, two waits to close, and three closes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119610 (0x80119610).
void systemMenuSetPhase(s32 phase) {
    gSystemMenuState.started = runtimeGetElapsedMilliseconds();
    gSystemMenuState.phase = phase;
}


/// Save the current menu as the parent of the next submenu.
/// Requires an initialized current menu and a history depth from zero through two.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119644 (0x80119644).
void systemMenuPushHistory(void) {
    gSystemMenuState.history[gSystemMenuState.depth] = gSystemMenuState.menu;
    gSystemMenuState.depth = gSystemMenuState.depth + 1;
}

/// Persistent Root menu list and retained selection state.
/// \note Original identifier: \c D_8012ACC0 (0x8012ACC0).
extern MenuList gSystemMenuRootList;
/// Return to the parent menu, or the root menu when no parent remains.
/// Requires an initialized history with depth from zero through three.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119670 (0x80119670).
void systemMenuPopHistory(void) {
    MenuState *state = &gSystemMenuState;
    MenuList **menu = &gSystemMenuState.menu;
    if (--state->depth < 0) {
        gSystemMenuHistoryDepth = 0;
        *menu = &gSystemMenuRootList;
        return;
    }
    *menu = state->history[gSystemMenuHistoryDepth];
}

/// Find the item selected by a menu cursor.
/// \param menu Live menu list with an acyclic item chain; its current selection is queried.
/// \returns Borrowed selected menu item, valid while its list remains alive, or null when the cursor has no corresponding item.
/// \note Original decompilation identifier: \c func_801196C8 (0x801196C8).
MenuItem *systemMenuGetSelectedItem(MenuList *menu) {
    MenuItem *item = menu->head;
    s32 i = 0;

    while (item != NULL) {
        if (i == menu->selectedIndex) {
            return item;
        }
        item = item->next;
        i++;
    }
    return NULL;
}

/// Refresh the recorded number of items in a menu.
/// \param menu Live menu list with an acyclic item chain and an item count that fits in signed 32 bits.
/// \returns The number of menu items.
/// \note Original decompilation identifier: \c func_80119708 (0x80119708).
s32 systemMenuRefreshItemCount(MenuList *menu) {
    s32 var_v1;
    MenuItem *var_v0;

    var_v0 = menu->head;
    var_v1 = 0;
    if (var_v0 != NULL) {
        do {
            var_v0 = var_v0->next;
            var_v1 += 1;
        } while (var_v0 != NULL);
    }
    menu->count = var_v1;
    return var_v1;
}

/// Append text to the current system-menu detail message.
/// Requires an initialized accumulator with a current length from zero through 95 bytes.
/// \param text Null-terminated text to append; excess characters are discarded so the combined message contains at most 95 bytes. The text must not overlap the detail accumulator.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119734 (0x80119734).
void systemMenuAppendDetailText(u8 *text) {
    while (*text != 0) {
        if (gSystemMenuDetailTextLength >= 0x5F) {
            TRACE("right menu buffer overflow.\n");
            break;
        }
        gSystemMenuDetailText[gSystemMenuDetailTextLength] = *text;
        gSystemMenuDetailTextLength++;
        text++;
    }
    gSystemMenuDetailText[gSystemMenuDetailTextLength] = 0;
}

/// Shared text-image configurations selected by text panels, menus and message styles.
/// \note Original identifier: \c D_80074DE0 (0x80074DE0).
extern u8 gTextImageConfigurations[];
/// Prepare owned text images for the current detail message as up to six rows of sixteen bytes.
/// Requires an initialized detail accumulator and valid text-image buffers; each row must contain complete text characters and controls.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119798 (0x80119798).
void systemMenuPrepareDetailText(void) {
    u8 line[17];
    s32 count;
    s32 row;
    u8 *text;
    u8 c;
    gSystemMenuDetailImageBuffer ^= 1;
    for (row = 0; row != 6; row++) {
        if (gSystemMenuDetailImages[row][gSystemMenuDetailImageBuffer] != NULL) {
            textImageDestroy((TextImage *)gSystemMenuDetailImages[row][gSystemMenuDetailImageBuffer]);
            gSystemMenuDetailImages[row][gSystemMenuDetailImageBuffer] = NULL;
        }
    }
    for (text = (u8 *)gSystemMenuDetailText, row = 0; row != 6; row++) {
        c = *text;
        count = 0;
        line[0] = 0;
        while (c != 0) {
            if (c == '\n') {
                text++;
                break;
            }
            line[count++] = c;
            text++;
            if (count >= 16) {
                if (*text == '\n')
                    text++;
                break;
            }
            c = *text;
        }
        line[count] = 0;
        if (line[0] != 0)
            textImageCreateFromText((TextImage **)((s32)&gSystemMenuDetailImages[row][gSystemMenuDetailImageBuffer]),
                                    (u8 *)((s32)line), (FontImage *)((s32)gTextImageConfigurations));
    }
}

void systemMenuAppendDetailText();     /* extern */
void systemMenuPrepareDetailText(void); /* extern */
/// Shared bounded text accumulator for selected-item details.
/// \note Original identifier: \c D_801341B8 (0x801341B8).
extern s8 gSystemMenuDetailText[0x60];
/// Number of bytes currently stored in the selected-item detail accumulator.
/// \note Original identifier: \c D_80134218 (0x80134218).
extern s32 gSystemMenuDetailTextLength;

/// Refresh the detail message for the selected menu item, including its dynamic information.
/// \param state Initialized menu state with a live, selected item whose detail text is requested.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119908 (0x80119908).
void systemMenuRefreshSelectedDetails(MenuState *state) {
    void (*appendDetails)(void);
    char *detailText;
    MenuItem *var_v0;

    gSystemMenuDetailTextLength = 0;
    gSystemMenuDetailText[0] = 0;
    var_v0 = state->selected;
    detailText = var_v0->text;
    if (detailText != NULL) {
        systemMenuAppendDetailText((u8 *)detailText);
        var_v0 = state->selected;
    }
    appendDetails = var_v0->appendDetails;
    if (appendDetails != NULL) {
        appendDetails();
    }
    systemMenuPrepareDetailText();
}

/// Refresh and draw the selected system-menu item details.
/// \param displayList Display-list cursor that receives the drawing commands and advances past them.
/// \returns Nothing.
/// \note Portability: The adjacent row initialization and loop preserve the original IDO instruction scheduling.
/// \note Original decompilation identifier: \c func_80119970 (0x80119970).
void systemMenuDrawDetails(Gfx **displayList) {
    u8 *(*row)[2];
    s32 y;
    systemMenuRefreshSelectedDetails(&gSystemMenuState);
    textDrawBegin(*displayList);
    textDrawSetFont(0);
    textDrawReset();
    textDrawSetPalette(gDefaultTextPalette);
    textDrawSetSpacingMode(1);
    textDrawSetScale(1.0f, 1.0f);
    textDrawSetArea(0, 0, 319, 239);
    textDrawSetColor(0, 0, 0, 254);
    y = 38;
    // clang-format off
    row = gSystemMenuDetailImages; do {
        // clang-format on
        if ((*row)[gSystemMenuDetailImageBuffer] == NULL) {
            y += 16;
        } else {
            textDrawSetCursor(183, y);
            textDrawImageAtCursor((TextImage *)(*row)[gSystemMenuDetailImageBuffer]);
            y += 16;
        }
        row++;
    } while (row != (u8 * (*)[2]) & gSystemMenuRowImageBuffer);
    textDrawGetDisplayListCursor(displayList);
}

u32 randomBelow(s32);
/// Randomly selected text row used by the menu closing effect.
/// \note Original identifier: \c D_8012A978 (0x8012A978).
extern s32 gSystemMenuClosingEffectRow;
/// Refresh the menu title, visible items, and current selection for display.
/// \param state Initialized menu state with a live current list and a valid visible-row offset; updates the selected item when visible.
/// The text buffers must be initialized, and every menu label must contain valid text characters and controls.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119A90 (0x80119A90).
void systemMenuRefreshItemDisplay(MenuState *state) {
    MenuList *menu;
    MenuItem *item;
    s32 i;
    s32 row;
    u8 *font;

    gSystemMenuRowImageBuffer ^= 1;
    menu = (MenuList *)state->menu;
    for (i = 0; i < 6; i++) {
        if (*(u8 **)((u8 *)gSystemMenuRowImages + i * 8 + gSystemMenuRowImageBuffer * 4) != NULL) {
            textImageDestroy((TextImage *)*(u8 **)((u8 *)gSystemMenuRowImages + i * 8 + gSystemMenuRowImageBuffer * 4));
            *(u8 **)((u8 *)gSystemMenuRowImages + i * 8 + gSystemMenuRowImageBuffer * 4) = NULL;
        }
    }
    font = gTextImageConfigurations;
    if (state->phase == 2) {
        gSystemMenuClosingEffectRow = randomBelow(6);
        textImageCreateFromText(
            (TextImage **)((s32)(u8 **)((u8 *)gSystemMenuRowImages + gSystemMenuClosingEffectRow * 8 +
                                        gSystemMenuRowImageBuffer * 4)),
            (u8 *)((s32) "@j@j@j@j@j@j@j@j@j"), (FontImage *)((s32)font));
        return;
    }
    row = 0;
    textImageCreateFromText(
        (TextImage **)((s32)(u8 **)((u8 *)gSystemMenuRowImages + row * 8 + gSystemMenuRowImageBuffer * 4)),
        (u8 *)((s32)menu->title), (FontImage *)((s32)font));
    item = menu->head;
    for (i = 0; i < menu->firstVisibleIndex; i++) {
        item = item->next;
    }
    for (i = row + 1; i != 6; i++) {
        if (item == NULL) {
            textImageCreateFromText(
                (TextImage **)((s32)(u8 **)((u8 *)gSystemMenuRowImages + i * 8 + gSystemMenuRowImageBuffer * 4)),
                (u8 *)((s32) ""), (FontImage *)((s32)font));
        } else {
            if ((i - 1) + menu->firstVisibleIndex == menu->selectedIndex) {
                state->selected = item;
            }
            textImageCreateFromText(
                (TextImage **)((s32)(u8 **)((u8 *)gSystemMenuRowImages + i * 8 + gSystemMenuRowImageBuffer * 4)),
                (u8 *)((s32)item->label), (FontImage *)((s32)font));
            item = item->next;
        }
    }
}

/// Draw the system-menu title, visible items, and selection indicators, then present any menu-specific content.
/// Requires a live current menu, prepared item text, and initialized selection indicators.
/// \param displayList Display-list cursor that receives the drawing commands and advances past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119C74 (0x80119C74).
void systemMenuDrawItemList(Gfx **displayList) {
    s32 index;
    s32 y;
    MenuList *menu;
    void (*callback)(MenuList *);
    menu = (MenuList *)gSystemMenuState.menu;
    textDrawBegin(*displayList);
    textDrawSetFont(0);
    textDrawReset();
    textDrawSetPalette(gDefaultTextPalette);
    textDrawSetSpacingMode(1);
    textDrawSetScale(1.0f, 1.0f);
    textDrawSetArea(0, 0, 319, 239);
    y = 54;
    for (index = 0; index < 5; index++, y += 16) {
        if (menu->firstVisibleIndex + index == menu->selectedIndex) {
            textDrawSetCursor(40, y);
            textDrawSetColor(0, 0, 0, 254);
            textDrawImageAtCursor((TextImage *)gSystemMenuSelectedRowMarkerImage);
        }
        if (menu->firstVisibleIndex + index == menu->confirmedIndex) {
            textDrawSetCursor(40, y);
            textDrawSetColor(255, 0, 0, 254);
            textDrawImageAtCursor((TextImage *)gSystemMenuSettingRowMarkerImage);
        }
    }
    y = 38;
    for (index = 0; index < 6; index++, y += 16) {
        if (gSystemMenuRowImages[index][gSystemMenuRowImageBuffer] == NULL)
            break;
        if (index == 0) {
            textDrawSetCursor(40, y);
            textDrawSetColor(0, 0, 0, 254);
        } else {
            textDrawSetCursor(52, y);
            if ((index - 1) + menu->firstVisibleIndex == menu->confirmedIndex) {
                textDrawSetColor(255, 0, 0, 254);
            } else {
                textDrawSetColor(0, 0, 0, 254);
            }
        }
        textDrawImageAtCursor((TextImage *)gSystemMenuRowImages[index][gSystemMenuRowImageBuffer]);
    }
    textDrawGetDisplayListCursor(displayList);
    callback = menu->action;
    if (callback != NULL)
        callback(menu);
}

/// Draw the brief system-menu closing effect while it remains active.
/// Requires initialized closing state and a valid selected text row.
/// \param displayList Display-list cursor that receives drawing commands and advances past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119EA0 (0x80119EA0).
void systemMenuDrawClosingEffect(Gfx **displayList) {
    u8 **image;
    image = (u8 **)((u8 *)&gSystemMenuRowImages + gSystemMenuClosingEffectRow * 8 + gSystemMenuRowImageBuffer * 4);
    if (*image != NULL) {
        if (runtimeGetElapsedMilliseconds() - *(u64 *)&gSystemMenuPhaseStartMilliseconds > 150) {
            textImageDestroy((TextImage *)*image);
            *image = NULL;
            return;
        }
        textDrawBegin(*displayList);
        textDrawReset();
        textDrawSetPalette(gDefaultTextPalette);
        textDrawSetSpacingMode(1);
        textDrawSetScale(1.0f, 1.0f);
        textDrawSetArea(0, 0, 319, 239);
        textDrawSetCursor(36, gSystemMenuClosingEffectRow * 16 + 38);
        textDrawSetColor(0, 0, 0, 254);
        textDrawImageAtCursor((TextImage *)*image);
        textDrawGetDisplayListCursor(displayList);
    }
}

void cameraUpdateOverlayColors(s32, s32);   /* extern */
void cameraDrawOverlay(s32);            /* extern */
void cameraDrawSecondaryOverlay(s32);            /* extern */
void renderDrawColorOverlayWithDepth(u8, u8, u8, u8); /* extern */
void objectDrawGroup(s32);        /* extern */
void objectDrawHierarchyGroup(s32, s32);   /* extern */
void systemMenuDrawDetails(Gfx **);            /* extern */
void systemMenuDrawItemList(Gfx **);            /* extern */
void systemMenuDrawClosingEffect(Gfx **);            /* extern */
/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;
/// Interior view of the menu opening, interaction and closing phase.
/// \note Original identifier: \c D_80134100 (0x80134100).
extern s32 gSystemMenuPhase;
/// Interior view of the owned model used during menu opening and closing.
/// \note Original identifier: \c D_80134104 (0x80134104).
extern s32 gSystemMenuTransitionModel;
/// Interior view of the owned model drawn during menu interaction.
/// \note Original identifier: \c D_8013410C (0x8013410C).
extern s32 gSystemMenuInteractiveModel;

/// Draw the system menu and its surrounding scene for the current menu phase.
/// \param displayList Writable display-list cursor with sufficient command capacity; advances past appended commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80119FC4 (0x80119FC4).
void systemMenuDrawFrame(Gfx **displayList) {
    switch (gSystemMenuPhase) { /* irregular */
    case 1:
        backdropDraw(displayList);
        objectDrawHierarchyGroup(gSystemMenuInteractiveModel, 0x2000);
        systemMenuDrawItemList(displayList);
        systemMenuDrawDetails(displayList);
        break;
    case 2:
        backdropDraw(displayList);
        objectDrawHierarchyGroup(gSystemMenuInteractiveModel, 0x2000);
        systemMenuDrawClosingEffect(displayList);
        break;
    case 0:
    case 3:
        objectDrawGroup(0);
        renderDrawColorOverlayWithDepth(gSystemMenuBackdropColor[0], gSystemMenuBackdropColor[1],
                                        gSystemMenuBackdropColor[2], gSystemMenuBackdropColor[3]);
        cameraUpdateOverlayColors(gRenderCameraIndex, 0);
        cameraDrawOverlay(gRenderCameraIndex);
        cameraDrawSecondaryOverlay(gRenderCameraIndex);
        objectDrawHierarchyGroup(gSystemMenuTransitionModel, 0x2000);
        break;
    }
    textDrawReset();
    textDrawSetPalette(gDefaultTextPalette);
}

void playerRecenterWalkingView(u8 *);
void playerUpdateViewCamera(u8 *);
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern PlayerControlPrefix gPlayerState;
/// Primary controller new and auto-repeated button presses; aliases the repeated word of the processed sample.
/// \note Original identifier: \c D_800AD54C (0x800AD54C).
extern s32 gControllerPrimaryRepeatedButtons;
s32 audioPlaySoundEffect(s32);
/// Handle system-menu navigation, item activation, and requests to close the menu.
/// Requires initialized menu and player state, valid menu selection, and history capacity for any submenu.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011A0D8 (0x8011A0D8).
void systemMenuHandleInput(void) {
    s32 *pressed;
    MenuState *state;
    MenuList **menu;
    MenuItem *item;

    playerRecenterWalkingView((u8 *)&gPlayerState);
    playerUpdateViewCamera((u8 *)&gPlayerState);
    menu = &gSystemMenuState.menu;
    pressed = &gControllerPrimaryRepeatedButtons;
    state = &gSystemMenuState;
    if ((gSystemMenuPhase == 1) && (*pressed != 0)) {
        if (*pressed & 0x1000) {
            audioPlaySoundEffect(0xC2);
            systemMenuSetPhase(2);
            systemMenuRestoreAudio();
            systemMenuRefreshItemDisplay(state);
            return;
        }
        if (*pressed & 0x800) {
            if ((*menu)->selectedIndex > 0) {
                (*menu)->selectedIndex--;
                audioPlaySoundEffect(0xC0);
            }
        } else if (*pressed & 0x400) {
            if ((*menu)->selectedIndex < (*menu)->count - 1) {
                (*menu)->selectedIndex++;
                audioPlaySoundEffect(0xC0);
            }
        }
        if (*pressed & 0x8000) {
            if ((*menu)->suppressConfirmSound == 0) {
                audioPlaySoundEffect(0xC1);
            }
            item = (MenuItem *)systemMenuGetSelectedItem(*menu);
            if (item != NULL) {
                if ((*menu)->confirmedIndex >= 0) {
                    (*menu)->confirmedIndex = (*menu)->selectedIndex;
                }
                if (item->action != NULL) {
                    item->action(item->value);
                }
                if (item->submenu != NULL) {
                    systemMenuPushHistory();
                    *menu = item->submenu;
                    systemMenuRefreshItemCount(*menu);
                }
            }
        } else if (*pressed & 0x4000) {
            systemMenuRestoreAudio();
            if (gSystemMenuHistoryDepth == 0) {
                audioPlaySoundEffect(0xC2);
                systemMenuSetPhase(2);
                systemMenuRestoreAudio();
                systemMenuRefreshItemDisplay(state);
                return;
            }
            systemMenuPopHistory();
        }
        if ((*menu)->selectedIndex < (*menu)->firstVisibleIndex) {
            (*menu)->firstVisibleIndex = (*menu)->selectedIndex;
        }
        if ((*menu)->selectedIndex >= (*menu)->firstVisibleIndex + 4) {
            (*menu)->firstVisibleIndex = (*menu)->selectedIndex - 4;
        }
        TRACE("current menu item = %s\n", ((MenuItem *)systemMenuGetSelectedItem(*menu))->label);
        TRACE("sel = %d, max = %d, from = %d\n", (*menu)->selectedIndex, (*menu)->count, (*menu)->firstVisibleIndex);
        systemMenuRefreshItemDisplay(state);
    }
}

f32 mathSinDegrees(f32);
f32 mathCosDegrees(f32);
void matrix34TransformPointVector(void *, f32 *, void *);
void objectUpdateHierarchy(u8 *);
void backdropSelectForView(s32, s32, s32);
/// Separate symbol view of the primary player record's view yaw.
/// \note Original identifier: \c D_800EF6AC (0x800EF6AC).
/// Separate symbol view of the primary player record's horizontal view shake.
/// \note Original identifier: \c D_800EF6B8 (0x800EF6B8).
extern f32 gPlayerYaw, gPlayerViewShake;
/// Separate view of accumulated primary camera horizontal backdrop scroll.
/// \note Original identifier: \c D_800ABD90 (0x800ABD90).
extern MenuBackdropView gPrimaryBackdropScroll;

/// Advance the menu opening animation and enable menu interaction when it finishes.
/// Requires live initialized menu models, player state, and an opening-phase start time.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011A32C (0x8011A32C).
void systemMenuUpdateOpeningAnimation(void) {
    f32 position[3];
    f32 angle;
    f32 progress;

    progress = (f32)(runtimeGetElapsedMilliseconds() - gSystemMenuState.started) / 400.0f;
    position[0] = gSystemMenuModelPosition[0];
    position[1] = gSystemMenuModelPosition[1];
    position[2] = gSystemMenuModelPosition[2];
    position[1] -= 10.0f;
    position[1] += 10.0f * mathSinDegrees(90.0f * progress);
    angle = 90.0f * progress;
    position[2] -= 10.0f * mathCosDegrees(angle);
    matrix34TransformPointVector(&gSystemMenuInterfaceTransform, position, (u8 *)gSystemMenuState.model + 0x18);
    *(f32 *)((u8 *)gSystemMenuState.model + 0x10) = gPlayerViewShake + gPlayerYaw + 180.0f;
    *(f32 *)((u8 *)gSystemMenuState.model + 0xC) = (1.0f - progress) * -90.0f;
    *(f32 *)((u8 *)gSystemMenuState.secondary + 0x10) = angle;
    objectUpdateHierarchy((u8 *)gSystemMenuState.model);
    if (runtimeGetElapsedMilliseconds() - gSystemMenuState.started >= 400) {
        ((MenuBackdropView *)&gPrimaryBackdropScroll)->useBackdrop = 0;
        backdropSelectForView(0, 9, 0);
        systemMenuSetPhase(1);
    }
}

void systemMenuExit(void);
/// Advance the menu closing animation and return to gameplay when it finishes.
/// Requires live initialized menu models, player state, and a closing-phase start time.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011A4CC (0x8011A4CC).
void systemMenuUpdateClosingAnimation(void) {
    f32 position[3];
    f32 angle;
    f32 progress;

    progress = (f32)(runtimeGetElapsedMilliseconds() - gSystemMenuState.started) / 400.0f;
    position[0] = gSystemMenuModelPosition[0];
    position[1] = gSystemMenuModelPosition[1];
    position[2] = gSystemMenuModelPosition[2];
    position[1] -= 10.0f;
    position[1] += 10.0f * mathSinDegrees((1.0f - progress) * 90.0f);
    angle = (1.0f - progress) * 90.0f;
    position[2] -= 10.0f * mathCosDegrees(angle);
    matrix34TransformPointVector(&gSystemMenuInterfaceTransform, position, (u8 *)gSystemMenuState.model + 0x18);
    *(f32 *)((u8 *)gSystemMenuState.model + 0x10) = gPlayerViewShake + gPlayerYaw + 180.0f;
    *(f32 *)((u8 *)gSystemMenuState.model + 0xC) = -90.0f * progress;
    *(f32 *)((u8 *)gSystemMenuState.secondary + 0x10) = (1.0 - progress) * 90.0;
    objectUpdateHierarchy((u8 *)gSystemMenuState.model);
    if (runtimeGetElapsedMilliseconds() - gSystemMenuState.started >= 400) {
        systemMenuExit();
    }
}

/// Separate view of the primary controller signed horizontal stick input.
/// \note Original identifier: \c D_800AD550 (0x800AD550).
extern s32 gControllerPrimaryStickX;
/// Separate view of the primary controller signed vertical stick input.
/// \note Original identifier: \c D_800AD554 (0x800AD554).
extern s32 gControllerPrimaryStickY;
/// Separate symbol view of the primary player record's view yaw.
/// \note Original identifier: \c D_800EF6AC (0x800EF6AC).
extern f32 gPlayerYaw;
/// Separate symbol view of the primary player record's horizontal view shake.
/// \note Original identifier: \c D_800EF6B8 (0x800EF6B8).
extern f32 gPlayerViewShake;

/// Position the active menu model and adjust its tilt according to the control stick.
/// Requires a live initialized menu model, player state, and a valid presentation transform.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011A684 (0x8011A684).
void systemMenuUpdateModelTilt(void) {
    f32 position[3];
    u8 *object;
    f32 basePitch = -15.0f;
    f32 axisDivisor = 2.0f;
    position[0] = -6.8f;
    position[1] = -3.15f;
    object = (u8 *)gSystemMenuInteractiveModel;
    position[2] = -15.0f;
    matrix34TransformPointVector(&gSystemMenuInterfaceTransform, position, object + 0x18);
    *(f32 *)(object + 0xC) = basePitch;
    *(f32 *)(object + 0x10) = gPlayerViewShake + gPlayerYaw + 180.0f + 23.0f;
    *(f32 *)(object + 0x14) = 0.0f;
    *(f32 *)(object + 0xC) = basePitch - (f32)gControllerPrimaryStickY / axisDivisor;
    *(f32 *)(object + 0x10) += (f32)gControllerPrimaryStickX / axisDivisor;
    objectUpdateHierarchy(object);
}

void systemMenuUpdateOpeningAnimation(void);
void systemMenuUpdateClosingAnimation(void);
void systemMenuUpdateModelTilt(void);
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
/// Advance the system-menu animation and transitions for the current phase.
/// Requires initialized menu models, player, camera, and area state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011A778 (0x8011A778).
void systemMenuUpdateAnimation(void) {
    cameraBuildInterfaceTransform((f32 *)&gSystemMenuInterfaceTransform);
    switch (gSystemMenuPhase) {
    case 0:
        systemMenuUpdateOpeningAnimation();
        return;
    case 1:
        systemMenuUpdateModelTilt();
        return;
    case 2:
        if (runtimeGetElapsedMilliseconds() - *(u64 *)&gSystemMenuPhaseStartMilliseconds > 300) {
            ((MenuBackdropView *)&gPrimaryBackdropScroll)->useBackdrop = 1;
            backdropSelectForView(0, *(s32 *)(gCurrentAreaConfiguration + 0x64), 1);
            systemMenuSetPhase(3);
            systemMenuUpdateClosingAnimation();
            return;
        }
        systemMenuUpdateModelTilt();
        return;
    case 3:
        systemMenuUpdateClosingAnimation();
        return;
    }
}

void controllerRequestRumbleReset(s32);                                 /* extern */
void controllerConfigureStickButtons(s32 enabled, s32 upThreshold, s32 downThreshold, s32 leftThreshold, s32 rightThreshold); /* extern */
void runtimeRequestGameModeChange(s32);                                 /* extern */
void systemMenuSetPhase(s32 arg0);                                   /* extern */
void systemMenuRefreshSelectedDetails(MenuState *arg0);                                   /* extern */
void systemMenuRefreshItemDisplay(MenuState *);                                /* extern */
void systemMenuUpdateAnimation(void);                                       /* extern */
/// Audio cue retained on menu entry for restoration.
/// \note Original identifier: \c D_801340D0 (0x801340D0).
extern s32 gSystemMenuSavedAudioCue;
/// Audio playback rate retained on menu entry for restoration.
/// \note Original identifier: \c D_801340D8 (0x801340D8).
extern f64 gSystemMenuSavedAudioRate;
/// Tracked sound-effect preview stopped when the preview changes or the menu exits.
/// \note Original identifier: \c D_801340E0 (0x801340E0).
extern s32 gSystemMenuSoundPreviewHandle;

/// Enter the initialized system menu at its root and begin the opening animation.
/// Requires live menu models and text buffers and initialized player, camera, and area state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011A878 (0x8011A878).
void systemMenuEnter(void) {
    TRACE("Enter system menu mode.\n");
    controllerRequestRumbleReset(0);
    gSystemMenuSoundPreviewHandle = 0;
    audioGetCurrentCueState(&gSystemMenuSavedAudioCue, &gSystemMenuSavedAudioRate);
    runtimeRequestGameModeChange(2);
    systemMenuSetPhase(0);
    objectClearHierarchyFlags((u8 *)gSystemMenuState.model, 2);
    objectClearHierarchyFlags((u8 *)gSystemMenuState.tertiary, 2);
    systemMenuUpdateAnimation();
    gSystemMenuState.depth = 0;
    gSystemMenuState.menu = &gSystemMenuRootList;
    systemMenuRefreshItemDisplay(&gSystemMenuState);
    systemMenuRefreshSelectedDetails(&gSystemMenuState);
    controllerConfigureStickButtons(1, 0x1E, 0x1E, 0x1E, 0x1E);
}

void controllerConfigureStickButtons(s32 enabled, s32 upThreshold, s32 downThreshold, s32 leftThreshold, s32 rightThreshold); /* extern */

/// Leave the system menu, hide its models, and restore gameplay controls.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011A930 (0x8011A930).
void systemMenuExit(void) {
    TRACE("Exit system menu mode.\n");
    runtimeRequestGameModeChange(0);
    controllerConfigureStickButtons(0, 0x1E, 0x1E, 0x1E, 0x1E);
    objectSetHierarchyFlags((u8 *)gSystemMenuTransitionModel, 2);
    objectSetHierarchyFlags((u8 *)gSystemMenuInteractiveModel, 2);
}

/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

/// Append total free memory and the largest available allocation to the menu details.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011A98C (0x8011A98C).
void systemMenuAppendMemoryDetails(void) {
    char buf[80];
    s32 total;
    u32 max;

    total = heapGetFreeSpace(&gGameHeap, &max);
    sprintf(buf, "\nTotal\n%d\nMax\n%d\n", total, (s32)max);
    systemMenuAppendDetailText(buf);
}

s32 progressGetValue(s32);
/// Append debug objective flags and visit counters to the menu details.
/// The formatted report must fit in 79 bytes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011A9D4 (0x8011A9D4).
void systemMenuAppendProgressDetails(void) {
    char buffer[80];
    sprintf(buffer, "\n%s%s%s%s\n%s%s%s%s\n%s%s%s\ndis %d ang %d\nshop %d\n", progressGetValue(0xB9) ? "Pw" : "Xx",
            progressGetValue(0xAC) ? "Ck" : "Xx", progressGetValue(0xBA) ? "Tb" : "Xx", progressGetValue(0x89) ? "Pn" : "Xx",
            progressGetValue(0xAD) ? "Sp" : "Xx", progressGetValue(0xAF) ? "Dg" : "Xx", progressGetValue(0xBD) ? "Cy" : "Xx",
            progressGetValue(0xAE) ? "Hr" : "Xx", progressGetValue(0xE) ? "Gc" : "Xx", progressGetValue(0xF) ? "Cs" : "Xx",
            progressGetValue(0x10) ? "Cb" : "Xx", progressGetValue(0x13), progressGetValue(0x1A), progressGetValue(0x14));
    systemMenuAppendDetailText((u8 *)buffer);
}

s32 progressGetValue();    /* extern */
/// Append the number of days spent with Pikachu to the menu details.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011AC40 (0x8011AC40).
void systemMenuAppendDaysDetails(void) {
    char buf[64];
    s32 days;

    if (progressGetValue(6) >= 100000) {
        days = 99999;
    } else {
        days = progressGetValue(6);
    }
    sprintf(buf, "Days spent\n with PIKACHU\n\n%s%d day%s",
            (days < 10)      ? "#052"
            : (days < 100)   ? "#044"
            : (days < 1000)  ? "#036"
            : (days < 10000) ? "#026"
                             : "#016",
            days, (days >= 2) ? "s" : "");
    systemMenuAppendDetailText(buf);
}

/// Separate symbol view of the primary player profile points balance.
/// \note Original identifier: \c D_800EF790 (0x800EF790).
extern s32 gPlayerProfilePoints;
/// Player status voice count, incremented when a messenger delivers to Pikachu.
/// \note Original identifier: \c D_800EF79C (0x800EF79C).
extern s32 gPlayerVoiceMessageCount;

/// Append the player status title, points balance, and voice count to the menu details.
/// Requires initialized player state and a friendship title of at most 46 bytes.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011AD24 (0x8011AD24).
void systemMenuAppendPlayerStatusDetails(void) {
    char buf[48];

    sprintf(buf, "%s\n", playerGetFriendshipTitle(&gPlayerState));
    systemMenuAppendDetailText((u8 *)buf);
    sprintf(buf, "@Y@Z\n");
    systemMenuAppendDetailText((u8 *)buf);
    sprintf(buf, "#015%d\n", gPlayerProfilePoints);
    systemMenuAppendDetailText((u8 *)buf);
    sprintf(buf, "Voice\n");
    systemMenuAppendDetailText((u8 *)buf);
    sprintf(buf, "#015%d\n", gPlayerVoiceMessageCount);
    systemMenuAppendDetailText((u8 *)buf);
}

/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;
/// Draw a menu slider and highlight its knob when selected.
/// \param displayList Active shared display-list cursor with sufficient capacity; receives drawing commands and advances past them.
/// \param x Horizontal screen coordinate of the slider left endpoint.
/// \param y Vertical screen coordinate of the slider bar.
/// \param selected Nonzero to highlight the slider knob.
/// \param value Finite slider position from zero at the left endpoint to one at the right endpoint.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011ADE0 (0x8011ADE0).
void systemMenuDrawSlider(Gfx **displayList, s32 x, s32 y, s32 selected, f32 value) {
    s32 top;
    s32 bottom;
    s32 right;

    top = y;
    bottom = y + 3;
    right = x + 80;
    gDPPipeSync(gRenderDisplayListCursor++);
    gDPSetCycleType(gRenderDisplayListCursor++, G_CYC_1CYCLE);
    gDPSetRenderMode((*displayList)++, 0x507040, 0);
    gDPSetCombineMode((*displayList)++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor((*displayList)++, 0, 0, 0, 0, 0, 140);
    gDPFillRectangle((*displayList)++, x, top, right, bottom);
    if (selected != 0) {
        gDPSetPrimColor((*displayList)++, 0, 0, 255, 0, 0, 140);
    } else {
        gDPSetPrimColor((*displayList)++, 0, 0, 0, 0, 0, 140);
    }
    selected = (s32)(80.0f * value) + x;
    gDPFillRectangle((*displayList)++, selected - 2, top - 3, selected + 2, bottom + 3);
}

s32 progressGetMaximum(s32);
s32 progressSetValue(s32, s32);
void audioApplySavedMusicVolume(f32);
void audioApplySavedSoundEffectVolume(f32);

/// Adjust the selected music or sound volume from controller input and draw both volume sliders.
/// \param menu Live volume menu whose current row selects the setting to adjust.
/// Requires initialized audio, positive saved-volume maxima, and a shared display-list cursor with sufficient capacity.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011AF8C (0x8011AF8C).
void systemMenuUpdateVolumeSettings(MenuList *menu) {
    s32 channelAMaximum;
    s32 channelBMaximum;
    s32 channelASelected;
    s32 channelBSelected;
    s32 selection;
    s32 channelAValue;
    s32 channelBValue;
    s32 *pressed;

    pressed = &gControllerPrimaryRepeatedButtons;
    channelAMaximum = progressGetMaximum(0xA);
    channelBMaximum = progressGetMaximum(0xB);
    channelAValue = progressGetValue(0xA);
    channelBValue = progressGetValue(0xB);
    if (*pressed != 0) {
        selection = menu->selectedIndex;
        if (selection == 0) {
            if (gControllerPrimaryRepeatedButtons & 0x100) {
                channelAValue += 1;
            }
            if (gControllerPrimaryRepeatedButtons & 0x200) {
                channelAValue -= 1;
            }
            if (channelAValue < 0) {
                channelAValue = 0;
            } else if (channelAMaximum < channelAValue) {
                channelAValue = channelAMaximum;
            }
            progressSetValue(0xA, channelAValue);
            audioApplySavedMusicVolume(1.0f);
        } else if (selection == 1) {
            if (gControllerPrimaryRepeatedButtons & 0x100) {
                channelBValue += 1;
            }
            if (gControllerPrimaryRepeatedButtons & 0x200) {
                channelBValue -= 1;
            }
            if (channelBValue < 0) {
                channelBValue = 0;
            } else if (channelBMaximum < channelBValue) {
                channelBValue = channelBMaximum;
            }
            progressSetValue(0xB, channelBValue);
            audioApplySavedSoundEffectVolume(1.0f);
        }
    }
    channelASelected = menu->selectedIndex == 0 ? 1 : 0;
    systemMenuDrawSlider(&gRenderDisplayListCursor, 0xC1, 0x3A, channelASelected,
                         (f32)channelAValue / (f32)channelAMaximum);
    channelBSelected = menu->selectedIndex == 1 ? 1 : 0;
    systemMenuDrawSlider(&gRenderDisplayListCursor, 0xC1, 0x4F, channelBSelected,
                         (f32)channelBValue / (f32)channelBMaximum);
}

void systemMenuPopHistory(void); /* extern */

/// Cancel the current menu choice and return to its parent menu.
/// \param unused Unused menu action value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B164 (0x8011B164).
void systemMenuCancelSelection(s32 unused) {
    systemMenuPopHistory();
}


/// Leave the system menu and request the go-home completion transition.
/// \param unused Unused menu action value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B184 (0x8011B184).
void systemMenuGoHome(s32 unused) {
    ((MenuBackdropView *)&gPrimaryBackdropScroll)->useBackdrop = 1;
    backdropSelectForView(0, ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->backdrop, 1);
    systemMenuExit();
    runtimeRequestGameModeChange(0xC);
}

void areaRequestTransition(s32, s32); /* extern */

/// Leave the system menu and request an area transition at its default spawn.
/// \param area Destination area identifier.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B1D4 (0x8011B1D4).
void systemMenuChangeArea(s32 area) {
    ((MenuBackdropView *)&gPrimaryBackdropScroll)->useBackdrop = 1;
    backdropSelectForView(0, ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->backdrop, 1);
    systemMenuExit();
    areaRequestTransition(area, 0);
}

/// Selected eye-catch group index, or minus one when no interlude is selected.
/// \note Original identifier: \c D_8009B580 (0x8009B580).
extern s32 gEyeCatchSelectedGroup;

/// Select the next eye-catch interlude and leave the system menu.
/// \param group Valid eye-catch group identifier, or minus one to cancel the pending interlude.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B228 (0x8011B228).
void systemMenuSelectEyeCatchGroup(s32 group) {
    ((MenuBackdropView *)&gPrimaryBackdropScroll)->useBackdrop = 1;
    backdropSelectForView(0, ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->backdrop, 1);
    gEyeCatchSelectedGroup = group;
    systemMenuExit();
}


/// Award an unearned friendship milestone and return home.
/// \param milestone Friendship milestone index from zero through nineteen; an already earned milestone has no effect on friendship progress.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B278 (0x8011B278).
void systemMenuAwardFriendshipMilestoneAndGoHome(s32 milestone) {
    playerAwardFriendshipMilestone(&gPlayerState, milestone);
    systemMenuGoHome(0);
}

void progressSetChapterAndUnlockMilestones(); /* extern */

/// Invoke story chapter setting and milestone unlocks through the menu’s legacy callback interface.
/// \returns Nothing; the chapter value is unspecified through this declared no-argument interface.
/// \note Portability/UB: The unprototyped chapter helper expects a chapter argument, but this interface supplies none; the argument value is unspecified.
/// \note Original decompilation identifier: \c func_8011B2A8 (0x8011B2A8).
void systemMenuInvokeChapterUpdate(void) {
    progressSetChapterAndUnlockMilestones();
}


/// Leave the system menu and enter the world-map transition.
/// \param unused Unused menu action value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B2C8 (0x8011B2C8).
void systemMenuOpenWorldMap(s32 unused) {
    ((MenuBackdropView *)&gPrimaryBackdropScroll)->useBackdrop = 1;
    backdropSelectForView(0, ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->backdrop, 1);
    systemMenuExit();
    runtimeRequestGameModeChange(6);
}

/// Preview a music cue at its normal playback rate.
/// \param cueId Valid music cue identifier to preview.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B318 (0x8011B318).
void systemMenuPreviewMusic(s32 cueId) {
    TRACE("selectBGM : %d %s\n", cueId, ((MenuItem *)systemMenuGetSelectedItem(gSystemMenuState.menu))->label);
    audioChangeCurrentCue(cueId, 0.0, 0.1);
    audioSetCurrentCueRate(1.0);
}


/// Replace the current menu sound preview with a selected sound effect.
/// \param soundId Valid sound-effect identifier to preview.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B35C (0x8011B35C).
void systemMenuPreviewSoundEffect(s32 soundId) {
    if (gSystemMenuSoundPreviewHandle != 0) {
        audioStopPlayback(gSystemMenuSoundPreviewHandle, 0.0);
    }
    gSystemMenuSoundPreviewHandle = audioPlaySoundEffect(soundId);
    TRACE("selectSound : %d %s\n", soundId, ((MenuItem *)systemMenuGetSelectedItem(gSystemMenuState.menu))->label);
}

void audioSetOutputMode(s32);     /* extern */

/// Apply and save the selected mono or stereo output setting.
/// \param mode One selects mono; two selects stereo.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B3A0 (0x8011B3A0).
void systemMenuSetAudioOutputMode(s32 mode) {
    TRACE("selChannel %d\n", mode);
    audioSetOutputMode(mode);
    progressSetValue(9, 2 - mode);
}

/// Provide an inactive system-menu action hook.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B3D0 (0x8011B3D4).
void systemMenuUnusedActionHook(void) {
}

void playerSetChaseStage(s32); /* extern */

/// Set the current chase progress and retain the furthest progress reached.
/// \param value Requested chase progress; the current stage retains its low eight bits, while furthest progress uses the full value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B3DC (0x8011B3DC).
void systemMenuSetChaseProgress(s32 value) {
    TRACE("selChase %d\n", value);
    playerSetChaseStage(value);
}

/// Save the legacy debug difficulty selection.
/// \param arg0 Requested difficulty setting.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B3FC (0x8011B3FC).
void systemMenuSetDifficulty(s32 arg0) {
    TRACE("selDifficulty %d\n", arg0);
    progressSetValue(0x11, arg0);
}

/// Apply a selected value to all disappearance progress flags.
/// \param arg0 Requested disappearance flag value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B420 (0x8011B420).
void systemMenuSetDisappearanceFlags(s32 arg0) {
    TRACE("selIedeFlag %d\n", arg0);
    progressSetValue(0x1B, arg0);
    progressSetValue(0x1C, arg0);
    progressSetValue(0x1D, arg0);
    progressSetValue(0x1E, arg0);
    progressSetValue(0x1F, arg0);
    progressSetValue(0x20, arg0);
    progressSetValue(0x21, arg0);
    progressSetValue(0x22, arg0);
    progressSetValue(0x23, arg0);
}

/// Set the visit count used for early story progression.
/// \param arg0 Requested visit count.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B4B0 (0x8011B4B0).
void systemMenuSetEarlyVisitCount(s32 arg0) {
    TRACE("selEasyCountFlag %d\n", arg0);
    progressSetValue(0xD, arg0);
}

/// Set the saved game-clear progress.
/// \param arg0 Requested game-clear progress value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B4D4 (0x8011B4D4).
void systemMenuSetGameClearProgress(s32 arg0) {
    TRACE("selGameClearFlag %d\n", arg0);
    progressSetValue(0xE, arg0);
}

/// Save the selected vertical-look control setting.
/// \param normal Nonzero makes forward stick input look up; zero makes it look down.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B4F8 (0x8011B4F8).
void systemMenuSetVerticalLookMode(s32 normal) {
    TRACE("selStickReverse %d\n", normal);
    if (normal != 0) {
        progressSetValue(7, 1);
        return;
    }
    progressSetValue(7, 0);
}

s32 eepromGetQueuedTransferState(void);
/// Save-control byte transferred independently from the progress records.
/// \note Original identifier: \c D_8009A6F0 (0x8009A6F0).
extern u8 gEepromSaveControl;
/// Save request parameters; the first word selects the EEPROM save record.
/// \note Original identifier: \c D_800AE510 (0x800AE510).
extern s32 gRuntimeSaveParameters;
typedef struct FileStatusLabels {
    char *text[6];
} FileStatusLabels;
/// Text descriptions indexed by EEPROM transfer status.
/// \note Original identifier: \c D_8012ACF0 (0x8012ACF0).
FileStatusLabels gSystemMenuSaveStatusLabels = {
    {"Normal", "Reading", "Writeing", "Read Error", "CRC Error", "Write Error"}};
/// Update saved-file availability from the latest transfer status and append the save status to menu details.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B534 (0x8011B534).
void systemMenuAppendSaveStatusDetails(void) {
    char buffer[48];
    FileStatusLabels labels = gSystemMenuSaveStatusLabels;
    switch (eepromGetQueuedTransferState()) {
    case 2:
        switch (gRuntimeSaveParameters) {
        case 1:
            gEepromSaveControl |= 1;
            break;
        case 2:
            gEepromSaveControl |= 2;
            break;
        }
        break;
    case 3:
    case 4:
    case 5:
        switch (gRuntimeSaveParameters) {
        case 1:
            gEepromSaveControl &= ~1;
            break;
        case 2:
            gEepromSaveControl &= ~2;
            break;
        }
        break;
    }
    TRACE("pdFCB:%x\n", gEepromSaveControl);
    sprintf(buffer, "File%s\n  %s\nFCB %s%s\n", gRuntimeSaveParameters == 1 ? "1" : "2",
            labels.text[eepromGetQueuedTransferState()], gEepromSaveControl & 1 ? "1" : "x",
            gEepromSaveControl & 2 ? "2" : "x");
    systemMenuAppendDetailText((u8 *)buffer);
}

void progressSaveControlDataToEeprom(void); /* extern */
void progressLoadControlDataFromEeprom(void); /* extern */
void progressSaveRecordToEeprom(s32);  /* extern */
void progressLoadRecordFromEeprom(s32);  /* extern */

/// Request a selected saved-game or control-data transfer to or from EEPROM.
/// \param operation Zero or one loads saved-game file one or two; two or three saves file one or two; four loads control data and five saves control data. Other values request no transfer.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B6F8 (0x8011B6F8).
void systemMenuRequestSaveTransfer(s32 operation) {
    switch (operation) {
    case 0:
        progressLoadRecordFromEeprom(gRuntimeSaveParameters = 1);
        break;
    case 1:
        progressLoadRecordFromEeprom(gRuntimeSaveParameters = 2);
        break;
    case 2:
        progressSaveRecordToEeprom(gRuntimeSaveParameters = 1);
        break;
    case 3:
        progressSaveRecordToEeprom(gRuntimeSaveParameters = 2);
        break;
    case 4:
        progressLoadControlDataFromEeprom();
        break;
    case 5:
        progressSaveControlDataToEeprom();
        break;
    }
    TRACE("selEEPROM %d\n", gRuntimeSaveParameters);
}

void _bzero(void *, s32); /* extern */

/// Create an empty menu item ready for configuration.
/// \pre The game heap has room for one menu item.
/// \returns Owned, empty menu item; its strings and callbacks still need configuration.
/// \note Original decompilation identifier: \c func_8011B7B8 (0x8011B7B8).
MenuItem *systemMenuCreateItem(void) {
    MenuItem *item;

    item = heapAllocLocal(&gGameHeap, 0x1C, 0x736);
    _bzero(item, 0x1C);
    return item;
}

/// Append a menu item after the existing items in a menu.
/// \param menu Live menu list with an acyclic item chain; borrows the appended item for its lifetime.
/// \param item Item to append, distinct from existing items; its previous successor is discarded.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B7F8 (0x8011B7F8).
void systemMenuAppendItem(MenuList *menu, MenuItem *item) {
    MenuItem *node;

    if (menu->head == NULL) {
        menu->head = item;
    } else {
        node = menu->head;
        while (node->next != NULL) {
            node = node->next;
        }
        node->next = item;
    }
    item->next = NULL;
}

/* .data 0x8012AD08..0x8012B020: nine debug submenus, each followed by an empty
 * stub. Four stubs keep their submenu's value names as dead TRACEs. */
/// Area labels used to build the debug area-selection menu.
/// \note Original identifier: \c D_8012AD08 (0x8012AD08).
char *gSystemMenuAreaLabels[23] = {"Room",         "GENKAN",      "Yard",         "ViridianA",      "ViridianB",
                                   "ViridianC",    "SpringleafA", "SpringleafB",  "SpringleafC",    "OlivineA",
                                   "OlivineB",     "OchreA",      "OchreB",       "CobaltA",        "CobaltB",
                                   "Shop",         "Camp",        "OchreFishing", "OlivineFishing", "CobaltFishing",
                                   "ViridianEdge", "Backyard",    "Ending"};
/// Persistent AreaSelect menu list and retained selection state.
/// \note Original identifier: \c D_8012AD64 (0x8012AD64).
MenuList gSystemMenuAreaSelectList = {"Area Select", NULL, NULL, 0, 0, 0, 0, 0};
/// Persistent AreaSelect menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AD84 (0x8012AD84).
MenuItem gSystemMenuAreaSelectEntry = {
    "Area Select", NULL, 0, "Debug", systemMenuAppendMemoryDetails, NULL, &gSystemMenuAreaSelectList};
/// Provide an inactive area-selection debug-menu entry point; has no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B838 (0x8011B838).
void systemMenuAreaDebugNoOp(void) {
}

/// Labels used to build the debug eye-catch selection menu.
/// \note Original identifier: \c D_8012ADA0 (0x8012ADA0).
char *gSystemMenuEyeCatchLabels[6] = {"WashHand", "Cooking", "Dream", "ICE", "PotatoFailure", "PotatoSuccess"};
/// Persistent EyeCatch menu list and retained selection state.
/// \note Original identifier: \c D_8012ADB8 (0x8012ADB8).
MenuList gSystemMenuEyeCatchList = {"EyeCatch", NULL, NULL, 0, 0, 0, 0, 0};
/// Persistent EyeCatch menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012ADD8 (0x8012ADD8).
MenuItem gSystemMenuEyeCatchEntry = {"EyeCatch", NULL, 0, "Debug", NULL, NULL, &gSystemMenuEyeCatchList};
/// Provide an inactive eye-catch debug-menu entry point; has no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B840 (0x8011B840).
void systemMenuEyeCatchDebugNoOp(void) {
}

/// Labels used to build the debug chapter selection menu.
/// \note Original identifier: \c D_8012ADF4 (0x8012ADF4).
char *gSystemMenuChapterLabels[10] = {"Advertise", "Opening",  "Hello",   "Level 1", "Welcome",
                                      "Level 2",   "Training", "Level 3", "Final",   "Ending"};
/// Persistent Chapter menu list and retained selection state.
/// \note Original identifier: \c D_8012AE1C (0x8012AE1C).
MenuList gSystemMenuChapterList = {"Chapter", NULL, NULL, 0, 0, 0, 0, 0};
/// Persistent Chapter menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AE3C (0x8012AE3C).
MenuItem gSystemMenuChapterEntry = {
    "Chapter", NULL, 0, "Debug", systemMenuAppendProgressDetails, NULL, &gSystemMenuChapterList};
/// Provide an inactive chapter debug-menu entry point; has no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B848 (0x8011B848).
void systemMenuChapterDebugNoOp(void) {
}

/// Persistent Difficulty menu list and retained selection state.
/// \note Original identifier: \c D_8012AE58 (0x8012AE58).
MenuList gSystemMenuDifficultyList = {"Difficulty", NULL, NULL, 0, 0, 0, 0, 0};
/// Persistent Difficulty menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AE78 (0x8012AE78).
MenuItem gSystemMenuDifficultyEntry = {"Difficulty", NULL, 0, "Debug", NULL, NULL, &gSystemMenuDifficultyList};
/// Provide an inactive difficulty debug-menu entry point; has no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B850 (0x8011B850).
void systemMenuDifficultyDebugNoOp(void) {
    TRACE("Easy");
    TRACE("Normal");
    TRACE("Hard");
}

/// Labels used to build the debug checkpoint selection menu.
/// \note Original identifier: \c D_8012AE94 (0x8012AE94).
char *gSystemMenuCheckpointLabels[20] = {"HELLO",         "TOKIWA C 1",   "MOEGI A", "AKANE 1",    "AKANE FISHING",
                                         "ASAGI FISHING", "SUOU FISHING", "MOEGI C", "AKANE 2",    "ASAGI A",
                                         "SUOU A 2",      "SUOU B",       "TOKKUN",  "TOKIWA C 3", "MOEGI B",
                                         "ASAGI B",       "SUOU A 3",     "AKANE 3", "IEDE",       "FINAL"};
/// Persistent Checkpoint menu list and retained selection state.
/// \note Original identifier: \c D_8012AEE4 (0x8012AEE4).
MenuList gSystemMenuCheckpointList = {"CheckPoint", NULL, NULL, 0, 0, 0, 0, 0};
/// Persistent Checkpoint menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AF04 (0x8012AF04).
MenuItem gSystemMenuCheckpointEntry = {"CheckPoint", NULL, 0, "Debug", NULL, NULL, &gSystemMenuCheckpointList};
/// Provide an inactive checkpoint debug-menu entry point; has no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B858 (0x8011B858).
void systemMenuCheckpointDebugNoOp(void) {
}

/// Persistent DisappearFlags menu list and retained selection state.
/// \note Original identifier: \c D_8012AF20 (0x8012AF20).
MenuList gSystemMenuDisappearFlagsList = {"DisappearFlag", NULL, NULL, 0, 0, 0, 0, 0};
/// Persistent DisappearFlags menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AF40 (0x8012AF40).
MenuItem gSystemMenuDisappearFlagsEntry = {
    "DisappearFlag", NULL, 0, "Debug", NULL, NULL, &gSystemMenuDisappearFlagsList};
/// Provide an inactive disappearance debug-menu entry point; has no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B860 (0x8011B860).
void systemMenuDisappearDebugNoOp(void) {
    TRACE("Normal");
    TRACE("Disappear");
}

/// Persistent ItemFlags menu list and retained selection state.
/// \note Original identifier: \c D_8012AF5C (0x8012AF5C).
MenuList gSystemMenuItemFlagsList = {"ItemFlag", NULL, NULL, 0, 0, 0, 0, 0};
/// Persistent ItemFlags menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AF7C (0x8012AF7C).
MenuItem gSystemMenuItemFlagsEntry = {"ItemFlag", NULL, 0, "Debug", NULL, NULL, &gSystemMenuItemFlagsList};
/// Provide an inactive item-flag debug-menu entry point; has no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B868 (0x8011B868).
void systemMenuItemFlagDebugNoOp(void) {
    TRACE("Set");
    TRACE("Clear");
}

/// Persistent LevelOneCounts menu list and retained selection state.
/// \note Original identifier: \c D_8012AF98 (0x8012AF98).
MenuList gSystemMenuLevelOneCountsList = {"Level 1 counts", NULL, NULL, 0, 0, 0, 0, 0};
/// Persistent LevelOneCounts menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012AFB8 (0x8012AFB8).
MenuItem gSystemMenuLevelOneCountsEntry = {
    "Level 1 counts", NULL, 0, "Debug", NULL, NULL, &gSystemMenuLevelOneCountsList};
/// Provide an inactive visit-count debug-menu entry point; has no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B870 (0x8011B870).
void systemMenuVisitCountDebugNoOp(void) {
}

/// Labels used to build the debug clear-flag selection menu.
/// \note Original identifier: \c D_8012AFD4 (0x8012AFD4).
char *gSystemMenuClearFlagLabels[4] = {"0", "1", "2", "3"};
/// Persistent ClearFlag menu list and retained selection state.
/// \note Original identifier: \c D_8012AFE4 (0x8012AFE4).
MenuList gSystemMenuClearFlagList = {"ClearFlag", NULL, NULL, 0, 0, 0, 0, 0};
/// Persistent ClearFlag menu entry with its action, detail and submenu links.
/// \note Original identifier: \c D_8012B004 (0x8012B004).
MenuItem gSystemMenuClearFlagEntry = {
    "ClearFlag", NULL, 0, "Debug\nGameClearFlag", NULL, NULL, &gSystemMenuClearFlagList};
/// Provide an inactive completion-flag debug-menu entry point; has no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011B878 (0x8011B878).
void systemMenuClearFlagDebugNoOp(void) {
    TRACE("Reset");
    TRACE("Set");
}

MenuItem *systemMenuBuildMusicMenu(void);
MenuItem *systemMenuBuildSoundMenu(void);

/// Music cue labels used to build the preview menu.
/// \note Original identifier: \c D_8012BCA0 (0x8012BCA0).
extern char *gSystemMenuMusicCueLabels[];

/// Build a menu for previewing the game music cues.
/// \returns Borrowed persistent parent item opening the music menu, with newly allocated child entries.
/// Requires any previous child entries to have been released before rebuilding.
/// \note Original decompilation identifier: \c func_8011B880 (0x8011B880).
MenuItem *systemMenuBuildMusicMenu(void) {
    static MenuList list = {"Play music", NULL, NULL, 0, 0, 0, 0, 1};
    static MenuItem item = {"Play music@a", NULL, 0,    "Listen to the\n game's music.\n\n\nPress @W@X\n#036to open.",
                            NULL,           NULL, &list};
    s32 i;
    MenuItem *entry;

    list.head = NULL;
    list.selectedIndex = 0;
    list.confirmedIndex = 0;
    for (i = 0; i < 45; i++) {
        entry = systemMenuCreateItem();
        entry->label = gSystemMenuMusicCueLabels[i];
        entry->value = i;
        entry->action = systemMenuPreviewMusic;
        systemMenuAppendItem(&list, entry);
    }
    systemMenuRefreshItemCount(&list);
    return &item;
}

/// Build a menu for previewing the game sound effects.
/// \returns Borrowed persistent parent item opening the sound menu, with newly allocated child entries.
/// Requires any previous child entries to have been released before rebuilding.
/// \note Original decompilation identifier: \c func_8011B930 (0x8011B930).
MenuItem *systemMenuBuildSoundMenu(void) {
    MenuList *menu;
    s32 i;
    MenuItem *entry;
    char *labels[201] = {
        "Butterfree1", "Caterpie1",   "Caterpie2",   "Caterpie3",   "Magnemite1", "Diglett1",    "Diglett2",
        "Dodrio1",     "Venusaur1",   "Bulbasaur1",  "Bulbasaur2",  "Bulbasaur3", "Bulbasaur4",  "Bulbasaur5",
        "Bulbasaur6",  "Bulbasaur7",  "Bulbasaur8",  "Haunter1",    "Haunter2",   "Charmander1", "Abra1",
        "Gloom1",      "Gloom2",      "Poliwag1",    "Poliwag2",    "Poliwag3",   "Oddish1",     "Oddish2",
        "Oddish3",     "Fearow1",     "Pikachu1",    "Pikachu2",    "Pikachu3",   "Pikachu4",    "Pikachu5",
        "Pikachu6",    "Pikachu7",    "Pikachu8",    "Pikachu9",    "Pikachu10",  "Pikachu11",   "Pikachu12",
        "Pikachu13",   "Pikachu14",   "Pikachu15",   "Pikachu16",   "Pikachu17",  "Pikachu18",   "Pikachu19",
        "Pikachu20",   "Pikachu21",   "Pikachu22",   "Pikachu23",   "Pikachu24",  "Pikachu25",   "Pikachu26",
        "Pikachu27",   "Pikachu28",   "Pikachu29",   "Pikachu30",   "Pikachu31",  "Pikachu32",   "Pikachu33",
        "Pikachu34",   "Pikachu35",   "Pikachu36",   "Pikachu37",   "Pikachu38",  "Pikachu39",   "Pikachu40",
        "Pikachu41",   "Pikachu42",   "Pikachu43",   "Pikachu44",   "Pikachu45",  "Pikachu46",   "Pikachu47",
        "Pikachu48",   "Pikachu49",   "Pikachu50",   "Pikachu51",   "Pikachu52",  "Pikachu53",   "Pikachu54",
        "Pikachu55",   "Pikachu56",   "Pikachu57",   "Pikachu58",   "Pikachu59",  "Pikachu60",   "Pikachu61",
        "Pikachu62",   "Pikachu63",   "Pikachu64",   "Pikachu65",   "Pikachu66",  "Pikachu67",   "Pikachu68",
        "Pikachu69",   "Pikachu70",   "Pikachu71",   "Pikachu72",   "Pikachu73",  "Pikachu74",   "Pikachu75",
        "Pikachu76",   "Pikachu77",   "Pikachu78",   "Pikachu79",   "Pikachu80",  "Pikachu81",   "Pikachu82",
        "Pikachu83",   "Pikachu84",   "Pikachu85",   "Pikachu86",   "Pikachu87",  "Pikachu88",   "Pikachu89",
        "Pikachu90",   "Pikachu91",   "Pikachu92",   "Pikachu93",   "Pikachu94",  "Pikachu95",   "Vileplume1",
        "Togepi1",     "Togepi2",     "Slowpoke1",   "Squirtle1",   "Squirtle2",  "Squirtle3",   "Squirtle4",
        "Ambience1",   "Ambience2",   "Ambience3",   "Ambience4",   "Others1",    "Pikasound1",  "Pikasound2",
        "Pikasound3",  "Pikasound4",  "Pikasound5",  "Pikasound6",  "Pikasound7", "Pikasound8",  "Pikasound9",
        "Pikasound10", "Pikasound11", "Pikasound12", "Pikasound13", "Player1",    "Player2",     "Player3",
        "Stage1",      "Stage2",      "Stage3",      "Stage4",      "Stage5",     "Stage6",      "Stage7",
        "Stage8",      "Stage9",      "Stage10",     "Stage11",     "Stage12",    "Stage13",     "Stage14",
        "Stage15",     "Stage16",     "Stage17",     "Stage18",     "System1",    "System2",     "System3",
        "System4",     "System5",     "System6",     "System7",     "System8",    "System9",     "System10",
        "System11",    "System12",    "System13",    "System14",    "System15",   "System16",    "System17",
        "System18",    "System19",    "System20",    "System21",    "System22",   "System23",    "System24",
        "System25",    "System26",    "System27",    "System28",    "System29"};
    static MenuList list = {"Play sounds", NULL, NULL, 0, 0, 0, 0, 1};
    static MenuItem item = {"Play sounds@a",
                            NULL,
                            0,
                            "Listen to the\n game's sound\n effects.\n\nPress @W@X\n#036to open.",
                            NULL,
                            NULL,
                            &list};

    list.head = NULL;
    list.selectedIndex = 0;
    menu = &list;
    for (i = 0; i < 201; i++) {
        entry = systemMenuCreateItem();
        entry->label = labels[i];
        entry->value = i;
        entry->action = systemMenuPreviewSoundEffect;
        systemMenuAppendItem(menu, entry);
    }
    systemMenuRefreshItemCount(menu);
    return &item;
}

/// Build the extras menu containing music and sound previews.
/// \returns Borrowed persistent parent item opening the extras menu, with newly allocated preview entries.
/// Requires any previous preview entries to have been released before rebuilding.
/// \note Original decompilation identifier: \c func_8011BA08 (0x8011BA08).
MenuItem *systemMenuBuildExtrasMenu(void) {
    static MenuList list = {"Extras", NULL, NULL, 0, -1, 0, 0, 0};
    static MenuItem item = {"Extras@a", NULL, 0, "Extras\n\n\n\nPress @W@X\n#036to open.", NULL, NULL, &list};

    list.head = NULL;
    list.selectedIndex = 0;
    systemMenuAppendItem(&list, systemMenuBuildMusicMenu());
    systemMenuAppendItem(&list, systemMenuBuildSoundMenu());
    systemMenuRefreshItemCount(&list);
    return &item;
}

/// Persistent Control menu list and retained selection state.
/// \note Original identifier: \c D_8012AB48 (0x8012AB48).
extern MenuList gSystemMenuControlList;

/// Select the vertical-look menu row corresponding to the saved control setting.
/// \param unused Unused menu action value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011BA68 (0x8011BA68).
void systemMenuSyncLookSettingSelection(s32 unused) {
    s32 lookSetting;

    lookSetting = progressGetValue(7);
    gSystemMenuControlList.confirmedIndex = lookSetting;
    gSystemMenuControlList.selectedIndex = lookSetting;
}

/// Persistent Sound menu list and retained selection state.
/// \note Original identifier: \c D_8012AAF0 (0x8012AAF0).
extern MenuList gSystemMenuSoundList;

/// Select the sound-output menu row corresponding to the saved mono or stereo setting.
/// \param unused Unused menu action value.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011BA98 (0x8011BA98).
void systemMenuSyncSoundSettingSelection(s32 unused) {
    s32 soundSetting;

    soundSetting = progressGetValue(9);
    gSystemMenuSoundList.confirmedIndex = soundSetting;
    gSystemMenuSoundList.selectedIndex = soundSetting;
}

/// Identifier of the currently loaded area.
/// \note Original identifier: \c D_800AE4F8 (0x800AE4F8).
extern s32 gRuntimeCurrentArea;

/// Prepare system-menu state, owned models and text, and entries available for the current game progress.
/// Requires initialized area, object, and text systems, fresh static menu lists, and no unreleased menu resources.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011BAC8 (0x8011BAC8).
void systemMenuInitialize(void) {
    MenuModel *model;
    s32 scene;
    MenuState *state;

    state = &gSystemMenuState;
    _bzero(&gSystemMenuState, 0x30);
    textImageCreateFromText((TextImage **)((s32)&gSystemMenuSelectedRowMarkerImage), (u8 *)((s32) "@d"),
                            (FontImage *)((s32)gTextImageConfigurations));
    textImageCreateFromText((TextImage **)((s32)&gSystemMenuSettingRowMarkerImage), (u8 *)((s32) "@b"),
                            (FontImage *)((s32)gTextImageConfigurations));
    gSystemMenuRowImageBuffer = 0;
    gSystemMenuDetailImageBuffer = 0;
    _bzero(gSystemMenuRowImages, 0x30);
    _bzero(gSystemMenuDetailImages, 0x30);
    model = (MenuModel *)objectCreateHierarchyFromTemplate((ObjectTemplate *)&gSystemMenuTransitionModelTemplate);
    objectSetHierarchyFlags((u8 *)model, 0x2002);
    objectRegisterActiveHierarchy((u8 *)model);
    model->scale[0] = *(f32 *)(gCurrentAreaConfiguration + 0xC) * 5.0;
    model->scale[1] = *(f32 *)(gCurrentAreaConfiguration + 0xC) * 5.0;
    model->scale[2] = *(f32 *)(gCurrentAreaConfiguration + 0xC) * 5.0;
    state->model = (s32)model;
    state->secondary = (s32)model->next;
    ((MenuModel *)state->model)->transform[0] = 0.0f;
    ((MenuModel *)state->model)->transform[1] = 0.0f;
    ((MenuModel *)state->model)->transform[2] = 0.0f;
    ((MenuModel *)state->secondary)->transform[0] = 0.0f;
    ((MenuModel *)state->secondary)->transform[1] = 0.0f;
    ((MenuModel *)state->secondary)->transform[2] = 0.0f;
    ((MenuModel *)state->secondary)->transform[1] = 90.0f;
    model = (MenuModel *)objectCreateHierarchyFromTemplate((ObjectTemplate *)&gSystemMenuInteractiveModelTemplate);
    objectSetHierarchyFlags((u8 *)model, 0x2002);
    objectRegisterActiveHierarchy((u8 *)model);
    model->scale[0] = *(f32 *)(gCurrentAreaConfiguration + 0xC);
    model->scale[1] = *(f32 *)(gCurrentAreaConfiguration + 0xC);
    model->scale[2] = *(f32 *)(gCurrentAreaConfiguration + 0xC);
    state->tertiary = (s32)model;
    model->transform[2] = model->transform[1] = model->transform[0] = 0.0f;
    scene = progressGetCurrentChapter();
    if (scene != 4 && scene != 6 && scene != 8 && gRuntimeCurrentArea >= 3 && progressGetValue(0x12) == 0) {
        systemMenuAppendItem(&gSystemMenuRootList, &gSystemMenuGoHomeEntry);
    }
    if (progressGetValue(0xE)) {
        systemMenuAppendItem(&gSystemMenuRootList, systemMenuBuildExtrasMenu());
    }
    gSystemMenuRootList.firstVisibleIndex = 0;
    gSystemMenuRootList.selectedIndex = 0;
    systemMenuRefreshItemCount(&gSystemMenuRootList);
    gSystemMenuState.menu = &gSystemMenuRootList;
}


s32 scriptVmReadIntegerOperand();                       /* extern */
s32 scriptVmPopString();                       /* extern */
void stringPrependBounded(s8 *, s32, s32); /* extern */
/// Shared 80-byte text used by actor-script text and message commands.
/// \note Original identifier: \c D_80134220 (0x80134220).
extern s8 gActorScriptSharedTextBuffer;

void scriptVmPushFloat(u8 *, s32); /* extern */

s32 messageBarEnqueue(s32, s32, s8 *);     /* extern */

void messageBarClear(); /* extern */

void textWindowAppendText(s8 *);               /* extern */

void textWindowEnterMode(); /* extern */


void actorSetTargetToFallbackObject(); /* extern */
