#include "player_friendship.h"
/* harvested: matched m2c drafts; pending style pass */
#include "common.h"
#include "string_bytes.h"
#include "actor_parts.h"
#include "player_control.h"
#include "object_lookup.h"
#include "heap_alloc.h"
#include "scene_types.h"
#include "player_types.h"
#include "object_types.h"
#include "actor_script.h"
#include "controller_types.h"
#include "item_types.h"

/// Retained values for a descriptive or consumable interaction proxy.
typedef struct InteractionProxyState {
    s32 context;           ///< Caller context; consumable proxies use it as remaining uses.
    s32 interactionResult; ///< Player interaction classification supplied by the proxy.
    s32 metadata;          ///< Caller metadata sent with a consumption message.
    union {
        char *text;  ///< Borrowed description string.
        s32 address; ///< Original 32-bit description-address representation.
    } description;
} InteractionProxyState;
typedef char InteractionProxyStateSizeCheck[(sizeof(InteractionProxyState) == 0x10) ? 1 : -1];

/// Borrowed current-camera pose used to initialize a player transition.
typedef struct CameraPosePrefix {
    f32 eye[3];    ///< Current eye position.
    f32 target[3]; ///< Current look-at point.
} CameraPosePrefix;

/* .bss 0x80133E10..0x80134030, in address order (IDO lays globals out in declaration order) */
struct {
    s32 w[64];
} gSavedPlayerState;
/// Terrain query mode used for player movement and ground sampling.
/// \note Original identifier: \c D_80133F10 (0x80133F10).
s32 gPlayerCollisionQueryMode;
/// Nonzero while gameplay music is using its reduced volume.
/// \note Original identifier: \c D_80133F14 (0x80133F14).
s32 gGameplayMusicDucked;
/// Flags inhibiting entry into or exit from gameplay music ducking.
/// \note Original identifier: \c D_80133F18 (0x80133F18).
s32 gGameplayMusicDuckingInhibitFlags;
s32 D_80133F18_0;
/// First word of the shared 256-byte player help text buffer; storage continues through adjacent symbols.
/// \note Original identifier: \c D_80133F20 (0x80133F20).
u8 gPlayerHelpTextBuffer[4];
s32 D_80133F20_0;
struct {
    s32 w[62];
} D_80133F20_1;
/// Borrowed cursor into shared player help text while lines are consumed.
/// \note Original identifier: \c D_80134020 (0x80134020).
u8 *gPlayerHelpTextCursor;
s32 D_80134020_0;
struct {
    s32 w[2];
} D_80134020_1;

// clang-format off
#define TRACE if (0) osSyncPrintf
// clang-format on

char *gPlayerInteractionPromptSuffixes[10] = {
    "\n",
    "\nPress @I@J to pick it up.",
    "???",
    "???",
    "???",
    "\nPress @I@J to throw it.",
    "\n(You can't touch it.)",
    "\nPress @I@J to go through it.",
    "???",
    "???",
};

s32 playerRequestAreaInteractionEvent(s32);
s32 playerThrowHeldItemAndFocus(u8 *);
s32 playerTryEnterTargetPassage(u8 *);
s32 playerRequestSavePrompt(s32);
s32 playerConsumeInteractionProxy(u8 *);
void playerPickUpInteractionTarget(u8 *);
s32 playerRequestTargetCooperation(u8 *);

/// Action callbacks indexed by the pointed-object interaction action.
/// \note Original identifier: \c D_80123B98 (0x80123B98).
s32 (*gPlayerInteractionActionHandlers[10])() = {
    NULL,
    (s32 (*)())playerPickUpInteractionTarget,
    playerRequestTargetCooperation,
    NULL,
    NULL,
    playerThrowHeldItemAndFocus,
    playerRequestAreaInteractionEvent,
    playerTryEnterTargetPassage,
    playerRequestSavePrompt,
    playerConsumeInteractionProxy,
};

/* A player mode: per-frame handler (player, pad), leave handler, and the player flags it sets. */
typedef struct {
    /* 0x0 */ PlayerModeUpdate update;
    /* 0x4 */ PlayerModeLeave leave;
    /* 0x8 */ s32 flags;
} PlayerMode; /* size 0xC */

void playerUpdateNormalControl(u8 *, u8 *);
void playerUpdateStationaryLook(u8 *, u8 *);
void playerUpdateMenuOnly(u8 *, u8 *);
void playerFinishStun(u8 *);
void playerUpdateStun(u8 *, u8 *);
void playerUpdateBriefViewShake(u8 *, u8 *);
void playerFinishHelpDisplay(u8 *);
void playerUpdateHelpDisplay(u8 *, u8 *);
void playerFinishTargetFocus(u8 *);
void playerUpdateTargetFocus(u8 *, u8 *);
void playerUpdateRestrictedMovement(u8 *, u8 *);
void playerUpdateRestrictedControl(u8 *, u8 *);
void playerUpdateViewOnlyControl(u8 *, u8 *);
void playerUpdateMovementWithoutActions(u8 *, u8 *);
void playerUpdateEmptyHandInteraction(u8 *, u8 *);
void playerUpdateHeldItemInspection(u8 *, u8 *);
void playerUpdateCameraTransition(u8 *, u8 *);
void playerUnusedModeUpdate(u8 *, u8 *);

PlayerMode gPlayerModes[32] = {
    {playerUnusedModeUpdate, NULL, 0x08004003},
    {playerUpdateNormalControl, NULL, 0xF0003F2F},
    {playerUpdateStationaryLook, NULL, 0xF0009F2E},
    {playerUpdateStun, playerFinishStun, 0x20001100},
    {playerUpdateHelpDisplay, playerFinishHelpDisplay, 0x20001100},
    {playerUpdateTargetFocus, playerFinishTargetFocus, 0xE0001D2E},
    {playerUnusedModeUpdate, NULL, 0x08004803},
    {playerUpdateCameraTransition, NULL, 0x08004003},
    {playerUpdateMenuOnly, NULL, 0x0000110F},
    {playerUpdateBriefViewShake, NULL, 0x00000000},
    {playerUpdateMenuOnly, NULL, 0x08000003},
    {playerUpdateRestrictedControl, NULL, 0xC8000003},
    {playerUpdateRestrictedControl, NULL, 0x40010802},
    {playerUnusedModeUpdate, NULL, 0x08018800},
    {playerUnusedModeUpdate, NULL, 0x08000400},
    {playerUpdateRestrictedMovement, NULL, 0x00000001},
    {playerUpdateRestrictedMovement, NULL, 0x00010801},
    {playerUpdateRestrictedControl, NULL, 0x00000202},
    {playerUpdateRestrictedControl, NULL, 0x40000002},
    {playerUpdateRestrictedControl, NULL, 0x00000000},
    {playerUpdateRestrictedControl, NULL, 0x00018A02},
    {playerUpdateRestrictedControl, NULL, 0x00018800},
    {playerUpdateViewOnlyControl, NULL, 0x00008202},
    {playerUpdateViewOnlyControl, NULL, 0x00018E02},
    {playerUpdateRestrictedControl, NULL, 0x00000400},
    {playerUpdateRestrictedControl, NULL, 0x00010800},
    {playerUpdateMovementWithoutActions, NULL, 0xF0012A2F},
    {playerUpdateMovementWithoutActions, NULL, 0xE000222F},
    {playerUpdateEmptyHandInteraction, NULL, 0x00010C00},
    {playerUpdateEmptyHandInteraction, NULL, 0x00010900},
    {playerUpdateHeldItemInspection, NULL, 0x00010900},
    {playerUpdateRestrictedControl, NULL, 0x40018A02},
};

/* A control scheme: the player mode to enter, the controller port driving the player (-1: none)
 * and the pad bits it keeps (playerFilterControllerInput). */
typedef struct {
    /* 0x0 */ s32 mode;
    /* 0x4 */ s32 port;
    /* 0x8 */ u16 keepMask;
} ModeRec; /* size 0xC */

ModeRec gPlayerControlSchemes[28] = {
    {0, -1, 0},      {1, 0, 0xFFFF},  {6, -1, 0},      {7, -1, 0},      {8, 0, 0x1000},  {9, -1, 0},
    {10, 0, 0},      {11, 0, 0},      {12, 0, 0x4000}, {13, 0, 0},      {14, 0, 0},      {15, 0, 0x80},
    {16, 0, 0x4080}, {17, 0, 0x40},   {18, 0, 0x108},  {19, 0, 0x404},  {20, 0, 0x40C0}, {21, 0, 0x404},
    {22, 0, 0xC0},   {23, 0, 0xC0C0}, {24, 0, 0x8000}, {25, 0, 0x4000}, {26, 0, 0x60E3}, {27, 0, 0x20E3},
    {28, 0, 0xC000}, {29, 0, 0x5000}, {30, 0, 0x5000}, {31, 0, 0x41C8},
};

void mazeDestroyNode(u8 *); /* extern */

void objectPrepareDistributedKindSearches(u16, s32);   /* extern */
void mazeInitializeRoot();                   /* extern */
void mazeBuildNodeHierarchy(s32, s32, s32);  /* extern */
void mazeConfigureEventLinks();                   /* extern */
/// Borrowed configuration of the loaded scene overlay, valid while that overlay remains loaded.
/// \note Original identifier: \c D_80133DE4 (0x80133DE4).
extern u8 *gCurrentAreaConfiguration;

void areaRefreshVisibilityDistances(s32); /* extern */
void areaUpdateNodeVisibility(s32); /* extern */
/// Camera slot selected for the current drawing context.
/// \note Original identifier: \c D_800ABDD8 (0x800ABDD8).
extern s32 gRenderCameraIndex;

void runtimeRequestGameModeChange(s32); /* extern */

/// Set both the current and saved player control permissions.
/// \param player Player record receiving the permissions.
/// \param permissions Player control-permission mask to apply and retain for normal control.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010E980 (0x8010E980).
void playerSetControlPermissions(u8 *player, s32 permissions) {
    ((PlayerControlPrefix *)player)->savedPermissions = permissions;
    ((PlayerControlPrefix *)player)->permissions = permissions;
}

/// Provide an inactive player-control hook; has no effect.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010E98C (0x8010E98C).
void playerUnusedControlHook(void) {
}

/// Remember the current companion-control mode and its target for later restoration.
/// \param player Player record whose companion-control selection is saved.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010E994 (0x8010E994).
void playerSaveCompanionControlMode(u8 *player) {
    ((PlayerControlPrefix *)player)->savedCompanionTarget = (s32)((PlayerControlPrefix *)player)->companionTarget;
    ((PlayerControlPrefix *)player)->savedCompanionMode = (s32)((PlayerControlPrefix *)player)->companionMode;
}

/// Restore the saved companion-control selection without a status message, if the mode is currently
/// permitted.
/// \param player Player record with a previously saved companion-control selection.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010E9A8 (0x8010E9A8).
void playerRestoreCompanionControlMode(u8 *player) {
    playerSetCompanionControlMode(player, ((PlayerControlPrefix *)player)->savedCompanionMode, 0, ((PlayerControlPrefix *)player)->savedCompanionTarget);
}

u8 *objectCreateDefault();     /* extern */
u8 *objectMakeHierarchyRoot(u8 *); /* extern */
u8 *objectRegisterActiveHierarchy(u8 *); /* extern */
/// Create and register the object hierarchy used by player control.
/// \returns Registered player-control hierarchy root, managed by the object system.
/// \note Original decompilation identifier: \c func_8010E9D0 (0x8010E9D0).
u8 *playerCreateControlHierarchy(void) {
    u8 *controlRoot;
    u8 *sp20;
    u8 *secondCollisionObject;

    controlRoot = objectCreateDefault();
    sp20 = objectCreateDefault();
    secondCollisionObject = objectCreateDefault();
    ((RuntimeObjectPrefix *)controlRoot)->kind = 0x7F;
    ((RuntimeObjectPrefix *)controlRoot)->sibling.bytes = sp20;
    ((RuntimeObjectPrefix *)sp20)->sibling.bytes = secondCollisionObject;
    ((RuntimeObjectPrefix *)sp20)->radius = 4.5f;
    ((RuntimeObjectPrefix *)secondCollisionObject)->radius = 4.5f;
    objectRegisterActiveHierarchy(objectMakeHierarchyRoot(controlRoot));
    return controlRoot;
}

/// Smoothed frame duration in seconds used by gameplay and animation updates.
/// \note Original identifier: \c D_800AE4E4 (0x800AE4E4).
extern f32 gRuntimeFrameSeconds;

/// Calculate the permitted view-yaw change from horizontal controller input for the current frame.
/// \param object Player record providing view-control permissions.
/// \param controller Controller state supplying horizontal stick input.
/// \param invert Nonzero to reverse the turning direction.
/// \returns Yaw change in degrees, or zero when input is neutral or turning is disabled.
///
/// \note Controller stick values must remain in their supported range so magnitude calculations fit a signed
/// integer.
/// \note Original decompilation identifier: \c func_8010EA38 (0x8010EA38).
f32 playerCalculateYawInput(u8 *object, u8 *controller, s32 invert) {
    f32 normalized;
    f32 response;
    s32 stick;

    if (!(((PlayerControlPrefix *)object)->permissions & 0x200) ||
        ((stick = ((ControllerState *)controller)->stickX), (stick >= 0 ? stick : -stick) < 11)) {
        return 0.0f;
    }
    if (stick >= 0x32) {
        response = -60.0f * gRuntimeFrameSeconds;
    } else if (stick < -0x31) {
        response = 60.0f * gRuntimeFrameSeconds;
    } else {
        normalized = (f32)stick * 0.011764706f;
        response = (f32) - (normalized > 0.0f ? 1 : (normalized < 0.0f ? -1 : 0)) * gRuntimeFrameSeconds *
                               ((48.0f * (normalized * normalized)) + 12.0f);
    }
    if (invert != 0) {
        return -response;
    }
    return response;
}

/// Calculate the permitted view-pitch change from vertical controller input for the current frame.
/// \param player Player record providing view-control permissions.
/// \param controller Controller state supplying vertical stick input.
/// \param invert Nonzero to reverse the pitch direction.
/// \returns Pitch change in degrees, or zero when input is neutral or pitch control is disabled.
///
/// \note Controller stick values must remain in their supported range so magnitude calculations fit a signed
/// integer.
/// \note Original decompilation identifier: \c func_8010EB5C (0x8010EB5C).
f32 playerCalculatePitchInput(u8 *player, u8 *controller, s32 invert) {
    f32 delta;
    s32 flags;
    s32 input;
    flags = ((PlayerControlPrefix *)player)->permissions;
    if (!(flags & 0x200) || !(flags & 0x8000))
        return 0.0f;
    input = ((ControllerState *)controller)->stickY;
    if (input >= 50)
        delta = 60.0f * gRuntimeFrameSeconds;
    else if (input >= 20)
        delta = 12.0f * gRuntimeFrameSeconds;
    else if (input < -49)
        delta = -60.0f * gRuntimeFrameSeconds;
    else if (input < -19)
        delta = -12.0f * gRuntimeFrameSeconds;
    else
        return 0.0f;
    if (invert != 0)
        return -delta;
    return delta;
}

