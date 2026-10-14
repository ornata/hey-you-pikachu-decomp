#include "audio_control.h"
#include "eeprom_io.h"
#include "texture_image.h"
#include "texture_rectangles.h"
#include "pikachu_actor.h"
#include "object_groups.h"
#include "math_angles.h"
#include "backdrop_view.h"
#include "area_transition.h"
#include "runtime_control.h"
#include "controller_input.h"
#include "audio_sound_effect.h"
#include "text_image_configurations.h"
#include "script_vm_globals.h"
#include "story_chapter.h"
#include "story_milestones.h"
#include "progress_storage.h"
#include "camera_overlay.h"
#include "object_flags.h"
#include "backdrop_draw.h"
#include "title_screen.h"
#include "hud_draw.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "runtime_clock.h"
#include "text_image_create.h"
#include "audio_cue.h"
#include "text_draw.h"
#include "asset_loader.h"
#include "text_panel.h"
#include "text_image.h"

typedef struct {
    u8 *words[4];
} TextList4;
typedef struct {
    u8 *words[8];
} TextList8;
/// Four text frames used for the animated title-menu cursor.
/// \note Original identifier: \c D_8009B49C (0x8009B49C).
extern TextList4 gTitleCursorText;
/// Text labels used to prepare title-screen menu images.
/// \note Original identifier: \c D_8009B4AC (0x8009B4AC).
extern TextList8 gTitleMenuText;
/// Current title-screen menu choice index.
/// \note Original identifier: \c D_800F25A0 (0x800F25A0).
/// Nonzero when the file-selection menu loads an existing game.
/// \note Original identifier: \c D_800F25A4 (0x800F25A4).
extern s32 gTitleMenuSelection, gTitleLoadGameSelected;
/// Asset descriptor used to load the title logo image.
/// \note Original identifier: \c D_8009B390 (0x8009B390).
/// Asset descriptor used to load the startup artwork panel.
/// \note Original identifier: \c D_8009B3AC (0x8009B3AC).
/// Asset descriptor used to load the startup silhouette artwork.
/// \note Original identifier: \c D_8009B3C8 (0x8009B3C8).
extern AssetDescriptor gTitleLogoAsset, gTitleStartupPanelAsset, gTitleSilhouetteAsset;
/// Loaded title logo image used by logo drawing.
/// \note Original identifier: \c D_8009B3EC (0x8009B3EC).
/// Loaded artwork image used by startup panel drawing.
/// \note Original identifier: \c D_8009B3F0 (0x8009B3F0).
/// Loaded artwork image used by startup silhouette drawing.
/// \note Original identifier: \c D_8009B3F4 (0x8009B3F4).
extern s32 gTitleLogoImage, gTitleStartupPanelImage, gTitleSilhouetteImage;
/// Owned text images for the four animated menu-cursor frames.
/// \note Original identifier: \c D_800F2590 (0x800F2590).
/// Owned text images for the eight title-screen menu labels.
/// \note Original identifier: \c D_800F2570 (0x800F2570).
extern TextImage *gTitleCursorImages[4], *gTitleMenuImages[8];
/// Load title-screen artwork and menu text and reset menu selections.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800341C0 (0x800341C0).
void titleScreenInitialize(void) {
    TextImage **dest;
    TextList4 four = gTitleCursorText;
    TextList8 eight = gTitleMenuText;
    u8 **source;
    u8 **end;
    gTitleMenuSelection = 0;
    gTitleLoadGameSelected = 0;
    gTitleLogoImage = (s32)assetAllocateAndLoad(&gTitleLogoAsset);
    gTitleStartupPanelImage = (s32)assetAllocateAndLoad(&gTitleStartupPanelAsset);
    gTitleSilhouetteImage = (s32)assetAllocateAndLoad(&gTitleSilhouetteAsset);
    for (source = four.words, dest = gTitleCursorImages; source < four.words + 4; source++, dest++) {
        textImageCreateFromText((TextImage **)((u8 **)dest), (u8 *)(*source),
                                gTextImageConfigurations);
    }
    dest = gTitleMenuImages;
    for (source = eight.words, end = eight.words + 8; source != end; source++, dest++) {
        textImageCreateFromText((TextImage **)((u8 **)dest), (u8 *)(*source),
                                gTextImageConfigurations);
    }
}

/// Title-logo fade state; the middle value is reset but otherwise unused here.
typedef struct TitleLogoFade {
    s32 finished;
    f32 unknown4;
    f32 alpha;
} TitleLogoFade;
/// Title-logo fade state retaining completion and opacity.
/// \note Original identifier: \c D_800F25D8 (0x800F25D8).
extern TitleLogoFade gTitleLogoFade;

/// Reset the title logo to the beginning of its fade-in.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003431C (0x8003431C).
void titleScreenResetLogoFade(void) {
    gTitleLogoFade.finished = 0;
    gTitleLogoFade.unknown4 = 0.0f;
    gTitleLogoFade.alpha = 0.0f;
}

