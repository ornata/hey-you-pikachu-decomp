#include "progress_field_ids.h"
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
/// Resident eye-catch layout, selection weights, borrowed animation keys and backdrop colors.
typedef struct EyeCatchGroup {
    u8 unknown000[2];             ///< Bytes without an established consumer or meaning.
    u8 selectionWeights[2];       ///< Selection weights for progress at most twelve and greater than twelve.
    TextureRectangle *rectangles; ///< Borrowed negative-texture-terminated list, adjusted once at startup.
    s16 originX, originY;         ///< Base screen position for static and animated rectangles.
    s16 alternateSoundSet;  ///< Nonzero selects fixed entry/random exit sounds instead of random entry/fixed exit.
    s16 animatedLayerCount; ///< Number of animated layers and six-track key sets.
    ValueKey *(*animationKeys)[6];         ///< Borrowed key sets for X, Y, X/Y scale, texture and opacity.
    s16 foregroundWidth, foregroundHeight; ///< Foreground image drawing dimensions.
    u8 cornerColors[4][3];                 ///< Backdrop vertex RGB colors in corner order.
} EyeCatchGroup;

typedef char EyeCatchGroupSizeCheck[(sizeof(EyeCatchGroup) == 0x24) ? 1 : -1];
typedef char EyeCatchWeightsOffsetCheck[((u32) & ((EyeCatchGroup *)0)->selectionWeights == 2) ? 1 : -1];
typedef char EyeCatchRectanglesOffsetCheck[((u32) & ((EyeCatchGroup *)0)->rectangles == 4) ? 1 : -1];
typedef char EyeCatchKeysOffsetCheck[((u32) & ((EyeCatchGroup *)0)->animationKeys == 0x10) ? 1 : -1];
typedef char EyeCatchColorsOffsetCheck[((u32) & ((EyeCatchGroup *)0)->cornerColors == 0x18) ? 1 : -1];
/// Six eye-catch group descriptors containing artwork, layout, animation and selection weights.
/// \note Original identifier: \c D_8009C2AC (0x8009C2AC).
extern EyeCatchGroup gEyeCatchGroups[6];