f32 fabsf(f32);
#pragma intrinsic(fabsf)
void vector2RotateComponentsDegrees(f32, f32 *, f32 *);
/// Calculate the current frame movement request from player controls and update movement responsiveness.
/// \param pad Controller state supplying movement input.
/// \param obj Player record providing movement settings, orientation, and movement history.
/// \param dx Writable world-space horizontal X displacement.
/// \param dz Distinct writable world-space horizontal Z displacement.
/// \returns Nothing.
///
/// \note Requires a positive finite frame duration and initialized movement history. Portability: the
/// reserved words and one-element history array preserve the original IDO stack layout and stores.
/// \note Original decompilation identifier: \c func_8010EC3C (0x8010EC3C).
void playerCalculateMovementInput(u8 *pad, u8 *obj, f32 *dx, f32 *dz) {
    s32 padding[2];
    f32 previous[1];
    f32 x;
    f32 z;
    f32 speed;
    f32 factor;
    f32 change;
    f32 oldSpeed;
    f32 threshold = 20.0f;
    s32 buttons;
    *dx = 0.0f;
    *dz = 0.0f;
    x = ((PlayerControlPrefix *)obj)->permissions & 0x2000 ? (f32)((ControllerState *)pad)->stickX : 0.0f;
    z = (f32)((ControllerState *)pad)->stickY;
    previous[0] = (f32)((PlayerControlPrefix *)obj)->previousStickY;
    ((PlayerControlPrefix *)obj)->previousStickY = (s32)z;
    buttons = ((ControllerState *)pad)->held;
    if (buttons & 3) {
        x = buttons & 1 ? 50.0f : -50.0f;
    } else if ((fabsf(x) >= 60.0f || fabsf(z) <= 10.0f) && ((PlayerControlPrefix *)obj)->companionMode == 0) {
        if (fabsf(z) <= 10.0f)
            ((PlayerControlPrefix *)obj)->movementSpeed = 0.0f;
        return;
    }
    if (fabsf(z) >= threshold) {
        change = fabsf(z - previous[0]);
        factor = z > 0.0f ? 0.009f : 0.007f;
        speed = change * factor / gRuntimeFrameSeconds + 8.0f;
        oldSpeed = ((PlayerControlPrefix *)obj)->movementSpeed;
        speed = (oldSpeed < speed ? speed : oldSpeed) < 25.0f ? (oldSpeed < speed ? speed : oldSpeed) : 25.0f;
    } else {
        if (fabsf(x) > 10.0f)
            speed = fabsf(x) > threshold ? 25.0f : 8.0f;
        else
            speed = 0.0f;
    }
    ((PlayerControlPrefix *)obj)->movementSpeed = speed;
    if (!(speed < 0.001f) || !(speed > -0.001f)) {
        speed *= gRuntimeFrameSeconds;
        *dx = -x * 0.011764706f * speed;
        *dz = z * 0.011764706f * speed;
        vector2RotateComponentsDegrees(-((PlayerControlPrefix *)obj)->yaw, dx, dz);
    }
}

/// Set a player view orientation when view-angle changes are enabled.
/// \param player Player record to update.
/// \param x Requested yaw in degrees, wrapped to a full turn.
/// \param z Requested pitch in degrees, interpreted as a signed angle and limited to minus sixty through
/// sixty degrees.
/// \returns Nothing.
///
/// \note Angles must be finite and small enough for full-turn adjustments to change their floating-point
/// value.
/// \note Original decompilation identifier: \c func_8010EF00 (0x8010EF00).
void playerSetViewAngles(u8 *player, f32 x, f32 z) {
    if (((PlayerControlPrefix *)player)->permissions & 2) {
        while (x < 0.0f)
            x += 360.0f;
        while (x >= 360.0f)
            x -= 360.0f;
        while (z < 0.0f)
            z += 360.0f;
        while (z >= 360.0f)
            z -= 360.0f;
        if (z >= 180.0f)
            z -= 360.0f;
        if (z > 60.0f)
            z = 60.0f;
        else if (z < -60.0f)
            z = -60.0f;
        ((PlayerControlPrefix *)player)->yaw = x;
        ((PlayerControlPrefix *)player)->pitch = z;
    }
}


/// Resolve the companion-control target used for player view tracking.
/// \param player Player record containing the target selection.
/// \returns Borrowed available target or tracking part, or null if unavailable; valid only while the target
/// hierarchy remains live.
/// \note Original decompilation identifier: \c func_8010F024 (0x8010F024).
u8 *playerResolveCompanionViewTarget(u8 *player) {
    u8 *resolvedObject;
    u8 *var_a0;

    resolvedObject = objectResolveAvailableHandle(((PlayerControlPrefix *)player)->companionTarget);
    var_a0 = resolvedObject;
    if (resolvedObject == NULL) {
        return NULL;
    }
    if (((RuntimeObjectPrefix *)resolvedObject)->kind == 0x25) {
        var_a0 = actorFindPart(var_a0, 0x79);
    }
    return var_a0;
}

void vector3GetYawAndProjectedPitchDegrees(f32 *, f32 *, f32 *);
f32 mathMapRangeClamped(f32, f32, f32, f32, f32);
f32 mathApproachAngleDegrees(f32, f32, f32, f32, f32);
/// Turn the player view toward a companion-control target while companion control is active.
/// \param player Player record whose view is updated subject to view-control permissions.
/// \param controller Controller state influencing the tracking response.
/// \param target Available object to track.
/// \returns Nothing.
///
/// \note Controller stick values must remain in their supported range so magnitude calculations fit a signed
/// integer.
/// \note Original decompilation identifier: \c func_8010F074 (0x8010F074).
void playerTrackCompanionViewTarget(u8 *player, u8 *controller, u8 *target) {
    f32 direction[3];
    f32 yaw;
    f32 pitch;
    f32 speed;
    s32 x;
    s32 y;
    s32 input;
    f32 result;
    if (((PlayerControlPrefix *)player)->companionMode != 0) {
        input = ((ControllerState *)controller)->stickX;
        x = input >= 0 ? input : -input;
        input = ((ControllerState *)controller)->stickY;
        y = input >= 0 ? input : -input;
        speed = mathMapRangeClamped(0.0f, 60.0f, 50.0f, 10.0f, (f32)(y + x));
        direction[0] = ((RuntimeObjectPrefix *)target)->worldPosition[0] - ((PlayerControlPrefix *)player)->position[0];
        direction[1] = ((RuntimeObjectPrefix *)target)->worldPosition[1] - ((PlayerControlPrefix *)player)->position[1];
        direction[2] = ((RuntimeObjectPrefix *)target)->worldPosition[2] - ((PlayerControlPrefix *)player)->position[2];
        direction[1] -= ((PlayerControlPrefix *)player)->eyeHeight;
        vector3GetYawAndProjectedPitchDegrees(direction, &yaw, &pitch);
        result = mathApproachAngleDegrees(((PlayerControlPrefix *)player)->yaw, yaw, 0.2f, speed,
                                          60.0f * gRuntimeFrameSeconds);
        playerSetViewAngles((u8 *)player, result,
                            mathApproachAngleDegrees(((PlayerControlPrefix *)player)->pitch, pitch, 0.2f, speed,
                                                     60.0f * gRuntimeFrameSeconds));
    }
}

f32 vector3Length(f32 *);
/// Supply automatic forward or backward movement to maintain follow distance when follow mode is active and
/// the stick is neutral.
/// \param controller Controller state receiving the automatic movement request.
/// \param player Player record providing follow-mode selection and position.
/// \param target Available object to follow.
/// \returns Nothing.
///
/// \note Controller stick values must remain in their supported range so magnitude calculations fit a signed
/// integer.
/// \note Original decompilation identifier: \c func_8010F1C4 (0x8010F1C4).
void playerApplyFollowMovementInput(u8 *controller, u8 *player, u8 *target) {
    f32 direction[3];
    f32 distance;
    s32 input;
    s32 x;
    s32 y;
    if (((PlayerControlPrefix *)player)->companionMode == 2) {
        input = ((ControllerState *)controller)->stickX;
        x = input >= 0 ? input : -input;
        if (x < 10) {
            input = ((ControllerState *)controller)->stickY;
            y = input >= 0 ? input : -input;
            if (y < 10) {
                direction[0] =
                    ((PlayerControlPrefix *)player)->position[0] - ((RuntimeObjectPrefix *)target)->worldPosition[0];
                direction[1] =
                    ((PlayerControlPrefix *)player)->position[1] - ((RuntimeObjectPrefix *)target)->worldPosition[1];
                direction[2] =
                    ((PlayerControlPrefix *)player)->position[2] - ((RuntimeObjectPrefix *)target)->worldPosition[2];
                distance = vector3Length(direction);
                if (distance > 20.0f)
                    ((ControllerState *)controller)->stickY =
                        (s32)mathMapRangeClamped(25.0f, 20.0f, 40.0f, 50.0f, distance);
                if (distance < 12.0f)
                    ((ControllerState *)controller)->stickY =
                        (s32)mathMapRangeClamped(15.0f, -20.0f, 0.0f, -50.0f, distance);
            }
        }
    }
}

s32 terrainGetMaterialSound(s32, s32);  /* extern */
s32 audioPlaySoundEffect(s32);   /* extern */
void audioSetSoundEffectVolume(s32, s32); /* extern */
/// Footstep travel thresholds indexed by movement class; the airborne class suppresses footsteps.
/// \note Original identifier: \c D_80123E90 (0x80123E90).
f32 gPlayerFootstepDistanceThresholds[4] = {5.0f, 8.0f, 9.0f, 2147483648.0f};

/// Play a material-specific footstep when accumulated travel reaches the current movement class threshold.
/// \param player Player motion record providing movement class zero through three and accumulated travel
/// distance.
/// \param ground Ground material code whose low four bits identify a material from zero through seven.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010F300 (0x8010F300).
void playerPlayFootstepSound(u8 *player, s32 ground) {
    s32 snd;

    if (!(((PlayerControlPrefix *)player)->footstepDistance <
          gPlayerFootstepDistanceThresholds[((PlayerControlPrefix *)player)->movementClass])) {
        ((PlayerControlPrefix *)player)->footstepDistance = 0.0f;
        snd = terrainGetMaterialSound(1, ground);
        if (snd >= 0) {
            audioSetSoundEffectVolume(audioPlaySoundEffect(snd), 0xA0);
        }
    }
}

void controllerRequestRumblePattern(s32, f32, f32, f32);
s32 controllerIsRumblePatternActive(s32);
void backdropAdvanceScroll(f32);
f32 vector3Normalize(f32 *);
u32 randomNext(void);
f32 terrainGetWorldHeightForQuery(s32, f32, f32, f32, u8 **);
s32 terrainGetSurfaceAttributes(f32, f32, f32, u8 **);
s32 terrainConstrainMovement(s32, f32, f32, f32, f32, f32, f32, f32 *, f32 *, f32 *, u8 **);
void terrainGetDownhillHeightDifferences(f32, f32, f32, f32, f32, f32 *, f32 *, u8 **);
void playerPlayFootstepSound(u8 *, s32);
/// Borrowed reference to the active Pikachu actor created for the current area.
/// \note Original identifier: \c D_800760A8 (0x800760A8).
extern s32 gPikachuActor;
/// Terrain query mode used for player movement and ground sampling.
/// \note Original identifier: \c D_80133F10 (0x80133F10).
extern s32 gPlayerCollisionQueryMode;
/// Advance permitted player movement against the area terrain and nearby Pikachu, updating motion state and
/// movement feedback.
/// \param motion Player motion record to update; the active Pikachu actor must be available when movement is
/// enabled.
/// \param dx Requested world-space X displacement for the current frame.
/// \param dz Requested world-space Z displacement for the current frame.
/// \returns Nothing.
///
/// \note Portability: the empty corner-sample checks preserve the original IDO register allocation.
///
/// \note Requires initialized area terrain, collision policy zero through nine, and a positive finite frame
/// duration. Material codes used for sounds must have identifiers zero through seven.
/// \note Original decompilation identifier: \c func_8010F370 (0x8010F370).
void playerStepGroundMovement(u8 *motion, f32 dx, f32 dz) {
    f32 nextGround;
    f32 currentGround;
    f32 speed;
    f32 dy;
    u8 *node;
    s32 material;
    s32 blockedDrop;
    f32 normal[3];
    s32 flags;
    f32 height;
    f32 cornerHeight;
    f32 nextX;
    f32 nextY;
    f32 nextZ;
    f32 distance[3];
    f32 oldDistance;
    f32 rotatedX;
    f32 rotatedZ;

    node = NULL;
    flags = ((PlayerControlPrefix *)motion)->permissions;
    if (!(flags & 1)) {
        ((PlayerControlPrefix *)motion)->movementClass = 0;
        return;
    }
    material = 0;
    if (flags & 4) {
        material = terrainGetSurfaceAttributes(((PlayerControlPrefix *)motion)->position[0],
                                               ((PlayerControlPrefix *)motion)->position[1],
                                               ((PlayerControlPrefix *)motion)->position[2], &node);
        if ((((PlayerControlPrefix *)motion)->airborne == 0) && (material & 0x100)) {
            ((PlayerControlPrefix *)motion)->slopeSpeed =
                (f32)(((PlayerControlPrefix *)motion)->slopeSpeed + (20.0f * gRuntimeFrameSeconds));
            if (((PlayerControlPrefix *)motion)->slopeSpeed > 10.0f) {
                ((PlayerControlPrefix *)motion)->slopeSpeed = 10.0f;
            }
        } else {
            ((PlayerControlPrefix *)motion)->slopeSpeed =
                (f32)(((PlayerControlPrefix *)motion)->slopeSpeed - (30.0f * gRuntimeFrameSeconds));
            if (((PlayerControlPrefix *)motion)->slopeSpeed < 0.0f) {
                ((PlayerControlPrefix *)motion)->slopeSpeed = 0.0f;
            }
        }
        if (((PlayerControlPrefix *)motion)->slopeSpeed <= 0.0f) {
            dy = -((PlayerControlPrefix *)motion)->downwardVelocity - (3.2666667f * gRuntimeFrameSeconds);
            goto block_14;
        }
        normal[1] = 0.2f;
        terrainGetDownhillHeightDifferences(
            ((PlayerControlPrefix *)motion)->position[0], ((PlayerControlPrefix *)motion)->position[1],
            ((PlayerControlPrefix *)motion)->position[2], 50.0f, 0.2f, normal, normal + 2, &node);
        vector3Normalize(normal);
        dx += normal[0] * ((PlayerControlPrefix *)motion)->slopeSpeed;
        dz += normal[2] * ((PlayerControlPrefix *)motion)->slopeSpeed;
        dy = -((PlayerControlPrefix *)motion)->downwardVelocity - (3.2666667f * gRuntimeFrameSeconds * 3.0f);
    } else {
        ((PlayerControlPrefix *)motion)->slopeSpeed = 0.0f;
        dy = 0.0f;
    }
block_14:
    currentGround = terrainGetWorldHeightForQuery(
        gPlayerCollisionQueryMode, ((PlayerControlPrefix *)motion)->position[0],
        ((PlayerControlPrefix *)motion)->position[1] + 0.03f, ((PlayerControlPrefix *)motion)->position[2], &node);
    if (((PlayerControlPrefix *)motion)->permissions & 8) {
        if (material & 0x100) {
            height = 0.55f;
        } else {
            height = 5.5f;
        }
        node = NULL;
        if (((PlayerControlPrefix *)motion)->airborne != 0) {
            terrainConstrainMovement(gPlayerCollisionQueryMode, ((PlayerControlPrefix *)motion)->position[0],
                                     ((PlayerControlPrefix *)motion)->position[1] + 0.03f,
                                     ((PlayerControlPrefix *)motion)->position[2],
                                     ((PlayerControlPrefix *)motion)->collisionRadius,
                                     ((PlayerControlPrefix *)motion)->eyeHeight, height, &dx, &dy, &dz, &node);
        } else {
            terrainConstrainMovement(gPlayerCollisionQueryMode, ((PlayerControlPrefix *)motion)->position[0],
                                     currentGround + 0.03f, ((PlayerControlPrefix *)motion)->position[2],
                                     ((PlayerControlPrefix *)motion)->collisionRadius,
                                     ((PlayerControlPrefix *)motion)->eyeHeight, height, &dx, &dy, &dz, &node);
        }
    }
    blockedDrop = 0;
    if (dy < 0.0f) {
        nextX = ((PlayerControlPrefix *)motion)->position[0] + dx;
        nextY = ((PlayerControlPrefix *)motion)->position[1] + dy + 0.03f;
        nextZ = ((PlayerControlPrefix *)motion)->position[2] + dz;
        cornerHeight = terrainGetWorldHeightForQuery(gPlayerCollisionQueryMode, nextX, nextY, nextZ, &node);
        nextGround = terrainGetWorldHeightForQuery(gPlayerCollisionQueryMode, nextX + 0.1f, nextY, nextZ, &node);
        if (cornerHeight < nextGround && nextY < nextGround) {
        }
        nextGround = terrainGetWorldHeightForQuery(gPlayerCollisionQueryMode, nextX, nextY, nextZ + 0.1f, &node);
        if (cornerHeight < nextGround && nextY < nextGround) {
        }
        nextGround = terrainGetWorldHeightForQuery(gPlayerCollisionQueryMode, nextX + 0.1f, nextY, nextZ + 0.1f, &node);
        if (nextGround < cornerHeight) {
            nextGround = cornerHeight;
        }
        if (nextY < (nextGround + 0.03f)) {
            blockedDrop = 1;
            dy = 0.0f;
        }
    }
    nextGround =
        terrainGetWorldHeightForQuery(gPlayerCollisionQueryMode, ((PlayerControlPrefix *)motion)->position[0] + dx,
                                      ((PlayerControlPrefix *)motion)->position[1] + dy + 0.03f,
                                      ((PlayerControlPrefix *)motion)->position[2] + dz, &node);
    speed = sqrtf((dx * dx) + (dz * dz)) / gRuntimeFrameSeconds;
    if ((((PlayerControlPrefix *)motion)->permissions & 0x10) && ((currentGround - nextGround) > 1.0f) &&
        (speed < 16.5f)) {
        speed = 0.0f;
        dx = 0.0f;
        dz = 0.0f;
    }
    distance[0] =
        ((PlayerControlPrefix *)motion)->position[0] - ((RuntimeObjectPrefix *)(u8 *)gPikachuActor)->worldPosition[0];
    distance[1] =
        ((PlayerControlPrefix *)motion)->position[1] - ((RuntimeObjectPrefix *)(u8 *)gPikachuActor)->worldPosition[1];
    distance[2] =
        ((PlayerControlPrefix *)motion)->position[2] - ((RuntimeObjectPrefix *)(u8 *)gPikachuActor)->worldPosition[2];
    oldDistance = vector3Length(distance);
    if (oldDistance < 2.5f) {
        if (controllerIsRumblePatternActive(0) == 0) {
            if (((f64)randomNext() * 2.3283064365386963e-10) < 0.30000001192092896) {
                controllerRequestRumblePattern(0, 0.3f, 0.2f, 0.01f);
            }
        }
        distance[0] += dx;
        distance[1] += dy;
        distance[2] += dz;
        if (vector3Length(distance) < oldDistance) {
            ((PlayerControlPrefix *)motion)->viewShake =
                (f32)((((f64)randomNext() * 2.3283064365386963e-10) - 0.5) * 0.800000011920929);
            dx *= 0.7f;
            dz *= 0.7f;
            speed *= 0.7f;
        }
    }
    if (speed > 16.5f) {
        ((PlayerControlPrefix *)motion)->movementClass = 2;
    } else if (speed > 8.0f) {
        ((PlayerControlPrefix *)motion)->movementClass = 1;
    } else {
        ((PlayerControlPrefix *)motion)->movementClass = 0;
    }
    ((PlayerControlPrefix *)motion)->position[0] = (f32)(((PlayerControlPrefix *)motion)->position[0] + dx);
    ((PlayerControlPrefix *)motion)->position[1] = (f32)(((PlayerControlPrefix *)motion)->position[1] + dy);
    ((PlayerControlPrefix *)motion)->position[2] = (f32)(((PlayerControlPrefix *)motion)->position[2] + dz);
    ((PlayerControlPrefix *)motion)->footstepDistance =
        (f32)(((PlayerControlPrefix *)motion)->footstepDistance + (speed * gRuntimeFrameSeconds));
    rotatedX = dx;
    rotatedZ = dz;
    vector2RotateComponentsDegrees(((PlayerControlPrefix *)motion)->yaw, &rotatedX, &rotatedZ);
    backdropAdvanceScroll(-rotatedX);
    if ((nextGround < ((PlayerControlPrefix *)motion)->position[1]) && (blockedDrop == 0)) {
        ((PlayerControlPrefix *)motion)->movementClass = 3;
        ((PlayerControlPrefix *)motion)->airborne = 1;
        ((PlayerControlPrefix *)motion)->footstepDistance = 9.0f;
    } else {
        ((PlayerControlPrefix *)motion)->airborne = 0;
        playerPlayFootstepSound(motion, material);
    }
    if ((((PlayerControlPrefix *)motion)->airborne == 0) || !(((PlayerControlPrefix *)motion)->permissions & 4)) {
        if (((PlayerControlPrefix *)motion)->position[1] < nextGround) {
            ((PlayerControlPrefix *)motion)->position[1] = nextGround;
        }
        if (dy < 0.0f) {
            dy = 0.0f;
        }
    }
    if (dy > 0.7f) {
        dy = 0.7f;
    }
    ((PlayerControlPrefix *)motion)->downwardVelocity = (f32)-dy;
}

