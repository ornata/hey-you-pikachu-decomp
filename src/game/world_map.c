#include "progress_field_ids.h"
#include "area_catalog.h"
#include "runtime_control.h"
#include "runtime_lifecycle.h"
#include "controller_state.h"
#include "actor_script.h"
#include "player_chase.h"
#include "texture_rectangles.h"
#include "pikachu_actor.h"
#include "camera_storage.h"
#include "object_groups.h"
#include "math_angles.h"
#include "random.h"
#include "backdrop_view.h"
#include "area_transition.h"
#include "controller_input.h"
#include "microphone_requests.h"
#include "voice_recognition.h"
#include "script_message.h"
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

/* Debug trace: compiled out in this build, but each format string is still
 * emitted to .rodata. */
// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on


/// World-map destination choices, presentation focus and menu artwork.
typedef struct WorldMapDestination {
    s32 areaIds[3];       ///< Area identifier for each chase stage; negative disables that stage.
    s32 availabilityFlag; ///< Progress flag required for selection, or -1 for unconditional availability.
    Vec3f focusPosition;  ///< Actor look-at point in presentation coordinates.
    TextureRectangle icon; ///< Destination icon and its menu rectangle.
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
    vector3InterpolateSmoothTimed(
        (f32 *)&gPresentationFocusStart, (f32 *)&gPresentationFocusTarget, (f32 *)point, 600.0F,
        (f32)runtimeGetElapsedMilliseconds() - (f32)gPresentationFocusTransitionStartMilliseconds);
    gPresentationFocusCurrent.x = point->x;
    gPresentationFocusCurrent.y = point->y;
    gPresentationFocusCurrent.z = point->z;
}


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

/// One map sprite followed by its negative-texture list terminator.
typedef struct WorldMapSpriteList {
    TextureRectangle item; ///< Visible destination, badge or selection sprite.
    TextureRectangle end;  ///< Negative-texture terminator.
} WorldMapSpriteList;
typedef char WorldMapSpriteListSizeCheck[(sizeof(WorldMapSpriteList) == 0x48) ? 1 : -1];
/// Texture rectangle template for the selected destination highlight.
/// \note Original identifier: \c D_8009A11C (0x8009A11C).
extern WorldMapSpriteList gWorldMapSelectionTemplate;
/// Texture rectangle template for available destination icons.
/// \note Original identifier: \c D_8009A164 (0x8009A164).
extern WorldMapSpriteList gWorldMapDestinationTemplate;
/// Texture rectangle template for destination badges.
/// \note Original identifier: \c D_8009A1AC (0x8009A1AC).
extern WorldMapSpriteList gWorldMapBadgeTemplate;
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
#include "hud.h"
#include "player_interaction.h"
#include "runtime_services.h"
#include "toolbox.h"
#include "vector_operations.h"
#include "world_map.h"
/// Draw available map destinations, their badges, and the animated selection highlight.
/// \param gdl Display-list cursor to append to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D474 (0x8002D474).
void wholeMapDrawDestinations(Gfx **gdl) {
    WorldMapSpriteList selection = gWorldMapSelectionTemplate;
    WorldMapSpriteList icon = gWorldMapDestinationTemplate;
    WorldMapSpriteList badge = gWorldMapBadgeTemplate;
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
    textureDrawRectangles(rendererTable, (&selection.item), gdl);
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
            textureDrawRectangles(rendererTable, (&icon.item), gdl);
            texture = gWorldMapDestinations[index].badgeTextures[playerGetChaseStage()];
            if (texture >= 0) {
                if (index == 4) {
                    badge.item.x = icon.item.x - 27;
                } else {
                    badge.item.x = icon.item.x + 55;
                }
                alpha = runtimeGetUiPulseAlpha() & 255;
                badge.item.y = (s32)((((index & 1) ? 0.01F : -0.01F) * (f32)(u32)alpha) + (f32)icon.item.y);
                badge.item.texture = texture;
                textureDrawRectangles(rendererTable, (&badge.item), gdl);
            }
        }
    }
}

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

/// Animation angle in degrees for the locked-destination markers.
/// \note Original identifier: \c D_8009A1F4 (0x8009A1F4).
extern s32 gWorldMapLockedMarkerAngle;

