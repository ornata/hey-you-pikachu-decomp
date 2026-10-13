#include "text_image_configurations.h"
#include "text_window.h"
#include "message_bar.h"
#include "progress_value.h"
#include "action_script_playback.h"
#include "object_distance.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "text_image_create.h"

#include "string_bytes.h"
#include "text_draw.h"
#include "object_lookup.h"
#include "heap_alloc.h"
#include "object_types.h"
#include "text_image.h"
#include "heap_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata and the dead branch still ends a scheduling block. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/// Copied display text with its requested presentation settings.
typedef struct QueuedMessage {
    char text[60]; ///< Owned encoded text including its terminator.
    f32 duration; ///< Requested display duration in seconds.
    s32 style;    ///< Font-style index.
} QueuedMessage;

/// Retained message queue, transition timers and owned image slots.
typedef struct MessageBarState {
    f32 elapsed; ///< Time since the current message became active.
    f32 duration; ///< Current message display duration.
    s32 queueWriteIndex; ///< Next queue slot available for insertion.
    s32 queueReadIndex; ///< Next queued message awaiting activation.
    f32 transitionRemaining; ///< Remaining display-transition time.
    f32 scrollAngle; ///< Phase in degrees for wide-text scrolling.
    QueuedMessage queued[16]; ///< Owned pending-message ring.
    s32 displayImageIndex; ///< Image slot currently displayed.
    s32 workImageIndex; ///< Image slot available for preparation.
    TextImageHandle handles[3]; ///< Owned text images used during transitions.
    u8 unknown46C[0x3C]; ///< Retained storage with no established role.
} MessageBarState;
typedef char QueuedMessageSizeCheck[(sizeof(QueuedMessage) == 0x44) ? 1 : -1];
typedef char MessageBarStateSizeCheck[(sizeof(MessageBarState) == 0x4A8) ? 1 : -1];
#define MESSAGE_OFFSET(field) ((u32) & (((MessageBarState *)0)->field))
typedef char MessageQueueOffsetCheck[(MESSAGE_OFFSET(queued) == 0x18) ? 1 : -1];
typedef char MessageDisplaySlotOffsetCheck[(MESSAGE_OFFSET(displayImageIndex) == 0x458) ? 1 : -1];
typedef char MessageWorkSlotOffsetCheck[(MESSAGE_OFFSET(workImageIndex) == 0x45C) ? 1 : -1];
typedef char MessageImagesOffsetCheck[(MESSAGE_OFFSET(handles) == 0x460) ? 1 : -1];
#undef MESSAGE_OFFSET
/// Shared queued-message, timing, scrolling and text-image state for the message bar.
/// \note Original identifier: \c D_800F06C0 (0x800F06C0).
extern MessageBarState gMessageBarState;
void cameraUpdateOverlayColors(s32, s32);
void cameraDrawSecondaryOverlay(s32);
void objectDrawHierarchyGroup(s32, s32);
void presentationDrawAreaLabel(Gfx **);
void presentationDrawCardDecorations(Gfx **);
void presentationDrawCurrentCardPanels(Gfx **);
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern s32 gPikachuActor;
/// Friendship messages indexed by the cleared chapter.
/// \note Original identifier: \c D_800999F0 (0x800999F0).
extern s32 gChapterFriendshipMessages[];
/// Separate symbol view of the primary player friendship-title level.
/// \note Original identifier: \c D_800EF796 (0x800EF796).
extern u8 gPlayerFriendshipLevel;
/// Separate symbol view of the last awarded primary player friendship milestone, or minus one when none awaits presentation.
/// \note Original identifier: \c D_800EF798 (0x800EF798).
extern s32 gPlayerFriendshipMilestone;

void _bzero(void *, s32); /* extern */
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