f32 mathSinDegrees(f32);
f32 mathCosDegrees(f32);
void itemThrow(s32, f32 *, f32 *, f32 *, s32);
/// Interior motion-coefficient view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125A68 (0x80125A68).
extern f32 gItemKindMotionCoefficientEntries[];
/// Throw the player held item along the current view direction with rumble feedback.
/// \param player Player record providing held-item selection, eye position, and view orientation.
/// \returns The thrown item handle, or zero if the held item is unavailable.
/// \note Original decompilation identifier: \c func_8010FC48 (0x8010FC48).
s32 playerThrowHeldItem(u8 *player) {
    s32 objectId;
    f32 pitch;
    u8 *state;
    f32 yaw;
    f32 vertical;
    f32 horizontal;
    f32 volume;
    f32 direction[3];
    f32 origin[3];
    u8 *object;
    object = objectFindLiveByHandle(*(s32 *)(player + 0x84));
    if (object == NULL)
        return 0;
    state = *(u8 **)(object + 0x54);
    yaw = *(f32 *)(player + 0x18) + *(f32 *)(player + 0xC);
    pitch = *(f32 *)(player + 0x10) + *(f32 *)(player + 0x14);
    vertical = mathSinDegrees(pitch) * 50.0f;
    pitch = *(f32 *)(player + 0x10) + *(f32 *)(player + 0x14);
    horizontal = mathCosDegrees(pitch) * 50.0f;
    origin[0] = *(f32 *)player;
    origin[1] = *(f32 *)(player + 0x1C) + *(f32 *)(player + 4);
    origin[2] = *(f32 *)(player + 8);
    direction[0] = mathSinDegrees(yaw) * horizontal;
    direction[1] = -vertical;
    direction[2] = mathCosDegrees(yaw) * horizontal;
    itemThrow(*(s32 *)(object + 0x38), origin, (f32 *)(object + 0x18), direction, 1);
    volume = mathMapRangeClamped(0.5f, 0.02f, 2.0f, 0.2f,
                                 *(f32 *)((u8 *)gItemKindMotionCoefficientEntries + *(s16 *)(state + 4) * 0x74));
    controllerRequestRumblePattern(0, volume, volume, volume * 0.5f);
    objectId = *(s32 *)(object + 0x38);
    TRACE("holdItem:%p\n", object);
    TRACE("item:%p\n", state);
    return objectId;
}

/// Remove controller input that is not permitted by the player input mask.
/// \param keep Allowed button mask, with 0x40 enabling horizontal stick input and 0x80 enabling vertical
/// stick input.
/// \param pad Writable controller state containing four button words and two signed stick words; filtered in
/// place.
/// \returns Removed held-button bits, plus 0x40 or 0x80 for a disabled stick axis whose magnitude was at
/// least fifty.
///
/// \note Controller stick values must remain in their supported range so magnitude calculations fit a signed
/// integer.
/// \note Original decompilation identifier: \c func_8010FDC8 (0x8010FDC8).
u16 playerFilterControllerInput(u16 keep, ControllerState *pad) {
    u16 dropped = pad->held & ~keep;
    s32 v;

    pad->held &= keep;
    pad->pressed &= keep;
    pad->released &= keep;
    pad->repeated &= keep;
    if (!(keep & 0x40)) {
        v = (pad->stickX >= 0) ? pad->stickX : -pad->stickX;
        if (v >= 50) {
            dropped |= 0x40;
        }
        pad->stickX = 0;
    }
    if (!(keep & 0x80)) {
        v = (pad->stickY >= 0) ? pad->stickY : -pad->stickY;
        if (v >= 50) {
            dropped |= 0x80;
        }
        pad->stickY = 0;
    }
    TRACE("contMask:0x%04x  err:0x%04x\n", keep, dropped);
    return dropped;
}

s32 areaDispatchPositionEvent(); /* extern */
/// Request an interaction event for the area region containing the player position.
/// \param position Address of three floating-point world coordinates, represented as an integer.
/// \returns One if the area callback reports a positive result, or zero otherwise.
///
/// \note Requires initialized current-area events. Portability: the position address is passed as a 32-bit
/// integer.
/// \note Original decompilation identifier: \c func_8010FE84 (0x8010FE84).
s32 playerRequestAreaInteractionEvent(s32 position) {
    if (areaDispatchPositionEvent(position, 2) > 0) {
        return 1;
    }
    return 0;
}

s32 playerThrowHeldItem(u8 *);                          /* extern */
void playerBeginTargetFocus(u8 *arg0, s32 arg1, f32 arg2); /* extern */
/// Attempt to throw the held item and temporarily track it when the throw succeeds.
/// \param player Player record containing the held item and receiving the focus request.
/// \returns Zero if no item is held, or one if a throw was attempted, even if the held item could not be
/// resolved.
/// \note Original decompilation identifier: \c func_8010FEB0 (0x8010FEB0).
s32 playerThrowHeldItemAndFocus(u8 *player) {
    s32 thrownItemHandle;

    if (((PlayerControlPrefix *)player)->heldItemHandle == 0) {
        return 0;
    }
    thrownItemHandle = playerThrowHeldItem(player);
    if (thrownItemHandle != 0) {
        playerBeginTargetFocus(player, thrownItemHandle, 10.0f);
    }
    return 1;
}

void areaRequestTransition(s32, s32); /* extern */
void playerSetChaseStage(s32 arg0);    /* extern */
void scriptVmResetGlobals();         /* extern */
/// Identifier of the currently loaded area.
/// \note Original identifier: \c D_800AE4F8 (0x800AE4F8).
extern s32 gRuntimeCurrentArea;

/// Request the area or chase-stage transition associated with an accessible interaction passage.
/// \param player Player record whose interaction target must be a passage object.
/// \returns One if a transition was requested, or zero if the passage is unavailable, blocked, or leads to
/// the current area.
/// \note Original decompilation identifier: \c func_8010FEFC (0x8010FEFC).
s32 playerTryEnterTargetPassage(u8 *player) {
    s32 passageKind;
    u8 *liveObject;
    u8 *passageState;

    liveObject = objectFindLiveByHandle(((PlayerControlPrefix *)player)->interactionTarget);
    if (liveObject == NULL) {
        return 0;
    }
    passageState = ((RuntimeObjectPrefix *)liveObject)->state.bytes;
    if (((PassageState *)passageState)->accessState != 0) {
        return 0;
    }
    passageKind = ((PassageState *)passageState)->kind;
    if ((passageKind == 2) || (passageKind == 3)) {
        TRACE("difficulty = %d\n", ((PassageState *)passageState)->destination);
        playerSetChaseStage(((PassageState *)passageState)->destination);
        scriptVmResetGlobals();
        runtimeRequestGameModeChange(6);
        goto block_10;
    }
    if (gRuntimeCurrentArea == ((PassageState *)passageState)->destination) {
        return 0;
    }
    areaRequestTransition(((PassageState *)passageState)->destination, ((PassageState *)passageState)->entrance);
block_10:
    return 1;
}


/// Request the transition to the saved-game confirmation prompt.
/// \param unused Unused interaction-action argument.
/// \returns One, indicating that the transition was requested.
/// \note Original decompilation identifier: \c func_8010FFA0 (0x8010FFA0).
s32 playerRequestSavePrompt(s32 unused) {
    runtimeRequestGameModeChange(7);
    return 1;
}


void objectRequestHierarchyDestruction(u8 *);                /* extern */

/// Use one unit of the selected interaction proxy, notify Pikachu, and remove the proxy when exhausted.
/// \param player Player record whose current target is a consumable interaction proxy.
/// \returns One if the proxy was used, or zero if the target is unavailable.
///
/// \note Requires a live active Pikachu actor and initialized consumable-proxy state.
/// \note Original decompilation identifier: \c func_8010FFC8 (0x8010FFC8).
s32 playerConsumeInteractionProxy(u8 *player) {
    u8 *obj;
    ScriptMessage msg;
    u8 *state;

    obj = objectFindLiveByHandle(((PlayerControlPrefix *)player)->interactionTarget);
    if (obj == NULL) {
        return 0;
    }
    state = ((RuntimeObjectPrefix *)obj)->state.bytes;
    ((InteractionProxyState *)state)->context -= 1;
    msg.arg1 = 0;
    msg.type = 0x11;
    msg.arg0 = ((InteractionProxyState *)state)->metadata;
    msg.arg2 = ((InteractionProxyState *)state)->context;
    msg.arg3 = 0;
    msg.arg4 = ((RuntimeObjectPrefix *)obj)->handle;
    actorPostScriptMessage(gPikachuActor, msg, 1);
    if (((InteractionProxyState *)state)->context <= 0) {
        objectRequestHierarchyDestruction(obj);
    }
    return 1;
}

s32 playerPickUpItem(s32); /* extern */

/// Attempt to pick up the current interaction target.
/// \param player Player record containing the target item handle.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801100A8 (0x801100A8).
void playerPickUpInteractionTarget(u8 *player) {
    playerPickUpItem(((PlayerControlPrefix *)player)->interactionTarget);
}

s32 itemAllowsCooperation(u8 *); /* extern */
s32 actorConfigGetDisplayName(s32, s32);
s32 itemGetDisplayName(s32, s32);

/// Request Pikachu cooperation with an eligible interaction target.
/// \param player Player record containing the current interaction target.
/// \returns One if the cooperation request was sent, or zero if the target was unavailable or ineligible.
///
/// \note Requires a live active Pikachu actor and valid per-kind state for the selected target.
/// \note Original decompilation identifier: \c func_801100C8 (0x801100C8).
s32 playerRequestTargetCooperation(u8 *player) {
    u8 *obj;
    ScriptMessage msg;
    s32 kind;
    u8 *state;

    obj = objectFindLiveByHandle(((PlayerControlPrefix *)player)->interactionTarget);
    if (obj == NULL) {
        return 0;
    }
    kind = ((RuntimeObjectPrefix *)obj)->kind;
    if (kind == 0x10) {
        state = ((RuntimeObjectPrefix *)obj)->state.bytes;
        if (!(((ActorScriptState *)state)->behaviorInhibitFlags & 0x10)) {
            return 0;
        }
        TRACE("coop - model:%p(%s)\n", obj, actorConfigGetDisplayName(kind, 1));
    } else if (kind == 0x86) {
        if (itemAllowsCooperation(obj) == 0) {
            return 0;
        }
        TRACE("coop - item:%p(%s)\n", obj,
              itemGetDisplayName(((ItemStatePrefix *)((RuntimeObjectPrefix *)obj)->state.bytes)->kind, 1));
    } else {
        return 0;
    }
    msg.arg1 = 0;
    msg.type = 0xF;
    msg.arg0 = 0;
    msg.arg2 = 0;
    msg.arg3 = 0;
    msg.arg4 = ((PlayerControlPrefix *)player)->interactionTarget;
    actorPostScriptMessage(gPikachuActor, msg, 1);
    return 1;
}

/// Primary player control record and base of the player records with a 256-byte stride.
/// \note Original identifier: \c D_800EF6A0 (0x800EF6A0).
extern PlayerControlPrefix gPlayerState; /* the player record */
/// Separate symbol view of the primary player record's current control permissions.
/// \note Original identifier: \c D_800EF6C4 (0x800EF6C4).
extern s32 gPlayerPermissions;
/// Separate symbol view of the primary player companion-control target handle.
/// \note Original identifier: \c D_800EF6F0 (0x800EF6F0).
extern u8 gPlayerCompanionTarget[];
/// Separate symbol view of the primary player record's mode and camera-transition state.
/// \note Original identifier: \c D_800EF728 (0x800EF728).
extern u8 gPlayerModeState[];

