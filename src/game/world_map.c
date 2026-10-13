#include "controller_input.h"
#include "microphone_requests.h"
#include "audio_sound_effect.h"
#include "attached_effect.h"
#include "script_vm_globals.h"
#include "actor_behavior.h"
#include "progress_value.h"
#include "vector_interpolation.h"
#include "presentation_focus.h"
#include "vector3_types.h"
#include "matrix_types.h"
#include "matrix_point.h"
#include "camera_interface.h"
#include "action_continuation.h"
#include "action_script_stop.h"
#include "action_script_playback.h"
#include "action_script_query.h"
#include "camera_overlay.h"
#include "actor_rotation.h"
#include "object_flags.h"
#include "hud_draw.h"
#include "backdrop_draw.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "runtime_clock.h"
#include "actor_parts.h"
#include "audio_cue.h"
#include "actor_target.h"
#include "actor_script.h"

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

/// Texture rectangle used by destination artwork and locked markers.
typedef struct DrawRectangle {
    s32 texture;        ///< Texture identifier; negative terminates a drawing list.
    s32 x, y;           ///< Destination origin in menu pixels.
    s32 width, height;  ///< Source rectangle dimensions.
    s32 s, t;           ///< Source texture coordinates.
    f32 scaleX, scaleY; ///< Drawing scale along each axis.
} DrawRectangle;

/// World-map destination choices, presentation focus and menu artwork.
typedef struct WorldMapDestination {
    s32 areaIds[3];       ///< Area identifier for each chase stage; negative disables that stage.
    s32 availabilityFlag; ///< Progress flag required for selection, or -1 for unconditional availability.
    Vec3f focusPosition;  ///< Actor look-at point in presentation coordinates.
    DrawRectangle icon;   ///< Destination icon and its menu rectangle.
    s32 badgeTextures[3]; ///< Badge texture for each chase stage; negative hides the badge.
} WorldMapDestination;

typedef char WorldMapDestinationSizeCheck[(sizeof(WorldMapDestination) == 0x4C) ? 1 : -1];
typedef char WorldMapDestinationFocusOffsetCheck[((u32) & ((WorldMapDestination *)0)->focusPosition == 0x10) ? 1 : -1];
typedef char WorldMapDestinationIconOffsetCheck[((u32) & ((WorldMapDestination *)0)->icon == 0x1C) ? 1 : -1];
typedef char WorldMapDestinationBadgeOffsetCheck[((u32) & ((WorldMapDestination *)0)->badgeTextures == 0x40) ? 1 : -1];
/// Destination records containing area choices, unlock requirements, actor focus points and artwork.
/// \note Original identifier: \c D_80099DC0 (0x80099DC0).
extern WorldMapDestination gWorldMapDestinations[];

/// Presentation focus point at the start of the current transition.
/// \note Original identifier: \c D_800F1BB0 (0x800F1BB0).
extern Vec3f gPresentationFocusStart;
/// Requested presentation focus point at the end of the current transition.
/// \note Original identifier: \c D_800F1BC0 (0x800F1BC0).
extern Vec3f gPresentationFocusTarget;
/// Most recently evaluated presentation focus point.
/// \note Original identifier: \c D_800F1BD0 (0x800F1BD0).
extern Vec3f gPresentationFocusCurrent;
/// Elapsed-millisecond checkpoint for the current focus transition.
/// \note Original identifier: \c D_800F1BDC (0x800F1BDC).
extern u32 gPresentationFocusTransitionStartMilliseconds;

/// Begin a smooth 0.6-second transition of the shared presentation focus point.
/// \param target Target point in presentation coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D260 (0x8002D260).
void presentationSetFocusTarget(Vec3f *target) {
    gPresentationFocusStart.x = gPresentationFocusCurrent.x;
    gPresentationFocusStart.y = gPresentationFocusCurrent.y;
    gPresentationFocusStart.z = gPresentationFocusCurrent.z;
    gPresentationFocusTarget.x = target->x;
    gPresentationFocusTarget.y = target->y;
    gPresentationFocusTarget.z = target->z;
    gPresentationFocusTransitionStartMilliseconds = (u32)runtimeGetElapsedMilliseconds();
}

/// Set the shared presentation focus point immediately.
/// \param point Focus point in presentation coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D2C8 (0x8002D2C8).
void presentationResetFocusPoint(Vec3f *point) {
    gPresentationFocusStart.x = point->x;
    gPresentationFocusStart.y = point->y;
    gPresentationFocusStart.z = point->z;
    gPresentationFocusTarget.x = point->x;
    gPresentationFocusTarget.y = point->y;
    gPresentationFocusTarget.z = point->z;
    gPresentationFocusCurrent.x = point->x;
    gPresentationFocusCurrent.y = point->y;
    gPresentationFocusCurrent.z = point->z;
    gPresentationFocusTransitionStartMilliseconds = (u32)runtimeGetElapsedMilliseconds();
}


