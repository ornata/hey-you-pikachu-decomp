/// \file
/// Declares player view permissions, companion selection and tracked audio interfaces.

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

#endif