/// Change the primary player behavior mode, completing the previous mode and applying the new mode
/// permissions.
/// \param mode Player behavior mode identifier from zero through thirty-one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801101D0 (0x801101D0).
void playerChangeMode(s32 mode) {
    PlayerMode *m;
    u8 *mc = gPlayerModeState; /* the player's mode block (player + 0x88) */

    if (((PlayerControlPrefix *)&gPlayerState)->modeState.leave != 0) {
        ((PlayerModeState *)gPlayerModeState)->leave((u8 *)&gPlayerState);
    }
    if (mode == 9) {
        controllerRequestRumblePattern(0, 1.5f, 0.2f, 0.15f);
    }
    TRACE("change player type %d\n", mode);
    m = &gPlayerModes[mode];
    ((PlayerModeState *)mc)->update = m->update;
    ((PlayerModeState *)mc)->leave = m->leave;
    gPlayerPermissions = m->flags;
    ((PlayerModeState *)mc)->mode = mode;
    ((PlayerControlPrefix *)((u8 *)gPlayerCompanionTarget - 0x50))->modeElapsed = 0.0f;
}

/// Separate symbol view of the primary player record's controller port.
/// \note Original identifier: \c D_800EF6CC (0x800EF6CC).
extern s32 gPlayerControllerPort;
void playerChangeMode(s32); /* extern */

/// Apply a primary player control scheme, including behavior mode, controller assignment, and permitted
/// input.
/// \param scheme Control-scheme identifier from zero through twenty-seven.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80110288 (0x80110288).
void playerApplyControlScheme(s32 scheme) {
    ModeRec *rec = &gPlayerControlSchemes[scheme];

    playerChangeMode(rec->mode);
    ((PlayerControlPrefix *)((u8 *)&gPlayerControllerPort - 0x2C))->keptButtons = rec->keepMask;
    ((PlayerControlPrefix *)((u8 *)&gPlayerControllerPort - 0x2C))->controllerPort = rec->port;
}

/// Separate symbol view of the primary player record's eye height.
/// \note Original identifier: \c D_800EF6BC (0x800EF6BC).
extern f32 gPlayerEyeHeight;

/// Separate symbol view of the primary player record's saved control permissions.
/// \note Original identifier: \c D_800EF6C8 (0x800EF6C8).
extern s32 gPlayerSavedPermissions;

/// Return the primary player to normal control with its saved control permissions.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801102DC (0x801102DC).
void playerResumeNormalControl(void) {
    playerChangeMode(1);
    gPlayerPermissions = gPlayerSavedPermissions;
}

s32 progressGetValue();                       /* extern */
void toolboxOpen(s32, s32); /* extern */
u8 playerGetChaseStage(void);                  /* extern */
void systemMenuEnter();                 /* extern */

/// Handle a permitted menu request by opening the system menu or toolbox appropriate to current progress.
/// \param player Player record providing menu permissions and tracked playback.
/// \param controller Controller state supplying newly pressed buttons.
/// \returns One if a menu was requested, or zero if no permitted request was present.
/// \note Original decompilation identifier: \c func_80110308 (0x80110308).
s32 playerTryOpenMenu(u8 *player, u8 *controller) {
    if ((playerGetChaseStage() <= 0) || (progressGetValue(0x12) != 0)) {
        if ((((ControllerState *)controller)->pressed & 0x1000) &&
            (((PlayerControlPrefix *)player)->permissions & 0x1000)) {
            playerStopTrackedSoundEffect(player);
            systemMenuEnter();
            return 1;
        }
        goto block_8;
    }
    if ((((ControllerState *)controller)->pressed & 0x1000) && (((PlayerControlPrefix *)player)->permissions & 0x100)) {
        playerStopTrackedSoundEffect(player);
        toolboxOpen(-1, 1);
        return 1;
    }
block_8:
    return 0;
}

/// Dispatch a permitted activation request to the selected player interaction action.
/// \param player Player record providing activation permissions and a selected interaction action from zero
/// through nine.
/// \param controller Controller state supplying newly pressed activation input.
/// \returns The selected action result, which is unspecified for actions without a defined result; zero when
/// no action is dispatched.
///
/// \note Undefined behavior: action handlers have incompatible argument and return types. The original ABI
/// passes the player address as either a pointer or integer and ignores extra arguments; the pickup handler
/// has no defined return value.
/// \note Original decompilation identifier: \c func_801103C8 (0x801103C8).
s32 playerTryActivateInteraction(u8 *player, u8 *controller) {
    s32 (*handler)(u8 *, u8 *);

    if (!(((PlayerControlPrefix *)player)->permissions & 0x400)) {
        return 0;
    }
    handler = gPlayerInteractionActionHandlers[((PlayerControlPrefix *)player)->modeState.interactionAction];
    if ((handler != NULL) && (((ControllerState *)controller)->pressed & 0x8000)) {
        return handler(player, controller);
    }
    return 0;
}

/* Inspected world object: kind at 0x34, item ID at 0x38, per-kind state at 0x54;
 * the inspecting player keeps the target ID at 0x5C, flags at 0x60 and the
 * selected action at 0x98. */
typedef struct InspectState {
    s32 unk0;
    s16 unk4;
    u8 unknown6[6];
    s32 unkC;
} InspectState;
typedef struct InspectObject {
    u8 unknown0[0x34];
    u16 unk34;
    u16 unknown36;
    s32 unk38;
    u8 unknown3C[0x18];
    InspectState *unk54;
    s32 unknown58;
    s32 unk5C;
    s32 unk60;
    u8 unknown64[0x34];
    s32 unk98;
} InspectObject;
s32 playerPrepareInteractionDescription(InspectObject *);
s32 textWindowOpen(s32, s32, s32); /* extern */
void textWindowAppendText(u8 *); /* extern */
void textWindowEnterMode(void);      /* extern */
/// First word of the shared 256-byte player help text buffer; storage continues through adjacent symbols.
/// \note Original identifier: \c D_80133F20 (0x80133F20).
extern u8 gPlayerHelpTextBuffer[4];

/// Handle a permitted inspection request by displaying help for the current interaction target.
/// \param player Player record providing inspection permissions and target selection.
/// \param controller Controller state supplying newly pressed buttons.
/// \returns One if the inspection text window opened, or zero otherwise.
/// \note Original decompilation identifier: \c func_80110438 (0x80110438).
s32 playerTryInspectTarget(u8 *player, u8 *controller) {
    s32 ret;

    if (!(((PlayerControlPrefix *)player)->permissions & 0x800)) {
        return 0;
    }
    ret = 0;
    if (((ControllerState *)controller)->pressed & 0x4000) {
        playerPrepareInteractionDescription((InspectObject *)player);
        if (textWindowOpen(2, 0, (((PlayerControlPrefix *)player)->permissions & 0x10000) ? 1 : 0) == 0) {
            return 0;
        }
        textWindowAppendText(gPlayerHelpTextBuffer);
        textWindowEnterMode();
        return 1;
    }
    return ret;
}

s32 playerCanEnterStationaryLook(void); /* extern */

/// Handle a permitted stationary-look request and turn off companion tracking.
/// \param player Primary player record receiving the stationary-look selection.
/// \param controller Controller state supplying the look-mode request.
/// \returns One if stationary look was requested, or zero if no permitted request was present.
///
/// \note Requires a live active Pikachu actor when a permitted mode change is requested.
/// \note Original decompilation identifier: \c func_801104D8 (0x801104D8).
s32 playerTryEnterStationaryLook(u8 *player, u8 *controller) {
    if (playerCanEnterStationaryLook() == 0) {
        return 0;
    }
    if ((((ControllerState *)controller)->pressed & 0x404) || (((ControllerState *)controller)->held & 0x10)) {
        playerSetCompanionControlMode(player, 0, 0, ((RuntimeObjectPrefix *)(u8 *)gPikachuActor)->handle);
        playerChangeMode(2);
        if (((ControllerState *)controller)->held & 0x10) {
            ((PlayerControlPrefix *)player)->modeState.companionHoldRequired = 1;
        } else {
            ((PlayerControlPrefix *)player)->modeState.companionHoldRequired = 0;
        }
        return 1;
    }
    return 0;
}

s32 playerCanToggleCompanionControl();  /* extern */

/// Handle a permitted companion-control toggle request, selecting Pikachu or follow mode according to saved
/// preferences.
/// \param player Player record whose companion-control selection may change.
/// \param controller Controller state supplying the toggle request.
/// \returns One if the toggle request was handled, or zero if no permitted request was present.
///
/// \note Requires a live active Pikachu actor when a permitted mode change is requested.
/// \note Original decompilation identifier: \c func_80110590 (0x80110590).
s32 playerToggleCompanionControl(u8 *player, u8 *controller) {
    s32 companionMode;

    if (!(((ControllerState *)controller)->pressed & 0x108) || (playerCanToggleCompanionControl() == 0)) {
        return 0;
    }
    companionMode = ((PlayerControlPrefix *)player)->companionMode;
    switch (companionMode) {
    case 0:
        if (progressGetValue(8) != 0) {
            playerSetCompanionControlMode(player, 2, 0, ((RuntimeObjectPrefix *)(u8 *)gPikachuActor)->handle);
        } else {
            playerSetCompanionControlMode(player, 1, 0, ((RuntimeObjectPrefix *)(u8 *)gPikachuActor)->handle);
        }
        break;
    case 1:
        if (!(((PlayerControlPrefix *)player)->permissions & 0x40000000)) {
            return 0;
        }
        playerSetCompanionControlMode(player, 0, 0, ((RuntimeObjectPrefix *)(u8 *)gPikachuActor)->handle);
        return 1;
    case 2:
        if (!(((PlayerControlPrefix *)player)->permissions & 0x80000000)) {
            return 0;
        }
        playerSetCompanionControlMode(player, 0, 0, ((RuntimeObjectPrefix *)(u8 *)gPikachuActor)->handle);
        break;
    default:
        TRACE("ERROR: unknown markMode:%d\n", companionMode);
        break;
    }
    return 1;
}

/// Service area events at the player position when area-event processing is permitted.
/// \param player Player record providing world position and area-event permission.
/// \returns The area callback result, minus one if no callback is defined, or zero when processing is
/// disabled or a repeated event is deferred.
/// \note Original decompilation identifier: \c func_801106C0 (0x801106C0).
s32 playerUpdateAreaEvents(u8 *player) {
    if (((PlayerControlPrefix *)player)->permissions & 0x20) {
        return areaDispatchPositionEvent(player, 1);
    }
    return 0;
}

f32 playerCalculateYawInput(u8 *, u8 *, s32);
void playerTrackCompanionViewTarget(u8 *, u8 *, u8 *);
void playerApplyFollowMovementInput(u8 *, u8 *, u8 *);
void playerRecenterWalkingView(u8 *);
/// Update manual view control or companion tracking and follow movement according to the selected
/// companion-control mode.
/// \param player Player record providing companion-control selection and view state.
/// \param controller Controller state supplying manual input and receiving automatic follow movement when
/// applicable.
/// \returns Zero if a required companion target is unavailable, or one otherwise.
/// \note Original decompilation identifier: \c func_801106FC (0x801106FC).
s32 playerUpdateCompanionControl(u8 *player, u8 *controller) {
    s32 mode;
    u8 *target;
    mode = ((PlayerControlPrefix *)player)->companionMode;
    switch (mode) {
    case 0:
        playerRecenterWalkingView(player);
        playerSetViewAngles((u8 *)player,
                            playerCalculateYawInput(player, controller, 0) +
                                (*(f32 *)((s8 *)player + (u32) & ((PlayerControlPrefix *)0)->yaw)),
                            ((PlayerControlPrefix *)player)->pitch);
        break;
    case 1:
        target = playerResolveCompanionViewTarget(player);
        if (target == NULL)
            return 0;
        playerTrackCompanionViewTarget(player, controller, target);
        break;
    case 2:
        target = playerResolveCompanionViewTarget(player);
        if (target == NULL)
            return 0;
        playerTrackCompanionViewTarget(player, controller, target);
        playerApplyFollowMovementInput(controller, player, target);
        break;
    default:
        TRACE("ERROR: unknown markMode:%d\n", mode);
        break;
    }
    return 1;
}

void playerCalculateMovementInput(u8 *, u8 *, f32 *, f32 *); /* extern */
void playerStepGroundMovement(u8 *, f32, f32);
s32 playerTryOpenMenu(u8 *arg0, u8 *arg1); /* extern */
s32 playerTryActivateInteraction();                   /* extern */
s32 playerTryInspectTarget();                         /* extern */
s32 playerTryEnterStationaryLook(u8 *arg0, u8 *arg1); /* extern */
s32 playerToggleCompanionControl(u8 *arg0, u8 *arg1); /* extern */
s32 playerUpdateAreaEvents(u8 *arg0);           /* extern */
s32 playerUpdateCompanionControl();             /* extern */
/// Advance normal player movement, view control, interactions, and permitted menu requests.
/// \param player Player record to update.
/// \param controller Controller state address.
/// \returns Nothing.
///
/// \note A checked member-offset angle read preserves the original IDO floating-point operand order.
/// \note Original decompilation identifier: \c func_801107F0 (0x801107F0).
void playerUpdateNormalControl(u8 *player, u8 *controller) {
    f32 sp24;
    f32 sp20;

    if ((playerTryOpenMenu(player, controller) == 0) && (playerTryActivateInteraction(player, controller) == 0) && (playerTryInspectTarget(player, controller) == 0) &&
        (playerTryEnterStationaryLook(player, controller) == 0) && (playerToggleCompanionControl(player, controller) == 0)) {
        playerUpdateCompanionControl(player, controller);
        playerCalculateMovementInput(controller, player, &sp24, &sp20);
        playerStepGroundMovement(player, sp24, sp20);
        playerUpdateAreaEvents(player);
    }
}

f32 playerCalculatePitchInput(u8 *, u8 *, s32);
/// Advance stationary view control and permitted interactions, returning to normal control when this mode
/// ends.
/// \param player Player record whose stationary view mode is updated.
/// \param controller Controller state supplying view and interaction input.
/// \returns Nothing.
/// \note A checked member-offset angle read preserves the original IDO floating-point operand order.
/// \note Original decompilation identifier: \c func_8011089C (0x8011089C).
void playerUpdateStationaryLook(u8 *player, u8 *controller) {
    f32 dx;
    s32 mode;
    if (playerTryInspectTarget(player, controller) == 0) {
        if ((((ControllerState *)controller)->pressed & 0x404) ||
            ((mode = ((PlayerControlPrefix *)player)->modeState.companionHoldRequired, mode != 0) &&
             !(((ControllerState *)controller)->held & 0x10)) ||
            ((mode == 0) && (((ControllerState *)controller)->held & 0x10)) ||
            playerTryActivateInteraction(player, controller) != 0 ||
            playerToggleCompanionControl(player, controller) != 0 || playerTryOpenMenu(player, controller) != 0) {
            playerResumeNormalControl();
            return;
        }
        dx = playerCalculateYawInput(player, controller, 0);
        playerSetViewAngles((u8 *)player, ((PlayerControlPrefix *)player)->yaw + dx,
                            playerCalculatePitchInput(player, controller, progressGetValue(7)) +
                                (*(f32 *)((s8 *)player + (u32) & ((PlayerControlPrefix *)0)->pitch)));
        playerStepGroundMovement(player, 0.0f, 0.0f);
        playerUpdateAreaEvents(player);
    }
}

/// Service permitted menu requests while the player remains in a mode without movement updates.
/// \param player Player record providing menu permissions.
/// \param controller Controller state supplying menu input.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801109C8 (0x801109C8).
void playerUpdateMenuOnly(u8 *player, u8 *controller) {
    playerTryOpenMenu(player, controller);
}

/// Advance the alternating horizontal view shake used by player reaction effects.
/// \param player Player record receiving the view shake.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801109E8 (0x801109E8).
void playerToggleViewShake(u8 *player) {
    if (((PlayerControlPrefix *)player)->viewShake < 0.0f) {
        ((PlayerControlPrefix *)player)->viewShake = 0.2f;
        return;
    }
    ((PlayerControlPrefix *)player)->viewShake = -0.2f;
}

void cameraFadeOverlayToColor(s32, s32, s32, s32, s32, f32); /* extern */
void controllerRequestRumbleStop(s32);                                      /* extern */

/// End player stun feedback by clearing view shake, fading the screen flash, and stopping rumble.
/// \param player Player record whose view shake is cleared.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80110A24 (0x80110A24).
void playerFinishStun(u8 *player) {
    ((PlayerControlPrefix *)player)->viewShake = 0.0f;
    cameraFadeOverlayToColor(0, 0, 0, 0, 0, 0.8f);
    controllerRequestRumbleStop(0);
}