/// Update and obtain the shared presentation focus point during its transition.
/// \param point Receives the current point in presentation coordinates.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D348 (0x8002D348).
void presentationUpdateFocusPoint(Vec3f *point) {
    vector3InterpolateSmoothTimed((f32 *)&gPresentationFocusStart, (f32 *)&gPresentationFocusTarget, (f32 *)point, 600.0f,
                                  (f32)runtimeGetElapsedMilliseconds() -
                                      (f32)gPresentationFocusTransitionStartMilliseconds);
    gPresentationFocusCurrent.x = point->x;
    gPresentationFocusCurrent.y = point->y;
    gPresentationFocusCurrent.z = point->z;
}

u8 playerGetChaseStage(void);

/// Check whether a map destination can be selected at the current difficulty.
/// \param destination Valid index in the map-destination table.
/// \returns Nonzero if the destination exists and its progress requirement is satisfied, or zero otherwise.
/// \note Original decompilation identifier: \c func_8002D3DC (0x8002D3DC).
s32 wholeMapIsDestinationAvailable(s32 destination) {
    s32 availabilityFlag;

    if (gWorldMapDestinations[destination].areaIds[playerGetChaseStage()] < 0) {
        return 0;
    }
    availabilityFlag = gWorldMapDestinations[destination].availabilityFlag;
    if (availabilityFlag != -1) {
        return progressGetValue(availabilityFlag);
    }
    return 1;
}

/* textureDrawRectangles list: one drawn record, then the template's negative-texture
 * terminator record. */
typedef struct {
    DrawRectangle item;
    DrawRectangle end;
} DrawList474;
/// Texture rectangle template for the selected destination highlight.
/// \note Original identifier: \c D_8009A11C (0x8009A11C).
extern DrawList474 gWorldMapSelectionTemplate;
/// Texture rectangle template for available destination icons.
/// \note Original identifier: \c D_8009A164 (0x8009A164).
extern DrawList474 gWorldMapDestinationTemplate;
/// Texture rectangle template for destination badges.
/// \note Original identifier: \c D_8009A1AC (0x8009A1AC).
extern DrawList474 gWorldMapBadgeTemplate;
/// Current world-map menu and transition state.
/// \note Original identifier: \c D_8009A110 (0x8009A110).
extern s32 gWorldMapState;
/// Opacity of the pulsing selected-destination highlight.
/// \note Original identifier: \c D_8009A114 (0x8009A114).
extern s32 gWorldMapSelectionAlpha;
/// Signed opacity step for the selected-destination pulse.
/// \note Original identifier: \c D_8009A118 (0x8009A118).
extern s32 gWorldMapSelectionAlphaStep;
/// Index of the currently selected world-map destination.
/// \note Original identifier: \c D_800F1B74 (0x800F1B74).
extern s32 gWorldMapSelectedDestination;
void textureDrawRectangles(s32, DrawRectangle *, Gfx **);
s32 runtimeGetUiPulseAlpha(void);
/// Draw available map destinations, their badges, and the animated selection highlight.
/// \param gdl Display-list cursor to append to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D474 (0x8002D474).
void wholeMapDrawDestinations(Gfx **gdl) {
    DrawList474 selection = gWorldMapSelectionTemplate;
    DrawList474 icon = gWorldMapDestinationTemplate;
    DrawList474 badge = gWorldMapBadgeTemplate;
    s32 rendererTable = 7;
    s32 index;
    s32 lastIndex;
    s32 texture;
    s32 change;
    s32 alpha;
    gDPPipeSync((*gdl)++);
    gSPSetOtherMode((*gdl)++, G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, 0x504240);
    gDPSetCombine((*gdl)++, 0xFF97FF, 0xFF2CFE7F);
    gDPSetPrimColor((*gdl)++, 0, 0, 255, 0, 0, gWorldMapSelectionAlpha);
    switch (gWorldMapSelectedDestination) {
    case 2:
    case 5:
    case 7:
        selection.item.texture = 37;
        break;
    default:
        selection.item.texture = 16;
        break;
    }
    selection.item.x = gWorldMapDestinations[gWorldMapSelectedDestination].icon.x;
    selection.item.y = gWorldMapDestinations[gWorldMapSelectedDestination].icon.y;
    textureDrawRectangles(rendererTable, &selection.item, gdl);
    if (gWorldMapState == 3) {
        change = gWorldMapSelectionAlphaStep * 16;
    } else {
        change = gWorldMapSelectionAlphaStep;
    }
    gWorldMapSelectionAlpha += change;
    if (gWorldMapSelectionAlpha >= 256) {
        gWorldMapSelectionAlpha = 255;
        gWorldMapSelectionAlphaStep = -gWorldMapSelectionAlphaStep;
    } else if (gWorldMapSelectionAlpha < 0) {
        gWorldMapSelectionAlpha = 0;
        gWorldMapSelectionAlphaStep = -gWorldMapSelectionAlphaStep;
    }
    lastIndex = 8;
    gDPPipeSync((*gdl)++);
    gSPSetOtherMode((*gdl)++, G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, 0x504240);
    gDPSetCombine((*gdl)++, 0xFFFFFF, 0xFFFCF279);
    for (index = 0; index <= lastIndex; index++) {
        if (wholeMapIsDestinationAvailable(index)) {
            icon.item = gWorldMapDestinations[index].icon;
            textureDrawRectangles(rendererTable, &icon.item, gdl);
            texture = gWorldMapDestinations[index].badgeTextures[playerGetChaseStage()];
            if (texture >= 0) {
                if (index == 4) {
                    badge.item.x = icon.item.x - 27;
                } else {
                    badge.item.x = icon.item.x + 55;
                }
                alpha = runtimeGetUiPulseAlpha() & 255;
                badge.item.y = (s32)(((index & 1) ? 0.01f : -0.01f) * (f32)(u32)alpha + (f32)icon.item.y);
                badge.item.texture = texture;
                textureDrawRectangles(rendererTable, &badge.item, gdl);
            }
        }
    }
}

