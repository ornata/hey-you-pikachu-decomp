#include "texture_image.h"
#include "texture_descriptor.h"
#include "texture_rectangles.h"
#include "pikachu_actor.h"
#include "random.h"
#include "microphone_requests.h"
#include "text_image_configurations.h"
#include "texture_animation_programs.h"
#include "camera_visibility.h"
#include "hud_draw.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "text_image_create.h"
#include "audio_cue.h"
#include "text_draw.h"
#include "integer_animation.h"
#include "player_control.h"
#include "object_types.h"
#include "text_image.h"
#include "integer_animation_types.h"
#include "texture_draw_types.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata and the dead branch still ends a scheduling block. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

void hudDrawAlternateOverlay(Gfx **);
void hudDrawAuxiliaryIcon(Gfx **);
void hudDrawMotionIndicators(Gfx **);
/// Animation track for the currently selected HUD motion indicator.
/// \note Original identifier: \c D_800F0268 (0x800F0268).
extern ValueTrack gHudMotionIndicatorTrack;
/// Animation identifier retained for the HUD motion indicator.
/// \note Original identifier: \c D_800F0274 (0x800F0274).
extern s32 gHudMotionIndicatorAnimation;
/// HUD guide drawing callbacks indexed by guide style.
/// \note Original identifier: \c D_800890F0 (0x800890F0).
extern void (*gHudGuideDrawCallbacks[])(Gfx **);
/// Selected HUD guide style; zero disables the guide.
/// \note Original identifier: \c D_800F01F4 (0x800F01F4).
extern s32 gHudGuideStyle;
/// Draw callback selected for the active HUD guide style.
/// \note Original identifier: \c D_800F01F8 (0x800F01F8).
extern void (*gHudGuideDrawCallback)(Gfx **);

/// Provide the inactive HUD program-initialization hook for this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800215A0 (0x800215A0).
void hudProgramInitializeNoOp(void) {
}

/// Provide the inactive per-player HUD update hook for this build.
/// \param player Player identifier supplied by the runtime; unused in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800215A8 (0x800215A8).
void hudPlayerUpdateNoOp(s32 player) {
}

/// Provide an unused HUD callback with no effect in this build.
/// \param unused Unused callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800215B0 (0x800215B0).
void hudUnusedArgumentNoOp(s32 unused) {
}

/// Provide an unused HUD callback with no effect in this build.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800215B8 (0x800215B8).
void hudUnusedNoOp(void) {
}

/// Three animation tracks drawing horizontal guidance toward off-screen Pikachu.
/// \note Original identifier: \c D_800F0200 (0x800F0200).
extern u8 gHudHorizontalDirectionTracks[0x24];
/// Separate view of the middle horizontal Pikachu guidance animation track.
/// \note Original identifier: \c D_800F020C (0x800F020C).
extern ValueTrack gHudHorizontalDirectionMiddleTrack;
/// Separate view of the innermost horizontal Pikachu guidance animation track.
/// \note Original identifier: \c D_800F0218 (0x800F0218).
extern ValueTrack gHudHorizontalDirectionInnerTrack;
/// Three animation tracks drawing vertical guidance toward off-screen Pikachu.
/// \note Original identifier: \c D_800F0224 (0x800F0224).
extern u8 gHudVerticalDirectionTracks[0x24];
/// Separate view of the middle vertical Pikachu guidance animation track.
/// \note Original identifier: \c D_800F0230 (0x800F0230).
extern ValueTrack gHudVerticalDirectionMiddleTrack;
/// Separate view of the innermost vertical Pikachu guidance animation track.
/// \note Original identifier: \c D_800F023C (0x800F023C).
extern ValueTrack gHudVerticalDirectionInnerTrack;