void playerResumeNormalControl(void);              /* extern */
void playerToggleViewShake(u8 *arg0);          /* extern */

/// Advance the timed player stun reaction, allowing menus and returning to the chapter-appropriate control
/// mode when it ends.
/// \param player Player record containing the remaining stun time.
/// \param controller Controller state supplying menu requests.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80110A74 (0x80110A74).
void playerUpdateStun(u8 *player, u8 *controller) {
    if (playerTryOpenMenu(player, controller) == 0) {
        ((PlayerControlPrefix *)player)->stunRemaining =
            (f32)(((PlayerControlPrefix *)player)->stunRemaining - gRuntimeFrameSeconds);
        if (((PlayerControlPrefix *)player)->stunRemaining < 0.0f) {
            if (progressGetCurrentChapter() == 4) {
                playerChangeMode(0x1B);
                return;
            }
            playerResumeNormalControl();
            return;
        }
        playerToggleViewShake(player);
    }
}

/// Apply a brief view shake during the initial portion of the current player mode.
/// \param player Player record providing elapsed mode time and receiving view shake.
/// \param unused Unused mode-update argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80110B04 (0x80110B04).
void playerUpdateBriefViewShake(u8 *player, u8 *unused) {
    if (((PlayerControlPrefix *)player)->modeElapsed < 1.5f) {
        playerToggleViewShake(player);
    }
}

s32 messageBarEnqueue(s32, f32, u8 *);
/// Display the next prepared interaction-help line, if one remains.
/// \returns One if more help text remains after this line, or zero when no further text remains.
///
/// \note Requires a prepared, null-terminated help-text cursor. Each line before a newline or terminator must
/// contain at most 255 bytes.
/// \note Original decompilation identifier: \c func_80110B44 (0x80110B44).
s32 playerShowNextHelpLine(void) {
    u8 line[0x100];
    u8 *dst;
    if (((u32) * (u8 *)gPlayerHelpTextCursor) == 0)
        return 0;
    dst = line;
    while (((u32) * (u8 *)gPlayerHelpTextCursor) != 0 && ((u32) * (u8 *)gPlayerHelpTextCursor) != '\n') {
        *dst++ = ((u32) * (u8 *)gPlayerHelpTextCursor);
        gPlayerHelpTextCursor = (u8 *)((u8 *)gPlayerHelpTextCursor + 1);
    }
    *dst = 0;
    if (((u32) * (u8 *)gPlayerHelpTextCursor) == '\n')
        gPlayerHelpTextCursor = (u8 *)((u8 *)gPlayerHelpTextCursor + 1);
    messageBarEnqueue(0, 2147483648.0f, line);
    if (*(u8 *)gPlayerHelpTextCursor != 0)
        return 1;
    return 0;
}

s8 *doorGetDescription(u8 *);
u8 *doorGetAccessDescription(u8 *);
s32 actorConfigGetDescription(s32, s32);
s8 *landmarkGetCannonDescription(s32);
s8 *caveGetDescription(u8 *);
void playerSetInteractionHelpText(u8 *);
u8 *itemGetClassLabelSuffix(s32);
s32 itemGetHelpText(s32, s32);
/* Item records are 0x74 bytes; the class byte is at +1. */
/// Separate classification-byte view of the item catalog, with a 116-byte stride.
/// \note Original identifier: \c D_80125AB1 (0x80125AB1).
extern s8 gItemKindClassEntries;
#define ITEM_CLASS(kind) (*(&gItemKindClassEntries + ((kind) * 0x74)))

/// Prepare descriptive help for the player interaction target, including reachability and action prompts
/// where applicable.
/// \param player Player record providing the target, reachability, and selected interaction action from zero
/// through nine.
/// \returns The described target handle, or zero if no supported target is available; an absent target
/// prepares guidance for selecting one.
///
/// \note Target state and catalog identifiers must be valid for their object kind. The formatted description,
/// including prompts, must fit in 255 bytes plus its terminator.
/// \note Original decompilation identifier: \c func_80110C0C (0x80110C0C).
s32 playerPrepareInteractionDescription(InspectObject *player) {
    s8 text[0x100];
    InspectObject *item;
    InspectState *state;
    s8 *suffix;
    s8 *reachSuffix;
    u16 kind;

    item = (InspectObject *)objectFindLiveByHandle(player->unk5C);
    if (item == NULL) {
        playerSetInteractionHelpText(
            (u8 *)"(There\'s nothing to check.)\nMove the @A@B cursor to any\nitem you want to check "
                  "and\npress @K@L to see a description\nof it.");
        return 0;
    }
    kind = item->unk34;
    if (kind == 0x25) {
        playerSetInteractionHelpText((u8 *)"???");
        return item->unk38;
    }
    if (kind == 0x86) {
        state = item->unk54;
        if (player->unk60 & 2) {
            sprintf(text, "%s%-22s^^%s\n%s%s",
                    ((ITEM_CLASS(state->unk4) == 2) || (ITEM_CLASS(state->unk4) == 5)) ? "^^" : "^4",
                    (s8 *)itemGetDisplayName(state->unk4, 1), (s8 *)itemGetClassLabelSuffix(state->unk4),
                    (s8 *)itemGetHelpText(state->unk4, 1), gPlayerInteractionPromptSuffixes[player->unk98]);
        } else {
            sprintf(text, "%s%s^^?\n%s%s",
                    ((ITEM_CLASS(state->unk4) == 2) || (ITEM_CLASS(state->unk4) == 5)) ? "^^" : "^4",
                    (s8 *)itemGetDisplayName(state->unk4, 0), (s8 *)itemGetHelpText(state->unk4, 0),
                    "\n^9You can't reach it from here.^^");
        }
        playerSetInteractionHelpText((u8 *)text);
        return item->unk38;
    }
    if (((s32)kind > 0) && ((s32)kind < 0x26)) {
        if (player->unk60 & 2) {
            sprintf(text, "^4%s^^\n%s", (s8 *)actorConfigGetDisplayName(item->unk34, 1), (s8 *)actorConfigGetDescription(item->unk34, 1));
        } else {
            sprintf(text, "^4%s^^?\n%s%s", (s8 *)actorConfigGetDisplayName(item->unk34, 0), (s8 *)actorConfigGetDescription(item->unk34, 0),
                    ((item->unk34 == 5) || (item->unk34 == 6)) ? "\n^9You can't reach it from here.^^" : "");
        }
        playerSetInteractionHelpText((u8 *)text);
        return item->unk38;
    }
    if (kind == 0x87) {
        if (player->unk60 & 2) {
            state = item->unk54;
            if (state->unkC != 0) {
                suffix = (s8 *)doorGetAccessDescription((u8 *)item);
            } else if (state->unk0 == 1) {
                suffix = "\nPress @I@J to go on.";
            } else {
                suffix = gPlayerInteractionPromptSuffixes[7];
            }
            sprintf(text, "%s%s", doorGetDescription((u8 *)item), suffix);
        } else {
            sprintf(text, "%s%s", doorGetDescription((u8 *)item), "\n^9You can't reach it from here.^^");
        }
        playerSetInteractionHelpText((u8 *)text);
        return item->unk38;
    }
    if (kind == 0x89) {
        /// Encoded interaction label for object kind 0x89.
        /// \note Original identifier: \c D_80123EA0 (0x80123EA0).
        static char *gPlayerKind89InteractionLabel = "\xA1\xA9\xA1\xA9\xA1\xA9";

        if (player->unk60 & 2) {
            sprintf(text, "%s%s", gPlayerKind89InteractionLabel, gPlayerInteractionPromptSuffixes[8]);
        } else {
            sprintf(text, "%s%s", gPlayerKind89InteractionLabel, "\n^9You can't reach it from here.^^");
        }
        playerSetInteractionHelpText((u8 *)text);
        return item->unk38;
    }
    if (kind == 0xB0) {
        sprintf(text, "%s", caveGetDescription((u8 *)item));
        playerSetInteractionHelpText((u8 *)text);
        return item->unk38;
    }
    if (kind == 0x8F) {
        sprintf(text, "%s", landmarkGetCannonDescription((s32)item));
        playerSetInteractionHelpText((u8 *)text);
        return item->unk38;
    }
    if (kind == 0x8A) {
        reachSuffix = (s8 *)item->unk54;
        sprintf(text, "%s", (s8 *)((InspectState *)reachSuffix)->unkC);
        TRACE("Illegal difficulty:%d\n", ((InspectState *)reachSuffix)->unk0);
        playerSetInteractionHelpText((u8 *)text);
        return item->unk38;
    }
    return 0;
}


/// Clear the primary camera screen tint when leaving the help-display mode.
/// \param unused Unused mode-exit argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801110D8 (0x801110D8).
void playerFinishHelpDisplay(u8 *unused) {
    cameraFadeOverlayToColor(0, 0, 0, 0, 0, 0.0f);
}

s32 playerShowNextHelpLine();                   /* extern */
/// Advance prepared help text on confirmation input and resume normal control after the last line, while
/// allowing menu requests.
/// \param player Player record providing menu permissions.
/// \param controller Controller state supplying confirmation and menu input.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80111114 (0x80111114).
void playerUpdateHelpDisplay(u8 *player, u8 *controller) {
    if ((playerTryOpenMenu(player, controller) == 0) && (((ControllerState *)controller)->pressed & 0xC000) &&
        (playerShowNextHelpLine() == 0)) {
        playerResumeNormalControl();
    }
}

void playerRestoreCompanionControlMode(); /* extern */
/// Restore the previous companion-control selection when target focus ends, if that selection is currently
/// permitted.
/// \param player Player record whose saved companion-control selection is restored.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80111164 (0x80111164).
void playerFinishTargetFocus(u8 *player) {
    playerRestoreCompanionControlMode(player);
}

void playerSaveCompanionControlMode(u8 *arg0); /* extern */
/// Temporarily focus the player view on a target while preserving the previous companion-control selection.
/// \param player Primary player record receiving the focus request.
/// \param targetHandle Object handle to focus on.
/// \param duration Maximum focus duration in seconds; player input or an unavailable target can end focus
/// earlier.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80111184 (0x80111184).
void playerBeginTargetFocus(u8 *player, s32 targetHandle, f32 duration) {
    playerChangeMode(5);
    ((PlayerControlPrefix *)player)->focusRemaining = duration;
    playerSaveCompanionControlMode(player);
    playerSetCompanionControlMode(player, 1, 0, targetHandle);
}

/// Advance temporary target focus while allowing permitted interactions and ending focus when interrupted or
/// expired.
/// \param player Player record containing the focus target and remaining focus time.
/// \param controller Controller state supplying interactions and focus interruption input.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801111D4 (0x801111D4).
void playerUpdateTargetFocus(u8 *player, u8 *controller) {
    u8 *target;
    s32 x;
    s32 y;
    if (playerTryOpenMenu(player, controller) == 0) {
        if (playerTryActivateInteraction(player, controller) != 0) {
            ((PlayerControlPrefix *)player)->focusRemaining = 0.0f;
            return;
        }
        if (playerTryInspectTarget(player, controller) == 0) {
            target = playerResolveCompanionViewTarget(player);
            ((PlayerControlPrefix *)player)->focusRemaining -= gRuntimeFrameSeconds;
            if (((PlayerControlPrefix *)player)->focusRemaining < 0.0f ||
                ((ControllerState *)controller)->pressed != 0 ||
                ((x = ((ControllerState *)controller)->stickX) >= 0 ? x : -x) >= 11 ||
                ((y = ((ControllerState *)controller)->stickY) >= 0 ? y : -y) >= 11 || target == NULL) {
                playerResumeNormalControl();
            } else {
                playerTrackCompanionViewTarget(player, controller, target);
                playerStepGroundMovement(player, 0.0f, 0.0f);
                playerUpdateAreaEvents(player);
            }
        }
    }
}

/// Advance player movement with backward travel disabled, while servicing permitted interactions and menu
/// requests.
/// \param player Player record receiving movement and rejected-input status updates.
/// \param controller Controller state filtered in place for the restricted movement mode.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801112FC (0x801112FC).
void playerUpdateRestrictedMovement(u8 *player, u8 *controller) {
    f32 sp2C;
    f32 sp28;

    if ((playerTryOpenMenu(player, controller) == 0) && (playerTryActivateInteraction(player, controller) == 0) && (playerTryInspectTarget(player, controller) == 0)) {
        if (((ControllerState *)controller)->stickY != 0) {
            ((ControllerState *)controller)->stickX = 0;
        }
        if (((ControllerState *)controller)->stickY < 0) {
            if (((ControllerState *)controller)->stickY < -0x32) {
                ((PlayerControlPrefix *)player)->handledButtons =
                    (u16)(((PlayerControlPrefix *)player)->handledButtons | 0x80);
            }
            ((ControllerState *)controller)->stickY = 0;
        } else {
            ((PlayerControlPrefix *)player)->handledButtons =
                (u16)(((PlayerControlPrefix *)player)->handledButtons & 0xFF7F);
        }
        playerCalculateMovementInput(controller, player, &sp2C, &sp28);
        playerStepGroundMovement(player, sp2C, sp28);
    }
}

/// Advance the selected restricted player-control scheme, servicing permitted movement or view control and
/// interactions.
/// \param player Player record providing control permissions and receiving handled-input status.
/// \param controller Controller state supplying movement, view, and interaction requests.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801113B8 (0x801113B8).
void playerUpdateRestrictedControl(u8 *player, u8 *controller) {
    f32 dx;
    f32 dz;
    f32 x;
    f32 y;
    if (playerTryOpenMenu(player, controller) == 0) {
        if (playerTryActivateInteraction(player, controller) != 0) {
            ((PlayerControlPrefix *)player)->handledButtons |= 0x8000;
            return;
        }
        if (playerTryInspectTarget(player, controller) != 0) {
            ((PlayerControlPrefix *)player)->handledButtons |= 0x4000;
            return;
        }
        if (playerToggleCompanionControl(player, controller) == 0) {
            if (((PlayerControlPrefix *)player)->permissions & 0x8000) {
                dx = playerCalculateYawInput(player, controller, 0);
                dz = playerCalculatePitchInput(player, controller, progressGetValue(7));
                playerSetViewAngles((u8 *)player, ((PlayerControlPrefix *)player)->yaw + dx,
                                    dz + ((PlayerControlPrefix *)player)->pitch);
                if (((ControllerState *)controller)->pressed & 0x404) {
                    playerApplyControlScheme(0x14);
                }
            } else {
                playerUpdateCompanionControl(player, controller);
                playerCalculateMovementInput(controller, player, &x, &y);
                playerStepGroundMovement(player, x, y);
                if (((ControllerState *)controller)->pressed & 0x404) {
                    playerApplyControlScheme(9);
                }
            }
        }
    }
}


/// Advance view control and permitted interactions for a player mode without movement updates.
/// \param player Player record whose view and control scheme may change.
/// \param pad Controller state supplying view, interaction, and menu requests.
/// \returns Nothing.
/// \note A checked member-offset angle read preserves the original IDO floating-point operand order.
/// \note Original decompilation identifier: \c func_80111508 (0x80111508).
void playerUpdateViewOnlyControl(u8 *player, u8 *pad) {
    f32 dx;

    if (playerTryOpenMenu(player, pad) != 0) {
        return;
    }
    if (playerTryActivateInteraction(player, pad) != 0) {
        playerApplyControlScheme(0x12);
        return;
    }
    if (playerTryInspectTarget(player, pad) != 0) {
        return;
    }
    dx = playerCalculateYawInput(player, pad, 0);
    playerSetViewAngles((u8 *)player, ((PlayerControlPrefix *)player)->yaw + dx,
                        playerCalculatePitchInput(player, pad, progressGetValue(7)) +
                            (*(f32 *)((s8 *)player + (u32) & ((PlayerControlPrefix *)0)->pitch)));
}

/// Advance player movement and view control while allowing inspection and menus.
/// \param player Player record to update.
/// \param controller Processed controller sample for the current update.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_801115C0 (0x801115C0).
void playerUpdateMovementWithoutActions(u8 *player, u8 *controller) {
    f32 sp24;
    f32 sp20;

    if ((playerTryOpenMenu(player, controller) == 0) && (playerTryInspectTarget(player, controller) == 0)) {
        playerUpdateCompanionControl(player, controller);
        playerCalculateMovementInput(controller, player, &sp24, &sp20);
        playerStepGroundMovement(player, sp24, sp20);
        playerUpdateAreaEvents(player);
    }
}