#include "eye_catch.h"
/// Prepare eye-catch layouts and reset interlude state once at startup.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80036A60 (0x80036A60).
void eyeCatchInitialize(void) {
    s32 i;
    EyeCatchGroup *g;
    TextureRectangle *r;

    for (i = 0, g = gEyeCatchGroups; i < 6; i++, g++) {
        r = g->rectangles;
        if (r->texture >= 0) {
            do {
                r->x += g->originX;
                r->y += g->originY;
                r++;
            } while (r->texture >= 0);
        }
    }
    gEyeCatchStageSeconds = 0.0F;
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

#define EYE_GROUP ((EyeCatchGroup *)gEyeCatchActiveGroup)

/// Six animation channels for each eye-catch layer.
/// Six animation cursors owned by one animated eye-catch layer.
typedef ValueTrack EyeCatchLayerTracks[6];
typedef char EyeCatchLayerTracksSizeCheck[(sizeof(EyeCatchLayerTracks) == 0x48) ? 1 : -1];
#define EYE_TRACKS(group) (*(EyeCatchLayerTracks *)((group) * sizeof(EyeCatchLayerTracks) + gEyeCatchAnimationTracks))

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
    count = EYE_GROUP->animatedLayerCount;
    if (count != 0) {
        gEyeCatchAnimationTracks = (s32)heapAllocLocal((&gGameHeap), count * sizeof(EyeCatchLayerTracks), 0x508);
        for (group = 0; group < EYE_GROUP->animatedLayerCount; group++) {
            animationIntegerTrackInitialize(&EYE_TRACKS(group)[0], EYE_GROUP->animationKeys[group][0]);
            animationIntegerTrackInitialize(&EYE_TRACKS(group)[1], EYE_GROUP->animationKeys[group][1]);
            animationIntegerTrackInitialize(&EYE_TRACKS(group)[2], EYE_GROUP->animationKeys[group][2]);
            animationIntegerTrackInitialize(&EYE_TRACKS(group)[3], EYE_GROUP->animationKeys[group][3]);
            animationIntegerTrackInitialize(&EYE_TRACKS(group)[4], EYE_GROUP->animationKeys[group][4]);
            animationIntegerTrackInitialize(&EYE_TRACKS(group)[5], EYE_GROUP->animationKeys[group][5]);
        }
    }
    gEyeCatchForegroundImage = (s32)assetAllocateAndLoad(&gEyeCatchForegroundAssetDescriptor);
    gEyeCatchBackdropVertices = (s32)heapAllocLocal((&gGameHeap), 4 * sizeof(Vtx), 0x521);
    for (corner = 0; corner < 4; corner++) {
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.ob[0] = gEyeCatchBackdropCorners[corner][0];
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.ob[1] = gEyeCatchBackdropCorners[corner][1];
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.ob[2] = 10;
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.flag = 0;
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.tc[0] = 0;
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.tc[1] = 0;
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.cn[0] = EYE_GROUP->cornerColors[corner][0];
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.cn[1] = EYE_GROUP->cornerColors[corner][1];
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.cn[2] = EYE_GROUP->cornerColors[corner][2];
        ((Vtx *)gEyeCatchBackdropVertices)[corner].v.cn[3] = 0xFF;
    }
    gEyeCatchProjectionMatrix = (s32)heapAllocLocal((&gGameHeap), sizeof(Mtx), 0x530);
    guOrtho((Mtx *)gEyeCatchProjectionMatrix, 0.0F, 320.0F, 0.0F, 240.0F, -1.0F, -1000.0F, 1.0F);
    gEyeCatchModelViewMatrix = (s32)heapAllocLocal((&gGameHeap), sizeof(Mtx), 0x537);
    guRotate((Mtx *)gEyeCatchModelViewMatrix, 0.0F, 0.0F, 0.0F, 1.0F);
    cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.0F);
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
    gEyeCatchStageSeconds = 0.0F;
    gEyeCatchStage = 0;
    gEyeCatchForegroundVisible = 0;
    gEyeCatchAudioStage = 0;
    gEyeCatchSelectedGroup = -1;
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Advance and draw the active eye-catch backdrop and animated image layers.
/// Requires resources prepared by eyeCatchBegin.
/// \param displayList Display-list cursor receiving drawing commands and advanced past them.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80037034 (0x80037034).
void eyeCatchDrawLayers(Gfx **displayList) {
    s32 animationAddress;
    s32 layerIndex;

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
    textureDrawRectangles(9, ((EyeCatchGroup *)gEyeCatchActiveGroup)->rectangles, displayList);
    gDPPipeSync((*displayList)++);
    gDPSetRenderMode((*displayList)++, 0x504240, 0);
    gDPSetCombineLERP((*displayList)++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE,
                      0);
    {
        s32 spA0;
        s32 sp9C;
        TextureRectangle desc[2];
        animationAddress = gEyeCatchAnimationTracks;
        for (layerIndex = 0; layerIndex < ((EyeCatchGroup *)gEyeCatchActiveGroup)->animatedLayerCount; layerIndex++) {
            desc[0].width = desc[0].height = -1;
            desc[0].s = desc[0].t = 0;
            animationIntegerTrackEvaluate((ValueTrack *)(void *)animationAddress, &sp9C, gRuntimeFrameSeconds);
            desc[0].x = ((EyeCatchGroup *)gEyeCatchActiveGroup)->originX + sp9C;
            animationIntegerTrackEvaluate((ValueTrack *)(void *)(animationAddress + 0xC), &sp9C, gRuntimeFrameSeconds);
            desc[0].y = ((EyeCatchGroup *)gEyeCatchActiveGroup)->originY + sp9C;
            animationIntegerTrackEvaluate((ValueTrack *)(void *)(animationAddress + 0x18), &sp9C, gRuntimeFrameSeconds);
            desc[0].scaleX = (f32)sp9C * 0.1F;
            animationIntegerTrackEvaluate((ValueTrack *)(void *)(animationAddress + 0x24), &sp9C, gRuntimeFrameSeconds);
            desc[0].scaleY = (f32)sp9C * 0.1F;
            animationIntegerTrackEvaluate((ValueTrack *)(void *)(animationAddress + 0x30), &desc[0].texture,
                                          gRuntimeFrameSeconds);
            animationIntegerTrackEvaluate((ValueTrack *)(void *)(animationAddress + 0x3C), &spA0, gRuntimeFrameSeconds);
            if (spA0 < 0) {
                spA0 = 0;
            } else if (spA0 >= 0x100) {
                spA0 = 0xFF;
            }
            gDPSetPrimColor((*displayList)++, 0, 0, 0, 0, 0, spA0);
            desc[1].texture = -1;
            textureDrawRectangles(9, desc, displayList);
            animationAddress += 0x48;
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
        textureDrawScaledRepeated(gdl, &tex, ((EyeCatchGroup *)gEyeCatchActiveGroup)->foregroundWidth,
                                  ((EyeCatchGroup *)gEyeCatchActiveGroup)->foregroundHeight, 2, 2, 0, 0, 0.5F, 0.5F, 0,
                                  0);
        gEyeCatchForegroundAlpha += 127.5F * gRuntimeFrameSeconds;
        if (gEyeCatchForegroundAlpha < 0.0F) {
            gEyeCatchForegroundAlpha = 0.0F;
        } else if (gEyeCatchForegroundAlpha > 255.0F) {
            gEyeCatchForegroundAlpha = 255.0F;
        }
    }
}

/// Control signals distinct from the four nonnegative eye-catch stage indices.
typedef enum EyeCatchStageSignal {
    EYE_CATCH_KEEP_STAGE = -10, ///< Continue the current stage without restarting its clock.
    EYE_CATCH_COMPLETE = -20    ///< Finish the interlude.
} EyeCatchStageSignal;

/// Advance the active eye-catch interlude stage.
/// \pre Interlude resources and stage state are initialized and remain live for the call.
/// \returns A next-stage index from zero through three, or an EyeCatchStageSignal.
typedef s32 (*EyeCatchStageHandler)(void);

/// Begin the screen reveal for an eye-catch interlude.
/// \returns One, selecting the playback stage.
/// \note Original decompilation identifier: \c func_800375D4 (0x800375D4).
s32 eyeCatchStartReveal(void) {
    cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 1.2000000476837158F);
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

    if ((gEyeCatchForegroundVisible == 0) && (gEyeCatchStageSeconds > 1.5F)) {
        gEyeCatchForegroundVisible = 1;
        gEyeCatchForegroundAlpha = 0.0F;
    }
    switch (gEyeCatchAudioStage) {
    case 0:
        if (gEyeCatchStageSeconds > 0.0F) {
            audioChangeCurrentCue(4, 0.1F, 0.1F);
            gEyeCatchAudioStage += 1;
        }
        break;
    case 1:
        if (gEyeCatchStageSeconds > 2.5F) {
            if (((EyeCatchGroup *)gEyeCatchActiveGroup)->alternateSoundSet != 0) {
                entrySound = audioPlaySoundEffect(0x6A);
            } else {
                entrySound = audioPlaySoundEffect(gEyeCatchEntrySoundChoices[randomBelow(8)]);
            }
            audioSetSoundEffectVolume(entrySound, 0xFF);
            gEyeCatchAudioStage += 1;
        }
        break;
    case 2:
        if (gEyeCatchStageSeconds > 3.77F) {
            if (((EyeCatchGroup *)gEyeCatchActiveGroup)->alternateSoundSet != 0) {
                exitSound = audioPlaySoundEffect(gEyeCatchExitSoundChoices[randomBelow(8)]);
            } else {
                exitSound = audioPlaySoundEffect(0x66);
            }
            audioSetSoundEffectVolume(exitSound, 0xFF);
            gEyeCatchAudioStage += 1;
        }
        break;
    }
    if ((gControllerPrimaryPressedButtons != 0) || gEyeCatchStageSeconds > 7.5F) {
        cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFF, 0.8F);
        audioStopCategories(AUDIO_CATEGORY_ALL, 0.8F);
        return 2;
    }
    return EYE_CATCH_KEEP_STAGE;
}