/// Initialize the animated indicators that guide the player toward Pikachu.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800215C0 (0x800215C0).
void hudInitializeDirectionIndicators(void) {
    animationIntegerTrackInitialize((ValueTrack *)(f32 *)&gHudHorizontalDirectionTracks, gTextureFrameAnimationPrograms[221]);
    animationIntegerTrackInitialize((ValueTrack *)(f32 *)&gHudHorizontalDirectionMiddleTrack, gTextureFrameAnimationPrograms[222]);
    animationIntegerTrackInitialize((ValueTrack *)(f32 *)&gHudHorizontalDirectionInnerTrack, gTextureFrameAnimationPrograms[223]);
    animationIntegerTrackInitialize((ValueTrack *)(f32 *)&gHudVerticalDirectionTracks, gTextureFrameAnimationPrograms[224]);
    animationIntegerTrackInitialize((ValueTrack *)(f32 *)&gHudVerticalDirectionMiddleTrack, gTextureFrameAnimationPrograms[225]);
    animationIntegerTrackInitialize((ValueTrack *)(f32 *)&gHudVerticalDirectionInnerTrack, gTextureFrameAnimationPrograms[226]);
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;
/// Animation track for the vertical movement indicator; also ends the guidance track storage.
/// \note Original identifier: \c D_800F0248 (0x800F0248).
extern s32 gHudVerticalMotionTrack;
/// Draw animated directional guidance toward Pikachu when he is outside the current view.
/// Requires an active Pikachu actor and initialized direction-indicator animations.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80021648 (0x80021648).
void hudDrawDirectionIndicators(Gfx **gdl) {
    f32 direction[3];
    s32 frame;
    TextureImage tex;
    TextureDescriptor desc;
    u8 *anim;
    u8 *verticalAnim;
    s32 offset;
    s32 position;
    f32 limit;
    if (areaTestCameraVisibility(((RuntimeObjectPrefix *)((u8 *)gPikachuActor))->radius * 2, (f32 *)(((u8 *)gPikachuActor) + 0x44),
                                 direction) == 0) {
        gDPPipeSync((*gdl)++);
        gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineMode((*gdl)++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 255);
        limit = -direction[2];
        anim = (u8 *)&gHudHorizontalDirectionTracks;
        if (limit <= direction[0] || limit <= -direction[0]) {
            offset = 0;
            do {
                animationIntegerTrackEvaluate((ValueTrack *)anim, &frame, gRuntimeFrameSeconds);
                textureGetDescriptor(5, frame, &desc);
                textureImageInitialize(&tex, desc.image, desc.palette, desc.format, desc.size, desc.width, desc.height);
                if (direction[0] > 0.0f)
                    position = 260 - offset;
                else
                    position = offset + 48;
                textureDrawScaledRepeated(gdl, &tex, position, (240 - desc.height) / 2 - 8, 2, 2, 1, 1, 1.0f, 1.0f,
                              direction[0] > 0.0f ? 0 : 1, 0);
                offset += 12;
                anim += 12;
            } while (anim != (u8 *)&gHudVerticalDirectionTracks);
            return;
        }
        verticalAnim = (u8 *)&gHudVerticalDirectionTracks;
        offset = 0;
        if (limit <= direction[1] || limit <= -direction[1]) {
            do {
                animationIntegerTrackEvaluate((ValueTrack *)verticalAnim, &frame, gRuntimeFrameSeconds);
                textureGetDescriptor(5, frame, &desc);
                textureImageInitialize(&tex, desc.image, desc.palette, desc.format, desc.size, desc.width, desc.height);
                if (direction[1] > 0.0f)
                    position = offset + 40;
                else
                    position = 188 - offset;
                textureDrawScaledRepeated(gdl, &tex, (320 - desc.width) / 2, position, 2, 2, 1, 1, 1.0f, 1.0f, 0,
                              direction[1] > 0.0f ? 0 : 1);
                offset += 12;
                verticalAnim += 12;
            } while (verticalAnim != (u8 *)&gHudVerticalMotionTrack);
        }
    }
}

typedef struct Tmpl6C {
    s32 w[27];
} Tmpl6C;
/// Sprite descriptors drawn for HUD guide style one.
/// \note Original identifier: \c D_80088F40 (0x80088F40).
extern Tmpl6C gHudGuideStyleOneSprites;
void hudDrawDirectionIndicators(Gfx **);

/// Draw HUD guide style 1 and directional guidance toward Pikachu.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800219DC (0x800219DC).
void hudDrawGuideStyle1(Gfx **gdl) {
    Tmpl6C t = gHudGuideStyleOneSprites;

    gDPPipeSync((*gdl)++);
    gDPSetRenderMode((*gdl)++, 0x504240, 0);
    gDPSetCombine((*gdl)++, 0x119623, 0xFF2FFFFF);
    gDPSetPrimColor((*gdl)++, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF);
    textureDrawRectangles(5, (TextureDrawC354 *)(TextureDrawC354 *)&t, gdl);
    hudDrawDirectionIndicators(gdl);
}

/// Sprite descriptors drawn for HUD guide style two.
/// \note Original identifier: \c D_80088FAC (0x80088FAC).
extern TextureDrawC354 gHudGuideStyleTwoSprites[];

/// Draw HUD guide style 2 and directional guidance toward Pikachu.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80021AC0 (0x80021AC0).
void hudDrawGuideStyle2(Gfx **gdl) {
    gDPPipeSync((*gdl)++);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode((*gdl)++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor((*gdl)++, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF);
    textureDrawRectangles(5, (TextureDrawC354 *)gHudGuideStyleTwoSprites, gdl);
    hudDrawDirectionIndicators(gdl);
}

/// Sprite descriptors drawn for HUD guide style three.
/// \note Original identifier: \c D_80089018 (0x80089018).
extern TextureDrawC354 gHudGuideStyleThreeSprites[];

/// Draw HUD guide style 3 and directional guidance toward Pikachu.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80021B74 (0x80021B74).
void hudDrawGuideStyle3(Gfx **gdl) {
    gDPPipeSync((*gdl)++);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode((*gdl)++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor((*gdl)++, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF);
    textureDrawRectangles(5, (TextureDrawC354 *)gHudGuideStyleThreeSprites, gdl);
    hudDrawDirectionIndicators(gdl);
}

/// Sprite descriptors drawn for HUD guide style four.
/// \note Original identifier: \c D_80089084 (0x80089084).
extern TextureDrawC354 gHudGuideStyleFourSprites[];

/// Draw HUD guide style 4 and directional guidance toward Pikachu.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80021C28 (0x80021C28).
void hudDrawGuideStyle4(Gfx **gdl) {
    gDPPipeSync((*gdl)++);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode((*gdl)++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor((*gdl)++, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF);
    textureDrawRectangles(5, (TextureDrawC354 *)gHudGuideStyleFourSprites, gdl);
    hudDrawDirectionIndicators(gdl);
}

/// Select the HUD guide style, playing selection feedback when a different guide is enabled.
/// \param style Guide style identifier, from 1 through 4, or zero to disable the guide.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80021CDC (0x80021CDC).
void hudSetGuideStyle(s32 style) {
    if ((style != 0) && (style != gHudGuideStyle)) {
        audioPlayCue(0x21);
    }
    gHudGuideStyle = style;
    gHudGuideDrawCallback = gHudGuideDrawCallbacks[style];
}


/// Query the selected HUD guide style.
/// \returns The guide style identifier, or zero when the guide is disabled.
/// \note Original decompilation identifier: \c func_80021D34 (0x80021D34).
s32 hudGetGuideStyle(void) {
    return gHudGuideStyle;
}

/// Current microphone availability and trigger/recognition presentation state.
/// \note Original identifier: \c D_800F01DC (0x800F01DC).
extern s32 gHudMicrophoneIndicatorState;
/// Current recognition presentation state selecting its animation and scale.
/// \note Original identifier: \c D_800F01EC (0x800F01EC).
extern s32 gHudRecognitionIndicatorState;

/// Reset microphone and recognition indicators for their next update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80021D40 (0x80021D40).
void hudResetMicrophoneIndicators(void) {
    gHudMicrophoneIndicatorState = 8;
    gHudRecognitionIndicatorState = 3;
}

typedef struct {
    s32 animation;
    f32 scale;
} HudState;
s32 micQueryTriggerStateNoResult(void);
s32 micQueryDevicePortNoResult(void);
s32 voiceRecognitionGetCommandStatus(void);
/// Whether gameplay exposes microphone interaction and its HUD indicator.
/// \note Original identifier: \c D_800722D0 (0x800722D0).
extern s32 gGameplayMicrophoneAvailable;
/// Recognition animation and starting scale indexed by recognition presentation state.
/// \note Original identifier: \c D_80089134 (0x80089134).
extern HudState gHudRecognitionPresentations[];
/// Animation track used by the recognition status indicator.
/// \note Original identifier: \c D_800F01E0 (0x800F01E0).
extern ValueTrack gHudRecognitionTrack;
/// Retained recognition-indicator scale, increased while recognition is active.
/// \note Original identifier: \c D_800F01F0 (0x800F01F0).
extern f32 gHudRecognitionScale;
/// Feature mask controlling the enabled HUD indicators.
/// \note Original identifier: \c D_800F0284 (0x800F0284).
extern s32 gHudEnabledFeatures;
/// Update and draw the enabled voice-recognition status indicator when a microphone is connected.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80021D5C (0x80021D5C).
void hudDrawRecognitionIndicator(Gfx **gdl) {
    s32 frame;
    s32 mode;
    Gfx *gp;
    TextureImage tex;
    TextureDescriptor desc;
    s32 x;
    s32 y;
    f32 scale;
    s32 state;
    if (gGameplayMicrophoneAvailable != 0 && (gHudEnabledFeatures & 4) && micQueryDevicePortNoResult() >= 0) {
        mode = voiceRecognitionGetCommandStatus();
        switch (mode) {
        case 3:
            state = 2;
            break;
        case 5:
            state = 1;
            break;
        default:
            state = 0;
            break;
        }
        if (state != gHudRecognitionIndicatorState) {
            gHudRecognitionIndicatorState = state;
            gHudRecognitionScale = gHudRecognitionPresentations[state].scale;
            animationIntegerTrackInitialize((ValueTrack *)(f32 *)&gHudRecognitionTrack, (ValueKey *)gHudRecognitionPresentations[state].animation);
        }
        animationIntegerTrackEvaluate((ValueTrack *)&gHudRecognitionTrack, &frame, gRuntimeFrameSeconds);
        if (frame >= 0) {
            gp = *gdl;
            scale = gHudRecognitionScale;
            if (gHudRecognitionIndicatorState == 1) {
                if (micQueryTriggerStateNoResult() != 0) {
                    scale += randomNext() * 2.3283064365386963e-10 * 0.15f;
                } else {
                    scale += randomNext() * 2.3283064365386963e-10 * 0.05f;
                }
                gHudRecognitionScale += gRuntimeFrameSeconds * 0.2f;
                if (gHudRecognitionScale > 0.8f)
                    gHudRecognitionScale = 0.8f;
            }
            gDPPipeSync(gp++);
            gDPSetCombineMode(gp++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
            gDPSetRenderMode(gp++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
            gDPSetPrimColor(gp++, 0, 0, 255, 255, 255, 255);
            textureGetDescriptor(5, frame, &desc);
            x = (242.0f - desc.width * scale * 0.5f) * 1;
            y = 196.0f - (f32)(u32)desc.height * scale * 0.5f;
            textureImageInitialize(&tex, desc.image, desc.palette, desc.format, desc.size, desc.width, desc.height);
            textureDrawScaledRepeated(&gp, &tex, x, y, 2, 2, 1, 1, 1 * scale, scale, 0, 1);
            *gdl = gp;
        }
    }
}

s32 micQueryTriggerReadinessNoResult(void);
void audioPlaySoundEffect(s32);
/// Whether gameplay permits recognized voice commands to launch messenger effects.
/// \note Original identifier: \c D_800722D4 (0x800722D4).
extern s32 gGameplayVoiceCommandsAllowed;
/// Animation identifiers indexed by microphone presentation state.
/// \note Original identifier: \c D_8008914C (0x8008914C).
extern s32 gHudMicrophoneAnimationIds[];
/// RGBA colors indexed by microphone presentation state.
/// \note Original identifier: \c D_8008916C (0x8008916C).
extern u8 gHudMicrophoneColors[][4];
/// Animation track used by the microphone status indicator.
/// \note Original identifier: \c D_800F01D0 (0x800F01D0).
extern ValueTrack gHudMicrophoneTrack;
/// Separate view of the microphone animation frame retained for transition feedback.
/// \note Original identifier: \c D_800F01D8 (0x800F01D8).
extern s16 gHudMicrophoneTrackFrame;
/// Update and draw the enabled microphone and voice-recognition status indicators.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022110 (0x80022110).
void hudDrawMicrophoneIndicators(Gfx **gdl) {
    s32 previousFrame;
    s32 frame;
    s32 state;
    s32 mode;
    f32 scale;
    Gfx *gp;
    TextureDescriptor desc;
    s32 x;
    s32 y;
    TextureImage tex;
    if (gGameplayMicrophoneAvailable != 0 && (gHudEnabledFeatures & 4)) {
        hudDrawRecognitionIndicator(gdl);
        if (micQueryDevicePortNoResult() < 0) {
            scale = 1.0f;
            state = 1;
        } else if (gGameplayVoiceCommandsAllowed == 0 || micQueryTriggerReadinessNoResult() == 0) {
            state = 2;
            scale = 0.7f;
        } else {
            if (micQueryTriggerStateNoResult() == 0) {
                state = 3;
                scale = 0.8f;
            } else {
                if (gHudMicrophoneIndicatorState == 3 || gHudMicrophoneIndicatorState == 4)
                    state = 4;
                else
                    state = 5;
                scale = 0.85f;
                mode = voiceRecognitionGetCommandStatus();
                switch (mode) {
                case 3:
                    state = 6;
                    break;
                case 5:
                    state = 7;
                    scale = randomNext() * 2.3283064365386963e-10 * 0.1f + 0.95f;
                    break;
                }
            }
        }
        if (state != gHudMicrophoneIndicatorState) {
            TRACE("micStateChange: %d -> %d\n", gHudMicrophoneIndicatorState, state);
            gHudMicrophoneIndicatorState = state;
            if (state == 4)
                audioPlaySoundEffect(0xBF);
            animationIntegerTrackInitialize((ValueTrack *)(f32 *)&gHudMicrophoneTrack, gTextureFrameAnimationPrograms[gHudMicrophoneAnimationIds[gHudMicrophoneIndicatorState]]);
        }
        previousFrame = gHudMicrophoneTrackFrame;
        animationIntegerTrackEvaluate((ValueTrack *)&gHudMicrophoneTrack, &frame, gRuntimeFrameSeconds);
        gp = *gdl;
        gDPPipeSync(gp++);
        if (gHudMicrophoneIndicatorState == 6 && frame != 0x50) {
            gDPSetCombine(gp++, 0x70FEE1, 0xFFFFF3F9);
        } else {
            gDPSetCombineMode(gp++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        }
        gDPSetRenderMode(gp++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetPrimColor(gp++, 0, 0, gHudMicrophoneColors[gHudMicrophoneIndicatorState][0],
                        gHudMicrophoneColors[gHudMicrophoneIndicatorState][1],
                        gHudMicrophoneColors[gHudMicrophoneIndicatorState][2],
                        gHudMicrophoneColors[gHudMicrophoneIndicatorState][3]);
        textureGetDescriptor(5, frame, &desc);
        if (gHudMicrophoneIndicatorState == 1) {
            x = 300.0f - desc.width * scale;
            y = 220.0f - (f32)(u32)desc.height * scale;
            if (frame != previousFrame && frame == 0x59)
                audioPlayCue(0x1A);
        } else {
            x = (274.0f - desc.width * scale * 0.5f) * 1;
            y = 200.0f - (f32)(u32)desc.height * scale * 0.5f;
        }
        textureImageInitialize(&tex, desc.image, desc.palette, desc.format, desc.size, desc.width, desc.height);
        textureDrawScaledRepeated(&gp, &tex, x, y, 2, 2, 1, 1, 1 * scale, scale, 0, 1);
        *gdl = gp;
    }
}

/* Nine-word draw record consumed by textureDrawRectangles; texture -1 terminates a list. */
typedef TextureDrawC354 DrawRecord;
/// Sprite descriptors for the animated alternate HUD overlay.
/// \note Original identifier: \c D_8008918C (0x8008918C).
extern DrawRecord gHudAlternateOverlaySprites[9];

/// Advance and draw the animated alternate HUD overlay.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022608 (0x80022608).
void hudDrawAlternateOverlay(Gfx **gdl) {
    static f32 phase = 0.0f;
    s32 offset;

    phase -= gRuntimeFrameSeconds * 32.0f;
    while (phase < 0.0f) {
        phase += 32.0f;
    }
    while (phase >= 32.0f) {
        phase -= 32.0f;
    }
    offset = phase;
    gHudAlternateOverlaySprites[1].t = offset;
    gHudAlternateOverlaySprites[0].t = offset;
    gDPPipeSync((*gdl)++);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode((*gdl)++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor((*gdl)++, 0, 0, 0, 0, 0, 0x50);
    textureDrawRectangles(5, (TextureDrawC354 *)(TextureDrawC354 *)gHudAlternateOverlaySprites, gdl);
}

/// Terminated sprite descriptors for the auxiliary HUD icon.
/// \note Original identifier: \c D_800892D4 (0x800892D4).
DrawRecord gHudAuxiliaryIconSprites[2] = {
    {0x71, 0xAC, 0xB9, 0x40, 0x20, 0, 0, 1.0f, 1.0f},
    {-1, 0, 0, 0, 0, 0, 0, 0.0f, 0.0f},
};

/// Draw the auxiliary HUD icon.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022740 (0x80022740).
void hudDrawAuxiliaryIcon(Gfx **gdl) {
    gDPPipeSync((*gdl)++);
    gSPSetOtherMode((*gdl)++, G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, 0x504240);
    gDPSetCombineMode((*gdl)++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor((*gdl)++, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF);
    textureDrawRectangles(5, (TextureDrawC354 *)(TextureDrawC354 *)gHudAuxiliaryIconSprites, gdl);
}

/// Animation identifiers for the five vertical movement-indicator bands.
/// \note Original identifier: \c D_8008931C (0x8008931C).
s32 gHudVerticalMotionAnimationIds[] = {0xEC, 0xED, 0xE9, 0xEB, 0xEA};

/// Choose the HUD animation for a vertical movement-indicator position.
/// \param position Vertical indicator position on its nominal 0-through-31 scale; out-of-range values select an endpoint animation.
/// \returns The animation identifier for the corresponding movement range.
/// \note Original decompilation identifier: \c func_800227E8 (0x800227E8).
s32 hudGetVerticalMotionAnimation(s32 position) {
    position = (s32)((f32)position * 0.15625f);
    if (position < 0) {
        position = 0;
    } else if (position >= 5) {
        position = 4;
    }
    return gHudVerticalMotionAnimationIds[position];
}

/// Remaining time during which the horizontal motion indicator is displayed.
/// \note Original identifier: \c D_800F0264 (0x800F0264).
extern f32 gHudHorizontalMotionRemaining;
s32 hudGetVerticalMotionAnimation(s32); /* extern */
/// Integer animation program used by the horizontal movement indicator.
/// \note Original identifier: \c D_8006B448 (0x8006B448).
extern s32 gHudHorizontalMotionAnimation;
/// Animation track for the horizontal movement indicator.
/// \note Original identifier: \c D_800F0258 (0x800F0258).
extern s32 gHudHorizontalMotionTrack;

/// Initialize movement-indicator animations and begin the initial horizontal display period.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022838 (0x80022838).
void hudInitializeMotionIndicators(void) {
    animationIntegerTrackInitialize((ValueTrack *)(f32 *)&gHudVerticalMotionTrack, gTextureFrameAnimationPrograms[hudGetVerticalMotionAnimation(0)]);
    animationIntegerTrackInitialize((ValueTrack *)(f32 *)&gHudHorizontalMotionTrack, (ValueKey *)gHudHorizontalMotionAnimation);
    gHudHorizontalMotionRemaining = 5.0f;
}

/// Select and restart the HUD action-indicator animation.
/// \param animation Valid animation-table index for the action indicator.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022894 (0x80022894).
void hudSetActionIndicatorAnimation(s32 animation) {
    animationIntegerTrackInitialize((ValueTrack *)(f32 *)&gHudMotionIndicatorTrack, gTextureFrameAnimationPrograms[animation]);
    gHudMotionIndicatorAnimation = animation;
}

typedef struct {
    u8 unknown[0x3F4];
    f32 targetX, targetY, x, y;
} MenuMotion228;
/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern u8 gPlayerState[];
/// Action-indicator animations indexed by whether stationary look can be entered.
/// \note Original identifier: \c D_80089450 (0x80089450).
/// Action-indicator animations indexed by whether companion control can be toggled.
/// \note Original identifier: \c D_80089458 (0x80089458).
extern s32 gHudStationaryLookActionAnimations[], gHudCompanionActionAnimations[];
/// Sprite descriptors updated with vertical movement position and animation.
/// \note Original identifier: \c D_80089330 (0x80089330).
/// Sprite descriptors updated with horizontal movement position and animation.
/// \note Original identifier: \c D_8008939C (0x8008939C).
/// Sprite descriptors updated with the selected action-indicator animation.
/// \note Original identifier: \c D_80089408 (0x80089408).
extern DrawRecord gHudVerticalMotionSprites[], gHudHorizontalMotionSprites[], gHudActionIndicatorSprites[];
s32 playerCanToggleCompanionControl(void);
s32 playerCanEnterStationaryLook(void);

/// Update and draw the movement indicators and applicable action indicator.
/// Requires an active Pikachu actor and initialized motion-indicator animations.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800228D8 (0x800228D8).
void hudDrawMotionIndicators(Gfx **gdl) {
    s32 frameY, frameX;
    MenuMotion228 *motion;
    s32 x, y;
    s32 showMotion;
    s32 showSelected;
    s32 selected;
    f32 dx, dy, step, negativeStep;

    selected = gHudMotionIndicatorAnimation;
    motion = *(MenuMotion228 **)(((u8 *)gPikachuActor) + 0x54);
    if (playerIsPitchControlEnabled(gPlayerState)) {
        selected = gHudStationaryLookActionAnimations[playerCanEnterStationaryLook()];
        showMotion = 1;
        showSelected = 1;
    } else if (playerGetCompanionControlMode(gPlayerState) &&
               *(s32 *)(((u8 *)gPikachuActor) + 0x38) == *(s32 *)(gPlayerState + 0x50)) {
        selected = gHudCompanionActionAnimations[playerCanToggleCompanionControl()];
        showMotion = 1;
        showSelected = 1;
    } else {
        showMotion = 0;
        showSelected = 0;
    }
    if (!(gHudEnabledFeatures & 1))
        showSelected = 0;
    if (!(gHudEnabledFeatures & 2))
        showMotion = 0;
    if (showSelected) {
        if (selected != gHudMotionIndicatorAnimation)
            hudSetActionIndicatorAnimation(selected);
        animationIntegerTrackEvaluate((ValueTrack *)&gHudMotionIndicatorTrack, &gHudActionIndicatorSprites[0].texture,
                                      gRuntimeFrameSeconds);
        gDPPipeSync((*gdl)++);
        gSPSetOtherMode((*gdl)++, G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, 0x504240);
        gDPSetCombineMode((*gdl)++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 136);
        textureDrawRectangles(5, (TextureDrawC354 *)(void *)gHudActionIndicatorSprites, gdl);
        if (!showMotion)
            return;
    }
    {
        s32 oldBand;
        s32 newBand;
        oldBand = hudGetVerticalMotionAnimation((s32)motion->y);
        dx = motion->targetX * 0.32f - motion->x;
        dy = motion->targetY * 0.32f - motion->y;
        if (gHudHorizontalMotionRemaining > 0.0f)
            gHudHorizontalMotionRemaining -= gRuntimeFrameSeconds;
        step = gRuntimeFrameSeconds * 32.0f;
        negativeStep = -step;
        if (dx < negativeStep)
            dx = negativeStep;
        else if (step < dx)
            dx = step;
        if (dy < negativeStep)
            dy = negativeStep;
        else if (step < dy)
            dy = step;
        motion->x += dx;
        motion->y += dy;
        newBand = hudGetVerticalMotionAnimation((s32)motion->y);
        if (oldBand != newBand)
            animationIntegerTrackInitialize((ValueTrack *)(f32 *)&gHudVerticalMotionTrack, gTextureFrameAnimationPrograms[newBand]);
    }
    x = (s32)(motion->x + 0.5f);
    y = (s32)(motion->y + 0.5f);
    if (x < 0)
        x = 0;
    else if (x >= 32)
        x = 31;
    if (y < 0)
        y = 0;
    else if (y >= 32)
        y = 31;
    animationIntegerTrackEvaluate((ValueTrack *)&gHudVerticalMotionTrack, &frameY, gRuntimeFrameSeconds);
    animationIntegerTrackEvaluate((ValueTrack *)&gHudHorizontalMotionTrack, &frameX, gRuntimeFrameSeconds);
    gDPPipeSync((*gdl)++);
    gSPSetOtherMode((*gdl)++, G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, 0x504240);
    gDPSetCombineMode((*gdl)++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor((*gdl)++, 0, 0, 255, 255, 255, 160);
    if (gHudHorizontalMotionRemaining > 0.0f) {
        gHudHorizontalMotionSprites[0].t = x;
        gHudHorizontalMotionSprites[1].texture = frameX;
        textureDrawRectangles(5, (TextureDrawC354 *)(void *)gHudHorizontalMotionSprites, gdl);
        return;
    }
    gHudVerticalMotionSprites[0].t = y;
    gHudVerticalMotionSprites[1].texture = frameY;
    textureDrawRectangles(5, (TextureDrawC354 *)(void *)gHudVerticalMotionSprites, gdl);
}

char *areaGetDisplayName(s32);                        /* extern */
/// Owned text-image handle for the current area name.
/// \note Original identifier: \c D_80089460 (0x80089460).
extern s32 gHudAreaLabelImage;
/// Identifier of the currently loaded area.
/// \note Original identifier: \c D_800AE4F8 (0x800AE4F8).
extern s32 gRuntimeCurrentArea;

/// Prepare the current area name for HUD display.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022D78 (0x80022D78).
void hudInitializeAreaLabel(void) {
    gHudAreaLabelImage = 0;
    textImageCreateFromText((TextImage **)(&gHudAreaLabelImage), (u8 *)(areaGetDisplayName(gRuntimeCurrentArea)),
                            gTextImageConfigurations);
}

/// Nonzero when the prepared current-area label is visible.
/// \note Original identifier: \c D_800F0278 (0x800F0278).
extern s32 gHudAreaLabelVisible;
/// Draw the current area name when its label is visible.
/// Requires a prepared area-label image when the label is shown.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022DBC (0x80022DBC).
void hudDrawAreaLabel(Gfx **gdl) {
    if (gHudAreaLabelVisible != 0) {
        gDPPipeSync((*gdl)++);
        gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
        gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPSetCombineMode((*gdl)++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
        gDPSetPrimColor((*gdl)++, 0, 0, 0, 0, 0, 0xB4);
        gDPFillRectangle((*gdl)++, 53, 97, 266, 143);
        textDrawBegin(*gdl);
        textDrawSetFont(0);
        textDrawReset();
        textDrawSetPalette(gDefaultTextPalette);
        textDrawSetArea(53, 97, 266, 143);
        textDrawSetCursor((213 - ((TextImage *)gHudAreaLabelImage)->width) / 2,
                          (46 - ((TextImage *)gHudAreaLabelImage)->height) / 2);
        textDrawSetColor(255, 255, 255, 254);
        textDrawImageAtCursor((TextImage *)gHudAreaLabelImage);
        textDrawGetDisplayListCursor(gdl);
        textDrawReset();
        textDrawSetPalette(gDefaultTextPalette);
    }
}


/// Set whether the current area name is visible.
/// \param visible Nonzero to show the area label, or zero to hide it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022F68 (0x80022F68).
void hudSetAreaLabelVisible(s32 visible) {
    gHudAreaLabelVisible = visible;
}

/// Nonzero when alternate overlay drawing replaces normal HUD drawing.
/// \note Original identifier: \c D_800F027C (0x800F027C).
extern s32 gHudAlternateOverlayEnabled;

/// Select the alternate HUD overlay, disabling microphone input when enabled.
/// \param enabled Nonzero to select the alternate overlay and disable microphone input, or zero to restore normal HUD drawing without re-enabling microphone input.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022F74 (0x80022F74).
void hudSetOverlayMode(s32 enabled) {
    gHudAlternateOverlayEnabled = enabled;
    if (enabled != 0) {
        micRequestRecognitionOverride(0);
        micRequestRecognitionFollowsTrigger(0);
        gGameplayMicrophoneAvailable = 0;
        gGameplayVoiceCommandsAllowed = 0;
    }
}

/// Nonzero when the auxiliary HUD icon is visible.
/// \note Original identifier: \c D_800F0280 (0x800F0280).
extern s32 gHudAuxiliaryIconVisible;

/// Set whether the auxiliary HUD icon is visible.
/// \param visible Nonzero to show the auxiliary icon, or zero to hide it.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022FB8 (0x80022FB8).
void hudSetAuxiliaryIconVisible(s32 visible) {
    gHudAuxiliaryIconVisible = visible;
}


/// Select which movement and microphone HUD indicators are enabled.
/// \param features Feature mask: bit zero enables the selected action indicator, bit one enables movement indicators, and bit two enables microphone indicators.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022FC4 (0x80022FC4).
void hudSetFeatureMask(s32 features) {
    gHudEnabledFeatures = features;
}

/// Draw the currently enabled HUD indicators and overlays.
/// \param gdl Display-list cursor to append drawing commands to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80022FD0 (0x80022FD0).
void hudDraw(Gfx **gdl) {
    gDPPipeSync((*gdl)++);
    gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
    if (gHudAlternateOverlayEnabled != 0) {
        hudDrawAlternateOverlay(gdl);
    } else {
        if (gHudGuideDrawCallback != NULL) {
            gHudGuideDrawCallback(gdl);
        }
        if (gHudEnabledFeatures != 0) {
            hudDrawMotionIndicators(gdl);
            hudDrawMicrophoneIndicators(gdl);
        }
    }
    if (gHudAuxiliaryIconVisible != 0) {
        hudDrawAuxiliaryIcon(gdl);
    }
}

void hudInitializeMotionIndicators(void);

/// Initialize HUD animations and the area label and restore the default visibility settings.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80023098 (0x80023098).
void hudInitialize(void) {
    hudResetMicrophoneIndicators();
    hudInitializeDirectionIndicators();
    hudSetActionIndicatorAnimation(0xF2);
    hudInitializeMotionIndicators();
    hudInitializeAreaLabel();
    hudSetAreaLabelVisible(0);
    hudSetOverlayMode(0);
    hudSetAuxiliaryIconVisible(0);
    hudSetFeatureMask(7);
    hudSetGuideStyle(0);
}


/// Keep the horizontal movement indicator visible for at least the requested remaining duration.
/// \param seconds Nonnegative minimum remaining horizontal-indicator display time in seconds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80023100 (0x80023100).
void hudExtendHorizontalMotionDisplay(f32 seconds) {
    if (gHudHorizontalMotionRemaining <= seconds) {
        gHudHorizontalMotionRemaining = seconds;
    }
}