/// Service permitted interactions and menu requests while the player has no available held item.
/// \param player Player record providing held-item selection and interaction permissions.
/// \param controller Processed controller sample for the current update.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_80111638 (0x80111638).
void playerUpdateEmptyHandInteraction(u8 *player, u8 *controller) {
    if ((objectFindLiveByHandle(((PlayerControlPrefix *)player)->heldItemHandle) == 0) &&
        (playerTryOpenMenu(player, controller) == 0) && (playerTryActivateInteraction(player, controller) == 0)) {
        playerTryInspectTarget(player, controller);
    }
}

/// Service permitted inspection and menu requests while the player has an available held item.
/// \param player Player record providing held-item selection and inspection permissions.
/// \param controller Processed controller sample for the current update.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_80111690 (0x80111690).
void playerUpdateHeldItemInspection(u8 *player, u8 *controller) {
    if ((objectFindLiveByHandle(((PlayerControlPrefix *)player)->heldItemHandle) != 0) &&
        (playerTryOpenMenu(player, controller) == 0)) {
        playerTryInspectTarget(player, controller);
    }
}

void cameraSetEyeAndTarget(s32, f32 *, f32 *);       /* extern */
s32 vector3InterpolateSmoothTimed(u8 *, u8 *, f32 *, f32, f32); /* extern */

/// Advance the configured smooth camera transition and apply the resulting primary camera view.
/// \param player Player record containing the configured camera transition.
/// \param unused Unused mode-update argument.
/// \returns Nothing.
///
/// \note Requires a transition prepared for this player and a valid primary camera. Portability: the local
/// interpolation declaration passes its duration as an integer to a helper defined with a floating-point
/// parameter.
/// \note Original decompilation identifier: \c func_801116D8 (0x801116D8).
void playerUpdateCameraTransition(u8 *player, u8 *unused) {
    f32 sp3C[3];
    f32 sp30[3];

    ((PlayerControlPrefix *)player)->modeState.cameraRemaining =
        (f32)(((PlayerControlPrefix *)player)->modeState.cameraRemaining - gRuntimeFrameSeconds);
    if (((PlayerControlPrefix *)player)->modeState.cameraRemaining < 0.0f) {
        ((PlayerControlPrefix *)player)->modeState.cameraRemaining = 0.0f;
    }
    vector3InterpolateSmoothTimed(player + 0xAC, player + 0xA0, sp3C,
                                  ((PlayerControlPrefix *)player)->modeState.cameraDuration,
                                  ((PlayerControlPrefix *)player)->modeState.cameraRemaining);
    vector3InterpolateSmoothTimed(player + 0xC4, player + 0xB8, sp30,
                                  ((PlayerControlPrefix *)player)->modeState.cameraDuration,
                                  ((PlayerControlPrefix *)player)->modeState.cameraRemaining);
    cameraSetEyeAndTarget(0, sp3C, sp30);
}

/// Provide an inactive player-mode update callback; has no effect.
/// \param arg0 Unused callback argument.
/// \param arg1 Unused callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80111778 (0x80111778).
void playerUnusedModeUpdate(u8 *arg0, u8 *arg1) {
}

s32 itemCanBePickedUp(u8 *, s32);
/// Select the player interaction target and action, and determine its interaction-marker style.
/// \param player Player record receiving target, reachability, and action selection.
/// \param object Live candidate with valid per-kind state, or null to clear the selection.
/// \param flags Target-query status flags; bit 0x2 indicates the target is within interaction reach.
/// \returns The interaction-marker style identifier, or zero when the marker should be hidden.
/// \note Original decompilation identifier: \c func_80111784 (0x80111784).
s32 playerClassifyInteractionTarget(u8 *player, u8 *object, s32 flags) {
    s32 kind;
    u8 *state;
    *(s32 *)(player + 0x60) = flags;
    *(s32 *)(player + 0x98) = 6;
    *(s32 *)(player + 0x5C) = 0;
    if (object == NULL || !(*(s32 *)(player + 0x24) & 0xC00))
        return 0;
    *(s32 *)(player + 0x5C) = *(s32 *)(object + 0x38);
    if (playerGetChaseStage() <= 0)
        return 2;
    if (*(s32 *)(object + 0x38) == *(s32 *)(player + 0x84)) {
        if (*(s16 *)(*(u8 **)(object + 0x54) + 4) != 0x8A)
            *(s32 *)(player + 0x98) = 5;
        return 0;
    }
    kind = *(u16 *)(object + 0x34);
    if (kind == 0x25)
        return 2;
    if (kind == 0xB0 || kind == 0x8F)
        return 2;
    if (!(flags & 2) || !(*(s32 *)(player + 0x24) & 0x400)) {
        if (kind == 0x87)
            return 9;
        return 2;
    }
    if (kind > 0 && kind < 0x26) {
        state = *(u8 **)(object + 0x54);
        if (*(u16 *)(state + 0x1E8) & 0x10) {
            *(s32 *)(player + 0x98) = 2;
            return 4;
        }
        return 2;
    }
    if (kind == 0x89) {
        *(s32 *)(player + 0x98) = 8;
        return 4;
    }
    if (kind == 0x86) {
        if (itemAllowsCooperation(object)) {
            *(s32 *)(player + 0x98) = 2;
            return 4;
        }
        if (!itemCanBePickedUp(object, 1))
            return 2;
        *(s32 *)(player + 0x98) = 1;
        return 3;
    }
    if (kind == 0x87) {
        state = *(u8 **)(object + 0x54);
        if (*(s32 *)(state + 0xC))
            return 5;
        *(s32 *)(player + 0x98) = 7;
        if (*(s32 *)state == 2 || *(s32 *)state == 3) {
            switch (*(s32 *)(state + 4)) {
            case 0:
                return 6;
            case 1:
                return 7;
            case 2:
                return 8;
            }
        } else
            return 4;
    }
    kind = *(u16 *)(object + 0x34);
    if (kind == 0x8A) {
        u8 *resultState = *(u8 **)(object + 0x54);
        *(s32 *)(player + 0x98) = 9;
        return *(s32 *)(resultState + 4);
    }
    return 4;
}


/// Assign the controller used by the primary player.
/// \param port Controller port from zero through three; other values supply neutral input.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801119F4 (0x801119F4).
void playerSetControllerPort(s32 port) {
    ((PlayerControlPrefix *)((u8 *)&gPlayerEyeHeight - 0x1C))->controllerPort = port;
}

void playerApplyControlScheme(); /* extern */
/// Separate symbol view of the primary player record's controller port.
/// \note Original identifier: \c D_800EF6CC (0x800EF6CC).
extern s32 gPlayerControllerPort;

/// Select the primary player control scheme and its controller assignment.
/// \param scheme Control-scheme identifier from zero through twenty-seven.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80111A00 (0x80111A00).
void playerSelectControlScheme(s32 scheme) {
    playerApplyControlScheme(scheme);
    gPlayerControllerPort = gPlayerControlSchemes[scheme].port;
}

void cameraFadeOverlayToColor(s32, s32, s32, s32, s32, f32); /* extern */

/// Start a permitted timed player stun reaction with screen-flash and rumble feedback.
/// \param player Primary player record providing stun permission and receiving the reaction.
/// \param duration Requested stun duration in seconds.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80111A40 (0x80111A40).
void playerBeginStun(u8 *player, f32 duration) {
    if (((PlayerControlPrefix *)player)->permissions & 0x20000000) {
        ((PlayerControlPrefix *)player)->stunRemaining = duration;
        playerChangeMode(3);
        cameraFadeOverlayToColor(0, 0xFF, 0xFF, 0xFF, 0x96, 0.6f);
        controllerRequestRumblePattern(0, duration, 0.2f, 0.18f);
    }
}

typedef struct {
    s32 w[64];
} Blob100;


/// Save a temporary snapshot of the primary player state for later restoration.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80111AC4 (0x80111AC4).
void playerSaveState(void) {
    *(Blob100 *)&gSavedPlayerState = *(Blob100 *)&gPlayerState;
}

void playerUpdateViewCamera(); /* extern */
/// Restore the saved primary player snapshot and refresh the view camera when player camera updates are
/// enabled.
/// \returns Nothing.
///
/// \note Requires a previous snapshot whose object handles and callback references are still valid.
/// \note Original decompilation identifier: \c func_80111B08 (0x80111B08).
void playerRestoreState(void) {
    *(Blob100 *)&gPlayerState = *(Blob100 *)&gSavedPlayerState;
    playerUpdateViewCamera(&gPlayerState);
}

void cameraSetPositionAndAngles(s32, f32, f32, f32, f32, f32, f32); /* extern */

/// Update the primary view camera from a player pose when player camera updates are enabled.
/// \param player Player record providing world position, eye height, and view orientation.
/// \returns Nothing.
///
/// \note Portability: the K&R definition permits the existing unprototyped call with an extra argument, which
/// the original MIPS ABI ignores.
/// \note Original decompilation identifier: \c func_80111B68 (0x80111B68).
void playerUpdateViewCamera(player) u8 *player;
{
    if (!(((PlayerControlPrefix *)player)->permissions & 0x4000)) {
        cameraSetPositionAndAngles(
            0, ((PlayerControlPrefix *)player)->position[0],
            ((PlayerControlPrefix *)player)->eyeHeight + ((PlayerControlPrefix *)player)->position[1],
            ((PlayerControlPrefix *)player)->position[2],
            ((PlayerControlPrefix *)player)->pitchOffset + ((PlayerControlPrefix *)player)->pitch,
            ((PlayerControlPrefix *)player)->yaw + ((PlayerControlPrefix *)player)->viewShake, 0.0f);
    }
}


/// Start a sound effect and remember its playback for later stopping.
/// \param player Player record receiving the new playback handle.
/// \param soundId Valid sound-effect identifier to play.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80111BE4 (0x80111BE4).
void playerPlayTrackedSoundEffect(u8 *player, s32 soundId) {
    ((PlayerControlPrefix *)player)->soundHandle = audioPlaySoundEffect(soundId);
}

s32 audioStopPlayback(s32, f64); /* extern */

/// Stop the player tracked sound effect immediately and clear its playback selection.
/// \param player Player record whose tracked playback should stop; no effect if none is selected.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80111C10 (0x80111C10).
void playerStopTrackedSoundEffect(u8 *player) {
    if (((PlayerControlPrefix *)player)->soundHandle != 0) {
        audioStopPlayback(((PlayerControlPrefix *)player)->soundHandle, 0.0);
        ((PlayerControlPrefix *)player)->soundHandle = 0;
    }
}

typedef struct {
    f32 x, y, z;
} MazeDims1C4C;
typedef struct {
    f32 x, y, z;
} Origin1C4C;
f32 vector3LengthApprox(u8 *);
/// Position the player control hierarchy from the current player view, terrain, and distance to Pikachu.
/// \param object Player-control hierarchy root with its required child objects; the active Pikachu actor must
/// be available.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80111C4C (0x80111C4C).
void playerUpdateControlHierarchy(void *object) {
    Origin1C4C *origin;
    f32 projected[3];
    f32 delta[3];
    f32 yaw;
    f32 distance;
    f32 cosine;
    void *grandchild;
    f32 sine;
    f32 reach;
    f32 hitZ;
    f32 hitY;
    f32 hitX;
    u8 *surface;
    void *child;

    origin = (Origin1C4C *)&gPlayerState;
    yaw = ((PlayerControlPrefix *)&gPlayerState)->yaw + ((PlayerControlPrefix *)&gPlayerState)->viewShake;
    delta[0] = ((Origin1C4C *)&gPlayerState)->x - ((RuntimeObjectPrefix *)gPikachuActor)->worldPosition[0];
    delta[1] = ((Origin1C4C *)&gPlayerState)->y - ((RuntimeObjectPrefix *)gPikachuActor)->worldPosition[1];
    delta[2] = ((Origin1C4C *)&gPlayerState)->z - ((RuntimeObjectPrefix *)gPikachuActor)->worldPosition[2];
    distance = vector3LengthApprox((u8 *)&delta[0]);
    sine = mathSinDegrees(yaw);
    cosine = mathCosDegrees(yaw);
    reach = ((MazeDims1C4C *)gCurrentAreaConfiguration)->x;
    if ((reach * 1.5f) < distance) {
        projected[0] = origin->x + (sine * reach);
        projected[1] = origin->y;
        projected[2] = origin->z + (cosine * ((MazeDims1C4C *)gCurrentAreaConfiguration)->z);
    } else {
        projected[0] = origin->x + (sine * distance * 0.6666667f);
        projected[1] = origin->y;
        projected[2] = origin->z + (cosine * distance * 0.6666667f);
    }
    child = ((RuntimeObjectPrefix *)object)->sibling.pointer;
    ((RuntimeObjectPrefix *)object)->position[0] = projected[0];
    ((RuntimeObjectPrefix *)object)->position[1] = projected[1];
    ((RuntimeObjectPrefix *)object)->position[2] = projected[2];
    ((RuntimeObjectPrefix *)object)->worldPosition[0] = projected[0];
    ((RuntimeObjectPrefix *)object)->worldPosition[1] = projected[1];
    ((RuntimeObjectPrefix *)object)->worldPosition[2] = projected[2];
    grandchild = ((RuntimeObjectPrefix *)child)->sibling.pointer;
    ((RuntimeObjectPrefix *)child)->position[0] = (f32)origin->x;
    ((RuntimeObjectPrefix *)child)->position[1] = (f32)origin->y;
    ((RuntimeObjectPrefix *)child)->position[2] = (f32)origin->z;
    ((RuntimeObjectPrefix *)child)->worldPosition[0] = (f32)origin->x;
    ((RuntimeObjectPrefix *)child)->worldPosition[1] = (f32)origin->y;
    ((RuntimeObjectPrefix *)child)->worldPosition[2] = (f32)origin->z;
    surface = NULL;
    hitX = sine * 20.0f;
    hitY = 0.0f;
    hitZ = cosine * 20.0f;
    terrainConstrainMovement(1, origin->x, origin->y + 0.03f, origin->z, *(f32 *)((u8 *)origin + 0x20),
                  *(f32 *)((u8 *)origin + 0x1C), 5.5f, &hitX, &hitY, &hitZ, &surface);
    hitX += origin->x;
    hitY += origin->y;
    hitZ += origin->z;
    hitY = terrainGetWorldHeightForQuery(1, hitX, hitY, hitZ, &surface);
    ((RuntimeObjectPrefix *)grandchild)->position[0] = hitX;
    ((RuntimeObjectPrefix *)grandchild)->position[1] = hitY;
    ((RuntimeObjectPrefix *)grandchild)->position[2] = hitZ;
    ((RuntimeObjectPrefix *)grandchild)->worldPosition[0] = ((RuntimeObjectPrefix *)grandchild)->position[0];
    ((RuntimeObjectPrefix *)grandchild)->worldPosition[1] = ((RuntimeObjectPrefix *)grandchild)->position[1];
    ((RuntimeObjectPrefix *)grandchild)->worldPosition[2] = ((RuntimeObjectPrefix *)grandchild)->position[2];
}

typedef struct {
    f32 x, y, z, yaw;
    u8 remainder[0x10];
} Spawn11F40;
void playerInitializeCollisionQueryMode(s32);
void audioApplySavedMusicVolume(f32);
void audioSetMusicDuckingInhibitFlags(s32);
/// Entrance index used to initialize the player and companion in the current area.
/// \note Original identifier: \c D_800AE4F4 (0x800AE4F4).
extern s32 gRuntimeCurrentEntrance;

