/// \file
/// Declares friendship milestones and display titles stored in player progress.

#ifndef PLAYER_FRIENDSHIP_H
#define PLAYER_FRIENDSHIP_H

#include "player_types.h"

/// Find the saved progress flag for a friendship milestone.
/// \param milestone Friendship milestone index from zero through nineteen.
/// \returns The progress flag identifier for the milestone.
/// \note Original decompilation identifier: \c func_80112598 (0x80112598).
s32 playerGetFriendshipMilestoneFlag(s32 milestone);

/// Award an unearned friendship milestone and advance friendship when appropriate.
/// \param player Initialized player record receiving the award.
/// \param milestone Friendship milestone index from zero through nineteen; zero records the initial meeting without increasing the friendship level.
/// \returns Awarded progress flag identifier, or minus one if the milestone was already earned.
/// \note Original decompilation identifier: \c func_801125AC (0x801125AC).
s32 playerAwardFriendshipMilestone(PlayerControlPrefix *player, s32 milestone);

/// Look up the display title for the player friendship level.
/// \param player Initialized player record with a friendship level from zero through nineteen.
/// \returns Borrowed persistent title string, including display line breaks.
/// \note Original decompilation identifier: \c func_80112620 (0x80112620).
const char *playerGetFriendshipTitle(const PlayerControlPrefix *player);

#endif