s32 wholeMapIsDestinationAvailable(s32); /* extern */

/// Check whether a map destination exists but remains locked at the current difficulty.
/// \param destination Valid index in the map-destination table.
/// \returns One if the destination exists but is unavailable, or zero otherwise.
/// \note Original decompilation identifier: \c func_8002D88C (0x8002D88C).
s32 wholeMapIsDestinationLocked(s32 destination) {
    if (gWorldMapDestinations[destination].areaIds[playerGetChaseStage()] < 0) {
        return 0;
    }
    if (wholeMapIsDestinationAvailable(destination) != 0) {
        return 0;
    }
    return 1;
}

f32 mathSinDegrees(f32);
/// Animation angle in degrees for the locked-destination markers.
/// \note Original identifier: \c D_8009A1F4 (0x8009A1F4).
extern s32 gWorldMapLockedMarkerAngle;

/// Draw and animate the markers for locked map destinations.
/// \param gdl Display-list cursor to append to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D904 (0x8002D904).
void wholeMapDrawLockedDestinations(Gfx **gdl) {
    DrawRectangle rectangles[3];
    s32 i;
    s32 x, y;
    s32 angle;
    WorldMapDestination *row;
    s32 *counter;

    counter = &gWorldMapLockedMarkerAngle;
    for (i = 0; i != 2; i++) {
        rectangles[i].width = 64;
        rectangles[i].height = 64;
        rectangles[i].s = 0;
        rectangles[i].t = 0;
        rectangles[i].scaleX = 1.0f;
        rectangles[i].scaleY = 1.0f;
    }
    for (i = 0; i < 9; i++) {
        if (wholeMapIsDestinationLocked(i)) {
            row = &gWorldMapDestinations[i];
            x = row->icon.x;
            y = row->icon.y;
            rectangles[0].texture = 6;
            rectangles[1].texture = 7;
            rectangles[2].texture = -1;
            rectangles[0].x = x - 16;
            rectangles[0].y = y + 8;
            rectangles[1].x = rectangles[0].x + rectangles[0].width;
            rectangles[1].y = rectangles[0].y;
            gDPPipeSync((*gdl)++);
            gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
            gDPSetCombineLERP((*gdl)++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE,
                              0);
            gDPSetPrimColor((*gdl)++, 0, 0, 0, 0, 0, 128);
            textureDrawRectangles(7, rectangles, gdl);
        }
    }
    for (i = 0; i < 9; i++) {
        if (!wholeMapIsDestinationLocked(i)) {
            continue;
        }
        row = &gWorldMapDestinations[i];
        x = row->icon.x;
        angle = *counter;
        y = (s32)(mathSinDegrees(angle) * 2.5);
        y += row->icon.y;
        rectangles[0].texture = 8;
        rectangles[1].texture = 9;
        rectangles[2].texture = -1;
        rectangles[0].x = x - 16;
        rectangles[0].y = y - 8;
        rectangles[1].x = rectangles[0].x + rectangles[0].width;
        rectangles[1].y = rectangles[0].y;
        gDPPipeSync((*gdl)++);
        gDPSetCombineMode((*gdl)++, G_CC_DECALRGBA, G_CC_DECALRGBA);
        textureDrawRectangles(7, rectangles, gdl);
    }
    angle = (s32)((u32)*counter + 5U);
    while (angle < 0)
        angle += 360;
    *counter = angle;
    if (angle >= 360) {
        do {
            angle -= 360;
        } while (angle >= 360);
        *counter = angle;
    }
}

