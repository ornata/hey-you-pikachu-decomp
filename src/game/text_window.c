#include "player_control.h"
#include "render_state.h"
#include "controller_state.h"
#include "texture_rectangles.h"
#include "math_scalar.h"
#include "object_groups.h"
#include "camera_overlay.h"
#include "runtime_control.h"
#include "text_image_configurations.h"
#include "text_window.h"
#include "texture_animation_programs.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "runtime_clock.h"
#include "text_window_images.h"
#include "text_image_create.h"
#include "text_draw.h"
#include "integer_animation.h"
#include "text_image.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata in source order. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

#include "hud.h"
#include "player_interaction.h"
#include "runtime_services.h"

void _bzero(void *, s32);
/// Animation identifiers used to initialize the six window indicator tracks.
/// \note Original identifier: \c D_80099DA4 (0x80099DA4).
extern s32 gTextWindowIndicatorAnimationIds[6];
/// Shared accumulated message text from which window pages are prepared.
/// \note Original identifier: \c D_800F10B0 (0x800F10B0).
extern u8 gTextWindowPendingText[];
/// Scene-view mode controlling overlays and scene updates while the window is active.
/// \note Original identifier: \c D_800F1A74 (0x800F1A74).
extern s32 gTextWindowSceneViewMode;
/// Text-image style used when preparing window pages.
/// \note Original identifier: \c D_800F1A78 (0x800F1A78).
extern s32 gTextWindowTextStyle;
/// Animated window indicator identifier; zero hides the indicator.
/// \note Original identifier: \c D_800F1A7C (0x800F1A7C).
extern s32 gTextWindowIndicator;
/// Borrowed text remaining after the displayed page, or null at the message end.
/// \note Original identifier: \c D_800F1A80 (0x800F1A80).
extern u8 *gTextWindowContinuation;
/// Next write position in the window page-history ring.
/// \note Original identifier: \c D_800F1AB4 (0x800F1AB4).
extern s32 gTextWindowHistoryWriteIndex;
/// Oldest retained boundary in the window page-history ring.
/// \note Original identifier: \c D_800F1AB8 (0x800F1AB8).
extern s32 gTextWindowHistoryOldestIndex;
/// Phases of the shared text-window presentation lifecycle.
typedef enum TextWindowState {
    TEXT_WINDOW_OPENING = 0,       ///< Window opening transition.
    TEXT_WINDOW_AWAITING_INPUT = 1, ///< Current page is ready for advance input.
    TEXT_WINDOW_CHANGING_PAGE = 2, ///< Transition to the next page.
    TEXT_WINDOW_CLOSING = 3,       ///< Window closing transition.
    TEXT_WINDOW_INACTIVE = 4       ///< No active window presentation.
} TextWindowState;

/// Current shared text-window transition state; four is inactive.
/// \note Original identifier: \c D_800F1AC0 (0x800F1AC0).
extern s32 gTextWindowState;
/// Six indicator animation tracks initialized for the shared text window.
/// \note Original identifier: \c D_800F1AD4 (0x800F1AD4).
extern ValueTrack gTextWindowInitializedIndicatorTracks[6];

/// Initialize the shared text window and its page history.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002B6B0 (0x8002B6B0).
void textWindowInitialize(void) {
    s32 i;

    _bzero(gTextWindowPageImages, 0x28);
    _bzero(gTextWindowPendingText, 0x9C4);
    gTextWindowPromptImage = 0;
    gTextWindowHistoryWriteIndex = 0;
    gTextWindowHistoryOldestIndex = 1;
    gTextWindowContinuation = 0;
    gTextWindowPageImageBuffer = 0;
    gTextWindowSceneViewMode = 0;
    gTextWindowTextStyle = 0;
    gTextWindowIndicator = 1;
    gTextWindowState = TEXT_WINDOW_INACTIVE;
    for (i = 0; i < 6; i++) {
        animationIntegerTrackInitialize(&gTextWindowInitializedIndicatorTracks[i], gTextureFrameAnimationPrograms[gTextWindowIndicatorAnimationIds[i]]);
    }
}