/// Initialize the primary player at the selected area spawn with its control objects and initial camera and
/// audio settings.
/// \returns Nothing.
///
/// \note Requires a loaded area configuration, initialized maze and object system, and a valid selected
/// spawn. Existing player-control hierarchies must be released before reinitialization.
/// \note Original decompilation identifier: \c func_80111F40 (0x80111F40).
void playerInitializeForArea(void) {
    u8 *state;
    Spawn11F40 *spawns;
    u8 *model;
    s32 index;

    playerInitializeCollisionQueryMode(1);
    playerSetInteractionHelpText((u8 *)"");
    for (index = 0; index != 1; index++) {
        state = (u8 *)&gPlayerState + index * 0x100;
        spawns = *(Spawn11F40 **)(gCurrentAreaConfiguration + 0x20);
        *(f32 *)(state + 0) = spawns[gRuntimeCurrentEntrance].x;
        *(f32 *)(state + 8) = spawns[gRuntimeCurrentEntrance].z;
        *(f32 *)(state + 4) = terrainGetWorldHeightForQuery(gPlayerCollisionQueryMode, *(f32 *)(state + 0),
                                                            spawns[gRuntimeCurrentEntrance].y, *(f32 *)(state + 8), 0);
        *(f32 *)(state + 0x1C) = 6.0f;
        *(f32 *)(state + 0xC) = spawns[gRuntimeCurrentEntrance].yaw;
        *(f32 *)(state + 0x10) = 0.0f;
        *(f32 *)(state + 0x14) = 5.0f;
        *(f32 *)(state + 0x18) = 0.0f;
        playerChangeMode(1);
        playerSetControlPermissions(state, 0xF0003F2F);
        *(s32 *)(state + 0x2C) = index;
        *(u16 *)(state + 0x30) = 0xFFFF;
        *(u16 *)(state + 0x32) = 0;
        *(s32 *)(state + 0x4C) = 0;
        *(s32 *)(state + 0x50) = 0;
        *(s32 *)(state + 0x64) = 0;
        *(f32 *)(state + 0x68) = 0.0f;
        *(f32 *)(state + 0x20) = 4.5f;
        *(s32 *)(state + 0x84) = 0;
        *(s32 *)(state + 0x78) = 0;
        *(s32 *)(state + 0x80) = 0;
        model = playerCreateControlHierarchy();
        *(s32 *)(state + 0x88) = *(s32 *)(model + 0x38);
        *(s32 *)(state + 0x8C) = *(s32 *)(*(u8 **)(model + 0x28) + 0x38);
        *(s32 *)(state + 0x90) = *(s32 *)(*(u8 **)(*(u8 **)(model + 0x28) + 0x28) + 0x38);
        *(s32 *)(state + 0x98) = 6;
    }
    playerUpdateViewCamera((u8 *)&gPlayerState);
    playerChangeMode(0);
    gGameplayMusicDucked = 0;
    audioApplySavedMusicVolume(1.0f);
    audioSetMusicDuckingInhibitFlags(3);
}

/// Gradually return the player view toward its default walking orientation.
/// \param state Player record whose view pitch and horizontal view offset are recentered.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80112138 (0x80112138).
void playerRecenterWalkingView(u8 *state) {
    f32 scale = 2.0f;

    if (*(f32 *)(state + 0x18) > 0.0f) {
        *(f32 *)(state + 0x18) = *(f32 *)(state + 0x18) - (60.0f * gRuntimeFrameSeconds * scale);
        if (*(f32 *)(state + 0x18) < 0.0f) {
            *(f32 *)(state + 0x18) = 0.0f;
        }
    } else if (*(f32 *)(state + 0x18) < 0.0f) {
        *(f32 *)(state + 0x18) = *(f32 *)(state + 0x18) + (60.0f * gRuntimeFrameSeconds * scale);
        if (*(f32 *)(state + 0x18) > 0.0f) {
            *(f32 *)(state + 0x18) = 0.0f;
        }
    }

    if (*(f32 *)(state + 0x14) > 5.0f) {
        *(f32 *)(state + 0x14) = *(f32 *)(state + 0x14) - (60.0f * gRuntimeFrameSeconds * scale);
        if (*(f32 *)(state + 0x14) < 5.0f) {
            *(f32 *)(state + 0x14) = 5.0f;
        }
    } else if (*(f32 *)(state + 0x14) < 5.0f) {
        *(f32 *)(state + 0x14) = *(f32 *)(state + 0x14) + (60.0f * gRuntimeFrameSeconds * scale);
        if (*(f32 *)(state + 0x14) > 5.0f) {
            *(f32 *)(state + 0x14) = 5.0f;
        }
    }
}

/// Select a permitted companion-control mode and its target, optionally showing a status message.
/// \param player Player record receiving the selection.
/// \param mode Zero for off, one for Pikachu mode, or two for follow mode; unavailable modes leave the
/// selection unchanged.
/// \param showMessage Nonzero to show the selected mode after a successful change.
/// \param targetHandle Object handle to associate with the companion-control selection.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80112294 (0x80112294).
void playerSetCompanionControlMode(u8 *player, s32 mode, s32 showMessage, s32 targetHandle) {
    /// Status messages indexed by companion-control mode.
    /// \note Original identifier: \c D_80123EA4 (0x80123EA4).
    static char *gCompanionControlModeMessages[3] = {"^4PIKACHU^^Mode^1OFF^^", "^4PIKACHU^^Mode^4ON",
                                                     "^4Follow^^Mode^4ON"};

    if (((mode != COMPANION_CONTROL_PIKACHU) || (((PlayerControlPrefix *)player)->permissions & 0x40000000)) &&
        ((mode != COMPANION_CONTROL_FOLLOW) || (((PlayerControlPrefix *)player)->permissions & 0x80000000))) {
        ((PlayerControlPrefix *)player)->companionTarget = targetHandle;
        ((PlayerControlPrefix *)player)->companionMode = mode;
        if (showMessage != 0) {
            messageBarEnqueue(0, 2.0f, (u8 *)gCompanionControlModeMessages[mode]);
        }
    }
}

/// Read the player companion-control mode.
/// \param player Player record to query.
/// \returns Zero for off, one for Pikachu mode, or two for follow mode.
/// \note Original decompilation identifier: \c func_80112318 (0x80112318).
s32 playerGetCompanionControlMode(u8 *player) {
    return ((PlayerControlPrefix *)player)->companionMode;
}

/// Check the player pitch-control permission.
/// \param player Player record to query.
/// \returns One if pitch control is enabled, or zero otherwise.
/// \note Original decompilation identifier: \c func_80112320 (0x80112320).
s32 playerIsPitchControlEnabled(u8 *player) {
    if (((PlayerControlPrefix *)player)->permissions & 0x8000) {
        return 1;
    }
    return 0;
}

void _bzero(); /* extern */

/// Processed button and stick samples for all controller ports; scalar declarations view the primary held-button word.
/// \note Original identifier: \c D_800AD540 (0x800AD540).
extern ControllerState gControllerInputState[4];

/// Advance a player control update using the assigned controller input.
/// \param idx Player record index zero; a missing controller supplies neutral input.
/// \returns Nothing.
///
/// \note Requires an initialized player record with a callable mode-update handler and a valid active camera.
/// \note Original decompilation identifier: \c func_80112344 (0x80112344).
void playerUpdateInput(s32 idx) {
    u8 *p;
    ControllerState pad;
    s32 port;

    p = (u8 *)&gPlayerState + (idx << 8);
    port = ((PlayerControlPrefix *)p)->controllerPort;
    ((PlayerControlPrefix *)p)->modeElapsed += gRuntimeFrameSeconds;
    if ((port < 0) || (port >= 4)) {
        _bzero(&pad, sizeof(pad));
    } else {
        pad = gControllerInputState[port];
        ((PlayerControlPrefix *)p)->handledButtons =
            playerFilterControllerInput(((PlayerControlPrefix *)p)->keptButtons, &pad);
    }
    ((PlayerControlPrefix *)p)->modeState.update(p, (u8 *)&pad);
    playerUpdateViewCamera(p);
}

s32 strlen();                                /* extern */
/// Borrowed cursor into shared player help text while lines are consumed.
/// \note Original identifier: \c D_80134020 (0x80134020).
extern u8 *gPlayerHelpTextCursor;

/// Prepare interaction help text for display, using the unknown-target message for empty text.
/// \param text Readable null-terminated help text of at most 255 bytes, not overlapping the shared help
/// buffer; copied during this call.
/// \returns Nothing.
///
/// \note Undefined behavior: the unprototyped strlen call supplies no argument and relies on the incoming
/// text pointer remaining in the original MIPS argument register.
/// \note The help buffer spans adjacent linker symbols and relies on their target-specific layout.
/// \note Original decompilation identifier: \c func_80112424 (0x80112424).
void playerSetInteractionHelpText(u8 *text) {
    if (strlen() > 0) {
        stringCopy(gPlayerHelpTextBuffer, text);
    } else {
        stringCopy(gPlayerHelpTextBuffer, (u8 *)"Target Unknown");
    }
    gPlayerHelpTextCursor = gPlayerHelpTextBuffer;
    TRACE("help(%s)\n", text);
}

/// Move the primary player to a requested world position and refresh the view camera when player camera
/// updates are enabled.
/// \param position Three consecutive floating-point world coordinates in X, Y, Z order.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80112480 (0x80112480).
void playerSetWorldPosition(u8 *position) {
    ((PlayerControlPrefix *)&gPlayerState)->downwardVelocity = 0.0f;
    ((PlayerControlPrefix *)&gPlayerState)->position[0] = (f32)((f32 *)position)[0];
    ((PlayerControlPrefix *)&gPlayerState)->position[1] = (f32)((f32 *)position)[1];
    ((PlayerControlPrefix *)&gPlayerState)->position[2] = (f32)((f32 *)position)[2];
    playerUpdateViewCamera(&gPlayerState, position);
    TRACE("playerPassCheckPoint(%d)\n", position);
}


/// Set the primary player view yaw when view-angle changes are enabled.
/// \param yaw Requested yaw in degrees, wrapped to a full turn.
/// \returns Nothing.
///
/// \note Angles must be finite and small enough for full-turn adjustments to change their floating-point
/// value.
/// \note Original decompilation identifier: \c func_801124C8 (0x801124C8).
void playerSetViewYaw(f32 yaw) {
    playerSetViewAngles((u8 *)&gPlayerState, yaw, ((PlayerControlPrefix *)&gPlayerState)->pitch);
}

/// Set the primary player view pitch when view-angle changes are enabled.
/// \param pitch Requested pitch in degrees, interpreted as a signed angle and limited to minus sixty through
/// sixty degrees.
/// \returns Nothing.
///
/// \note Angles must be finite and small enough for full-turn adjustments to change their floating-point
/// value.
/// \note Original decompilation identifier: \c func_801124F4 (0x801124F4).
void playerSetViewPitch(f32 pitch) {
    playerSetViewAngles((u8 *)&gPlayerState, ((PlayerControlPrefix *)&gPlayerState)->yaw, pitch);
}


/// Reset the primary player view orientation and immediately refresh the primary camera.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_80112520 (0x80112520).
void playerResetViewOrientation(void) {
    ((PlayerControlPrefix *)&gPlayerState)->pitch = 0.0f;
    ((PlayerControlPrefix *)&gPlayerState)->viewShake = 0.0f;
    ((PlayerControlPrefix *)&gPlayerState)->yaw = 0.0f;
    ((PlayerControlPrefix *)&gPlayerState)->pitchOffset = 0.0f;
    cameraSetPositionAndAngles(
        0, ((PlayerControlPrefix *)&gPlayerState)->position[0],
        ((PlayerControlPrefix *)&gPlayerState)->eyeHeight + ((PlayerControlPrefix *)&gPlayerState)->position[1],
        ((PlayerControlPrefix *)&gPlayerState)->position[2],
        ((PlayerControlPrefix *)&gPlayerState)->pitchOffset + ((PlayerControlPrefix *)&gPlayerState)->pitch,
        ((PlayerControlPrefix *)&gPlayerState)->yaw + ((PlayerControlPrefix *)&gPlayerState)->viewShake, 0.0f);
}

/// Saved progress flag identifiers indexed by friendship milestone.
/// \note Original identifier: \c D_80123EB0 (0x80123EB0).
s32 gFriendshipMilestoneProgressFlags[20] = {0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9,
                                             0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3};

/// Find the saved progress flag for a friendship milestone.
/// \param milestone Friendship milestone index from zero through nineteen.
/// \returns The progress flag identifier for the milestone.
/// \note Original decompilation identifier: \c func_80112598 (0x80112598).
s32 playerGetFriendshipMilestoneFlag(s32 milestone) {
    return gFriendshipMilestoneProgressFlags[milestone];
}

s32 progressSetValue(s32, s32); /* extern */
/// Award an unearned friendship milestone and advance the player friendship level when appropriate.
/// \param player Player record receiving the friendship award.
/// \param milestone Friendship milestone index from zero through nineteen; zero records the initial meeting
/// without increasing the friendship level.
/// \returns The awarded progress flag identifier, or minus one if the milestone was already earned.
/// \note Original decompilation identifier: \c func_801125AC (0x801125AC).
s32 playerAwardFriendshipMilestone(PlayerControlPrefix *player, s32 milestone) {
    s32 sp1C;
    s32 milestoneFlag;

    milestoneFlag = playerGetFriendshipMilestoneFlag(milestone);
    sp1C = milestoneFlag;
    if (progressGetValue(milestoneFlag) != 0) {
        return -1;
    }
    progressSetValue(sp1C, 1);
    TRACE("level %d -> %d\n", player->friendshipMilestone, milestone);
    if (milestone != 0) {
        player->friendshipLevel = (u8)(player->friendshipLevel + 1);
    }
    player->friendshipMilestone = milestone;
    return sp1C;
}

/// Look up the display title for the player friendship level.
/// \param player Player record with a friendship level from zero through nineteen.
/// \returns The persistent friendship title string, including display line breaks.
/// \note Original decompilation identifier: \c func_80112620 (0x80112620).
const char *playerGetFriendshipTitle(const PlayerControlPrefix *player) {
    char *titles[20] = {
        "You've\n      just met",        "Getting to\nknow PIKACHU",       "PIKACHU's\n        playmate",
        "PIKACHU's\n      companion",    "PIKACHU's\n          friend",    "PIKACHU's\n    roommate",
        "PIKACHU's\n            chum",   "PIKACHU's\n        sidekick",    "PIKACHU's\n       good pal",
        "PIKACHU's\n    best buddy",     "You're one\n    of the gang",    "PIKACHU's\n         cohort",
        "PIKACHU\n    trusts you",       "Tried and true\n    friends",    "You share\n    all secrets",
        "You're like\n          family", "Telepathically\n    connected!", "Nothing comes\n   between you",
        "Friends\n        forever!",     "Pi-pika\n  pika-Pikachu",
    };

    return titles[player->friendshipLevel];
}


/// Select the default player terrain-collision query mode.
/// \param unused Unused initialization argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80112684 (0x80112684).
void playerInitializeCollisionQueryMode(s32 unused) {
    gPlayerCollisionQueryMode = 1;
}

/// Camera storage used for view transforms, viewports and color overlays.
/// \note Original identifier: \c D_800ABCF0 (0x800ABCF0).
extern CameraPosePrefix gRenderCameras;

/// Configure a smooth primary-camera transition from the current view for the camera-transition mode.
/// \param eye Readable destination camera eye position as world X, Y, Z coordinates; copied during this call.
/// \param target Readable destination look-at point as world X, Y, Z coordinates; copied during this call.
/// \param duration Positive finite requested transition duration in seconds, representable as a signed
/// integer.
/// \returns Nothing.
///
/// \note Original decompilation identifier: \c func_80112698 (0x80112698).
void playerPrepareCameraTransition(u8 *eye, u8 *target, f32 duration) {
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraStartEye[0] =
        (f32)((CameraPosePrefix *)&gRenderCameras)->eye[0];
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraStartEye[1] =
        (f32)((CameraPosePrefix *)&gRenderCameras)->eye[1];
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraStartEye[2] =
        (f32)((CameraPosePrefix *)&gRenderCameras)->eye[2];
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraStartTarget[0] =
        (f32)((CameraPosePrefix *)&gRenderCameras)->target[0];
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraStartTarget[1] =
        (f32)((CameraPosePrefix *)&gRenderCameras)->target[1];
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraStartTarget[2] =
        (f32)((CameraPosePrefix *)&gRenderCameras)->target[2];
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraEndEye[0] = (f32)((f32 *)eye)[0];
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraEndEye[1] = (f32)((f32 *)eye)[1];
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraEndEye[2] = (f32)((f32 *)eye)[2];
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraEndTarget[0] = (f32)((f32 *)target)[0];
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraEndTarget[1] = (f32)((f32 *)target)[1];
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraEndTarget[2] = (f32)((f32 *)target)[2];
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraDuration = duration;
    ((PlayerControlPrefix *)&gPlayerState)->modeState.cameraRemaining =
        (f32)((PlayerControlPrefix *)&gPlayerState)->modeState.cameraDuration;
}