/// Remaining duration of the primary camera secondary overlay transition.
/// \note Original identifier: \c D_800ABDCC (0x800ABDCC).
extern f32 gPrimarySecondaryOverlayRemainingSeconds;

/// Wait for the eye-catch exit fade to finish.
/// \returns Three when the screen fade is complete, or minus ten to remain in this stage.
/// \note Original decompilation identifier: \c func_80037854 (0x80037854).
s32 eyeCatchWaitForExitFade(void) {
    if (gPrimarySecondaryOverlayRemainingSeconds == 0.0F) {
        return 3;
    }
    return EYE_CATCH_KEEP_STAGE;
}

/// Release the eye-catch interlude and signal completion.
/// \returns Minus twenty, the interlude completion signal.
/// \note Original decompilation identifier: \c func_80037884 (0x80037884).
s32 eyeCatchFinish(void) {
    eyeCatchRelease();
    return EYE_CATCH_COMPLETE;
}

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
extern EyeCatchStageHandler gEyeCatchStageHandlers[];

/// Advance the active eye-catch stage and report when the interlude completes.
/// Requires an interlude prepared by eyeCatchBegin; stop updating after completion.
/// \returns One when the interlude completes, otherwise zero.
/// \note Original decompilation identifier: \c func_800378F8 (0x800378F8).
s32 eyeCatchUpdate(void) {
    s32 next;

    gEyeCatchStageSeconds += gRuntimeFrameSeconds;
    next = gEyeCatchStageHandlers[gEyeCatchStage]();
    if (next != EYE_CATCH_KEEP_STAGE && next != gEyeCatchStage) {
        gEyeCatchStage = next;
        gEyeCatchStageSeconds = 0.0F;
    }
    if (gEyeCatchStage == EYE_CATCH_COMPLETE) {
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

    level = (f32)progressGetValue(PROGRESS_FIELD_FRIENDSHIP_LEVEL);
    if (level <= 12.0F) {
        column = 0;
    } else {
        column = 1;
    }
    total = 0;
    for (i = 0; i < 6; i++) {
        total += gEyeCatchGroups[i].selectionWeights[column];
    }
    choice = randomBelow(total);
    for (i = 0; i < 6; i++) {
        choice -= gEyeCatchGroups[i].selectionWeights[column];
        if (choice < 0) {
            return i;
        }
    }
    return 1;
}
