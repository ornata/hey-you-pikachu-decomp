#include "game_heap.h"
#include "texture_image.h"
#include "math_scalar.h"
#include "pikachu_actor.h"
#include "camera_storage.h"
#include "vector_metrics.h"
#include "microphone_requests.h"
#include "messenger.h"
#include "camera_types.h"
#include "object_types.h"
#include "item_types.h"
#include "text_image_configurations.h"
#include "interaction.h"
#include "object_target_search.h"
#include "heap_free.h"
/// \file
/// Voice-command translation and scrolling text-panel support.
/// \ingroup VoiceInteraction
///
/// See docs/VOICE_AND_SCRIPT_VM.md for the subsystem walkthrough.

/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "text_image_create.h"
#include "actor_parts.h"
#include "text_draw.h"
#include "object_lookup.h"
#include "text_panel.h"
#include "heap_alloc.h"
#include "voice_commands.h"
void _bzero(s32, s32);
/// Empty text used to reset a scrolling text panel.
/// \note Original identifier: \c D_800A268C (0x800A268C).
extern u8 gTextPanelClearedEmptyText[];

/// Whether gameplay exposes microphone interaction and its HUD indicator.
/// \note Original identifier: \c D_800722D0 (0x800722D0).
/// Whether gameplay permits recognized voice commands to launch messenger effects.
/// \note Original identifier: \c D_800722D4 (0x800722D4).
/// Minimum nonzero voice level for a command without a quiet-voice warning.
/// \note Original identifier: \c D_800722E8 (0x800722E8).
extern s32 gGameplayMicrophoneAvailable, gGameplayVoiceCommandsAllowed, gVoiceCommandMinimumVoiceLevel;
/// Messenger scale at the quiet end of the voice-level mapping.
/// \note Original identifier: \c D_800A2668 (0x800A2668).
extern f32 gVoiceCommandMinimumEffectScale;
/// Identifier of the currently active runtime program.
/// \note Original identifier: \c D_800AE4F0 (0x800AE4F0).
/// Identifier of the currently loaded area.
/// \note Original identifier: \c D_800AE4F8 (0x800AE4F8).
/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex, gRuntimeCurrentProgram, gRuntimeCurrentArea, gRuntimeGameMode;
/// Recognition result consumed and retained by gameplay command translation.
/// \note Original identifier: \c D_800B6840 (0x800B6840).
extern OSVoiceData gVoiceCommandRecognitionResult;
/// Separate symbol view of the primary player record's view yaw.
/// \note Original identifier: \c D_800EF6AC (0x800EF6AC).
/// Separate symbol view of the primary player record's horizontal view shake.
/// \note Original identifier: \c D_800EF6B8 (0x800EF6B8).
extern f32 gPlayerYaw, gPlayerViewShake;
/// Separate symbol view of the primary player record's held item handle.
/// \note Original identifier: \c D_800EF724 (0x800EF724).
extern s32 gPlayerHeldItemHandle;
/// Deliver a recognized voice command through a target-directed visual effect when gameplay permits.
/// \param message Recognized command type, value, warning flags, voice level, voice duration, and candidate object identifier.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800121B0 (0x800121B0).
/// \note Delivery depends on gameplay state and subsequent messenger contact;
/// creating the effect does not guarantee that the recipient accepts the message.
/// \see messengerDeliver
/// \see actorPostScriptMessage
void voiceCommandLaunchEffect(ScriptMessage message) {
    f32 scale;
    u8 *target;
    u8 *nearby;
    f32 position[3];
    f32 velocity[3];
    f32 cameraX;
    s32 special;
    s32 mode;
    f32 cameraY, cameraZ;
    f32 rotation[3];
    f32 targetDelta[3];
    f32 nearbyDelta[3];
    f32 targetDistance;
    u8 *object;
    u8 *state;
    s32 kind;
    if (gGameplayMicrophoneAvailable != 0 && gGameplayVoiceCommandsAllowed != 0 && gRuntimeCurrentProgram == 0 &&
        (gRuntimeGameMode == 0 || gRuntimeGameMode == 8 || gRuntimeGameMode == 0x12)) {
        special = 0;
        object = objectFindLiveByHandle(gPlayerHeldItemHandle);
        if (object != NULL &&
            ((state = ((RuntimeObjectPrefix *)object)->state.bytes, kind = ((ItemStatePrefix *)state)->kind) == 0x51 ||
             kind == 0x8A)) {
            special = 1;
            message.arg0 |= message.type << 20;
            message.type = 1;
        }
        scale = mathMapRangeClamped((f32)gVoiceCommandMinimumVoiceLevel, gVoiceCommandMinimumEffectScale, 8000.0F, 0.2F,
                                    (f32)(u32)gVoiceCommandRecognitionResult.voice_level);
        if (special != 0) {
            scale *= 2.0F;
        }
        if (gVoiceCommandRecognitionResult.warning != 0) {
            target = actorFindPart(((u8 *)gPikachuActor), 0x2C);
        } else {
            target = actorFindPart(((u8 *)gPikachuActor), 0x2E);
        }
        if (gRuntimeCurrentArea == 7) {
            rotation[1] = gPlayerYaw + gPlayerViewShake;
            rotation[0] = 0.0F;
            rotation[2] = 0.0F;
            nearby = objectFindNearestTargetInView(NULL, (Vec3f *)gRenderCameras[gRenderCameraIndex].eye, 15.0F,
                                                   rotation, 60.0F, 0x82, 0x82);
            if (nearby != NULL) {
                cameraX = gRenderCameras[gRenderCameraIndex].eye[0];
                cameraY = gRenderCameras[gRenderCameraIndex].eye[1];
                cameraZ = gRenderCameras[gRenderCameraIndex].eye[2];
                targetDelta[0] = ((RuntimeObjectPrefix *)target)->worldPosition[0] - cameraX;
                targetDelta[1] = ((RuntimeObjectPrefix *)target)->worldPosition[1] - cameraY;
                targetDelta[2] = ((RuntimeObjectPrefix *)target)->worldPosition[2] - cameraZ;
                nearbyDelta[0] = ((RuntimeObjectPrefix *)nearby)->worldPosition[0] - cameraX;
                nearbyDelta[1] = ((RuntimeObjectPrefix *)nearby)->worldPosition[1] - cameraY;
                nearbyDelta[2] = ((RuntimeObjectPrefix *)nearby)->worldPosition[2] - cameraZ;
                targetDistance = vector3Length(targetDelta);
                if (vector3Length(nearbyDelta) < targetDistance) {
                    target = nearby;
                    velocity[0] = 0.0F;
                    velocity[1] = 0.0F;
                    velocity[2] = 0.0F;
                }
            }
        }
        if (gRuntimeGameMode == 8 || gRuntimeGameMode == 0x12) {
            mode = 1;
        } else {
            mode = 0;
        }
        messengerGetCameraLaunch(position, velocity, special);
        messengerCreate(((RuntimeObjectPrefix *)target)->handle, position, velocity, scale, message, special, mode);
    }
}

