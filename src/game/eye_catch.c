#include "runtime_lifecycle.h"
#include "audio_sound_effect.h"
#include "audio_control.h"
#include "game_heap.h"
#include "texture_image.h"
#include "texture_rectangles.h"
#include "random.h"
#include "controller_rumble.h"
#include "progress_value.h"
#include "camera_overlay.h"
#include "heap_free.h"
#include "common.h"
#include "audio_cue.h"
#include "asset_loader.h"
#include "integer_animation.h"
#include "heap_alloc.h"
#include "heap_types.h"

/* Diagnostic format strings whose callers were compiled out. */
static const char sInvalidEyeCatchFmt[] = "Invalid eyeCatch table.\n_eyeCatchTbl[%d].catch = %d\n";
static const char sEcInitFmt[] = "ecInit(%d)\n";
static const char sChipFmt[] = "chip:%d/%d\n";

/// Selected eye-catch group index, or minus one when no interlude is selected.
/// \note Original identifier: \c D_8009B580 (0x8009B580).
extern s32 gEyeCatchSelectedGroup;
/// Elapsed seconds in the current eye-catch stage.
/// \note Original identifier: \c D_800F25F0 (0x800F25F0).
extern f32 gEyeCatchStageSeconds;
/// Owned per-layer animation tracks for the active eye-catch interlude.
/// \note Original identifier: \c D_800F25F4 (0x800F25F4).
extern s32 gEyeCatchAnimationTracks;
/// Borrowed descriptor of the active eye-catch group, or zero when inactive.
/// \note Original identifier: \c D_800F25F8 (0x800F25F8).
extern s32 gEyeCatchActiveGroup;
/// Current eye-catch playback stage.
/// \note Original identifier: \c D_800F25FC (0x800F25FC).
extern s32 gEyeCatchStage;
/// Owned image asset faded into the eye-catch foreground.
/// \note Original identifier: \c D_800F2600 (0x800F2600).
extern s32 gEyeCatchForegroundImage;
typedef struct {
    s32 id;
    s32 x;
    s32 y;
    u8 pad[0x18];
} Rec36A60;
/// Eye-catch artwork, rectangle layout, animation keys and corner colors.
typedef struct {
    s32 image;
    Rec36A60 *records;
    s16 x, y;
    s16 flags, count;
    s32 (*keys)[6];
    s16 width, height;
    u8 rgb[4][3];
} EyeGroup36A;
/// Six eye-catch group descriptors containing artwork, layout, animation and selection weights.
/// \note Original identifier: \c D_8009C2AC (0x8009C2AC).
extern EyeGroup36A gEyeCatchGroups[6];

#include "eye_catch.h"
/// Prepare eye-catch layouts and reset interlude state once at startup.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80036A60 (0x80036A60).
void eyeCatchInitialize(void) {
    s32 i;
    EyeGroup36A *g;
    Rec36A60 *r;

    for (i = 0, g = gEyeCatchGroups; i < 6; i++, g++) {
        r = g->records;
        if (r->id >= 0) {
            do {
                r->x += g->x;
                r->y += g->y;
                r++;
            } while (r->id >= 0);
        }
    }
    gEyeCatchStageSeconds = 0.0f;
    gEyeCatchAnimationTracks = 0;
    gEyeCatchActiveGroup = 0;
    gEyeCatchStage = 0;
    gEyeCatchForegroundImage = 0;
    gEyeCatchSelectedGroup = -1;
}

/// Asset descriptor used to load the eye-catch foreground image.
/// \note Original identifier: \c D_8009B588 (0x8009B588).
extern AssetDescriptor gEyeCatchForegroundAssetDescriptor;
/// Screen coordinates of the four eye-catch backdrop corners.
/// \note Original identifier: \c D_8009C3A4 (0x8009C3A4).
extern s16 gEyeCatchBackdropCorners[4][2];
/// Owned colored quad vertices for the eye-catch backdrop.
/// \note Original identifier: \c D_800F2610 (0x800F2610).
extern s32 gEyeCatchBackdropVertices;
/// Owned orthographic projection matrix for eye-catch drawing.
/// \note Original identifier: \c D_800F2614 (0x800F2614).
extern s32 gEyeCatchProjectionMatrix;
/// Owned model-view matrix for eye-catch drawing.
/// \note Original identifier: \c D_800F2618 (0x800F2618).
extern s32 gEyeCatchModelViewMatrix;