s32 itemTryRelease(u8 *, s32);  /* extern */
void itemSetMode(u8 *, s32); /* extern */
/// Separate symbol view of the primary player record's held item handle.
/// \note Original identifier: \c D_800EF724 (0x800EF724).
extern s32 gPlayerHeldItemHandle; /* the held item's id */

/// Take an eligible item into the primary player hand, releasing the previously held item.
/// \param id Available item handle to pick up.
/// \returns One if the item was picked up, or zero if it is unavailable or cannot be picked up.
/// \note Original decompilation identifier: \c func_8011271C (0x8011271C).
s32 playerPickUpItem(s32 id) {
    u8 *obj;
    u8 *held;
    s32 heldId;
    u8 *rec;

    obj = objectResolveAvailableHandle(id);
    TRACE("grab objID:%d\n", id);
    if ((obj == NULL) || (itemCanBePickedUp(obj, 1) == 0)) {
        return 0;
    }
    heldId = gPlayerHeldItemHandle;
    held = (u8 *)objectFindLiveByHandle(heldId);
    if (held != NULL) {
        itemTryRelease(held, 1);
    }
    rec = ((RuntimeObjectPrefix *)obj)->state.bytes;
    itemTryRelease(obj, 1);
    gPlayerHeldItemHandle = ((RuntimeObjectPrefix *)obj)->handle;
    ((ItemStatePrefix *)rec)->flags |= 0x42000000;
    ((ItemStatePrefix *)rec)->flags &= 0xE3FFFFFF;
    ((ItemStatePrefix *)rec)->state = 1;
    ((ItemStatePrefix *)rec)->owner = 0;
    itemSetMode(obj, 3);
    return 1;
}

/// Separate symbol view of the primary player current chase stage.
/// \note Original identifier: \c D_800EF797 (0x800EF797).
extern u8 gPlayerChaseStage;

/// Select the current chase stage and retain the furthest stage reached in saved progress.
/// \param stage Requested chase stage; its low eight bits become the current stage, while saved furthest
/// progress uses the full value.
/// \returns Nothing.
///
/// \note Portability: the unprototyped progress lookup passes an extra argument ignored by the original ABI.
/// \note Original decompilation identifier: \c func_801127DC (0x801127DC).
void playerSetChaseStage(s32 stage) {
    gPlayerChaseStage = stage;
    if (stage >= progressGetValue(0xC, stage)) {
        progressSetValue(0xC, stage);
    }
}


/// Read the currently selected chase stage.
/// \returns The current chase stage.
/// \note Original decompilation identifier: \c func_80112824 (0x80112824).
u8 playerGetChaseStage(void) {
    return gPlayerChaseStage;
}


/// Check whether story progress and the primary player input permissions allow companion-control toggling.
/// \returns One if toggling is available, or zero otherwise.
/// \note Original decompilation identifier: \c func_80112830 (0x80112830).
s32 playerCanToggleCompanionControl(void) {
    if ((((PlayerControlPrefix *)((u8 *)&gPlayerEyeHeight - 0x1C))->keptButtons & 0x108) &&
        (progressGetCurrentChapter() >= 6) && (playerGetChaseStage() > 0)) {
        return 1;
    }
    return 0;
}


/// Check whether story progress and the primary player input permissions allow stationary look control.
/// \returns One if stationary look control is available, or zero otherwise.
/// \note Original decompilation identifier: \c func_8011288C (0x8011288C).
s32 playerCanEnterStationaryLook(void) {
    if ((((PlayerControlPrefix *)((u8 *)&gPlayerEyeHeight - 0x1C))->keptButtons & 0x404) &&
        (progressGetCurrentChapter() >= 6) && (playerGetChaseStage() > 0)) {
        return 1;
    }
    return 0;
}

void _bzero(void *, s32); /* extern */
/// Global game heap header used by runtime resource and object allocations.
/// \note Original identifier: \c D_800F2960 (0x800F2960).
extern HpHdr gGameHeap;

/// Initialize a descriptive interaction proxy with default interaction values and placeholder help text.
/// \param object New interaction-proxy object without owned state, receiving its initial state.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801128E8 (0x801128E8).
void interactionProxyInitialize(u8 *object) {
    u8 *sp1C;
    u8 *allocation;

    allocation = heapAllocLocal(&gGameHeap, 0x10, 0x1011);
    sp1C = allocation;
    _bzero(allocation, 0x10);
    ((InteractionProxyState *)sp1C)->description.text = "\xA1\xA9";
    ((RuntimeObjectPrefix *)object)->state.bytes = sp1C;
}

u8 *objectCreateHierarchyFromTemplate(void *); /* extern */
/// Unit-scale object template for descriptive interaction proxies of kind 0x8A.
/// \note Original identifier: \c D_80123F50 (0x80123F50).
struct {
    /* 0x00 */ f32 scale[3];
    /* 0x0C */ u8 unknownC[0x28];
    /* 0x34 */ u16 kind;
    /* 0x36 */ u8 unknown36[0xA];
} gInteractionProxyObjectTemplate = {{1.0f, 1.0f, 1.0f}, {0}, 0x8A};

/// Create and register a descriptive interaction target at a world position.
/// \param position Three consecutive floating-point world coordinates in X, Y, Z order.
/// \param context Caller-defined context value associated with the proxy.
/// \param interactionResult Result code supplied to player target classification when the proxy is selected.
/// \param metadata Caller-defined additional proxy metadata.
/// \param description Borrowed null-terminated help text address, represented as an integer; must remain
/// valid throughout the proxy lifetime and fit the interaction-description buffer.
/// \returns Registered interaction proxy managed by the object system.
///
/// \note Portability: the description address is stored as a 32-bit integer.
/// \note Original decompilation identifier: \c func_80112940 (0x80112940).
u8 *interactionProxyCreate(u8 *position, s32 context, s32 interactionResult, s32 metadata, s32 description) {
    u8 *sp24;
    u8 *proxyState;

    sp24 = objectCreateHierarchyFromTemplate((u8 *)&gInteractionProxyObjectTemplate);
    proxyState = ((RuntimeObjectPrefix *)sp24)->state.bytes;
    ((RuntimeObjectPrefix *)sp24)->position[0] = (f32)((f32 *)position)[0];
    ((RuntimeObjectPrefix *)sp24)->position[1] = (f32)((f32 *)position)[1];
    ((RuntimeObjectPrefix *)sp24)->position[2] = (f32)((f32 *)position)[2];
    ((RuntimeObjectPrefix *)sp24)->worldPosition[0] = (f32)((f32 *)position)[0];
    ((RuntimeObjectPrefix *)sp24)->worldPosition[1] = (f32)((f32 *)position)[1];
    ((RuntimeObjectPrefix *)sp24)->worldPosition[2] = (f32)((f32 *)position)[2];
    ((InteractionProxyState *)proxyState)->context = context;
    ((InteractionProxyState *)proxyState)->interactionResult = interactionResult;
    ((InteractionProxyState *)proxyState)->metadata = metadata;
    ((InteractionProxyState *)proxyState)->description.address = description;
    objectRegisterActiveHierarchy(sp24);
    return sp24;
}

/// Flags inhibiting entry into or exit from gameplay music ducking.
/// \note Original identifier: \c D_80133F18 (0x80133F18).
extern s32 gGameplayMusicDuckingInhibitFlags;

/// Control which automatic music-volume transitions are inhibited.
/// \param flags Inhibit mask: value one prevents reducing music volume and value two prevents restoring it.
/// Combine them to prevent both; zero allows both transitions.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801129DC (0x801129DC).
void audioSetMusicDuckingInhibitFlags(s32 flags) {
    gGameplayMusicDuckingInhibitFlags = flags;
}

void audioSetCategoryVolume(s32, s32); /* extern */
s32 progressGetMaximum(s32);          /* extern */

/// Apply the saved music-volume setting with a temporary volume multiplier.
/// \param multiplier Finite multiplier of the saved volume; the scaled value must fit a signed integer before
/// output clamping.
/// \returns Nothing.
///
/// \note Requires an initialized saved-volume setting with a positive maximum and a volume conversion whose
/// integer products fit the signed integer range.
/// \note Original decompilation identifier: \c func_801129E8 (0x801129E8).
void audioApplySavedMusicVolume(f32 multiplier) {
    s32 scaledVolume;
    s32 sp18;
    s32 var_a1;

    sp18 = progressGetValue(0xA);
    scaledVolume = (s32)((f32)((s32)(sp18 * 0x7FFF) / progressGetMaximum(0xA)) * multiplier);
    var_a1 = scaledVolume;
    if (scaledVolume < 0) {
        var_a1 = 0;
    } else if (scaledVolume >= 0x8000) {
        var_a1 = 0x7FFF;
    }
    TRACE("setSongVolume(%f) vcur: %d/%d vol:%d\n", multiplier, sp18, progressGetMaximum(0xA), var_a1);
    audioSetCategoryVolume(2, var_a1);
}


/// Apply the saved sound-effect volume setting with a temporary volume multiplier.
/// \param multiplier Finite multiplier of the saved volume; the scaled value must fit a signed integer before
/// output clamping.
/// \returns Nothing.
///
/// \note Requires an initialized saved-volume setting with a positive maximum and a volume conversion whose
/// integer products fit the signed integer range.
/// \note Original decompilation identifier: \c func_80112A98 (0x80112A98).
void audioApplySavedSoundEffectVolume(f32 multiplier) {
    s32 scaledVolume;
    s32 sp18;
    s32 var_a1;

    sp18 = progressGetValue(0xB);
    scaledVolume = (s32)((f32)((s32)(sp18 * 0xFFFF) / progressGetMaximum(0xB)) * multiplier);
    var_a1 = scaledVolume;
    if (scaledVolume < 0) {
        var_a1 = 0;
    } else if (scaledVolume >= 0x10000) {
        var_a1 = 0xFFFF;
    }
    TRACE("setSfxVolume(%f) vcur: %d/%d vol:%d\n", multiplier, sp18, progressGetMaximum(0xB), var_a1);
    audioSetCategoryVolume(1, var_a1);
}

/// Whether gameplay exposes microphone interaction and its HUD indicator.
/// \note Original identifier: \c D_800722D0 (0x800722D0).
extern s32 gGameplayMicrophoneAvailable;
/// Whether gameplay permits recognized voice commands to launch messenger effects.
/// \note Original identifier: \c D_800722D4 (0x800722D4).
extern s32 gGameplayVoiceCommandsAllowed;
/// Current gameplay, menu or transition mode.
/// \note Original identifier: \c D_800AE504 (0x800AE504).
extern s32 gRuntimeGameMode;
/// Update automatic music attenuation for the current gameplay and voice-interaction state, respecting
/// inhibited transitions.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80112B48 (0x80112B48).
void audioUpdateGameplayMusicDucking(void) {
    s32 active = 0;
    switch (gRuntimeGameMode) {
    case 0:
        if ((gPlayerPermissions & 0x08000000) &&
            (gGameplayVoiceCommandsAllowed == 0 || gGameplayMicrophoneAvailable == 0) && gRuntimeCurrentArea != 0x16)
            active = 1;
        break;
    case 3:
        active = 1;
        break;
    }
    if (active != 0) {
        if (gGameplayMusicDucked == 0 && !(gGameplayMusicDuckingInhibitFlags & 1)) {
            gGameplayMusicDucked = 1;
            audioApplySavedMusicVolume(0.75f);
        }
    } else if (gGameplayMusicDucked != 0 && !(gGameplayMusicDuckingInhibitFlags & 2)) {
        gGameplayMusicDucked = 0;
        audioApplySavedMusicVolume(1.0f);
    }
}

/// Adjust the player points balance within its supported limits.
/// \param amount Signed points adjustment, negative to spend points; the sum with the current balance must
/// fit a signed integer before clamping.
/// \returns The updated points balance, limited to zero through 999999.
/// \note Original decompilation identifier: \c func_80112C58 (0x80112C58).
s32 playerAddPoints(s32 amount) {
    s32 result;

    ((PlayerControlPrefix *)&gPlayerState)->points += amount;
    result = ((PlayerControlPrefix *)&gPlayerState)->points;
    if (result < 0) {
        result = 0;
        ((PlayerControlPrefix *)&gPlayerState)->points = result;
    } else if (result >= 1000000) {
        ((PlayerControlPrefix *)&gPlayerState)->points = 999999;
        result = ((PlayerControlPrefix *)&gPlayerState)->points;
    }
    return result;
}

u8 *actorCreateForKind(s32, f32, f32, f32, f32, f32, f32, u8 *, s32); /* extern */
/// Current area scenario index shared by actor scripts, scene population, cards and collection titles.
/// \note Original identifier: \c D_800F0B70 (0x800F0B70).
extern s32 gAreaScenarioIndex;

/// Create the active Pikachu actor at the selected area spawn with the current scenario script.
/// \returns Nothing.
///
/// \note Requires a loaded area configuration, valid spawn and scenario indices, and initialized actor
/// resources. Release any previous active Pikachu before replacing it. Portability: actor and script
/// addresses use 32-bit integer storage.
/// \note Original decompilation identifier: \c func_80112CA0 (0x80112CA0).
void pikachuSpawnForArea(void) {
    s32 kind;
    SceneSpawnPoint *spots;

    spots = ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->spawns;
    kind = (s32)((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->actorScripts[gAreaScenarioIndex];
    gPikachuActor = (s32)actorCreateForKind(
        0x25, spots[gRuntimeCurrentEntrance].actorPosition[0], spots[gRuntimeCurrentEntrance].actorPosition[1],
        spots[gRuntimeCurrentEntrance].actorPosition[2], 0.0f, spots[gRuntimeCurrentEntrance].actorYaw,
        ((SceneConfigurationPrefix *)gCurrentAreaConfiguration)->objectScale, (u8 *)kind, 0);
}

s32 actionGetAttributes(u8 *); /* extern */

/// Constrain a requested actor heading according to the active action facing restrictions.
/// \param object Live actor with initialized action state and facing restrictions.
/// \param angle Requested heading in degrees.
/// \returns The permitted heading in degrees, or the original angle when the active action has no facing
/// restriction.
///
/// \note Angles must be finite and small enough for full-turn adjustments to change their floating-point
/// value.
/// \note Original decompilation identifier: \c func_80112D24 (0x80112D24).
f32 actorConstrainFacingHeading(u8 *object, f32 angle) {
    u8 *state;
    u16 flags;

    state = *(u8 **)(object + 0x54);
    flags = *(u16 *)(state + 0x1EA);
    if (!(actionGetAttributes(object) & 0x10)) {
        return angle;
    } else {
        if (angle < 0.0f) {
            do {
                angle += 360.0f;
            } while (angle < 0.0f);
        }
        if (angle >= 360.0f) {
            do {
                angle -= 360.0f;
            } while (angle >= 360.0f);
        }
        if ((angle >= 0) && (angle < 90.0f)) {
            if (flags & 0x24) {
                angle = 90.0f;
            }
            if (flags & 9) {
                angle = 0;
            }
        } else if ((angle >= 90.0f) && (angle < 180.0f)) {
            if (flags & 0x24) {
                angle = 90.0f;
            }
            if (flags & 9) {
                angle = 180.0f;
            }
        } else if ((angle >= 180.0f) && (angle < 270.0f)) {
            if (flags & 0x24) {
                angle = 270.0f;
            }
            if (flags & 9) {
                angle = 180.0f;
            }
        } else if ((angle >= 270.0f) && (angle <= 360.0f)) {
            if (flags & 0x24) {
                angle = 270.0f;
            }
            if (flags & 9) {
                angle = 0;
            }
        }
    }
    return angle;
}

void actorUpdateScriptVm(s32); /* extern */

/// Advance the active Pikachu script and service area events at its world position; has no effect when no
/// Pikachu actor is selected.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80112F10 (0x80112F10).
void pikachuUpdateScriptAndTriggers(void) {
    if (gPikachuActor != 0) {
        actorUpdateScriptVm(gPikachuActor);
        areaDispatchPositionEvent(gPikachuActor + 0x44, 3);
    }
}