/// Draw and animate the markers for locked map destinations.
/// \param gdl Display-list cursor to append to and advance.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002D904 (0x8002D904).
void wholeMapDrawLockedDestinations(Gfx **gdl) {
    TextureRectangle rectangles[3];
    s32 i;
    s32 x;
    s32 y;
    s32 angle;
    WorldMapDestination *row;
    s32 *counter;

    counter = &gWorldMapLockedMarkerAngle;
    for (i = 0; i != 2; i++) {
        rectangles[i].width = 64;
        rectangles[i].height = 64;
        rectangles[i].s = 0;
        rectangles[i].t = 0;
        rectangles[i].scaleX = 1.0F;
        rectangles[i].scaleY = 1.0F;
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
            textureDrawRectangles(7, (TextureRectangle *)rectangles, gdl);
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
        textureDrawRectangles(7, (TextureRectangle *)rectangles, gdl);
    }
    angle = (s32)((u32)*counter + 5U);
    while (angle < 0) {
        angle += 360;
    }
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
    textureDrawRectangles(7, ((TextureRectangle *)((playerGetChaseStage() * 0x90) + gWorldMapDifficultyOverlays)), gdl);
}

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
    objectDrawGroup(OBJECT_DRAW_GROUP_ALTERNATE);
    gDPPipeSync((*gdl)++);
    gDPSetCycleType((*gdl)++, G_CYC_1CYCLE);
    wholeMapDrawDifficultyOverlay(gdl);
    hudDrawMicrophoneIndicators(gdl);
    cameraUpdateOverlayColors(gRenderCameraIndex, 0);
    cameraDrawSecondaryOverlay(0);
}

/// Transform from map presentation coordinates to world coordinates.
/// \note Original identifier: \c D_800F1B78 (0x800F1B78).
extern Matrix34 gWorldMapToWorldTransform;
/// Camera-relative actor position on the world-map screen.
/// \note Original identifier: \c D_8009A104 (0x8009A104).
extern Vec3f gWorldMapActorPresentationPosition;

/// Update the map actor's position and focus while preserving its ground clearance.
/// \pre Requires an active actor and the map presentation transform.
/// \returns Nothing.
/// \note Checked object-member offsets retain matching target compiler accesses to the active actor.
/// \note Original decompilation identifier: \c func_8002DD70 (0x8002DD70).
void wholeMapUpdateActorPlacement(void) {
    Vec3f position;
    s32 firstJointId = 0x47;
    s32 secondJointId = 0x4C;
    f32 averageDivisor = 2.0F;
    Vec3f reference;
    f32 height;
    f32 otherHeight;
    u8 *firstJoint;
    u8 *secondJoint;
    u8 *state;
    state = gPikachuActor->state.bytes;
    presentationUpdateFocusPoint(&reference);
    matrix34TransformPointVector((f32 *)(&gWorldMapToWorldTransform), (f32 *)(&reference),
                                 ((ActorScriptState *)state)->targetPosition);
    reference.x = gWorldMapActorPresentationPosition.x;
    reference.y = gWorldMapActorPresentationPosition.y;
    reference.z = gWorldMapActorPresentationPosition.z;
    matrix34TransformPointVector((f32 *)(&gWorldMapToWorldTransform), (f32 *)(&reference), (f32 *)(&position));
    firstJoint = actorFindPart((u8 *)((s32)gPikachuActor), firstJointId);
    height = *(f32 *)((u8 *)((s32)gPikachuActor) + (u32) & ((RuntimeObjectPrefix *)0)->worldPosition[1]) -
             ((RuntimeObjectPrefix *)firstJoint)->worldPosition[1];
    secondJoint = actorFindPart((u8 *)((s32)gPikachuActor), secondJointId);
    otherHeight = (*(f32 *)((u8 *)((s32)gPikachuActor) + (u32) & ((RuntimeObjectPrefix *)0)->worldPosition[1]) -
                   ((RuntimeObjectPrefix *)secondJoint)->worldPosition[1]);
    position.y += (otherHeight + height) / averageDivisor;
    *(f32 *)((u8 *)((s32)gPikachuActor) + (u32) & ((RuntimeObjectPrefix *)0)->position[0]) = position.x;
    gPikachuActor->position[1] = position.y;
    gPikachuActor->position[2] = position.z;
    gPikachuActor->worldPosition[0] = position.x;
    *(f32 *)((u8 *)((s32)gPikachuActor) + (u32) & ((RuntimeObjectPrefix *)0)->worldPosition[1]) = position.y;
    gPikachuActor->worldPosition[2] = position.z;
}