#define EYE_GROUP ((EyeGroup36A *)gEyeCatchActiveGroup)

/// Six animation channels for each eye-catch layer.
typedef f32 EyeTracks36A[6][3];
#define EYE_TRACKS(group) (*(EyeTracks36A *)((group) * sizeof(EyeTracks36A) + gEyeCatchAnimationTracks))

/// Load and initialize the selected eye-catch interlude.
/// Requires a selected group from zero through five and no unreleased interlude.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80036AF4 (0x80036AF4).
void eyeCatchBegin(void) {
    s16 count;
    s32 group;
    s32 corner;

    controllerRequestRumbleReset(0);
    gEyeCatchActiveGroup = (s32)&gEyeCatchGroups[gEyeCatchSelectedGroup];
    count = EYE_GROUP->count;
    if (count != 0) {
        gEyeCatchAnimationTracks = (s32)heapAllocLocal((HpHdr *)&gGameHeap, count * sizeof(EyeTracks36A), 0x508);
        for (group = 0; group < EYE_GROUP->count; group++) {
            animationIntegerTrackInitialize((ValueTrack *)EYE_TRACKS(group)[0], (ValueKey *)EYE_GROUP->keys[group][0]);
            animationIntegerTrackInitialize((ValueTrack *)EYE_TRACKS(group)[1], (ValueKey *)EYE_GROUP->keys[group][1]);
            animationIntegerTrackInitialize((ValueTrack *)EYE_TRACKS(group)[2], (ValueKey *)EYE_GROUP->keys[group][2]);
            animationIntegerTrackInitialize((ValueTrack *)EYE_TRACKS(group)[3], (ValueKey *)EYE_GROUP->keys[group][3]);
            animationIntegerTrackInitialize((ValueTrack *)EYE_TRACKS(group)[4], (ValueKey *)EYE_GROUP->keys[group][4]);
            animationIntegerTrackInitialize((ValueTrack *)EYE_TRACKS(group)[5], (ValueKey *)EYE_GROUP->keys[group][5]);
        }
    }
    gEyeCatchForegroundImage = (s32)assetAllocateAndLoad(&gEyeCatchForegroundAssetDescriptor);
    gEyeCatchBackdropVertices = (s32)heapAllocLocal((HpHdr *)&gGameHeap, 4 * sizeof(Vtx), 0x521);
    for (corner = 0; corner < 4; corner++) {
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.ob[0] = gEyeCatchBackdropCorners[corner][0];
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.ob[1] = gEyeCatchBackdropCorners[corner][1];
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.ob[2] = 10;
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.flag = 0;
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.tc[0] = 0;
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.tc[1] = 0;
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.cn[0] = EYE_GROUP->rgb[corner][0];
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.cn[1] = EYE_GROUP->rgb[corner][1];
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.cn[2] = EYE_GROUP->rgb[corner][2];
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.cn[3] = 0xFF;
    }
    gEyeCatchProjectionMatrix = (s32)heapAllocLocal((HpHdr *)&gGameHeap, sizeof(Mtx), 0x530);
    guOrtho((Mtx *)gEyeCatchProjectionMatrix, 0.0f, 320.0f, 0.0f, 240.0f, -1.0f, -1000.0f, 1.0f);
    gEyeCatchModelViewMatrix = (s32)heapAllocLocal((HpHdr *)&gGameHeap, sizeof(Mtx), 0x537);
    guRotate((Mtx *)gEyeCatchModelViewMatrix, 0.0f, 0.0f, 0.0f, 1.0f);
    cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.0f);
    cameraUpdateOverlayColors(0, 0);
}
#undef EYE_TRACKS
#undef EYE_GROUP

/// Whether foreground drawing and its fade-in are enabled.
/// \note Original identifier: \c D_800F2604 (0x800F2604).
extern s32 gEyeCatchForegroundVisible;
/// Next timed audio event in eye-catch playback.
/// \note Original identifier: \c D_800F260C (0x800F260C).
extern s32 gEyeCatchAudioStage;