/// Borrowed page-text pointers retained while their backing message remains valid.
/// \note Original identifier: \c D_800F1A88 (0x800F1A88).
extern u8 *gTextWindowPageHistory[11];
#define RING_WRAP(x)                                                                                                   \
    while ((x) < 0) {                                                                                                  \
        (x) += 11;                                                                                                     \
    }                                                                                                                  \
    while ((x) >= 11) {                                                                                                \
        (x) -= 11;                                                                                                     \
    }

/// Retain a text page for subsequent backward navigation.
/// \param pageText Borrowed page text that remains valid while retained in history.
/// \returns The supplied page text.
/// \note Original decompilation identifier: \c func_8002B798 (0x8002B798).
u8 *textWindowPushPage(u8 *pageText) {
    gTextWindowPageHistory[gTextWindowHistoryWriteIndex] = pageText;
    gTextWindowHistoryWriteIndex++;
    RING_WRAP(gTextWindowHistoryWriteIndex);
    if (gTextWindowHistoryWriteIndex == gTextWindowHistoryOldestIndex) {
        TRACE("page stack overflow.\n");
        gTextWindowHistoryOldestIndex++;
        RING_WRAP(gTextWindowHistoryOldestIndex);
    }
    return pageText;
}

/// Retrieve the most recently retained text page for backward navigation.
/// Requires at least one retained page.
/// \returns The borrowed page address, remaining at the oldest available page when history is exhausted.
/// \note Original decompilation identifier: \c func_8002B844 (0x8002B844).
u8 *textWindowPopPage(void) {
    gTextWindowHistoryWriteIndex--;
    RING_WRAP(gTextWindowHistoryWriteIndex);
    if (gTextWindowHistoryWriteIndex == gTextWindowHistoryOldestIndex) {
        TRACE("page stack exhausted.\n");
        gTextWindowHistoryWriteIndex++;
        RING_WRAP(gTextWindowHistoryWriteIndex);
    }
    return gTextWindowPageHistory[gTextWindowHistoryWriteIndex];
}

/// Elapsed-time checkpoint recorded when the text-window state changes.
/// \note Original identifier: \c D_800F1AC4 (0x800F1AC4).
extern u32 gTextWindowStateStartMilliseconds;

/// Begin a text-window state and its transition timing.
/// \param state Window state: 0 opening, 1 awaiting input, 2 changing pages, 3 closing, or 4 inactive.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002B8E8 (0x8002B8E8).
void textWindowSetState(s32 state) {
    gTextWindowState = state;
    gTextWindowStateStartMilliseconds = (u32)runtimeGetElapsedMilliseconds();
}

/// Separate indicator-indexed animation view; initialized tracks begin at index one.
/// \note Original identifier: \c D_800F1AC8 (0x800F1AC8).
extern ValueTrack gTextWindowIndicatorTrackIndexBase[];
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Draw the active animated text-window indicator.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \param alpha Indicator opacity, from 0 transparent to 255 opaque.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002B910 (0x8002B910).
void textWindowDrawIndicator(Gfx **gdl, s32 alpha) {
    TextureRectangle rectangles[2];
    s32 origin = 31;
    s32 size = 48;
    f32 scale = 1.0f;
    s32 rendererTable = 5;
    s32 texture;
    TRACE("alpha:%d\n", alpha);
    if (gTextWindowIndicator != 0) {
        gDPPipeSync((*gdl)++);
        gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineMode((*gdl)++, G_CC_MODULATERGB_PRIM, G_CC_MODULATERGB_PRIM);
        gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, alpha);
        gDPPipeSync((*gdl)++);
        gDPSetTexturePersp((*gdl)++, G_TP_NONE);
        gDPSetTextureLOD((*gdl)++, G_TL_TILE);
        gDPSetTextureFilter((*gdl)++, G_TF_BILERP);
        gDPSetTextureConvert((*gdl)++, G_TC_FILT);
        gDPSetTextureDetail((*gdl)++, G_TD_CLAMP);
        gDPSetTextureLUT((*gdl)++, G_TT_RGBA16);
        gSPTexture((*gdl)++, 0x7FFF, 0, 0, G_TX_RENDERTILE, G_ON);
        gMoveWd((*gdl)++, G_MW_FOG, 0x10, 0x7FFF);
        gDPPipeSync((*gdl)++);
        animationIntegerTrackEvaluate(&gTextWindowIndicatorTrackIndexBase[gTextWindowIndicator], &texture,
                                      gRuntimeFrameSeconds);
        rectangles[0].texture = texture;
        rectangles[0].x = origin;
        rectangles[0].y = origin;
        rectangles[0].width = size;
        rectangles[0].height = size;
        rectangles[0].s = 0;
        rectangles[0].t = 0;
        rectangles[0].scaleX = scale;
        rectangles[0].scaleY = scale;
        rectangles[1].texture = -1;
        textureDrawRectangles(rendererTable, (TextureRectangle *)rectangles, gdl);
    }
}

