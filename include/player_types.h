/// \file
/// Defines the recovered player view and companion-control state prefix.

#ifndef PLAYER_TYPES_H
#define PLAYER_TYPES_H

#include <ultra64.h>
#include "controller_types.h"

/// Companion-control selections accepted by the player-control interface.
typedef enum CompanionControlMode {
    COMPANION_CONTROL_OFF = 0,
    COMPANION_CONTROL_PIKACHU = 1,
    COMPANION_CONTROL_FOLLOW = 2
} CompanionControlMode;

/// Advance a player behavior mode for one input update.
/// \param player Borrowed player record whose active mode is being serviced.
/// \param controller Writable filtered input sample, borrowed only for the call.
/// Changes do not alter the global controller cache.
/// \returns Nothing.
typedef void (*PlayerModeUpdate)(u8 *player, ControllerState *controller);

/// Complete a player behavior mode before switching to another mode.
/// \param player Borrowed player record leaving the active mode.
/// \returns Nothing.
typedef void (*PlayerModeLeave)(u8 *player);

/// Retained control hierarchy, interaction selection and camera-transition state.
/// This block begins at offset 0x88 in the player record.
typedef struct PlayerModeState {
    s32 objectHandles[3];      ///< Handles of the three player-control hierarchy objects.
    s32 mode;                  ///< Primary behavior mode selecting control callbacks.
    s32 interactionAction;     ///< Action category for the current pointed object.
    s32 companionHoldRequired; ///< Nonzero when companion mode requires the control button to stay held.
    f32 cameraStartEye[3];     ///< Eye position at the beginning of a camera transition.
    f32 cameraEndEye[3];       ///< Requested eye position at transition completion.
    f32 cameraStartTarget[3];  ///< Look-at point at the beginning of the transition.
    f32 cameraEndTarget[3];    ///< Requested look-at point at completion.
    f32 cameraRemaining;       ///< Remaining camera-transition time.
    f32 cameraDuration;        ///< Total requested camera-transition time.
    PlayerModeUpdate update;  ///< Active mode update; must be callable while the mode is active.
    PlayerModeLeave leave;    ///< Optional callback completing the mode before a transition.
} PlayerModeState;

/// Recovered prefix of the player control record, distinct from scene objects.
/// The observed record stride is 0x100. Offset-named storage remains unresolved;
/// callers must not treat this view as proof of ownership of borrowed resources.
typedef struct PlayerControlPrefix {
    f32 position[3]; ///< Player world position.
    f32 yaw;         ///< Player view yaw in degrees.
    f32 pitchOffset; ///< Additional view pitch used by walking motion.
    f32 pitch; ///< Player view pitch in degrees.
    f32 viewShake; ///< Horizontal view-shake offset added to yaw.
    f32 eyeHeight; ///< View height above the player's position.
    f32 collisionRadius;  ///< Radius supplied to terrain movement constraints.
    s32 permissions;      ///< Current control-permission mask.
    s32 savedPermissions; ///< Permissions retained for normal control.
    s32 controllerPort;   ///< Assigned controller port, or -1 for neutral input.
    u16 keptButtons;      ///< Controller-button mask retained by the active scheme.
    u16 handledButtons;   ///< Filtered or handled button status for the current frame.
    s32 previousStickY;   ///< Previous vertical stick input used to adjust movement speed.
    f32 movementSpeed;    ///< Retained speed selected from controller input.
    f32 focusRemaining;   ///< Time remaining in temporary target-focus mode.
    u8 unknown040[0xC];
    s32 companionMode;        ///< Current CompanionControlMode selection.
    s32 companionTarget;      ///< Object handle selected for companion control.
    s32 savedCompanionMode;   ///< Saved selection for later restoration.
    s32 savedCompanionTarget; ///< Saved companion target handle.
    s32 interactionTarget;    ///< Current interaction target handle.
    s32 interactionFlags;     ///< Target reachability/query status.
    s32 airborne;             ///< Nonzero while the player is above the ground.
    f32 downwardVelocity;     ///< Downward-positive displacement retained by movement updates.
    f32 slopeSpeed;           ///< Retained speed along slippery terrain.
    f32 stunRemaining;        ///< Remaining stun-reaction time.
    f32 modeElapsed;          ///< Time elapsed in the current primary mode.
    s32 movementClass;        ///< Zero through two for walking speeds, or three while airborne.
    f32 footstepDistance;     ///< Accumulated travel since the last footstep sound.
    s32 soundHandle;          ///< Tracked player sound-effect playback.
    s32 heldItemHandle;       ///< Handle of the player's held item, or zero.
    PlayerModeState modeState; ///< Current behavior and camera-transition state.
    u8 unknown0E0[0x10];
    s32 points; ///< Player profile points balance.
    s8 unknown0F4;      ///< Profile byte initialized to six; interpretation remains unresolved.
    s8 unknown0F5;      ///< Profile byte initialized to eight; interpretation remains unresolved.
    u8 friendshipLevel; ///< Index of the player friendship title.
    u8 unknown0F7;
    s32 friendshipMilestone; ///< Last awarded friendship milestone.
    s32 unknown0FC;          ///< Profile word reset to zero; interpretation remains unresolved.
} PlayerControlPrefix;