/// Release the active eye-catch resources and clear its selection and playback state.
/// Requires an interlude previously prepared by eyeCatchBegin and not yet released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80036F74 (0x80036F74).
void eyeCatchRelease(void) {
    void *tracks;
    heapFreeLocal(&gGameHeap, (void *)gEyeCatchModelViewMatrix);
    heapFreeLocal(&gGameHeap, (void *)gEyeCatchProjectionMatrix);
    heapFreeLocal(&gGameHeap, (void *)gEyeCatchBackdropVertices);
    assetFree((void *)gEyeCatchForegroundImage);
    tracks = (void *)gEyeCatchAnimationTracks;
    if (tracks != NULL) {
        heapFreeLocal(&gGameHeap, tracks);
    }
    gEyeCatchForegroundImage = 0;
    gEyeCatchActiveGroup = 0;
    gEyeCatchAnimationTracks = 0;
    gEyeCatchStageSeconds = 0.0f;
    gEyeCatchStage = 0;
    gEyeCatchForegroundVisible = 0;
    gEyeCatchAudioStage = 0;
    gEyeCatchSelectedGroup = -1;
}

typedef struct TextureDraw370 {
    s32 texture, x, y, width, height, s, t;
    f32 scaleX, scaleY;
} TextureDraw370;
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Advance and draw the active eye-catch backdrop and animated image layers.
/// Requires resources prepared by eyeCatchBegin.
/// \param displayList Display-list cursor receiving drawing commands and advanced past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80037034 (0x80037034).
void eyeCatchDrawLayers(Gfx **displayList) {
    s32 var_s0;
    s32 var_s4;

    renderDrawColorOverlayWithDepth(0, 0, 0, 0xFF);
    gDPPipeSync((*displayList)++);
    gDPSetCycleType((*displayList)++, G_CYC_1CYCLE);
    gSPLoadGeometryMode((*displayList)++, G_SHADE | G_SHADING_SMOOTH);
    gDPSetRenderMode((*displayList)++, 0x0F0A4000, 0);
    gDPSetCombine((*displayList)++, 0xFFFFFF, 0xFFFE793C);
    gSPMatrix((*displayList)++, gEyeCatchProjectionMatrix + 0x80000000, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix((*displayList)++, gEyeCatchModelViewMatrix + 0x80000000, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPVertex((*displayList)++, gEyeCatchBackdropVertices + 0x80000000, 4, 0);
    gSP2Triangles((*displayList)++, 0, 1, 2, 0, 0, 2, 3, 0);
    gDPPipeSync((*displayList)++);
    gDPSetRenderMode((*displayList)++, 0x504240, 0);
    gDPSetCombine((*displayList)++, 0xFFFFFF, 0xFFFCF279);
    textureDrawRectangles(9, (TextureDrawC354 *)(s32 *)((EyeGroup36A *)gEyeCatchActiveGroup)->records, displayList);
    gDPPipeSync((*displayList)++);
    gDPSetRenderMode((*displayList)++, 0x504240, 0);
    gDPSetCombineLERP((*displayList)++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE,
                      0);
    {
        s32 spA0;
        s32 sp9C;
        TextureDraw370 desc[2];
        var_s0 = gEyeCatchAnimationTracks;
        for (var_s4 = 0; var_s4 < ((EyeGroup36A *)gEyeCatchActiveGroup)->count; var_s4++) {
            desc[0].width = desc[0].height = -1;
            desc[0].s = desc[0].t = 0;
            animationIntegerTrackEvaluate((ValueTrack *)(void *)var_s0, &sp9C, gRuntimeFrameSeconds);
            desc[0].x = ((EyeGroup36A *)gEyeCatchActiveGroup)->x + sp9C;
            animationIntegerTrackEvaluate((ValueTrack *)(void *)(var_s0 + 0xC), &sp9C, gRuntimeFrameSeconds);
            desc[0].y = ((EyeGroup36A *)gEyeCatchActiveGroup)->y + sp9C;
            animationIntegerTrackEvaluate((ValueTrack *)(void *)(var_s0 + 0x18), &sp9C, gRuntimeFrameSeconds);
            desc[0].scaleX = (f32)sp9C * 0.1f;
            animationIntegerTrackEvaluate((ValueTrack *)(void *)(var_s0 + 0x24), &sp9C, gRuntimeFrameSeconds);
            desc[0].scaleY = (f32)sp9C * 0.1f;
            animationIntegerTrackEvaluate((ValueTrack *)(void *)(var_s0 + 0x30), &desc[0].texture, gRuntimeFrameSeconds);
            animationIntegerTrackEvaluate((ValueTrack *)(void *)(var_s0 + 0x3C), &spA0, gRuntimeFrameSeconds);
            if (spA0 < 0) {
                spA0 = 0;
            } else if (spA0 >= 0x100) {
                spA0 = 0xFF;
            }
            gDPSetPrimColor((*displayList)++, 0, 0, 0, 0, 0, spA0);
            desc[1].texture = -1;
            textureDrawRectangles(9, (TextureDrawC354 *)&desc, displayList);
            var_s0 += 0x48;
        }
    }
}

/// Foreground opacity on the zero-through-255 scale.
/// \note Original identifier: \c D_800F2608 (0x800F2608).
extern f32 gEyeCatchForegroundAlpha;

/// Draw the eye-catch foreground image and advance its fade-in when enabled.
/// \param gdl Display-list cursor receiving drawing commands and advanced past them.
/// \returns Nothing.
///
/// \note NONPORTABLE: the existing caller carries gdl in an s32 (32-bit pointers).
/// \note Original decompilation identifier: \c func_8003740C (0x8003740C).
void eyeCatchDrawFadingImage(Gfx **gdl) {
    TextureImage tex;

    if (gEyeCatchForegroundVisible != 0) {
        textureImageInitialize(&tex, gEyeCatchForegroundImage, 0, 0, 2, 0x108, 0x7E);
        gDPPipeSync((*gdl)++);
        gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
        gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, (s32)gEyeCatchForegroundAlpha);
        gDPSetCombineLERP((*gdl)++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0);
        textureDrawScaledRepeated(gdl, &tex, ((EyeGroup36A *)gEyeCatchActiveGroup)->width,
                                  ((EyeGroup36A *)gEyeCatchActiveGroup)->height, 2, 2, 0, 0, 0.5f, 0.5f, 0, 0);
        gEyeCatchForegroundAlpha += 127.5f * gRuntimeFrameSeconds;
        if (gEyeCatchForegroundAlpha < 0.0f) {
            gEyeCatchForegroundAlpha = 0.0f;
        } else if (gEyeCatchForegroundAlpha > 255.0f) {
            gEyeCatchForegroundAlpha = 255.0f;
        }
    }
}


/// Begin the screen reveal for an eye-catch interlude.
/// \returns One, selecting the playback stage.
/// \note Original decompilation identifier: \c func_800375D4 (0x800375D4).
s32 eyeCatchStartReveal(void) {
    cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 1.2000000476837158f);
    return 1;
}