/// Borrowed encoded display name retained by voice-command translation.
/// \note The unsigned byte view preserves the ROM's text-byte load convention.
/// \note Original identifier: \c D_800722D8 (0x800722D8).
extern u8 *gVoiceCommandDisplayName;
/// Borrowed fallback display text used when a recognized word has no display name.
/// \note Original identifier: \c D_800722EC (0x800722EC).
extern u8 *gVoiceCommandUnrecognizedName;
/// Recognition command status sampled by the most recent gameplay voice update.
/// \note Original identifier: \c D_800722DC (0x800722DC).
/// Dictionary lookup index retained by translation, including its legacy distance-based fallback.
/// \note Original identifier: \c D_800722E0 (0x800722E0).
/// Recognition distance threshold for distant-match warnings and legacy fallback selection.
/// \note Original identifier: \c D_800722E4 (0x800722E4).
/// Minimum nonzero voice level for a command without a quiet-voice warning.
/// \note Original identifier: \c D_800722E8 (0x800722E8).
extern s32 gVoiceCommandLastRecognitionStatus, gVoiceCommandResultIndex, gVoiceCommandMaximumMatchDistance,
    gVoiceCommandMinimumVoiceLevel;

/// Consume a recognition result and prepare a gameplay voice-command message.
/// \param out Destination for the command type, value, warning flags, voice level, voice duration, and initially empty candidate identifier.
/// \returns One if a usable message was produced, otherwise zero with out unchanged.
/// \note Original decompilation identifier: \c func_80012508 (0x80012508).
/// \note Consumes the recognition result even when it cannot produce a message.
/// A prepared message has not yet been delivered to an actor.
/// \see voiceRecognitionTakeResult
/// \see voiceCommandLaunchEffect
s32 voiceCommandTakeMessage(ScriptMessage *out) {
    s32 mismatch;
    s32 index;
    s32 quiet;
    s32 quietFlag;
    s32 next;

    gVoiceCommandLastRecognitionStatus = voiceRecognitionGetCommandStatus();
    gVoiceCommandResultIndex = voiceRecognitionTakeResult(&gVoiceCommandRecognitionResult);
    if (gVoiceCommandResultIndex < 0) {
        return 0;
    }
    micRequestAmplifierOff();
    index = 1;
    while (1) {
        if (*(gVoiceCommandDisplayName = (u8 *)voiceDictionaryGetWordName(((u16 *)&gVoiceCommandResultIndex)[1])) == 0) {
            gVoiceCommandDisplayName = gVoiceCommandUnrecognizedName;
            return 0;
        }
        if (voiceDictionaryGetCommandType(((u16 *)&gVoiceCommandResultIndex)[1]) != 0xD) {
            break;
        }
        if (index == 5) {
            break;
        }
        quietFlag = gVoiceCommandMaximumMatchDistance;
        next = gVoiceCommandRecognitionResult.distance[index];
        index++;
        if (quietFlag >= next) {
            gVoiceCommandResultIndex = next;
        } else {
            break;
        }
    }
    quiet = gVoiceCommandRecognitionResult.voice_level < gVoiceCommandMinimumVoiceLevel;
    if (quiet == 0) {
        quiet = gVoiceCommandRecognitionResult.voice_level == 0;
    }
    quietFlag = gVoiceCommandMaximumMatchDistance;
    mismatch = quietFlag < gVoiceCommandRecognitionResult.distance[0];
    out->type = voiceDictionaryGetCommandType(((u16 *)&gVoiceCommandResultIndex)[1]);
    out->arg0 = voiceDictionaryGetCommandValue(((u16 *)&gVoiceCommandResultIndex)[1]);
    quietFlag = quiet ? VOICE_COMMAND_QUIET : 0;
    quiet = mismatch ? VOICE_COMMAND_DISTANT_MATCH : 0;
    out->arg1 = quiet | (u32)gVoiceCommandRecognitionResult.warning | quietFlag;
    out->arg2 = gVoiceCommandRecognitionResult.voice_level;
    out->arg3 = gVoiceCommandRecognitionResult.voice_time;
    out->arg4 = 0;
    return 1;
}