/// Loaded title logo image used by logo drawing.
/// \note Original identifier: \c D_8009B3EC (0x8009B3EC).
extern s32 gTitleLogoImage;
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Advance the title-logo fade and draw the logo.
/// \pre Title-screen artwork must be initialized.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
///
/// \note NONPORTABLE: callers pass the display-list cursor as s32 (32-bit pointers)
/// \note Original decompilation identifier: \c func_80034338 (0x80034338).
void titleScreenDrawLogo(Gfx **gdl) {
    TextureImage tex;

    if (gTitleLogoFade.finished == 0) {
        gTitleLogoFade.alpha += 300.0f * gRuntimeFrameSeconds;
        if (gTitleLogoFade.alpha > 255.0f) {
            gTitleLogoFade.alpha = 255.0f;
            gTitleLogoFade.finished = 1;
        }
    } else {
        gTitleLogoFade.alpha = 255.0f;
    }
    textureImageInitialize(&tex, gTitleLogoImage, 0, 0, 2, 0x108, 0x7E);
    gDPPipeSync((*gdl)++);
    gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, (s32)gTitleLogoFade.alpha);
    gDPSetCombineLERP((*gdl)++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0);
    textureDrawScaledRepeated(gdl, &tex, 0x1C, 0x19, 0, 0, 0, 0, 1.0f, 1.0f, 0, 0);
}

/// Loaded artwork image used by startup panel drawing.
/// \note Original identifier: \c D_8009B3F0 (0x8009B3F0).
extern s32 gTitleStartupPanelImage;

/// Draw the startup artwork panel on its background.
/// \pre Title-screen artwork must be initialized.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
///
/// \note NONPORTABLE: callers pass the display-list cursor as s32 (32-bit pointers)
/// \note Original decompilation identifier: \c func_800344E8 (0x800344E8).
void titleScreenDrawStartupPanel(Gfx **gdl) {
    TextureImage tex;

    textureImageInitialize(&tex, gTitleStartupPanelImage, 0, 0, 2, 0x80, 0x6E);
    gDPPipeSync((*gdl)++);
    gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
    gDPSetRenderMode((*gdl)++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetPrimColor((*gdl)++, 0, 0, 245, 245, 245, 255);
    gDPSetCombineMode((*gdl)++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPFillRectangle((*gdl)++, 0, 0, 319, 239);
    gDPPipeSync((*gdl)++);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 255);
    gDPSetCombineLERP((*gdl)++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0);
    textureDrawScaledRepeated(gdl, &tex, 0x60, 0x41, 2, 2, 0, 0, 1.0f, 1.0f, 0, 0);
    gDPPipeSync((*gdl)++);
    gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
}

/// Start-prompt drawing snapshot, including its negative-texture terminator.
typedef struct TitlePromptLayout {
    TextureRectangle rectangles[3]; ///< Two visible rectangles and the list terminator.
} TitlePromptLayout;
typedef char TitlePromptLayoutSizeCheck[(sizeof(TitlePromptLayout) == 0x6C) ? 1 : -1];
/// Sprite descriptors used to draw the pulsing Start prompt.
/// \note Original identifier: \c D_8009B4CC (0x8009B4CC).
extern TitlePromptLayout gTitleStartPromptSprites;
/// Elapsed-time checkpoint for title introduction, prompt and demonstration transitions.
/// \note Original identifier: \c D_800F2560 (0x800F2560).
extern u32 gTitleIntroductionMilliseconds;
/// Angular phase controlling the pulsing Start prompt opacity.
/// \note Original identifier: \c D_800F2564 (0x800F2564).
extern f32 gTitleStartPromptPhase;

/// Draw the pulsing start prompt nine seconds after the title-screen introduction begins.
/// \pre The introduction start time and prompt animation must be initialized.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
///
/// \note NONPORTABLE: callers pass the display-list cursor as s32 (32-bit pointers)
/// \note Original decompilation identifier: \c func_800346D8 (0x800346D8).
void titleScreenDrawStartPrompt(Gfx **gdl) {
    TitlePromptLayout t = gTitleStartPromptSprites;
    s32 a;

    if ((u64)runtimeGetElapsedMilliseconds() - (u32)gTitleIntroductionMilliseconds >= 0x2328) {
        gTitleStartPromptPhase += 120.0f * gRuntimeFrameSeconds;
        while (gTitleStartPromptPhase < 0.0f) {
            gTitleStartPromptPhase += 360.0f;
        }
        while (gTitleStartPromptPhase >= 360.0f) {
            gTitleStartPromptPhase -= 360.0f;
        }
        a = mathSinDegrees(gTitleStartPromptPhase) * 175.0f + 80.0f;
        if (a < 0) {
            a = 0;
        } else if (a >= 0x100) {
            a = 0xFF;
        }
        gDPPipeSync((*gdl)++);
        gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
        gDPPipeSync((*gdl)++);
        gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, a);
        gDPSetCombineLERP((*gdl)++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0);
        textureDrawRectangles(10, t.rectangles, gdl);
    }
}