/// Texture rectangle lists indexed by the current chase stage.
/// \note Original identifier: \c D_8009A1F8 (0x8009A1F8).
extern u8 gWorldMapDifficultyOverlays[];

/// Draw the map overlay for the current difficulty.
/// \param gdl Display-list cursor to append to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002DBE0 (0x8002DBE0).
void wholeMapDrawDifficultyOverlay(Gfx **gdl) {
    gDPPipeSync((*gdl)++);
    gDPSetRenderMode((*gdl)++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineLERP((*gdl)++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetPrimColor((*gdl)++, 0, 0, 0, 0, 0, 0xC0);
    textureDrawRectangles(7, (DrawRectangle *)((playerGetChaseStage() * 0x90) + gWorldMapDifficultyOverlays), gdl);
}

void cameraUpdateOverlayColors(s32, s32); /* extern */
void cameraDrawSecondaryOverlay(s32);      /* extern */
void objectDrawGroup(s32);      /* extern */
void wholeMapDrawDestinations(Gfx **);          /* extern */
void wholeMapDrawLockedDestinations(Gfx **);          /* extern */
void wholeMapDrawDifficultyOverlay(Gfx **);          /* extern */
/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;

/// Draw the whole-map destination selection screen.
/// \param gdl Display-list cursor to append to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002DCA0 (0x8002DCA0).
void wholeMapDraw(Gfx **gdl) {
    backdropDraw(gdl);
    gDPPipeSync((*gdl)++);
    gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
    wholeMapDrawDestinations(gdl);
    wholeMapDrawLockedDestinations(gdl);
    objectDrawGroup(0x2000);
    gDPPipeSync((*gdl)++);
    gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
    wholeMapDrawDifficultyOverlay(gdl);
    hudDrawMicrophoneIndicators(gdl);
    cameraUpdateOverlayColors(gRenderCameraIndex, 0);
    cameraDrawSecondaryOverlay(0);
}

/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern s32 gPikachuActor;
/// Transform from map presentation coordinates to world coordinates.
/// \note Original identifier: \c D_800F1B78 (0x800F1B78).
extern Matrix34 gWorldMapToWorldTransform;
/// Camera-relative actor position on the world-map screen.
/// \note Original identifier: \c D_8009A104 (0x8009A104).
extern Vec3f gWorldMapActorPresentationPosition;

/// Update the map actor's position and focus while preserving its ground clearance.
/// \pre Requires an active actor and the map presentation transform.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002DD70 (0x8002DD70).
void wholeMapUpdateActorPlacement(void) {
    Vec3f position;
    s32 firstJointId = 0x47;
    s32 secondJointId = 0x4C;
    f32 averageDivisor = 2.0f;
    Vec3f reference;
    f32 height;
    f32 otherHeight;
    u8 *firstJoint;
    u8 *secondJoint;
    u8 *state;
    state = *(u8 **)((u8 *)gPikachuActor + 0x54);
    presentationUpdateFocusPoint(&reference);
    matrix34TransformPointVector((f32 *)(&gWorldMapToWorldTransform), (f32 *)(&reference), (f32 *)(state + 0x1EC));
    reference.x = gWorldMapActorPresentationPosition.x;
    reference.y = gWorldMapActorPresentationPosition.y;
    reference.z = gWorldMapActorPresentationPosition.z;
    matrix34TransformPointVector((f32 *)(&gWorldMapToWorldTransform), (f32 *)(&reference), (f32 *)(&position));
    firstJoint = actorFindPart((u8 *)gPikachuActor, firstJointId);
    height = *(f32 *)((u8 *)gPikachuActor + 0x48) - *(f32 *)(firstJoint + 0x48);
    secondJoint = actorFindPart((u8 *)gPikachuActor, secondJointId);
    otherHeight = (*(f32 *)((u8 *)gPikachuActor + 0x48) - *(f32 *)(secondJoint + 0x48));
    position.y += (otherHeight + height) / averageDivisor;
    *(f32 *)((u8 *)gPikachuActor + 0x18) = position.x;
    *(f32 *)((u8 *)gPikachuActor + 0x1C) = position.y;
    *(f32 *)((u8 *)gPikachuActor + 0x20) = position.z;
    *(f32 *)((u8 *)gPikachuActor + 0x44) = position.x;
    *(f32 *)((u8 *)gPikachuActor + 0x48) = position.y;
    *(f32 *)((u8 *)gPikachuActor + 0x4C) = position.z;
}

/// Index of the currently selected world-map destination.
/// \note Original identifier: \c D_800F1B74 (0x800F1B74).
extern s32 gWorldMapSelectedDestination;
/// Consecutive map input updates with directional buttons held, used to select the actor pose.
/// \note Original identifier: \c D_800F1B70 (0x800F1B70).
extern s32 gWorldMapDirectionHeldFrames;
/// Camera storage used for view transforms, viewports and color overlays.
/// \note Original identifier: \c D_800ABCF0 (0x800ABCF0).
extern u8 gRenderCameras[];
void wholeMapUpdateActorPlacement(void);
f32 vector3GetYawDegrees(f32 *);

/// Prepare the active actor to face the camera and track the selected map destination.
/// \pre Requires an active actor, camera, and selected map destination.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002DE94 (0x8002DE94).
void wholeMapPrepareActor(void) {
    s32 action = 0x57;
    s32 actionMode = 2;
    s32 animation = 0xC;
    f32 delta[3];
    s32 flags = 0x6000;
    u8 *camera;
    s32 pitchAxis = 0;
    f32 angle;
    u8 *state;
    state = *(u8 **)((u8 *)gPikachuActor + 0x54);
    camera = gRenderCameras + gRenderCameraIndex * 0xE0;
    objectSetHierarchyFlags((u8 *)gPikachuActor, flags);
    wholeMapUpdateActorPlacement();
    actionScriptStop((u8 *)gPikachuActor);
    actionStopPlayback((u8 *)gPikachuActor);
    delta[0] = *(f32 *)(camera + 0) - *(f32 *)((u8 *)gPikachuActor + 0x44);
    delta[1] = *(f32 *)(camera + 4) - *(f32 *)((u8 *)gPikachuActor + 0x48);
    delta[2] = *(f32 *)(camera + 8) - *(f32 *)((u8 *)gPikachuActor + 0x4C);
    angle = vector3GetYawDegrees(delta);
    actorAddRotationAxisDegrees((u8 *)gPikachuActor, -*(f32 *)(state + 0xC), pitchAxis);
    actorAddRotationAxisDegrees((u8 *)gPikachuActor, angle - *(f32 *)(state + 0x10), 1);
    *(s32 *)(state + 0x19C) = 0;
    matrix34TransformPointVector((f32 *)(&gWorldMapToWorldTransform), (f32 *)&gWorldMapDestinations[gWorldMapSelectedDestination].focusPosition, (f32 *)(state + 0x1EC));
    presentationResetFocusPoint(&gWorldMapDestinations[gWorldMapSelectedDestination].focusPosition);
    gWorldMapDirectionHeldFrames = 0;
    actionScriptStartWithMode((u8 *)gPikachuActor, action, actionMode);
    actorSetBehaviorInhibitFlags((u8 *)gPikachuActor, animation);
}

/// Current world-map menu and transition state.
/// \note Original identifier: \c D_8009A110 (0x8009A110).
extern s32 gWorldMapState;
/// Elapsed-millisecond checkpoint when the current map state began.
/// \note Original identifier: \c D_800F1BA8 (0x800F1BA8).
extern u32 gWorldMapStateStartMilliseconds;

/// Enter a whole-map menu state and restart its elapsed-time measurement.
/// \param state Menu state to enter: zero initializes, one fades in, two accepts input, three confirms, four cancels, and five departs.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002E020 (0x8002E020).
void wholeMapSetState(s32 state) {
    gWorldMapStateStartMilliseconds = (u32)runtimeGetElapsedMilliseconds();
    gWorldMapState = state;
}

s32 randomBelow(s32);

#define PLAYER (*(volatile s32 *)&gPikachuActor)

/// Play the active actor's confirmation animation for the selected destination.
/// \pre Requires an active actor and selected map destination.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002E050 (0x8002E050).
void wholeMapPlayConfirmationAnimation(void) {
    s32 form;

    actionScriptStop((u8 *)PLAYER);
    form = gWorldMapDestinations[gWorldMapSelectedDestination].areaIds[playerGetChaseStage()];
    if (form == 9 || form == 10) {
        actionScriptStart((u8 *)PLAYER, 0x2C);
        return;
    }
    switch (randomBelow(3)) {
    case 0:
        *(s16 *)(*(u8 **)(PLAYER + 0x54) + 0x13C) = 3;
        actionScriptStart((u8 *)PLAYER, 0x39);
        break;
    case 1:
        actionScriptStart((u8 *)PLAYER, 0x3F);
        break;
    case 2:
        actionScriptStart((u8 *)PLAYER, 0x54);
        break;
    }
}

#undef PLAYER

/// Advance an unavailable map selection until an available destination is found.
/// \param min First valid destination index in the inclusive selection range, no greater than max.
/// \param max Last destination index in the inclusive selection range.
/// \param step Direction and amount to advance, wrapping at the range bounds; zero uses one.
/// \pre Advancing through the range must reach an available destination.
/// \returns Nothing.
///
/// \note NONPORTABLE: the debug retry check has a compiled-out body; its original threshold is not recoverable from the ROM.
/// \note Original decompilation identifier: \c func_8002E150 (0x8002E150).
void wholeMapEnsureAvailableSelection(s32 min, s32 max, s32 step) {
    s32 id;
    s32 tries;

    tries = 0;
    if (step == 0) {
        step = 1;
    }
    if (wholeMapIsDestinationAvailable(gWorldMapSelectedDestination) == 0) {
        do {
            id = gWorldMapSelectedDestination + step;
            gWorldMapSelectedDestination = id;
            if (max < id) {
                gWorldMapSelectedDestination = min;
                id = min;
            } else if (id < min) {
                gWorldMapSelectedDestination = max;
                id = max;
            }
            tries++;
            if (tries >= 1000) {
            }
        } while (wholeMapIsDestinationAvailable(id) == 0);
    }
}

void wholeMapEnsureAvailableSelection(s32, s32, s32);
typedef struct MenuController2E {
    u32 held, pressed, released, repeated;
    s32 stickX, stickY;
} MenuController2E;
/// Processed button and stick samples for all controller ports; scalar declarations view the primary held-button word.
/// \note Original identifier: \c D_800AD540 (0x800AD540).
extern MenuController2E gControllerInputState[4];
/// Primary controller new and auto-repeated button presses.
/// \note Original identifier: \c D_800AD54C (0x800AD54C).
extern s32 gControllerPrimaryRepeatedButtons;
/// Consecutive map input updates with directional buttons held, used to select the actor pose.
/// \note Original identifier: \c D_800F1B70 (0x800F1B70).
extern s32 gWorldMapDirectionHeldFrames;
/// Presentation focus target for the special destination selected by voice.
/// \note Original identifier: \c D_8009A3A8 (0x8009A3A8).
extern Vec3f gWorldMapSpecialDestinationFocus;
/// Presentation focus target used for actor messages other than destination selection.
/// \note Original identifier: \c D_8009A3B4 (0x8009A3B4).
extern Vec3f gWorldMapOtherMessageFocus;
/// Last destination whose selection change updated the actor focus and highlight.
/// \note Original identifier: \c D_8009A3C0 (0x8009A3C0).
extern s32 gWorldMapPreviousDestination;
/// Mapping from recognized destination values to map destination indices.
/// \note Original identifier: \c D_8009A3C4 (0x8009A3C4).
extern s32 gWorldMapVoiceDestinationIndices[];
/// Opacity of the pulsing selected-destination highlight.
/// \note Original identifier: \c D_8009A114 (0x8009A114).
extern s32 gWorldMapSelectionAlpha;
typedef struct {
    s32 type;
    s32 value;
} MenuInput;
/// Handle destination selection, confirmation, and cancellation from controller and voice input.
/// \pre Requires the whole-map menu and its active actor.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002E200 (0x8002E200).
void wholeMapHandleInput(void) {
    s32 *selected;
    s32 *pressed;
    s32 confirmSound = 0xC5;
    s32 direction;
    MenuInput *input;
    s32 selection;
    Vec3f specialPosition;
    Vec3f defaultPosition;
    s32 confirmMask;
    pressed = &gControllerPrimaryRepeatedButtons;
    if (*pressed & 0x4000) {
        cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFE, 0.6f);
        audioChangeCurrentCue(-1, 0.1, 1.0);
        wholeMapSetState(4);
        return;
    }
    confirmMask = *pressed & 0x8000;
    if (confirmMask) {
        audioPlaySoundEffect(confirmSound);
        TRACE("voice %d\n", confirmSound);
        wholeMapPlayConfirmationAnimation();
        wholeMapSetState(3);
        return;
    }
    if (gControllerInputState[0].held & 0xF00) {
        gWorldMapDirectionHeldFrames++;
    } else {
        gWorldMapDirectionHeldFrames = 0;
    }
    if (actorAcknowledgeScriptMessage((u8 *)gPikachuActor) != 0) {
        input = (MenuInput *)(*(u8 **)((u8 *)gPikachuActor + 0x54) + 0x21C);
        if (input->type == 0x12) {
            if (input->value == 10 && playerGetChaseStage() == 2) {
                specialPosition = gWorldMapSpecialDestinationFocus;
                gWorldMapSelectedDestination = 10;
                presentationSetFocusTarget(&specialPosition);
                audioPlaySoundEffect(confirmSound);
                wholeMapPlayConfirmationAnimation();
                wholeMapSetState(3);
                return;
            }
            if (input->value < 0) {
                input->value = 0;
            } else if (input->value >= 10) {
                input->value = 9;
            }
            selection = gWorldMapVoiceDestinationIndices[input->value];
            if (wholeMapIsDestinationAvailable(selection) != 0) {
                gWorldMapSelectedDestination = selection;
                attachedEffectCreate((u8 *)gPikachuActor, 1, 0xA4);
            }
        } else {
            defaultPosition = gWorldMapOtherMessageFocus;
            presentationSetFocusTarget(&defaultPosition);
        }
    }
    selected = &gWorldMapSelectedDestination;
    direction = 0;
    if (gControllerPrimaryRepeatedButtons & 0x500) {
        direction = 1;
        gWorldMapSelectedDestination++;
        audioPlaySoundEffect(0xAD);
    } else if (!(gControllerInputState[0].held & 0x500) && (gControllerPrimaryRepeatedButtons & 0xA00)) {
        direction = -1;
        gWorldMapSelectedDestination--;
        audioPlaySoundEffect(0xAD);
    }
    if (*selected < 0) {
        gWorldMapSelectedDestination = 8;
    } else if (*selected >= 9) {
        gWorldMapSelectedDestination = 0;
    }
    wholeMapEnsureAvailableSelection(0, 8, direction);
    if (gWorldMapPreviousDestination != *selected) {
        TRACE("index = %d, map = %d\n", *selected, gWorldMapDestinations[*selected].areaIds[0]);
        presentationSetFocusTarget(&gWorldMapDestinations[*selected].focusPosition);
        gWorldMapPreviousDestination = gWorldMapSelectedDestination;
        gWorldMapSelectionAlpha = 0xFF;
    }
}

/// Identifier of the currently loaded area.
/// \note Original identifier: \c D_800AE4F8 (0x800AE4F8).
extern u32 gRuntimeCurrentArea;

/// Choose an available initial map destination based on the current scene.
/// \pre Requires an available destination in the normal selection range.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002E538 (0x8002E538).
void wholeMapSelectInitialDestination(void) {
    switch (gRuntimeCurrentArea) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        gWorldMapSelectedDestination = 0;
        break;
    case 6:
    case 7:
    case 8:
        gWorldMapSelectedDestination = 3;
        break;
    case 11:
    case 12:
        gWorldMapSelectedDestination = 1;
        break;
    case 9:
    case 10:
        gWorldMapSelectedDestination = 8;
        break;
    case 13:
    case 14:
    case 16:
        gWorldMapSelectedDestination = 6;
        break;
    default:
        gWorldMapSelectedDestination = 0;
        break;
    }
    wholeMapEnsureAvailableSelection(0, 8, 1);
}