/// Consecutive map input updates with directional buttons held, used to select the actor pose.
/// \note Original identifier: \c D_800F1B70 (0x800F1B70).
extern s32 gWorldMapDirectionHeldFrames;

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
    state = *(u8 **)((u8 *)((s32)gPikachuActor) + (u32) & ((RuntimeObjectPrefix *)0)->state);
    camera = ((u8 *)gRenderCameras) + (gRenderCameraIndex * 0xE0);
    objectSetHierarchyFlags((u8 *)((s32)gPikachuActor), flags);
    wholeMapUpdateActorPlacement();
    actionScriptStop((u8 *)((s32)gPikachuActor));
    actionStopPlayback((u8 *)((s32)gPikachuActor));
    delta[0] = *(f32 *)(camera + 0) -
               *(f32 *)((u8 *)((s32)gPikachuActor) + (u32) & ((RuntimeObjectPrefix *)0)->worldPosition[0]);
    delta[1] = *(f32 *)(camera + 4) -
               *(f32 *)((u8 *)((s32)gPikachuActor) + (u32) & ((RuntimeObjectPrefix *)0)->worldPosition[1]);
    delta[2] = *(f32 *)(camera + 8) -
               *(f32 *)((u8 *)((s32)gPikachuActor) + (u32) & ((RuntimeObjectPrefix *)0)->worldPosition[2]);
    angle = vector3GetYawDegrees(delta);
    actorAddRotationAxisDegrees((u8 *)((s32)gPikachuActor), -((ActorScriptState *)state)->orientation[0], pitchAxis);
    actorAddRotationAxisDegrees((u8 *)((s32)gPikachuActor), angle - ((ActorScriptState *)state)->orientation[1], 1);
    ((ActorScriptState *)state)->targetMode = ACTOR_TARGET_FIXED;
    matrix34TransformPointVector((f32 *)(&gWorldMapToWorldTransform),
                                 (f32 *)&gWorldMapDestinations[gWorldMapSelectedDestination].focusPosition,
                                 ((ActorScriptState *)state)->targetPosition);
    presentationResetFocusPoint(&gWorldMapDestinations[gWorldMapSelectedDestination].focusPosition);
    gWorldMapDirectionHeldFrames = 0;
    actionScriptStartWithMode((u8 *)((s32)gPikachuActor), action, actionMode);
    actorSetBehaviorInhibitFlags((u8 *)((s32)gPikachuActor), animation);
}

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
        ((RuntimeObjectPrefix *)PLAYER)->state.actor->defaultActionMode = 3;
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

/// Primary controller new and auto-repeated button presses.
/// \note Original identifier: \c D_800AD54C (0x800AD54C).
extern s32 gControllerPrimaryRepeatedButtons;

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