/// Process an available voice command using the current interaction candidate.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800126B4 (0x800126B4).
void voiceCommandUpdate(void) {
    s32 pad;
    ScriptMessage s;

    if (voiceCommandTakeMessage(&s) != 0) {
        s.arg4 = interactionGetCandidateId();
        voiceCommandLaunchEffect(s);
    }
}

/// Pixel data for the scrolling text-panel backdrop strip.
/// \note Original identifier: \c D_80074EE0 (0x80074EE0).
/// Palette data for the scrolling text-panel backdrop strip.
/// \note Original identifier: \c D_80075EA0 (0x80075EA0).
/// Empty text used to initialize a scrolling text panel.
/// \note Original identifier: \c D_800A2670 (0x800A2670).
extern u8 gTextPanelBackdropPixels[], gTextPanelBackdropPalette[], gTextPanelInitialEmptyText[];
/// Retained backdrop descriptor borrowing the panel image and palette.
/// \note Original identifier: \c D_800B6860 (0x800B6860).
extern TextureImage gTextPanelBackdropImage;

/// Initialize a scrolling text panel with white text and its initial entry.
/// \param panel Writable panel storage of at least 32 bytes, valid until its entries are released.
/// \param visibleLines Nonnegative maximum number of visible text lines.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012730 (0x80012730).
void textPanelInitialize(TextPanel *panel, s32 visibleLines) {
    u32 allocationSize;
    u8 *allocation;

    visibleLines += 1;
    _bzero((s32)panel, 0x20);
    allocationSize = visibleLines * 8;
    allocation = heapAllocLocal(&gGameHeap, allocationSize, 0x33);
    panel->images = (TextImage **)allocation;
    _bzero((s32)allocation, allocationSize);
    panel->entries = panel->images + visibleLines;
    allocationSize = visibleLines * 4;
    allocation = heapAllocLocal(&gGameHeap, allocationSize, 0x3F);
    panel->colors = (u8 (*)[4])allocation;
    _bzero((s32)allocation, allocationSize);
    panel->capacity = visibleLines;
    panel->configuration = NULL;
    textPanelSetColor(panel, 0xFF, 0xFF, 0xFF, 0xFF);
    textureImageInitialize(&gTextPanelBackdropImage, (s32)gTextPanelBackdropPixels, (s32)gTextPanelBackdropPalette, 2, 0, 0x114,
                           0x1C);
    textPanelAppend((struct TextPanel *)panel, gTextPanelInitialEmptyText);
}

