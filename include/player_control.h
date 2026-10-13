/// \file
/// Declares player modes, saved state, view permissions and companion/audio controls.

#ifndef PLAYER_CONTROL_H
#define PLAYER_CONTROL_H

#include <ultra64.h>

/// Set the view orientation when angle changes are permitted.
/// \param player Borrowed initialized player record.
/// \param yaw Finite yaw in degrees, small enough for full-turn adjustments to change its value.
/// \param pitch Finite pitch in degrees, subject to the same magnitude constraint; limited to minus sixty through sixty.
/// \returns Nothing.
void playerSetViewAngles(u8 *player, f32 yaw, f32 pitch);

/// Select a permitted companion-control mode and target.
/// \param player Borrowed initialized player record receiving the selection.
/// \param mode CompanionControlMode value; unavailable selections leave the record unchanged.
/// \param showMessage Nonzero to display the selection after a successful change.
/// \param targetHandle Object-manager handle associated with the selection.
/// \returns Nothing.
void playerSetCompanionControlMode(u8 *player, s32 mode, s32 showMessage, s32 targetHandle);

/// Read a player's companion-control selection.
/// \param player Borrowed initialized player record.
/// \returns Current CompanionControlMode value.
s32 playerGetCompanionControlMode(u8 *player);

/// Query whether player pitch changes are enabled.
/// \param player Borrowed initialized player record.
/// \returns One when pitch changes are enabled, or zero otherwise.
s32 playerIsPitchControlEnabled(u8 *player);

/// Read the primary player's companion-control selection.
/// \returns Current CompanionControlMode value for the primary player.
s32 playerQueryInteractionMode(void);

/// Start a sound effect and retain its playback handle for later stopping.
/// \param player Borrowed initialized player record receiving the playback handle.
/// \param soundId Valid sound-effect identifier whose resources remain loaded during playback.
/// \returns Nothing.
void playerPlayTrackedSoundEffect(u8 *player, s32 soundId);

/// Stop the player's tracked sound effect and clear its selection.
/// \param player Borrowed initialized player record; no effect when no playback is selected.
/// \returns Nothing.
void playerStopTrackedSoundEffect(u8 *player);

/// Start a permitted timed stun reaction with visual and rumble feedback.
/// \param player Primary player record providing permission and receiving stun state.
/// \param duration Nonnegative requested reaction duration in seconds.
/// \returns Nothing; no effect when stun permission is disabled.
/// \note Original decompilation identifier: \c func_80111A40 (0x80111A40).
void playerBeginStun(u8 *player, f32 duration);

/// Change primary player behavior, completing the previous mode and applying new permissions.
/// \param mode Behavior mode identifier from zero through thirty-one.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801101D0 (0x801101D0).
void playerChangeMode(s32 mode);

/// Apply a control scheme's behavior, controller assignment and retained buttons.
/// \param scheme Control-scheme identifier from zero through twenty-seven.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80110288 (0x80110288).
void playerApplyControlScheme(s32 scheme);

/// Select the primary control scheme and controller assignment.
/// \param scheme Control-scheme identifier from zero through twenty-seven.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80111A00 (0x80111A00).
void playerSelectControlScheme(s32 scheme);

/// Return the primary player to normal control using saved permissions.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801102DC (0x801102DC).
void playerResumeNormalControl(void);

/// Set current and saved permissions for a player record.
/// \param player Borrowed initialized player record receiving permissions.
/// \param permissions Control-permission mask retained for subsequent normal control.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010E980 (0x8010E980).
void playerSetControlPermissions(u8 *player, s32 permissions);

/// Replace the single saved snapshot with the primary player's current state.
/// \returns Nothing; borrowed resources referenced by the snapshot remain caller-owned.
/// \note Original decompilation identifier: \c func_80111AC4 (0x80111AC4).
void playerSaveState(void);

/// Restore the saved primary-player snapshot and refresh its camera when permitted.
/// \returns Nothing.
/// \note Requires a prior snapshot whose handles and callback references remain valid.
/// \note Original decompilation identifier: \c func_80111B08 (0x80111B08).
void playerRestoreState(void);

/// Advance primary player input, behavior and view-camera state for this frame.
/// \param idx Primary player record index zero; unavailable controller input is neutral.
/// \returns Nothing.
/// \note Requires an initialized player with a callable mode update and active camera.
/// \note Original decompilation identifier: \c func_80112344 (0x80112344).
void playerUpdateInput(s32 idx);

#endif