/// Handle destination selection, confirmation, and cancellation from controller and voice input.
/// \pre Requires the whole-map menu and its active actor.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8002E200 (0x8002E200).
void wholeMapHandleInput(void) {
    s32 *selected;
    s32 *pressed;
    s32 confirmSound = 0xC5;
    s32 direction;
    ScriptMessage *input;
    s32 selection;
    Vec3f specialPosition;
    Vec3f defaultPosition;
    s32 confirmMask;
    pressed = &gControllerPrimaryRepeatedButtons;
    if (*pressed & 0x4000) {
        cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFE, 0.6F);
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
    if (actorAcknowledgeScriptMessage((u8 *)((s32)gPikachuActor)) != 0) {
        input = &gPikachuActor->state.actor->message;
        if (input->type == SCRIPT_MESSAGE_WORLD_MAP_DESTINATION) {
            if (input->arg0 == 10 && playerGetChaseStage() == 2) {
                specialPosition = gWorldMapSpecialDestinationFocus;
                gWorldMapSelectedDestination = 10;
                presentationSetFocusTarget(&specialPosition);
                audioPlaySoundEffect(confirmSound);
                wholeMapPlayConfirmationAnimation();
                wholeMapSetState(3);
                return;
            }
            if (input->arg0 < 0) {
                input->arg0 = 0;
            } else if (input->arg0 >= 10) {
                input->arg0 = 9;
            }
            selection = gWorldMapVoiceDestinationIndices[input->arg0];
            if (wholeMapIsDestinationAvailable(selection) != 0) {
                gWorldMapSelectedDestination = selection;
                attachedEffectCreate((u8 *)((s32)gPikachuActor), 1, 0xA4);
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
    micRequestDictionary(VOICE_DICTIONARY_WORLD_MAP);
    actorSetMessengerContactMode((u8 *)((s32)gPikachuActor), 2);
    hudSetFeatureMask(4);
    micRequestRecognitionOverride(1);
    micRequestRecognitionFollowsTrigger(1);
    gGameplayMicrophoneAvailable = 1;
    gGameplayVoiceCommandsAllowed = 1;
}


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

    sp24 = gPikachuActor->state.bytes;
    playerResetViewOrientation();
    cameraBuildInterfaceTransform((f32 *)&gWorldMapToWorldTransform);
    if (actionScriptIsStopped((u8 *)((s32)gPikachuActor)) != 0) {
        actionScriptStartWithMode((u8 *)((s32)gPikachuActor), 0x57, 2);
    }
    if (gWorldMapDirectionHeldFrames >= 0x65) {
        if (((ActorScriptState *)sp24)->defaultActionMode != 0xE) {
            actionScriptStop((u8 *)((s32)gPikachuActor));
            actionScriptStartWithMode((u8 *)((s32)gPikachuActor), 0x57, 0xE);
        }
    } else if (((ActorScriptState *)sp24)->defaultActionMode != 2) {
        actionScriptStop((u8 *)((s32)gPikachuActor));
        actionScriptStartWithMode((u8 *)((s32)gPikachuActor), 0x57, 2);
    }
    switch (gWorldMapState) {
    case 0:
        wholeMapConfigureVoiceInput();
        wholeMapSelectBackdrop();
        cameraResetOverlayColor(0, 0, 0, 0, 0);
        cameraFadePrimarySecondaryOverlay(0, 0, 0, 0, 0.4F);
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
        objectUpdateGroup(OBJECT_UPDATE_GROUP_ALTERNATE);
        return;
    case 1:
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0F) {
            wholeMapSetState(2);
        }
        wholeMapUpdateActorPlacement();
        objectUpdateGroup(OBJECT_UPDATE_GROUP_ALTERNATE);
        return;
    case 2:
        wholeMapHandleInput();
        wholeMapUpdateActorPlacement();
        objectUpdateGroup(OBJECT_UPDATE_GROUP_ALTERNATE);
        return;
    case 3:
        if (runtimeGetElapsedMilliseconds() - (u64)gWorldMapStateStartMilliseconds >= 0x5DD) {
            cameraFadePrimarySecondaryOverlay(0, 0, 0, 0xFE, 0.6F);
            audioChangeCurrentCue(-1, 0.1, 1.0);
            wholeMapSetState(5);
        }
        wholeMapUpdateActorPlacement();
        objectUpdateGroup(OBJECT_UPDATE_GROUP_ALTERNATE);
        return;
    case 4:
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0F) {
            scriptVmSetGlobalInteger(0, 1);
            if (playerGetChaseStage() == 0) {
                areaSetNextDestination(AREA_BEDROOM, 2);
                runtimeRequestProgramChange(0);
            } else {
                if (playerGetChaseStage() == 1) {
                    areaSetNextDestination(AREA_FRONT_YARD, 1);
                } else {
                    areaSetNextDestination(AREA_FRONT_YARD, 2);
                }
                runtimeRequestProgramChange(0);
            }
            controllerConfigureStickButtons(0, 0x1E, 0x1E, 0x1E, 0x1E);
            playerSetChaseStage(progressGetValue(PROGRESS_FIELD_CHASE_STAGE));
            wholeMapSetState(0);
            TRACE("whole map canceled.\n");
        }
        wholeMapUpdateActorPlacement();
        objectUpdateGroup(OBJECT_UPDATE_GROUP_ALTERNATE);
        return;
    case 5:
        if (gPrimarySecondaryOverlayRemainingSeconds == 0.0F) {
            areaSetNextDestination(gWorldMapDestinations[gWorldMapSelectedDestination].areaIds[playerGetChaseStage()], 0);
            runtimeRequestProgramChange(0);
            controllerConfigureStickButtons(0, 0x1E, 0x1E, 0x1E, 0x1E);
            wholeMapSetState(0);
            TRACE("whole map finished.\n");
        }
        wholeMapUpdateActorPlacement();
        objectUpdateGroup(OBJECT_UPDATE_GROUP_ALTERNATE);
        return;
    default:
        return;
    }
}