/// Current title-screen menu choice index.
/// \note Original identifier: \c D_800F25A0 (0x800F25A0).
extern s32 gTitleMenuSelection;
/// Currently displayed text frame of the animated title-menu cursor.
/// \note Original identifier: \c D_800F25A8 (0x800F25A8).
extern s32 gTitleCursorFrame;
/// Draw title-screen menu choices and the current selection.
/// \pre Menu images must be initialized; the current selection must identify a listed row when the list is nonempty.
/// \param cursor Address of the display-list cursor to append commands to and advance.
/// \param items Menu-label identifiers from 0 through 7 in display order, terminated by a negative value; at most 31 rows are supported.
/// \param enabled Bit mask of enabled menu rows, with bit zero representing the first row.
/// \param unavailableRed Nonzero to show unselected unavailable choices in red; zero to use the alternate unavailable color.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80034910 (0x80034910).
void titleScreenDrawMenu(Gfx **cursor, const s32 *items, s32 enabled, s32 unavailableRed) {
    Gfx **gdl = cursor;
    s32 maxWidth = 0;
    s32 count = 0;
    s32 width;
    for (count = 0; items[count] >= 0; count++) {
        width = gTitleMenuImages[items[count]]->width;
        if (width >= maxWidth)
            maxWidth = width;
    }
    if (count != 0) {
        s32 x, y;
        s32 i;
        s32 textX;
        s32 cursorY;
        gDPPipeSync((*gdl)++);
        gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
        gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        maxWidth += 16;
        gDPSetPrimColor((*gdl)++, 0, 0, 0, 0, 0, 128);
        gDPSetCombineMode((*gdl)++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        x = 160 - maxWidth / 2;
        y = 200 - count * 16;
        gDPFillRectangle((*gdl)++, x - 10, y - 4, x + maxWidth + 10, 204);
        textDrawBegin(*gdl);
        textDrawReset();
        textDrawSetPalette(gDefaultTextPalette);
        textDrawSetSpacingMode(1);
        textDrawSetScale(1.0f, 1.0f);
        textDrawSetArea(0, 0, 319, 239);
        textX = x + 16;
        for (i = 0; i < count; i++) {
            textDrawSetCursor(textX, y + i * 16);
            if ((1 << i) & enabled) {
                if (i == gTitleMenuSelection)
                    textDrawSetColor(255, 255, 0, 255);
                else
                    textDrawSetColor(255, 255, 255, 255);
            } else if (i == gTitleMenuSelection)
                textDrawSetColor(255, 255, 40, 160);
            else if (unavailableRed)
                textDrawSetColor(255, 50, 50, 140);
            else
                textDrawSetColor(120, 120, 255, 160);
            textDrawImageAtCursor((TextImage *)gTitleMenuImages[items[i]]);
        }
        gTitleCursorFrame = (s32)((f32)runtimeGetElapsedMilliseconds() * 0.003f) % 4;
        cursorY = gTitleMenuSelection * 16 + y;
        textDrawSetCursor(x, cursorY);
        textDrawSetColor(255, 255, 0, 255);
        textDrawImageAtCursor((TextImage *)gTitleCursorImages[gTitleCursorFrame]);
        textDrawGetDisplayListCursor(gdl);
    }
}

/// Draw a black background for the startup screen.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
///
/// \note NONPORTABLE: callers pass the display-list cursor as s32 (32-bit pointers)
/// \note Original decompilation identifier: \c func_80034CD0 (0x80034CD0).
void titleScreenClearScreen(Gfx **gdl) {
    gDPPipeSync((*gdl)++);
    gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
    gDPSetRenderMode((*gdl)++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetPrimColor((*gdl)++, 0, 0, 0, 0, 0, 255);
    gDPSetCombineMode((*gdl)++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPFillRectangle((*gdl)++, 0, 0, 319, 239);
}

/// Owned text panel used for startup and save-selection messages.
/// \note Original identifier: \c D_800F25B8 (0x800F25B8).
extern TextPanel gTitleMessagePanel;
extern s32 profile_data_0000;
/// Draw the active title-screen message panel, if one is available.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80034D88 (0x80034D88).
void titleScreenDrawMessage(Gfx **gdl) {
    if (profile_data_0000 != 0) {
        gDPPipeSync((*gdl)++);
        gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
        textPanelDraw(&gTitleMessagePanel, gdl, 2);
    }
}

/// Loaded artwork image used by startup silhouette drawing.
/// \note Original identifier: \c D_8009B3F4 (0x8009B3F4).
extern s32 gTitleSilhouetteImage;

/// Draw the dark startup artwork silhouette.
/// \pre Title-screen artwork must be initialized.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
///
/// \note NONPORTABLE: callers pass the display-list cursor as s32 (32-bit pointers)
/// \note Original decompilation identifier: \c func_80034DF4 (0x80034DF4).
void titleScreenDrawStartupSilhouette(Gfx **gdl) {
    TextureImage tex;

    textureImageInitialize(&tex, gTitleSilhouetteImage, 0, 0, 2, 0x80, 0x70);
    gDPPipeSync((*gdl)++);
    gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetPrimColor((*gdl)++, 0, 0, 0, 0, 0, 255);
    gDPSetCombineLERP((*gdl)++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0);
    gDPPipeSync((*gdl)++);
    textureDrawScaledRepeated(gdl, &tex, 0x60, 0x40, 2, 2, 0, 0, 1.0f, 1.0f, 0, 0);
    gDPPipeSync((*gdl)++);
    gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
}

void titleScreenDrawLogo(Gfx **);
void titleScreenDrawStartupPanel(Gfx **);
void titleScreenDrawStartPrompt(Gfx **);
void titleScreenDrawMenu(Gfx **, const s32 *, s32, s32);
void titleScreenClearScreen(Gfx **);
void titleScreenDrawStartupSilhouette(Gfx **);
/// Current startup, title-menu or demonstration-transition state.
/// \note Original identifier: \c D_8009B3E4 (0x8009B3E4).
extern s32 gTitleScreenState;
/// Save-control byte transferred independently from the progress records.
/// \note Original identifier: \c D_8009A6F0 (0x8009A6F0).
extern u8 gEepromSaveControl;
/// Text-image indices for the New Game and Continue menu.
/// \note Original identifier: \c D_8009B464 (0x8009B464).
extern const s32 gTitleMainMenuItems[3];
/// Text-image indices for new-game file selection and cancellation.
/// \note Original identifier: \c D_8009B470 (0x8009B470).
extern const s32 gTitleNewGameFileMenuItems[4];
/// Text-image indices for existing-game file selection and cancellation.
/// \note Original identifier: \c D_8009B480 (0x8009B480).
extern const s32 gTitleLoadGameFileMenuItems[4];
/// Text-image indices for confirmation of overwriting an occupied file.
/// \note Original identifier: \c D_8009B490 (0x8009B490).
extern const s32 gTitleOverwriteMenuItems[3];
/// Nonzero when the file-selection menu loads an existing game.
/// \note Original identifier: \c D_800F25A4 (0x800F25A4).
extern s32 gTitleLoadGameSelected;
/// Bit mask of enabled file-selection choices.
/// \note Original identifier: \c D_800F25B0 (0x800F25B0).
extern s32 gTitleEnabledFileChoices;

/// Draw the current startup screen, title screen, or save-file menu.
/// \param cursor Address of the display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80034F6C (0x80034F6C).
void titleScreenDrawFrame(Gfx **cursor) {
    switch (gTitleScreenState) {
    case 0:
    case 1:
    case 2:
        break;
    case 3:
    case 4:
        titleScreenClearScreen(cursor);
        titleScreenDrawMessage(cursor);
        break;
    case 5:
    case 7:
        titleScreenClearScreen(cursor);
        titleScreenDrawStartupSilhouette(cursor);
        break;
    case 6:
    case 8:
    case 9:
        titleScreenDrawStartupPanel(cursor);
        break;
    case 10:
    case 11:
        backdropDraw(cursor);
        break;
    case 12:
    case 16:
        backdropDraw(cursor);
        titleScreenDrawLogo(cursor);
        objectDrawGroup(0x2000);
        hudDrawMicrophoneIndicators(cursor);
        titleScreenDrawStartPrompt(cursor);
        break;
    case 17:
        backdropDraw(cursor);
        titleScreenDrawLogo(cursor);
        objectDrawGroup(0x2000);
        hudDrawMicrophoneIndicators(cursor);
        break;
    case 13:
        backdropDraw(cursor);
        titleScreenDrawLogo(cursor);
        objectDrawGroup(0x2000);
        hudDrawMicrophoneIndicators(cursor);
        if (gEepromSaveControl != 0) {
            titleScreenDrawMenu(cursor, gTitleMainMenuItems, 3, 0);
        } else {
            titleScreenDrawMenu(cursor, gTitleMainMenuItems, 1, 1);
        }
        break;
    case 14:
        backdropDraw(cursor);
        titleScreenDrawLogo(cursor);
        objectDrawGroup(0x2000);
        hudDrawMicrophoneIndicators(cursor);
        if (gTitleLoadGameSelected != 0) {
            titleScreenDrawMenu(cursor, gTitleLoadGameFileMenuItems, gTitleEnabledFileChoices, 1);
        } else {
            titleScreenDrawMenu(cursor, gTitleNewGameFileMenuItems, ~gEepromSaveControl | 4, 0);
        }
        break;
    case 15:
        backdropDraw(cursor);
        titleScreenDrawLogo(cursor);
        objectDrawGroup(0x2000);
        hudDrawMicrophoneIndicators(cursor);
        titleScreenDrawMessage(cursor);
        titleScreenDrawMenu(cursor, gTitleOverwriteMenuItems, 0xFFFF, 0);
        break;
    case 18:
        backdropDraw(cursor);
        titleScreenDrawMessage(cursor);
        break;
    }
    cameraUpdateOverlayColors(0, 0);
    cameraDrawOverlay(0);
    cameraDrawSecondaryOverlay(0);
}


#include "microphone_connection.h"
#include "title_screen_state.h"
/// Query the current startup and title-screen state.
/// \returns The current title-screen state identifier.
/// \note Original decompilation identifier: \c func_800351C0 (0x800351C0).
s32 titleScreenGetState(void) {
    return gTitleScreenState;
}


/// Select Continue when a saved game is available, or New Game otherwise.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800351CC (0x800351CC).
void titleScreenSelectDefaultChoice(void) {
    if (gEepromSaveControl != 0) {
        gTitleMenuSelection = 1;
        return;
    }
    gTitleMenuSelection = 0;
}

/// Nonzero when an external request asks the waiting title screen to open its menu.
/// \note Original identifier: \c D_8009B3E8 (0x8009B3E8).
extern s32 gTitleMenuRequested;

/// Request the main menu when the title screen is awaiting Start.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800351FC (0x800351FC).
void titleScreenRequestMenu(void) {
    if ((gTitleMenuRequested == 0) && (gTitleScreenState == 0xC)) {
        gTitleMenuRequested = 1;
    }
}


/// Begin the transition to the next demonstration when the title screen is awaiting Start.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80035234 (0x80035234).
void titleScreenBeginDemoTransition(void) {
    if (gTitleScreenState == 0xC) {
        audioChangeCurrentCue(-1, 0.1f, 0.5);
        cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFE, 1.5f);
        gTitleScreenState = 0x10;
    }
}

/// Selected title-screen demonstration index from zero through three.
/// \note Original identifier: \c D_800F2568 (0x800F2568).
extern s32 gTitleDemoIndex;

/// Query the selected title-screen demonstration.
/// \returns The demonstration index, from 0 through 3.
/// \note Original decompilation identifier: \c func_800352AC (0x800352AC).
s32 titleScreenGetDemoIndex(void) {
    return gTitleDemoIndex;
}


/// Adjust the title-screen music volume.
/// \param level Relative music level, clamped to 0 through 1; its product with 32767 must be representable as a signed integer.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800352B8 (0x800352B8).
void titleScreenSetMusicVolume(f32 level) {
    s32 requestedVolume;
    s32 clampedVolume;

    requestedVolume = (s32)(32767.0f * level);
    clampedVolume = requestedVolume;
    if (requestedVolume < 0) {
        clampedVolume = 0;
    } else if (requestedVolume >= 0x8000) {
        clampedVolume = 0x7FFF;
    }
    audioSetCategoryVolume(AUDIO_CATEGORY_SEQUENCES, clampedVolume);
}

/// Prepare the title-screen message panel from a list of text entries.
/// \pre Any previous message entries must already be released.
/// \param list List of at least count null-terminated text entries, read during this call.
/// \param count Nonnegative number of text entries to display.
/// \param y Top position of the message panel in screen pixels.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80035314 (0x80035314).
void titleScreenSetMessage(void **list, s32 count, s32 y) {
    s32 i;
    textPanelInitialize(&gTitleMessagePanel, count);
    textPanelSetTextConfiguration(&gTitleMessagePanel, 0);
    textPanelSetPosition(&gTitleMessagePanel, 0, y);
    for (i = 0; i < count; i++)
        textPanelAppend(&gTitleMessagePanel, list[i]);
}


/// Release the initialized title-screen message and mark its panel inactive, retaining its color storage.
/// \returns Nothing.
/// \note Original decompilation identifier: \c osProfileStop (0x800353A4).
void titleScreenClearMessage(void) {
    textPanelReleaseEntries(&gTitleMessagePanel);
    profile_data_0000 = 0;
}

/// Find the next enabled save-file menu choice in the requested direction.
/// \param enabled Bit mask of enabled choices, with bit zero representing the first choice.
/// \param selection Current choice index, from 0 through 2.
/// \param step Search direction: -1 for preceding choices or 1 for following choices.
/// \returns The next enabled choice, or the original selection if the search reaches either end.
/// \note Original decompilation identifier: \c func_800353CC (0x800353CC).
s32 titleScreenFindEnabledChoice(s32 enabled, s32 selection, s32 step) {
    s32 i = selection;

    do {
        i += step;
        if (i >= 3 || i < 0) {
            i = selection;
            break;
        }
    } while (!((1 << i) & enabled));
    return i;
}

#include "runtime_lifecycle.h"
/// Primary controller new and auto-repeated button presses; aliases the repeated word of the processed sample.
/// \note Original identifier: \c D_800AD54C (0x800AD54C).
extern s32 gControllerPrimaryRepeatedButtons;
/// Save request parameters; the first word selects the EEPROM save record.
/// \note Original identifier: \c D_800AE510 (0x800AE510).
extern s32 gRuntimeSaveParameters;
/// Main-menu choice retained while a file-selection menu is active.
/// \note Original identifier: \c D_800F25AC (0x800F25AC).
extern s16 gTitleSavedMainMenuSelection;
/// File-menu choice retained while overwrite confirmation is active.
/// \note Original identifier: \c D_800F25AE (0x800F25AE).
extern s16 gTitleSavedFileSelection;

/// Handle save-file selection, cancellation, and requests to confirm overwriting an existing game.
/// \returns 1 when the selected game can begin loading or starting, or 0 while a menu remains active.
/// \note Original decompilation identifier: \c func_80035404 (0x80035404).
s32 titleScreenHandleFileSelection(void) {
    s32 buttons;
    s32 nextSelection;

    pikachuUpdateScriptAndTriggers();
    objectUpdateGroup(0x4000);
    if (audioCountActive(AUDIO_CATEGORY_ALL) == 0) {
        audioChangeCurrentCue(0, 0.0, 0.0);
    }
    buttons = gControllerPrimaryRepeatedButtons;
    if (buttons & 0xC00) {
        if (buttons & 0x800) {
            nextSelection = titleScreenFindEnabledChoice(gTitleEnabledFileChoices, gTitleMenuSelection, -1);
        } else if (buttons & 0x400) {
            nextSelection = titleScreenFindEnabledChoice(gTitleEnabledFileChoices, gTitleMenuSelection, 1);
        }
        if (nextSelection != gTitleMenuSelection) {
            gTitleMenuSelection = nextSelection;
            audioPlaySoundEffect(0xC0);
        }
        return 0;
    }
    if (buttons & 0x8000) {
        gTitleSavedFileSelection = (s16)gTitleMenuSelection;
        audioPlaySoundEffect(0xC5);
        switch (gTitleMenuSelection) {
        case 0:
            gRuntimeSaveParameters = 1;
            break;
        case 1:
            gRuntimeSaveParameters = 2;
            break;
        case 2:
            gTitleMenuSelection = gTitleSavedMainMenuSelection;
            gTitleScreenState = 0xD;
            return 0;
        }
        if (gTitleLoadGameSelected != 0) {
            progressLoadRecordFromEeprom(gRuntimeSaveParameters);
            gTitleScreenState = 0x11;
            return 1;
        }
        if (((gRuntimeSaveParameters == 1) && (gEepromSaveControl & 1)) ||
            ((gRuntimeSaveParameters == 2) && (gEepromSaveControl & 2))) {
            gTitleMenuSelection = 1;
            gTitleScreenState = 0xF;
            return 0;
        }
        gTitleScreenState = 0x11;
        return 1;
    } else {
        if (buttons & 0x4000) {
            gTitleMenuSelection = (s32)gTitleSavedMainMenuSelection;
            gTitleScreenState = 0xD;
            return 0;
        }
        return 0;
    }
}

/// Primary controller new and auto-repeated button presses; aliases the repeated word of the processed sample.
/// \note Original identifier: \c D_800AD54C (0x800AD54C).
/// Current startup, title-menu or demonstration-transition state.
/// \note Original identifier: \c D_8009B3E4 (0x8009B3E4).
extern s32 gControllerPrimaryRepeatedButtons, gTitleScreenState;
/// Handle confirmation or cancellation of starting a new game in an occupied save file.
/// \returns 1 when starting the new game is confirmed, or 0 otherwise.
/// \note Original decompilation identifier: \c func_8003563C (0x8003563C).
s32 titleScreenHandleOverwriteConfirmation(void) {
    pikachuUpdateScriptAndTriggers();
    objectUpdateGroup(0x4000);
    if (audioCountActive(AUDIO_CATEGORY_ALL) == 0)
        audioChangeCurrentCue(0, 0.0, 0.0);
    if (gControllerPrimaryRepeatedButtons & 0xC00) {
        if ((gControllerPrimaryRepeatedButtons & 0x800) && gTitleMenuSelection > 0) {
            audioPlaySoundEffect(0xC0);
            gTitleMenuSelection--;
        } else if ((gControllerPrimaryRepeatedButtons & 0x400) && gTitleMenuSelection <= 0) {
            audioPlaySoundEffect(0xC0);
            gTitleMenuSelection++;
        }
        return 0;
    }
    if (gControllerPrimaryRepeatedButtons & 0x8000) {
        audioPlaySoundEffect(0xC5);
        switch (gTitleMenuSelection) {
        case 0:
            gTitleScreenState = 17;
            return 1;
        case 1:
            gTitleMenuSelection = gTitleSavedFileSelection;
            gTitleScreenState = 14;
            return 0;
        }
    }
    if (gControllerPrimaryRepeatedButtons & 0x4000) {
        gTitleMenuSelection = gTitleSavedFileSelection;
        gTitleScreenState = 14;
        return 0;
    }
    return 0;
}

/// Backdrop identifiers indexed by title-screen demonstration.
/// \note Original identifier: \c D_8009B3F8 (0x8009B3F8).
extern s32 gTitleDemoBackdropIds[];
/// Text entries displayed when an EEPROM game-record load fails.
/// \note Original identifier: \c D_8009B408 (0x8009B408).
extern void *gTitleSaveLoadErrorMessage[7];
/// Text entries displayed when the primary standard controller is unavailable.
/// \note Original identifier: \c D_8009B424 (0x8009B424).
extern void *gTitleControllerMissingMessage[5];
/// Text entries displayed when the required microphone is unavailable.
/// \note Original identifier: \c D_8009B438 (0x8009B438).
extern void *gTitleMicrophoneMissingMessage[7];
/// Text entries displayed before confirming replacement of an occupied save file.
/// \note Original identifier: \c D_8009B454 (0x8009B454).
extern void *gTitleOverwriteMessage[4];
/// Remaining duration of the primary camera secondary overlay transition.
/// \note Original identifier: \c D_800ABDCC (0x800ABDCC).
extern f32 gPrimarySecondaryOverlayRemainingSeconds;
/// Primary controller newly pressed buttons; aliases the pressed word of the processed sample.
/// \note Original identifier: \c D_800AD544 (0x800AD544).
extern s32 gControllerPrimaryPressedButtons;
/// Elapsed-time checkpoint for the startup artwork display period.
/// \note Original identifier: \c D_800F25E4 (0x800F25E4).
extern u32 gTitleStartupPanelMilliseconds;
/// Elapsed-time checkpoint enforcing the silhouette minimum display period.
/// \note Original identifier: \c D_800F25E8 (0x800F25E8).
extern u32 gTitleSilhouetteMilliseconds;
#include "scene_configuration.h"
#include "scene_types.h"
void titleScreenInitialize(void);
void titleScreenResetLogoFade(void);
void titleScreenSetMessage(void **, s32, s32);
s32 titleScreenHandleFileSelection(void);
s32 titleScreenHandleOverwriteConfirmation(void);

/// Advance startup checks, title presentation, save-file menus, and transitions into gameplay or a demonstration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800357C4 (0x800357C4).
void titleScreenUpdate(void) {
    s32 *pressed = &gControllerPrimaryRepeatedButtons;

    switch (gTitleScreenState) {
    case 0:
        progressLoadControlDataFromEeprom();
        gTitleScreenState = 1;
        break;
    case 1:
        if (eepromGetQueuedTransferState() == EEPROM_TRANSFER_READING) {
            break;
        }
        if (eepromGetQueuedTransferState() != EEPROM_TRANSFER_IDLE) {
            gEepromSaveControl = 0;
        }
        if (!(gEepromSaveControl & 1) && (gEepromSaveControl & 2)) {
            gRuntimeSaveParameters = 2;
        } else {
            gRuntimeSaveParameters = 1;
        }
        progressLoadRecordFromEeprom(gRuntimeSaveParameters);
        if (controllerIsStandardPad(0) == 0) {
            titleScreenSetMessage(gTitleControllerMissingMessage, 5, 0x28);
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.0f);
            gTitleScreenState = 3;
            break;
        }
        if (micIsDeviceConnected(3) == 0) {
            titleScreenSetMessage(gTitleMicrophoneMissingMessage, 7, 0x28);
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.0f);
            gTitleScreenState = 4;
            break;
        }
        gTitleScreenState = 2;
        break;
    case 4:
        if (gControllerPrimaryPressedButtons & 0x1000) {
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.0f);
            titleScreenClearMessage();
            gTitleScreenState = 2;
        }
        break;
    case 2:
        titleScreenInitialize();
        controllerConfigureStickButtons(1, 0x1E, 0x1E, 0x1E, 0x1E);
        gTitleScreenState = 7;
        gTitleStartupPanelMilliseconds = runtimeGetElapsedMilliseconds();
        cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.0f);
        break;
    case 7:
        if (runtimeGetElapsedMilliseconds() - gTitleStartupPanelMilliseconds >= 0x579 ||
            (gEepromSaveControl != 0 && (*pressed & 0x9000))) {
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.4f);
            gTitleScreenState = 5;
        }
        break;
    case 5:
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.5f);
            titleScreenSetMusicVolume(1.0f);
            audioChangeCurrentCue(0, 0.5, 0.0);
            audioPlaySoundEffect(0x89);
            gTitleIntroductionMilliseconds = runtimeGetElapsedMilliseconds();
            titleScreenResetLogoFade();
            gTitleStartPromptPhase = 0.0f;
            gTitleScreenState = 6;
        }
        break;
    case 6:
        if (eepromGetQueuedTransferState() != EEPROM_TRANSFER_READING) {
            if (eepromGetQueuedTransferState() == EEPROM_TRANSFER_IDLE) {
                if (gRuntimeSaveParameters == 1) {
                    gEepromSaveControl |= 1;
                } else {
                    gEepromSaveControl |= 2;
                }
                progressSetChapterAndUnlockMilestones(0);
            } else {
                progressResetRecordToDefaults();
            }
            gTitleScreenState = 8;
        }
        break;
    case 8:
        if (runtimeGetElapsedMilliseconds() - gTitleIntroductionMilliseconds >= 0xBB9 || (*pressed & 0x9000)) {
            objectSetHierarchyFlags(((u8 *)gPikachuActor), 0x6000);
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.5f);
            gTitleScreenState = 9;
        }
        break;
    case 9:
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            backdropSelectForView(0, 0x17, 0);
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.5f);
            gTitleScreenState = 10;
            gTitleSilhouetteMilliseconds = runtimeGetElapsedMilliseconds();
        }
        break;
    case 10:
        if (runtimeGetElapsedMilliseconds() - gTitleIntroductionMilliseconds >= 0x1771 ||
            ((*pressed & 0x9000) && runtimeGetElapsedMilliseconds() - gTitleSilhouetteMilliseconds >= 0x3E9)) {
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.5f);
            gTitleScreenState = 11;
        }
        break;
    case 11:
        pikachuUpdateScriptAndTriggers();
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            backdropSelectForView(0, gTitleDemoBackdropIds[gTitleDemoIndex], 0);
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.5f);
            gTitleScreenState = 12;
        }
        break;
    case 12:
        pikachuUpdateScriptAndTriggers();
        objectUpdateGroup(0x4000);
        if (runtimeGetElapsedMilliseconds() - gTitleIntroductionMilliseconds >= 0x20103) {
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFE, 1.5f);
            gTitleScreenState = 16;
        } else if ((*pressed & 0x1000) || gTitleMenuRequested != 0) {
            titleScreenSetMusicVolume(0.75f);
            audioPlaySoundEffect(0xC5);
            titleScreenSelectDefaultChoice();
            gTitleScreenState = 13;
        }
        break;
    case 16:
        pikachuUpdateScriptAndTriggers();
        objectUpdateGroup(0x4000);
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            gTitleDemoIndex++;
            while (gTitleDemoIndex < 0) {
                gTitleDemoIndex += 4;
            }
            while (gTitleDemoIndex >= 4) {
                gTitleDemoIndex -= 4;
            }
            gTitleScreenState = 2;
            runtimeRequestGameModeChange(0x12);
            areaSetNextDestination(0x16, 0);
            runtimeRequestProgramChange(0);
        }
        break;
    case 13:
        pikachuUpdateScriptAndTriggers();
        objectUpdateGroup(0x4000);
        if (audioCountActive(AUDIO_CATEGORY_ALL) == 0) {
            audioChangeCurrentCue(0, 0.0, 0.0);
        }
        if (gEepromSaveControl != 0 && (*pressed & 0xC00)) {
            if ((*pressed & 0x800) && gTitleMenuSelection > 0) {
                audioPlaySoundEffect(0xC0);
                gTitleMenuSelection--;
            } else if ((*pressed & 0x400) && gTitleMenuSelection <= 0) {
                audioPlaySoundEffect(0xC0);
                gTitleMenuSelection++;
            }
        } else if (*pressed & 0x8000) {
            gTitleSavedMainMenuSelection = gTitleMenuSelection;
            audioPlaySoundEffect(0xC5);
            if (gTitleMenuSelection == 1) {
                gTitleLoadGameSelected = 1;
                gTitleEnabledFileChoices = gEepromSaveControl | 4;
                if (!(gEepromSaveControl & 1) && (gEepromSaveControl & 2)) {
                    gTitleMenuSelection = 1;
                } else {
                    gTitleMenuSelection = 0;
                }
            } else {
                gTitleLoadGameSelected = 0;
                gTitleEnabledFileChoices = 0xFFFF;
                if ((gEepromSaveControl & 1) && !(gEepromSaveControl & 2)) {
                    gTitleMenuSelection = 1;
                } else {
                    gTitleMenuSelection = 0;
                }
            }
            gTitleScreenState = 14;
        }
        break;
    case 14:
        if (titleScreenHandleFileSelection() != 0) {
            audioChangeCurrentCue(-1, 0.1f, 0.5);
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFE, 0.5f);
        }
        break;
    case 15:
        if (profile_data_0000 == 0) {
            titleScreenSetMessage(gTitleOverwriteMessage, 4, 0x40);
        }
        if (titleScreenHandleOverwriteConfirmation() != 0) {
            audioChangeCurrentCue(-1, 0.1f, 0.5);
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFE, 0.5f);
        }
        if (gTitleScreenState != 15) {
            titleScreenClearMessage();
        }
        break;
    case 17:
        pikachuUpdateScriptAndTriggers();
        objectUpdateGroup(0x4000);
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            if (gTitleLoadGameSelected == 0) {
                progressResetRecordToDefaults();
                progressUpdateChapterAfterVisit();
            } else if (eepromGetQueuedTransferState() == EEPROM_TRANSFER_IDLE) {
                progressApplyLoadedRecord();
            } else {
                if (eepromGetQueuedTransferState() == EEPROM_TRANSFER_READING) {
                    break;
                }
                if (gRuntimeSaveParameters == 1) {
                    gEepromSaveControl &= ~1;
                } else {
                    gEepromSaveControl &= ~2;
                }
                cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.0f);
                progressResetRecordToDefaults();
                titleScreenSetMessage(gTitleSaveLoadErrorMessage, 7, 0x28);
                gTitleScreenState = 18;
                break;
            }
            backdropSelectForView(0, gCurrentAreaConfiguration->backdrop, 0);
            gTitleScreenState = 0;
            controllerConfigureStickButtons(0, 0x1E, 0x1E, 0x1E, 0x1E);
            runtimeRequestGameModeChange(4);
            areaSetNextDestination(0, 0);
            scriptVmResetGlobals();
            runtimeRequestProgramChange(0);
        }
        break;
    case 18:
        if (gControllerPrimaryPressedButtons & 0x1000) {
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.0f);
            titleScreenClearMessage();
            gTitleScreenState = 5;
        }
        break;
    case 3:
        break;
    }
}