/// Initialize an object whose rotation responds to voice activity.
/// \param object Object receiving the initialized spinner state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80027430 (0x80027430).
void voiceReactiveSpinnerInitialize(u8 *object) {
    s32 sp1C;
    s32 allocationAddress;

    allocationAddress = (s32)heapAllocLocal((HpHdr *)&gGameHeap, 4, 0x27);
    sp1C = allocationAddress;
    _bzero((void *)allocationAddress, 4);
    ((RuntimeObjectPrefix *)object)->state.address = sp1C;
}

typedef struct {
    u8 raw[0xE0];
} Rec800ABCF0;
/// Camera storage used for view transforms, viewports and color overlays.
/// \note Original identifier: \c D_800ABCF0 (0x800ABCF0).
extern Rec800ABCF0 gRenderCameras[];
/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;
/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
s32 voiceRecognitionGetCommandStatus(void);
f32 mathMapRangeClamped(f32, f32, f32, f32, f32);

/// Update an object spinner in response to voice activity and viewer distance.
/// \param object Spinner object with initialized rotation-speed state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002747C (0x8002747C).
void voiceReactiveSpinnerUpdate(u8 *object) {
    f32 *speed = *(f32 **)(object + 0x54);
    volatile f32 *dt = &gRuntimeFrameSeconds;

    if (voiceRecognitionGetCommandStatus() == 5) {
        *speed += mathMapRangeClamped(0.0f, 2000.0f, 40.0f, 0.0f,
                                      objectGetApproxDistanceToPoint(object, (Vec3f *)gRenderCameras[gRenderCameraIndex].raw)) *
                  *dt;
        if (*speed > 600.0f) {
            *speed = 600.0f;
        }
    }
    if (*speed > 0.0f) {
        *speed -= 300.0f * *dt;
        if (*speed > 0.0f) {
            *(f32 *)(object + 0x10) += *speed * *dt;
            while (*(f32 *)(object + 0x10) < 0.0f) {
                *(f32 *)(object + 0x10) += 360.0f;
            }
            while (*(f32 *)(object + 0x10) >= 360.0f) {
                *(f32 *)(object + 0x10) -= 360.0f;
            }
        }
    }
}


/// Prepare a text image for a queued display message.
/// \param index Text-image slot to replace, from 0 through 2.
/// \param msg Message containing null-terminated text and a valid font style to render.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80027610 (0x80027610).
void messageBarPrepareImage(s32 index, QueuedMessage msg) {
    TextImage **slot = &gMessageBarState.handles[index].image;

    if (*slot != 0) {
        textImageDestroy((TextImage *)*slot);
    }
    TRACE("_setSprite:(%s) idx:%d\n", msg.text, index);
    textImageCreateFromText((TextImage **)(slot), (u8 *)((s8 *)msg.text),
                            &gTextImageConfigurations[msg.style]);
}

/// Retained displayed message text used to coalesce repeated message requests.
/// \note Original identifier: \c D_800F0B2C (0x800F0B2C).
extern u8 gMessageBarCurrentText[];

/// Activate the next queued message after its text image has been prepared.
/// Requires a queued message with a prepared image.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002767C (0x8002767C).
void messageBarActivateNext(void) {
    u8 *queueCursor = (u8 *)&gMessageBarState + gMessageBarState.queueReadIndex * 0x44;

    TRACE("_next: getIdx:%d setIdx:%d drawIdx:%d workIdx:%d\n", gMessageBarState.queueReadIndex, gMessageBarState.queueWriteIndex,
          gMessageBarState.displayImageIndex, gMessageBarState.workImageIndex);

    gMessageBarState.duration = *(f32 *)((u8 *)&gMessageBarState + gMessageBarState.queueReadIndex * 0x44 + 0x54);
    gMessageBarState.elapsed = 0.0f;
    gMessageBarState.scrollAngle = 0.0f;
    stringCopy(gMessageBarCurrentText, queueCursor + 0x18);
    if (++gMessageBarState.queueReadIndex >= 0x10) {
        gMessageBarState.queueReadIndex = 0;
    }
    if (++gMessageBarState.displayImageIndex >= 3) {
        gMessageBarState.displayImageIndex = 0;
    }
    gMessageBarState.transitionRemaining = 0.0f;
}