/// Clear a text panel and release its image-entry storage, retaining its color storage.
/// \param panel Initialized text panel to release.
/// \returns Nothing.
/// \note Original decompilation identifier: \c osStopTimer (0x8001282C).
void textPanelReleaseEntries(TextPanel *panel) {
    textPanelClear(panel);
    heapFreeLocal(&gGameHeap, panel->images);
}

/// Line-break text emitted between scrolling text-panel entries.
/// \note Original identifier: \c D_800A2688 (0x800A2688).
extern char gTextPanelLineBreakText[];
/// Horizontal scale used when drawing the text-panel backdrop strip.
/// \note Original identifier: \c D_800A2694 (0x800A2694).
extern f32 gTextPanelBackdropHorizontalScale;

/// Draw the panel's visible text lines with the selected backdrop style.
/// \param panel Initialized text panel to draw.
/// \param gdl Display-list cursor receiving drawing commands and advanced past them.
/// \param mode Backdrop style: zero for none, one for an image strip, or two for a dark rectangle; values at least three use no backdrop.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012860 (0x80012860).
void textPanelDraw(TextPanel *panel, Gfx **gdl, s32 mode) {
    TextImage **image;
    u8 *color;
    s32 x0, y0, x1, y1;
    s32 i;

    if (mode >= 3) {
        mode = 0;
    }
    textDrawSetArea(0, 0, 319, 239);
    textDrawGetDefaultArea(&x0, &y0, &x1, &y1);
    if (mode != 0) {
        gDPPipeSync((*gdl)++);
        gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        switch (mode) {
        case 1:
            textDrawSetScale(1.0F, 1.0F);
            gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 160);
            gDPSetCombineMode((*gdl)++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
            textureDrawScaledRegion(gdl, &gTextPanelBackdropImage, (u32)(x0 - 10.0F), panel->y + y0 - 6, 256, 0, 0, 0,
                                    2, 0, gTextPanelBackdropHorizontalScale, 1.0F);
            break;
        case 2:
            textDrawSetScale(1.0F, 1.0F);
            gDPSetPrimColor((*gdl)++, 0, 0, 0, 0, 0, 180);
            gDPSetCombineMode((*gdl)++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
            {
                Gfx *rectangle = (*gdl)++;
                rectangle->words.w0 = _SHIFTL(G_FILLRECT, 24, 8) |
                                      _SHIFTL(panel->y + y0 + panel->count * 16 - 14, 2, 10) | _SHIFTL(x1 + 8, 14, 10);
                rectangle->words.w1 = _SHIFTL(panel->y + y0 - 2, 2, 10) | _SHIFTL(x0 - 8, 14, 10);
            }
            break;
        }
    }
    textDrawBegin(*gdl);
    textDrawSetPalette(gDefaultTextPalette);
    image = panel->images + 1;
    for (i = 1; i < panel->count; i++, image++) {
        if (*image != NULL) {
            textDrawSetCursor(panel->x + x0, panel->y + y0 + (i * 16) - 16);
            color = panel->colors[i];
            textDrawSetColor(color[0], color[1], color[2], color[3]);
            textDrawImageAtCursor((TextImage *)*image);
        }
        textDrawPrintf(gTextPanelLineBreakText);
    }
    textDrawGetDisplayListCursor(gdl);
}

/// Set the display color of one text-panel line.
/// \param panel Initialized text panel.
/// \param line Color slot index within the panel's allocated capacity.
/// \param red Red component from zero to 255.
/// \param green Green component from zero to 255.
/// \param blue Blue component from zero to 255.
/// \param alpha Opacity from zero to 255.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012C10 (0x80012C10).
void textPanelSetLineColor(TextPanel *panel, s32 line, s32 red, s32 green, s32 blue, s32 alpha) {
    s32 off;

    off = line * 4;
    panel->colors[line][0] = red;
    panel->colors[line][1] = green;
    panel->colors[line][2] = blue;
    panel->colors[line][3] = alpha;
}

/// Set the display color of every text-panel line.
/// \param panel Initialized text panel.
/// \param red Red component from zero to 255.
/// \param green Green component from zero to 255.
/// \param blue Blue component from zero to 255.
/// \param alpha Opacity from zero to 255.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012C50 (0x80012C50).
void textPanelSetColor(TextPanel *panel, s32 red, s32 green, s32 blue, s32 alpha) {
    s32 var_s0;

    var_s0 = 0;
    if (((TextPanel *)panel)->capacity > 0) {
        do {
            textPanelSetLineColor(panel, var_s0, red, green, blue, alpha);
            var_s0 += 1;
        } while (var_s0 < ((TextPanel *)panel)->capacity);
    }
}

/// Set a text panel's display offset.
/// \param panel Initialized text panel to position.
/// \param x Horizontal offset in screen pixels.
/// \param y Vertical offset in screen pixels.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012CE4 (0x80012CE4).
void textPanelSetPosition(TextPanel *panel, s32 x, s32 y) {
    panel->x = x;
    panel->y = y;
}

/// Select the configuration used to prepare subsequent text entries.
/// \param panel Initialized text panel.
/// \param configuration Configuration index from zero through two; other values select the default configuration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012CF0 (0x80012CF0).
void textPanelSetTextConfiguration(TextPanel *panel, s32 configuration) {
    if ((configuration >= 0) && (configuration < 3)) {
        panel->configuration = &gTextImageConfigurations[configuration];
        return;
    }
    panel->configuration = NULL;
}


/// Append a text entry, discarding the oldest entry when the panel is full.
/// \param panel Initialized text panel receiving the entry.
/// \param text Null-terminated encoded text to prepare; an empty string appends a blank entry.
/// \returns A borrowed text-image address owned by the panel, or zero for a blank entry.
/// The image remains valid until discarded by an append, clear, or entry release.
/// \note Original decompilation identifier: \c func_80012D20 (0x80012D20).
s32 textPanelAppend(TextPanel *panel, void *text) {
    TextImage *oldImage;
    s32 index;
    TextImage **slot;
    s32 capacity;
    s32 i;
    capacity = panel->capacity;
    textDrawSetFont(0);
    textDrawSetSpacingMode(1);
    slot = &panel->entries[panel->nextEntry];
    oldImage = *slot;
    if (oldImage != 0) {
        textImageDestroy(oldImage);
        *slot = 0;
    }
    if (*(u8 *)text != 0) {
        textImageCreateFromText((TextImage **)((s32)slot), (u8 *)((s32)text), (FontImage *)((s32)panel->configuration));
    }
    panel->count += 1;
    if (capacity < panel->count) {
        panel->count = capacity;
    }
    index = (panel->nextEntry + capacity - panel->count + 1) % capacity;
    for (i = 0; i < panel->count; i++) {
        panel->images[i] = panel->entries[index];
        index++;
        if (index < 0) {
            do {
                index += capacity;
            } while (index < 0);
        }
        if (index >= capacity) {
            do {
                index -= capacity;
            } while (index >= capacity);
        }
    }
    panel->nextEntry += 1;
    if (panel->nextEntry < 0) {
        do {
            panel->nextEntry += capacity;
        } while (panel->nextEntry < 0);
    }
    if (panel->nextEntry >= capacity) {
        do {
            panel->nextEntry -= capacity;
        } while (panel->nextEntry >= capacity);
    }
    return (s32)*slot;
}

/// Discard the panel's text entries and restore its initial blank entry.
/// \param panel Initialized text panel to clear.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012EE4 (0x80012EE4).
void textPanelClear(TextPanel *panel) {
    s32 imageAddress;
    s32 imageByteOffset;
    s32 lineIndex;
    s32 lineCount;

    lineCount = panel->count;
    imageByteOffset = 0;
    lineIndex = 0;
    if (lineCount > 0) {
        do {
            imageAddress = *(s32 *)((u8 *)panel->images + imageByteOffset);
            if (imageAddress != 0) {
                textImageDestroy((TextImage *)imageAddress);
                *(s32 *)((u8 *)panel->images + imageByteOffset) = 0;
                lineCount = panel->count;
            }
            lineIndex += 1;
            imageByteOffset += 4;
        } while (lineIndex < lineCount);
    }
    _bzero((s32)panel->entries, (s32)(panel->capacity * 4));
    panel->nextEntry = 0;
    panel->count = 0;
    textPanelAppend((struct TextPanel *)panel, gTextPanelClearedEmptyText);
}

#include "text_image.h"
/* {0xF0, 0x0F}: nibble mask for even/odd pixels (rodata blob). */
typedef struct {
    u8 m[2];
} PixelMasks;
/// High- and low-nibble masks for paired four-bit pixels.
/// \note Original identifier: \c D_800A2690 (0x800A2690).
extern PixelMasks gImage4BitPixelMasks;

#include "image_outline.h"
/// Add a colored outline at a requested offset in a four-bit image.
/// \param image Writable image with an even pixel width and byte-aligned rows; unsupported formats or absent pixel data are ignored.
/// \param dx Horizontal outline offset in pixels.
/// \param dy Vertical outline offset in pixels.
/// \param background Background pixel value from zero through fifteen that may receive the outline.
/// \param outline Outline pixel value from zero through fifteen.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80012F8C (0x80012F8C).
void image4BitAddOffsetOutline(TextImage *image, s32 dx, s32 dy, s32 background, s32 outline) {
    PixelMasks masks = gImage4BitPixelMasks;
    u8 src[2];
    u8 dest[2];
    s32 y;
    s32 targetY;
    u8 *input;
    s32 x;
    s32 targetX;
    u8 *output;
    s32 parity;
    s32 targetParity;
    u8 current;

    if (image->pixels == NULL || image->format != 2) {
        return;
    }
    src[0] = background << 4;
    src[1] = background;
    dest[0] = outline << 4;
    dest[1] = outline;
    for (y = 0, targetY = dy; y < image->height; y++, targetY++) {
        if (targetY < 0 || targetY >= image->height) {
            continue;
        }
        input = image->pixels + ((image->width * y) >> 1);
        output = image->pixels + ((image->width * targetY) >> 1) + (dx >> 1);
        x = 0;
        targetX = dx;
        for (; x < image->width; x++, targetX++) {
            if (targetX >= 0 && targetX < image->width) {
                parity = x & 1;
                current = *input & masks.m[parity];
                if (current != src[parity] && current != dest[parity]) {
                    targetParity = targetX & 1;
                    current = *output;
                    if ((current & masks.m[targetParity]) == src[targetParity]) {
                        *output = (current & masks.m[targetParity ^ 1]) | dest[targetParity];
                    }
                }
            }
            if (x & 1) {
                input++;
            }
            if (targetX & 1) {
                output++;
            }
        }
    }
}