void hudSetFeatureMask(s32);      /* extern */
/// Whether gameplay exposes microphone interaction and its HUD indicator.
/// \note Original identifier: \c D_800722D0 (0x800722D0).
extern s32 gGameplayMicrophoneAvailable;
/// Whether gameplay permits recognized voice commands to launch messenger effects.
/// \note Original identifier: \c D_800722D4 (0x800722D4).
extern s32 gGameplayVoiceCommandsAllowed;

/// Configure the active actor and voice recognition for map destination selection.
/// \pre Requires an active actor.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002E5DC (0x8002E5DC).
void wholeMapConfigureVoiceInput(void) {
    TRACE("restart skit.\n");
    micRequestDictionary(0x1F);
    actorSetMessengerContactMode((u8 *)gPikachuActor, 2);
    hudSetFeatureMask(4);
    micRequestRecognitionOverride(1);
    micRequestRecognitionFollowsTrigger(1);
    gGameplayMicrophoneAvailable = 1;
    gGameplayVoiceCommandsAllowed = 1;
}

void backdropSelectForView(s32, s32, s32); /* extern */

/// Select the whole-map backdrop for the current difficulty.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002E634 (0x8002E634).
void wholeMapSelectBackdrop(void) {
    s32 chaseStage;

    chaseStage = playerGetChaseStage();
    switch (chaseStage) { /* irregular */
    case 0:
        backdropSelectForView(0, 0xC, 1);
        return;
    case 1:
        backdropSelectForView(0, 0xD, 1);
        return;
    case 2:
        backdropSelectForView(0, 0xE, 1);
        return;
    default:
        TRACE("Illegal difficulty:%d\n", chaseStage);
        backdropSelectForView(0, 0xE, 1);
        return;
    }
}