typedef char PlayerModeStateSizeCheck[(sizeof(PlayerModeState) == 0x58) ? 1 : -1];

typedef char PlayerControlPrefixSizeCheck[(sizeof(PlayerControlPrefix) == 0x100) ? 1 : -1];
#define PLAYER_OFFSET(field) ((u32) & (((PlayerControlPrefix *)0)->field))
typedef char PlayerPermissionsOffsetCheck[(PLAYER_OFFSET(permissions) == 0x24) ? 1 : -1];
typedef char PlayerCompanionModeOffsetCheck[(PLAYER_OFFSET(companionMode) == 0x4C) ? 1 : -1];
typedef char PlayerInteractionTargetOffsetCheck[(PLAYER_OFFSET(interactionTarget) == 0x5C) ? 1 : -1];
typedef char PlayerModeStateOffsetCheck[(PLAYER_OFFSET(modeState) == 0x88) ? 1 : -1];
typedef char PlayerModeCallbackOffsetCheck[(PLAYER_OFFSET(modeState.update) == 0xD8) ? 1 : -1];
typedef char PlayerModeLeaveOffsetCheck[(PLAYER_OFFSET(modeState.leave) == 0xDC) ? 1 : -1];
typedef char PlayerPointsOffsetCheck[(PLAYER_OFFSET(points) == 0xF0) ? 1 : -1];
typedef char PlayerMovementClassOffsetCheck[(PLAYER_OFFSET(movementClass) == 0x78) ? 1 : -1];
typedef char PlayerCameraStartEyeOffsetCheck[(PLAYER_OFFSET(modeState.cameraStartEye) == 0xA0) ? 1 : -1];
typedef char PlayerCameraEndEyeOffsetCheck[(PLAYER_OFFSET(modeState.cameraEndEye) == 0xAC) ? 1 : -1];
typedef char PlayerCameraStartTargetOffsetCheck[(PLAYER_OFFSET(modeState.cameraStartTarget) == 0xB8) ? 1 : -1];
typedef char PlayerCameraEndTargetOffsetCheck[(PLAYER_OFFSET(modeState.cameraEndTarget) == 0xC4) ? 1 : -1];
typedef char PlayerCameraRemainingOffsetCheck[(PLAYER_OFFSET(modeState.cameraRemaining) == 0xD0) ? 1 : -1];
typedef char PlayerCameraDurationOffsetCheck[(PLAYER_OFFSET(modeState.cameraDuration) == 0xD4) ? 1 : -1];
#undef PLAYER_OFFSET

#endif