/// Randomized sound choices for the eye-catch entry audio event.
/// \note Original identifier: \c D_8009C384 (0x8009C384).
extern s16 gEyeCatchEntrySoundChoices[];
/// Randomized sound choices for the eye-catch exit audio event.
/// \note Original identifier: \c D_8009C394 (0x8009C394).
extern s16 gEyeCatchExitSoundChoices[];
/// Primary controller newly pressed buttons; aliases the pressed word of the processed sample.
/// \note Original identifier: \c D_800AD544 (0x800AD544).
extern s32 gControllerPrimaryPressedButtons;

/// Advance eye-catch audio cues and begin the exit transition when finished or dismissed.
/// \returns Two when the exit transition starts, or minus ten to remain in the playback stage.
/// \note Matching compatibility: the volume setter is declared with an ignored integer result
/// here although its definition returns void. This target-specific declaration preserves IDO scheduling.
/// \note Original decompilation identifier: \c func_80037610 (0x80037610).
s32 eyeCatchUpdatePlayback(void) {
    s32 entrySound;
    s32 exitSound;

    if ((gEyeCatchForegroundVisible == 0) && (gEyeCatchStageSeconds > 1.5f)) {
        gEyeCatchForegroundVisible = 1;
        gEyeCatchForegroundAlpha = 0.0f;
    }
    switch (gEyeCatchAudioStage) {
    case 0:
        if (gEyeCatchStageSeconds > 0.0f) {
            audioChangeCurrentCue(4, 0.1f, 0.1f);
            gEyeCatchAudioStage += 1;
        }
        break;
    case 1:
        if (gEyeCatchStageSeconds > 2.5f) {
            if (((EyeGroup36A *)gEyeCatchActiveGroup)->flags != 0) {
                entrySound = audioPlaySoundEffect(0x6A);
            } else {
                entrySound = audioPlaySoundEffect(gEyeCatchEntrySoundChoices[randomBelow(8)]);
            }
            audioSetSoundEffectVolume(entrySound, 0xFF);
            gEyeCatchAudioStage += 1;
        }
        break;
    case 2:
        if (gEyeCatchStageSeconds > 3.77f) {
            if (((EyeGroup36A *)gEyeCatchActiveGroup)->flags != 0) {
                exitSound = audioPlaySoundEffect(gEyeCatchExitSoundChoices[randomBelow(8)]);
            } else {
                exitSound = audioPlaySoundEffect(0x66);
            }
            audioSetSoundEffectVolume(exitSound, 0xFF);
            gEyeCatchAudioStage += 1;
        }
        break;
    }
    if ((gControllerPrimaryPressedButtons != 0) || gEyeCatchStageSeconds > 7.5f) {
        cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.8f);
        audioStopCategories(AUDIO_CATEGORY_ALL, 0.8f);
        return 2;
    }
    return -10;
}