/// Advance the message bar display timer and queued-message transitions.
/// \returns 1 while a message or transition remains active, or 0 when the display is idle.
/// \note Original decompilation identifier: \c func_80027710 (0x80027710).
s32 messageBarUpdate(void) {
    s32 pending;
    f32 interval;

    gMessageBarState.elapsed += gRuntimeFrameSeconds;
    if (gMessageBarState.queueReadIndex == gMessageBarState.queueWriteIndex) {
        if (gMessageBarState.elapsed < gMessageBarState.duration) {
            return 1;
        }
        gMessageBarState.duration = 0.0f;
        gMessageBarState.elapsed = 0.0f;
        return 0;
    }
    pending = gMessageBarState.queueWriteIndex - gMessageBarState.queueReadIndex;
    if (gMessageBarState.transitionRemaining > 0.0f) {
        gMessageBarState.transitionRemaining -= gRuntimeFrameSeconds;
        if (gMessageBarState.transitionRemaining <= 0.0f) {
            messageBarActivateNext();
        }
        return 1;
    }
    if (pending < 0) {
        pending += 16;
    }
    interval = mathMapRangeClamped(1.0f, 0.05f, 4.0f, 0.05f, pending);
    if ((gMessageBarState.elapsed < gMessageBarState.duration) && (gMessageBarState.elapsed < interval)) {
        return 1;
    }
    messageBarPrepareImage(gMessageBarState.workImageIndex,
                           gMessageBarState.queued[gMessageBarState.queueReadIndex]);
    if (++gMessageBarState.workImageIndex >= 3) {
        gMessageBarState.workImageIndex = 0;
    }
    if (gMessageBarState.duration == 0.0f) {
        messageBarActivateNext();
        return 1;
    }
    gMessageBarState.transitionRemaining = 0.25f;
    return 1;
}

/// Separate view of the message-bar image slot prepared for the next transition.
/// \note Original identifier: \c D_800F0B1C (0x800F0B1C).
extern s32 gMessageBarWorkImageIndex;

/// Reset message-bar state to an empty display without releasing existing images.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800278F4 (0x800278F4).
void messageBarInitialize(void) {
    _bzero(&gMessageBarState, sizeof(MessageBarState));
    gMessageBarWorkImageIndex = 1;
}

/// Separate view of the three retained message-bar text-image handles.
/// \note Original identifier: \c D_800F0B20 (0x800F0B20).
extern TextImage *gMessageBarImageHandles[];

/// Reset message-bar display state and release retained text images.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80027924 (0x80027924).
void messageBarResetAndReleaseImages(void) {
    s32 i;
    TextImage **p;

    for (i = 0, p = gMessageBarImageHandles; i < 2; i++, p++) {
        if (*p != 0) {
            textImageDestroy((TextImage *)*p);
        }
    }
    messageBarInitialize();
}

s32 memoryCompareBytes(u8 *, u8 *, s32);
s32 strlen(const char *);

/// Queue a display message or refresh the matching displayed message timer.
/// \param style Valid font-style index used to render the text.
/// \param duration Display duration in seconds.
/// \param text Null-terminated message containing at most 59 non-terminator bytes; when no message is queued, a prefix match with displayed text refreshes its timer.
/// \returns 1 when queued or refreshed, or 0 for empty text or a full queue.
/// \note Original decompilation identifier: \c func_80027988 (0x80027988).
s32 messageBarEnqueue(s32 style, f32 duration, u8 *text) {
    QueuedMessage *entry;
    s32 next;

    TRACE("elEchoMessage(%d, %f, %s)\n", style, duration, text);
    if (strlen((char *)text) == 0) {
        return 0;
    }
    if (gMessageBarState.queueReadIndex == gMessageBarState.queueWriteIndex &&
        memoryCompareBytes(gMessageBarCurrentText, text, strlen((char *)text)) == 0) {
        gMessageBarState.elapsed = 0.0f;
        gMessageBarState.duration = duration;
        return 1;
    }
    next = gMessageBarState.queueWriteIndex + 1;
    if (next >= 16) {
        next = 0;
    }
    TRACE("setIdx:%d nextIdx:%d\n", gMessageBarState.queueWriteIndex, next);
    if (next == gMessageBarState.queueReadIndex) {
        return 0;
    }
    entry = &gMessageBarState.queued[gMessageBarState.queueWriteIndex];
    stringCopy((u8 *)entry->text, text);
    entry->duration = duration;
    entry->style = style;
    gMessageBarState.queueWriteIndex = next;
    return 1;
}