/// Draw the text-window background.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \param alpha Background opacity, from 0 transparent to 255 opaque.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_8002BB68 (0x8002BB68).
void textWindowDrawBackground(Gfx **gdl, s32 alpha) {
    gDPPipeSync((*gdl)++);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode((*gdl)++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor((*gdl)++, 0, 0, 0, 0, 0, alpha);
    gDPFillRectangle((*gdl)++, 47, 55, 273, 205);
}


/// Draw the current text page at a screen position.
/// \param list Display-list cursor to append drawing commands to and advance.
/// \param x Left edge of the page in screen pixels.
/// \param y Top edge of the page in screen pixels.
/// \param alpha Page opacity, clamped to the range 0 through 255.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002BC0C (0x8002BC0C).
void textWindowDrawPageAt(Gfx **list, s32 x, s32 y, s32 alpha) {
    s32 i;
    TextImage *image;

    if (alpha < 0) {
        alpha = 0;
    } else if (alpha >= 256) {
        alpha = 255;
    }
    gDPPipeSync((*list)++);
    gDPSetPrimColor((*list)++, 0, 0, 255, 255, 220, alpha);
    if (alpha < 255) {
        gDPSetRenderMode((*list)++, 0, 0x005041C8);
    } else {
        gDPSetRenderMode((*list)++, 0, 0x00553048);
    }
    for (i = 0; i < 5; i++) {
        image = (TextImage *)gTextWindowPageImages[i][gTextWindowPageImageBuffer];
        if (image != NULL) {
            textImageDrawRectangle(list, image, x, y, image->width + x, image->height + y, 0, 0, 1.0f, 1.0f);
            y += ((TextImage *)gTextWindowPageImages[i][gTextWindowPageImageBuffer])->height + 4;
        } else {
            y += 20;
        }
    }
}

void textWindowDrawPageAt(Gfx **, s32, s32, s32);

/// Draw the current text page with its page-transition appearance.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002BDB4 (0x8002BDB4).
void textWindowDrawPage(Gfx **gdl) {
    textDrawBegin(*gdl);
    textDrawReset();
    textDrawSetPalette(gDefaultTextPalette);
    textDrawGetDisplayListCursor(gdl);
    if (gTextWindowState == TEXT_WINDOW_CHANGING_PAGE) {
        textWindowDrawPageAt(
            gdl, 0x35, 0x53,
            mathMapRangeClamped(0.0f, 255.0f, 90.0f, 10.0f,
                                runtimeGetElapsedMilliseconds() - (u64)gTextWindowStateStartMilliseconds));
    } else {
        textWindowDrawPageAt(gdl, 0x36, 0x54, 0x50);
        textWindowDrawPageAt(gdl, 0x35, 0x53, 0xFF);
    }
}


/// Draw the pulsing text-window input prompt.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002BE9C (0x8002BE9C).
void textWindowDrawPrompt(Gfx **gdl) {
    textDrawBegin(*gdl);
    textDrawReset();
    textDrawSetPalette(gDefaultTextPalette);
    textDrawSetColor(0xFF, 0xFF, 0xFF, runtimeGetUiPulseAlpha());
    textDrawSetCursor((0x140 - gTextWindowPromptImage->width) / 2 - 0x20, 0x9F);
    textDrawImageAtCursor((TextImage *)gTextWindowPromptImage);
    textDrawGetDisplayListCursor(gdl);
    textDrawReset();
    textDrawSetPalette(gDefaultTextPalette);
}


/// Replace a line of the current text page.
/// \param row Zero-based line index, from 0 through 4.
/// \param text Null-terminated line text, or null or empty text to clear the line.
/// \param style Valid text-style table index for the line.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002BF48 (0x8002BF48).
void textWindowSetLine(s32 row, u8 *text, s32 style) {
    if (gTextWindowPageImages[row][gTextWindowPageImageBuffer] != 0) {
        textImageDestroy((TextImage *)gTextWindowPageImages[row][gTextWindowPageImageBuffer]);
        gTextWindowPageImages[row][gTextWindowPageImageBuffer] = 0;
    }
    if (text != NULL && *text != 0) {
        textImageCreateFromText((TextImage **)(&gTextWindowPageImages[row][gTextWindowPageImageBuffer]),
                                (u8 *)((s32)text), &gTextImageConfigurations[style]);
    }
}


/// Replace the text-window input prompt.
/// \param textAddress Address of the null-terminated prompt text.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002BFE8 (0x8002BFE8).
void textWindowSetPrompt(s32 textAddress) {
    if (gTextWindowPromptImage != 0) {
        textImageDestroy((TextImage *)gTextWindowPromptImage);
        gTextWindowPromptImage = 0;
    }
    textImageCreateFromText((TextImage **)((s32 *)&gTextWindowPromptImage), (u8 *)(textAddress),
                            gTextImageConfigurations);
}

/// Prepare a page of text for display using the requested text style.
/// \param text Null-terminated page text, with newlines separating lines and formfeeds separating pages.
/// Each page supports at most five lines of forty-nine bytes each.
/// \param style Valid text-style table index for the page.
/// \returns A borrowed continuation address in the input text, or zero when a text terminator is encountered.
/// \note Original decompilation identifier: \c func_8002C03C (0x8002C03C).
u8 *textWindowPreparePage(u8 *text, s32 style) {
    u8 line[50];
    u8 *input = (u8 *)text;
    s32 row;
    s32 i;
    u8 ch;

    for (row = 0; row < 5; row++) {
        for (i = 0; i < 49; i++) {
            ch = *input;
            if (ch == 0) {
                line[i] = 0;
                textWindowSetLine(row, line, style);
                for (++row; row < 5; row++) {
                    textWindowSetLine(row, NULL, style);
                }
                return 0;
            }
            if (ch == 12) {
                line[i] = 0;
                textWindowSetLine(row, line, style);
                for (++row; row < 5; row++) {
                    textWindowSetLine(row, NULL, style);
                }
                return input + 1;
            }
            if (ch == 10) {
                line[i] = 0;
                input++;
                break;
            }
            line[i] = ch;
            input++;
        }
        line[49] = 0;
        if (style == 13) {
            TRACE("line %d message '%s'\n", row, line);
        }
        textWindowSetLine(row, line, style);
    }
    return input + 1;
}


/// Select a text page for display and retain it for backward navigation.
/// \param text Page text to display using the current style; must remain valid while retained in history.
/// \returns A borrowed continuation address in the input text, or zero when a text terminator is encountered.
/// \note Original decompilation identifier: \c func_8002C1A0 (0x8002C1A0).
u8 *textWindowShowPage(u8 *text) {
    textWindowPushPage(text);
    gTextWindowPageImageBuffer ^= 1;
    return textWindowPreparePage(text, gTextWindowTextStyle);
}

/// Clear the window text and page history and restore the default input prompt.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002C1E4 (0x8002C1E4).
void textWindowResetText(void) {
    _bzero(gTextWindowPendingText, 0x9C4);
    gTextWindowContinuation = gTextWindowPendingText;
    gTextWindowHistoryOldestIndex = 0;
    gTextWindowHistoryWriteIndex = gTextWindowHistoryOldestIndex + 1;
    textWindowSetPrompt((s32) "Press the @I@J Button.");
}

/// Close the text window, release its displayed text, and return to gameplay.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002C240 (0x8002C240).
void textWindowFinish(void) {
    s32 i;
    s32 j;
    for (i = 0; i < 6; i++) {
        for (j = 0; j < 2; j++) {
            if (gTextWindowPageImages[i][j] != 0) {
                textImageDestroy((TextImage *)gTextWindowPageImages[i][j]);
            }
            gTextWindowPageImages[i][j] = 0;
        }
    }
    gTextWindowState = TEXT_WINDOW_INACTIVE;
    runtimeRequestGameModeChange(0);
    playerSetControllerPort(0);
}

/// Append text to the pending text-window message while space remains.
/// \param text Null-terminated text to append.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002C2E4 (0x8002C2E4).
void textWindowAppendText(u8 *text) {
    s32 i = 0;
    u8 c;

    if (gTextWindowPendingText[0] != 0) {
        do {
            if (i >= 0x9C5) {
                return;
            }
            i++;
        } while (gTextWindowPendingText[i] != 0);
    }
    c = *text;
    if (c != 0) {
        do {
            if (i >= 0x9C5) {
                return;
            }
            gTextWindowPendingText[i] = c;
            c = text[1];
            i++;
            text++;
        } while (c != 0);
    }
}

void textWindowResetText(void);

/// Open an inactive text window with the requested presentation settings.
/// \param indicator Animated window indicator identifier, or zero to hide it.
/// \param style Text style identifier for displayed pages.
/// \param viewMode Scene-view mode; nonzero also enables scene updates while the window is active.
/// \returns 1 when opened, or 0 when a text window is already active.
/// \note Original decompilation identifier: \c func_8002C348 (0x8002C348).
s32 textWindowOpen(s32 indicator, s32 style, s32 viewMode) {
    if (gTextWindowState != TEXT_WINDOW_INACTIVE) {
        return 0;
    }
    textWindowResetText();
    textWindowSetState(TEXT_WINDOW_OPENING);
    gTextWindowIndicator = indicator;
    gTextWindowSceneViewMode = viewMode;
    gTextWindowTextStyle = style;
    return 1;
}

void textWindowDrawPage(Gfx **);
void textWindowDrawPrompt(Gfx **);

/// Draw the text window and its input prompt within the current screen.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002C3BC (0x8002C3BC).
void textWindowDrawEmbedded(void) {
    textWindowDrawBackground(&gRenderDisplayListCursor, 0xBE);
    textWindowDrawPage(&gRenderDisplayListCursor);
    textWindowDrawPrompt(&gRenderDisplayListCursor);
}


/// Display the first page of the pending message.
/// \returns 1 when another page remains, or 0 otherwise.
/// \note Original decompilation identifier: \c func_8002C3FC (0x8002C3FC).
s32 textWindowShowFirstPage(void) {
    u8 *continuationAddress;

    continuationAddress = textWindowShowPage((u8 *)gTextWindowPendingText);
    gTextWindowContinuation = continuationAddress;
    if (!(&gTextWindowContinuation)) {
    }
    if (continuationAddress != 0) {
        return 1;
    }
    return 0;
}


/// Mark the text window inactive while retaining its text resources.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002C43C (0x8002C43C).
void textWindowDeactivate(void) {
    gTextWindowState = TEXT_WINDOW_INACTIVE;
}

void textWindowDrawIndicator(Gfx **, s32);
/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;

/// Draw the text-window game mode with its scene and current window transition.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002C44C (0x8002C44C).
void textWindowDrawFrame(void) {
    s32 v;

    objectDrawGroup(0);
    cameraUpdateOverlayColors(gRenderCameraIndex, gTextWindowSceneViewMode);
    cameraDrawOverlay(gRenderCameraIndex);
    hudDraw(&gRenderDisplayListCursor);
    cameraDrawSecondaryOverlay(gRenderCameraIndex);
    gDPPipeSync(gRenderDisplayListCursor++);
    gDPSetCycleType(gRenderDisplayListCursor++, G_CYC_1CYCLE);
    switch ((u32)gTextWindowState) {
    case TEXT_WINDOW_AWAITING_INPUT:
        textWindowDrawBackground(&gRenderDisplayListCursor, 0xAA);
        textWindowDrawIndicator(&gRenderDisplayListCursor, 0xFF);
        textWindowDrawPage(&gRenderDisplayListCursor);
        textWindowDrawPrompt(&gRenderDisplayListCursor);
        break;
    case TEXT_WINDOW_CHANGING_PAGE:
        textWindowDrawBackground(&gRenderDisplayListCursor, 0xAA);
        textWindowDrawIndicator(&gRenderDisplayListCursor, 0xFF);
        textWindowDrawPage(&gRenderDisplayListCursor);
        break;
    case TEXT_WINDOW_OPENING:
        v = 0xFF * (runtimeGetElapsedMilliseconds() - (u64)gTextWindowStateStartMilliseconds) / 0x12C;
        if (v < 0) {
            v = 0;
        } else if (v >= 0x100) {
            v = 0xFF;
        }
        textWindowDrawBackground(&gRenderDisplayListCursor, v / 2);
        textWindowDrawIndicator(&gRenderDisplayListCursor, v);
        break;
    case TEXT_WINDOW_CLOSING:
        v = 0xFF * (0x12C - runtimeGetElapsedMilliseconds() + gTextWindowStateStartMilliseconds) / 0x12C;
        if (v < 0) {
            v = 0;
        } else if (v >= 0x100) {
            v = 0xFF;
        }
        textWindowDrawBackground(&gRenderDisplayListCursor, v / 2);
        textWindowDrawIndicator(&gRenderDisplayListCursor, v);
        break;
    case TEXT_WINDOW_INACTIVE:
        break;
    }
}

void pikachuUpdateScriptAndTriggers(void);
/// Processed button and stick samples for all controller ports; scalar declarations view the primary held-button word.
/// Primary controller newly pressed buttons; aliases the pressed word of the processed sample.
/// \note Original identifier: \c D_800AD544 (0x800AD544).
/// Primary controller newly released buttons; aliases the released word of the processed sample.
/// \note Original identifier: \c D_800AD548 (0x800AD548).
/// Primary controller new and auto-repeated button presses; aliases the repeated word of the processed sample.
/// \note Original identifier: \c D_800AD54C (0x800AD54C).
extern s32 gControllerPrimaryPressedButtons, gControllerPrimaryReleasedButtons, gControllerPrimaryRepeatedButtons;
/// Advance text-window transitions, respond to page-advance input, and update the enabled scene.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002C6A4 (0x8002C6A4).
void textWindowUpdate(void) {
    s32 i;
    switch ((u32)gTextWindowState) {
    case TEXT_WINDOW_INACTIVE:
        break;
    case TEXT_WINDOW_AWAITING_INPUT:
        if (gControllerPrimaryRepeatedButtons & 0x8000) {
            if (gTextWindowContinuation == 0)
                textWindowSetState(TEXT_WINDOW_CLOSING);
            else
                textWindowSetState(TEXT_WINDOW_CHANGING_PAGE);
        }
        gControllerPrimaryRepeatedButtons = 0;
        gControllerPrimaryReleasedButtons = 0;
        gControllerPrimaryPressedButtons = 0;
        gControllerInputState[0].held = 0;
        break;
    case TEXT_WINDOW_CHANGING_PAGE:
        if (runtimeGetElapsedMilliseconds() - (u64)gTextWindowStateStartMilliseconds > 90) {
            gTextWindowContinuation = textWindowShowPage((u8 *)gTextWindowContinuation);
            runtimeResetUiPulse();
            textWindowSetState(TEXT_WINDOW_AWAITING_INPUT);
        }
        break;
    case TEXT_WINDOW_OPENING:
        if (runtimeGetElapsedMilliseconds() - (u64)gTextWindowStateStartMilliseconds > 300) {
            runtimeResetUiPulse();
            textWindowSetState(TEXT_WINDOW_AWAITING_INPUT);
        }
        break;
    case TEXT_WINDOW_CLOSING:
        if (runtimeGetElapsedMilliseconds() - (u64)gTextWindowStateStartMilliseconds > 300)
            textWindowFinish();
        break;
    }
    i = 0;
    if (gTextWindowSceneViewMode != 0) {
        for (; i < 3; i++) {
            if (i > 0)
                continue;
            playerUpdateInput(i);
        }
        pikachuUpdateScriptAndTriggers();
        objectUpdateGroup(0);
    }
}

u8 *textWindowShowPage(u8 *);

/// Display the pending message and enter the text-window game mode.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002C874 (0x8002C874).
void textWindowEnterMode(void) {
    playerSetControllerPort(-1);
    gTextWindowContinuation = textWindowShowPage((u8 *)gTextWindowPendingText);
    runtimeRequestGameModeChange(3);
}

/// X-axis alignment offset used by collision spatial hashing.
/// \note Original identifier: \c D_800F1B40 (0x800F1B40).
extern f32 gCollisionGridAlignmentX;
/// Z-axis alignment offset used by collision spatial hashing.
/// \note Original identifier: \c D_800F1B44 (0x800F1B44).
extern f32 gCollisionGridAlignmentZ;