/// Remaining duration of the primary camera secondary overlay transition.
/// \note Original identifier: \c D_800ABDCC (0x800ABDCC).
extern f32 gPrimarySecondaryOverlayRemainingSeconds;

/// Wait for the eye-catch exit fade to finish.
/// \returns Three when the screen fade is complete, or minus ten to remain in this stage.
/// \note Original decompilation identifier: \c func_80037854 (0x80037854).
s32 eyeCatchWaitForExitFade(void) {
    if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
        return 3;
    }
    return -0xA;
}

/// Release the eye-catch interlude and signal completion.
/// \returns Minus twenty, the interlude completion signal.
/// \note Original decompilation identifier: \c func_80037884 (0x80037884).
s32 eyeCatchFinish(void) {
    eyeCatchRelease();
    return -0x14;
}

void eyeCatchDrawLayers(Gfx **);
void eyeCatchDrawFadingImage(Gfx **); /* extern */

/// Draw the active eye-catch interlude during its visible stages.
/// \param displayList Display-list cursor receiving drawing commands and advanced past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800378A8 (0x800378A8).
void eyeCatchDraw(Gfx **displayList) {
    if ((gEyeCatchActiveGroup != 0) && (gEyeCatchStage >= 0) && (gEyeCatchStage < 3)) {
        eyeCatchDrawLayers(displayList);
        eyeCatchDrawFadingImage(displayList);
    }
}

/// Stage callbacks returning the next stage or the retained playback/completion signals.
/// \note Original identifier: \c D_8009C3B4 (0x8009C3B4).
extern s32 (*gEyeCatchStageHandlers[])(void);

/// Advance the active eye-catch stage and report when the interlude completes.
/// Requires an interlude prepared by eyeCatchBegin; stop updating after completion.
/// \returns One when the interlude completes, otherwise zero.
/// \note Original decompilation identifier: \c func_800378F8 (0x800378F8).
s32 eyeCatchUpdate(void) {
    s32 next;

    gEyeCatchStageSeconds += gRuntimeFrameSeconds;
    next = gEyeCatchStageHandlers[gEyeCatchStage]();
    if (next != -10 && next != gEyeCatchStage) {
        gEyeCatchStage = next;
        gEyeCatchStageSeconds = 0.0f;
    }
    if (gEyeCatchStage == -20) {
        gEyeCatchStage = 0;
        return 1;
    }
    return 0;
}


/// Choose an eye-catch group using the current selection weights.
/// \returns The selected group index from zero through five.
///
/// \note NONPORTABLE: preserve the observed 0x28-byte frame; the two upper words have no identified
/// value and are never read or written by the ROM.
/// \note Original decompilation identifier: \c func_800379A0 (0x800379A0).
s32 eyeCatchChooseGroup(void) {
    s32 stackPad[2];
    f32 level;
    s32 column;
    s32 total;
    s32 i;
    s32 choice;

    level = (f32)progressGetValue(2);
    if (level <= 12.0f) {
        column = 0;
    } else {
        column = 1;
    }
    total = 0;
    for (i = 0; i < 6; i++) {
        total += ((u8 *)&gEyeCatchGroups[i])[column + 2];
    }
    choice = randomBelow(total);
    for (i = 0; i < 6; i++) {
        choice -= ((u8 *)&gEyeCatchGroups[i])[column + 2];
        if (choice < 0) {
            return i;
        }
    }
    return 1;
}