/// Clear queued messages and end the active message display.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80027A88 (0x80027A88).
void messageBarClear(void) {
    gMessageBarState.duration = 0.0f;
    gMessageBarState.elapsed = gMessageBarState.duration;
    gMessageBarState.transitionRemaining = 0.0f;
    gMessageBarState.queueReadIndex = gMessageBarState.queueWriteIndex;
    gMessageBarState.workImageIndex = gMessageBarState.displayImageIndex;
    gMessageBarState.workImageIndex++;
    if (gMessageBarState.workImageIndex >= 3) {
        gMessageBarState.workImageIndex = 0;
    }
}

f32 mathCosDegrees(f32);
/// Write cursor for the active graphics command list; also marks the camera-storage end.
/// \note Original identifier: \c D_800ABDD0 (0x800ABDD0).
extern Gfx *gRenderDisplayListCursor;

/// Advance and draw the message bar with its current transition and scrolling state.
/// \returns 1 when a message is drawn, or 0 when the display is idle.
/// \note Original decompilation identifier: \c func_80027AD0 (0x80027AD0).
s32 messageBarUpdateAndDraw(void) {
    f32 fadeTime;
    s32 slot;
    f32 scroll;
    f32 transition = 1.0f;
    f32 opacity = 1.0f;
    f32 textOpacity = 1.0f;
    s32 textTop = 22;
    s32 offset;
    s32 edge;

    if (messageBarUpdate() == 0) {
        return 0;
    }
    slot = gMessageBarState.displayImageIndex;
    if (gMessageBarState.elapsed >= 2.8f) {
        fadeTime = gMessageBarState.duration - 2.8f;
        if (fadeTime > 0.1f) {
            textOpacity = opacity = (gMessageBarState.duration - gMessageBarState.elapsed) / fadeTime;
        } else {
            textOpacity = opacity = 0.0f;
        }
    }
    if (opacity < 0.0f) {
        opacity = 0.0f;
    } else if (opacity > 1.0f) {
        opacity = 1.0f;
    }
    if (textOpacity < 0.1f) {
        textOpacity = 0.1f;
    } else if (textOpacity > 1.0f) {
        textOpacity = 1.0f;
    }
    if (gMessageBarState.transitionRemaining != 0.0f) {
        transition = gMessageBarState.transitionRemaining * 4.0f;
        if (transition < 0.0f) {
            transition = 0.0f;
        } else if (transition > 1.0f) {
            transition = 1.0f;
        }
        opacity = mathMapRangeClamped(0.0f, 1.0f, 1.0f, opacity, transition);
    }
    gDPPipeSync(gRenderDisplayListCursor++);
    gDPSetCycleType(gRenderDisplayListCursor++, G_CYC_1CYCLE);
    gDPSetRenderMode(gRenderDisplayListCursor++, 0, 0x504240);
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 10, 10, 10, (u32)(210.0f * opacity));
    gDPSetCombineMode(gRenderDisplayListCursor++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPFillRectangle(gRenderDisplayListCursor++, 20, 20, 299, 40);
    textDrawBegin(gRenderDisplayListCursor);
    textDrawReset();
    textDrawSetPalette(gDefaultTextPalette);
    textDrawSetColor(255, 255, 255, 255);
    textDrawGetDisplayListCursor(&gRenderDisplayListCursor);
    gSPTexture(gRenderDisplayListCursor++, 0x7FFF, 0, 0, 0, G_ON);
    gMoveWd(gRenderDisplayListCursor++, G_MW_FOG, 0x10, 0x7FFF);
    gDPSetRenderMode(gRenderDisplayListCursor++, 0, 0x504A50);
    gDPSetCombineLERP(gRenderDisplayListCursor++, 0, 0, 0, TEXEL0, PRIMITIVE, 0, TEXEL0, 0, 0, 0, 0, TEXEL0, PRIMITIVE,
                      0, TEXEL0, 0);
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 255, 255, 255, (u32)(255.0f * textOpacity));
    if (gMessageBarState.handles[slot].image->width < 272) {
        offset = 0;
    } else {
        scroll = -mathCosDegrees(gMessageBarState.scrollAngle) * 0.7f + 0.5f;
        if (scroll < 0.0f) {
            scroll = 0.0f;
        } else if (scroll > 1.0f) {
            scroll = 1.0f;
        }
        offset = (gMessageBarState.handles[slot].image->width - 271) * scroll;
        if (gMessageBarState.transitionRemaining == 0.0f) {
            gMessageBarState.scrollAngle += gRuntimeFrameSeconds * 75.0f;
            while (gMessageBarState.scrollAngle < 0.0f) {
                gMessageBarState.scrollAngle += 360.0f;
            }
            while (gMessageBarState.scrollAngle >= 360.0f) {
                gMessageBarState.scrollAngle -= 360.0f;
            }
            TRACE("drawIdx:%d\n", slot);
        }
    }
    edge = (s32)(271.0f * transition) + 24;
    textImageDrawRectangle(&gRenderDisplayListCursor, gMessageBarState.handles[slot].image, 24, textTop, edge, 38,
                           offset, 0, 1.0f, 1.0f);
    slot++;
    if (gMessageBarState.transitionRemaining == 0.0f) {
        return 1;
    }
    if (slot >= 3) {
        slot = 0;
    }
    gDPPipeSync(gRenderDisplayListCursor++);
    gDPSetRenderMode(gRenderDisplayListCursor++, 0, 0x504A50);
    gDPSetCombineLERP(gRenderDisplayListCursor++, 0, 0, 0, TEXEL0, PRIMITIVE, 0, TEXEL0, 0, 0, 0, 0, TEXEL0, PRIMITIVE,
                      0, TEXEL0, 0);
    gDPSetPrimColor(gRenderDisplayListCursor++, 0, 0, 255, 255, 255, (s32)((1.0f - transition) * 255.0f));
    textImageDrawRectangle(&gRenderDisplayListCursor, gMessageBarState.handles[slot].image, edge, textTop, 295, 38, 0,
                           0, 1.0f, 1.0f);
    gSPTexture(gRenderDisplayListCursor++, 0x7FFF, 0, 0, 0, G_OFF);
    gMoveWd(gRenderDisplayListCursor++, G_MW_FOG, 0x10, 0x7FFF);
    return 1;
}

/// Interior availability progress-flag view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125AB2 (0x80125AB2).
extern u8 gItemKindAvailabilityFlagEntries[];

/// Format string used to create the collection title text image.
/// \note Original identifier: \c D_800A558C (0x800A558C).
extern s8 gCollectionTitleTextFormat;
/// Owned text image for the collection title.
/// \note Original identifier: \c D_800F0BE8 (0x800F0BE8).
extern s32 gCollectionPresentationTitleImage;

/// Format string used to create the collection score text image.
/// \note Original identifier: \c D_800A559C (0x800A559C).
extern s8 gCollectionScoreTextFormat;
/// Accumulated collection value awarded to the player at the end of results.
/// \note Original identifier: \c D_800F0BD8 (0x800F0BD8).
extern s32 gCollectionPresentationScore;

/// Live item displayed during collection results, or zero when none is shown.
/// \note Original identifier: \c D_800F0BDC (0x800F0BDC).
extern s32 gCollectionPresentationItem;

/// Interior collection-value view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125AAC (0x80125AAC).
extern s32 gItemKindCollectionValueEntries[];
