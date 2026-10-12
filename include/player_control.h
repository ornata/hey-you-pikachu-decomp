/// \file
/// Declares player view permissions and companion-control selection interfaces.

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

#endif