void objectUpdateGroup();
void runtimeRequestProgramChange();
void wholeMapUpdateActorPlacement();
void wholeMapPrepareActor();
void wholeMapSetState();
void wholeMapHandleInput();
void wholeMapSelectInitialDestination();
void wholeMapConfigureVoiceInput();
void wholeMapSelectBackdrop();
void toolboxClearCollection();
void areaSetNextDestination();
void playerResetViewOrientation();
void playerSetChaseStage();
/// Remaining duration of the primary camera secondary overlay transition.
/// \note Original identifier: \c D_800ABDCC (0x800ABDCC).
extern f32 gPrimarySecondaryOverlayRemainingSeconds;
/// Whether collection results were bypassed because no kept items or chapter result required presentation.
/// \note Original identifier: \c D_8009B584 (0x8009B584).
extern s32 gCollectionResultsBypassed;

/// Update the whole-map menu, its actor, and transitions into or out of destination selection.
/// \pre Requires an active actor.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002E6C4 (0x8002E6C4).
void wholeMapUpdate(void) {
    u8 *sp24;

    sp24 = *(u8 **)((u8 *)gPikachuActor + 0x54);
    playerResetViewOrientation();
    cameraBuildInterfaceTransform((f32 *)&gWorldMapToWorldTransform);
    if (actionScriptIsStopped((u8 *)gPikachuActor) != 0) {
        actionScriptStartWithMode((u8 *)gPikachuActor, 0x57, 2);
    }
    if (gWorldMapDirectionHeldFrames >= 0x65) {
        if (*(s16 *)(sp24 + 0x13C) != 0xE) {
            actionScriptStop((u8 *)gPikachuActor);
            actionScriptStartWithMode((u8 *)gPikachuActor, 0x57, 0xE);
        }
    } else if (*(s16 *)(sp24 + 0x13C) != 2) {
        actionScriptStop((u8 *)gPikachuActor);
        actionScriptStartWithMode((u8 *)gPikachuActor, 0x57, 2);
    }
    switch (gWorldMapState) {
    case 0:
        wholeMapConfigureVoiceInput();
        wholeMapSelectBackdrop();
        cameraResetOverlayColor(0, 0, 0, 0, 0);
        cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.4f);
        wholeMapSelectInitialDestination();
        wholeMapPrepareActor();
        controllerConfigureStickButtons(1, 0x1E, 0x1E, 0x1E, 0x1E);
        toolboxClearCollection();
        switch (playerGetChaseStage()) {
        case 0:
            audioChangeCurrentCue(9, 0.1, 0.1);
            break;
        case 1:
            audioChangeCurrentCue(0xA, 0.1, 0.1);
            break;
        case 2:
            audioChangeCurrentCue(8, 0.1, 0.1);
            break;
        }
        wholeMapSetState(1);
        TRACE("whole map initialized.\n");
        objectUpdateGroup(0x4000);
        return;
    case 1:
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            wholeMapSetState(2);
        }
        wholeMapUpdateActorPlacement();
        objectUpdateGroup(0x4000);
        return;
    case 2:
        wholeMapHandleInput();
        wholeMapUpdateActorPlacement();
        objectUpdateGroup(0x4000);
        return;
    case 3:
        if (runtimeGetElapsedMilliseconds() - (u64)gWorldMapStateStartMilliseconds >= 0x5DD) {
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFE, 0.6f);
            audioChangeCurrentCue(-1, 0.1, 1.0);
            wholeMapSetState(5);
        }
        wholeMapUpdateActorPlacement();
        objectUpdateGroup(0x4000);
        return;
    case 4:
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            scriptVmSetGlobalInteger(0, 1);
            if (playerGetChaseStage() == 0) {
                areaSetNextDestination(0, 2);
                runtimeRequestProgramChange(0);
            } else {
                if (playerGetChaseStage() == 1) {
                    areaSetNextDestination(2, 1);
                } else {
                    areaSetNextDestination(2, 2);
                }
                runtimeRequestProgramChange(0);
            }
            controllerConfigureStickButtons(0, 0x1E, 0x1E, 0x1E, 0x1E);
            playerSetChaseStage(progressGetValue(0xC));
            wholeMapSetState(0);
            TRACE("whole map canceled.\n");
        }
        wholeMapUpdateActorPlacement();
        objectUpdateGroup(0x4000);
        return;
    case 5:
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0f) {
            areaSetNextDestination(gWorldMapDestinations[gWorldMapSelectedDestination].areaIds[playerGetChaseStage()], 0);
            runtimeRequestProgramChange(0);
            controllerConfigureStickButtons(0, 0x1E, 0x1E, 0x1E, 0x1E);
            wholeMapSetState(0);
            TRACE("whole map finished.\n");
        }
        wholeMapUpdateActorPlacement();
        objectUpdateGroup(0x4000);
        return;
    default:
        return;
    }
}
