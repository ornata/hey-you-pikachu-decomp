/// \file
/// Defines primary-player profile reset, points adjustments and persistent profile aliases.

#ifndef PLAYER_PROFILE_H
#define PLAYER_PROFILE_H

#include <ultra64.h>

/// Borrowed encoded player name in the primary player record, replaced by profile reset.
/// \note Original identifier: \c D_800EF780 (0x800EF780).
extern u8 gPlayerProfileName[];
/// Separate symbol view of the primary player's points balance.
/// \note Original identifier: \c D_800EF790 (0x800EF790).
extern s32 gPlayerProfilePoints;
/// Separate symbol view of the primary player's friendship-title level.
/// \note Original identifier: \c D_800EF796 (0x800EF796).
extern u8 gPlayerFriendshipLevel;
/// Friendship milestone retained for presentation, or minus one when cleared.
/// \note Original identifier: \c D_800EF798 (0x800EF798).
extern s32 gPlayerFriendshipMilestone;
/// Delivered voice-message count, incremented when a messenger reaches Pikachu.
/// \note Original identifier: \c D_800EF79C (0x800EF79C).
extern s32 gPlayerVoiceMessageCount;

/// Reset the primary player profile, script globals, counters and save state to defaults.
/// \returns Nothing; the default player name is "You".
/// \note Original decompilation identifier: \c func_800214E0 (0x800214E0).
void playerResetProfile(void);

/// Adjust the primary player's points balance within its supported limits.
/// \param amount Signed adjustment; its sum with the current balance must fit s32 before clamping.
/// \returns Updated points balance, limited to zero through 999999.
/// \note Original decompilation identifier: \c func_80112C58 (0x80112C58).
s32 playerAddPoints(s32 amount);

#endif
