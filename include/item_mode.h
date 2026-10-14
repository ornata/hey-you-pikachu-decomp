/// \file
/// Declares item behavior-mode changes and their world-participation contract.

#ifndef ITEM_MODE_H
#define ITEM_MODE_H

#include "ultra64.h"

/// Behavior callbacks selected by the signed-byte item mode.
typedef enum ItemMode {
    ITEM_MODE_GROUND_AND_AIR = 0, ///< Terrain-aware free motion.
    ITEM_MODE_DRIFTING_FLIGHT = 1, ///< Timed drifting flight.
    ITEM_MODE_STATIONARY = 2, ///< World item without autonomous motion.
    ITEM_MODE_PLAYER_HELD = 3, ///< Placement relative to the player camera.
    ITEM_MODE_ACTOR_HELD = 4, ///< Placement at the actor holder's attachment.
    ITEM_MODE_STORED = 5, ///< Stored item without autonomous motion.
    ITEM_MODE_SHRINKING_DISPOSAL = 6 ///< Shrink until destruction.
} ItemMode;

/// Advance one live item's active behavior for the current frame.
/// \param object Borrowed live item with initialized state appropriate to its mode.
/// The shrinking-disposal mode may request deferred destruction; borrowed state must not
/// survive the object-system destruction sweep.
/// \returns Nothing.
typedef void (*ItemModeUpdateHandler)(u8 *object);

/// Persistent non-null item behavior callbacks indexed by ItemMode zero through six.
/// \note Original identifier: \c D_8012B950 (0x8012B950).
extern ItemModeUpdateHandler gItemModeUpdateHandlers[];

/// Advance behavior and apply the item's orientation constraints.
/// \param object Live initialized item with a valid ItemMode and available mode-specific resources.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123390 (0x80123390).
void itemUpdateModeAndOrientation(u8 *object);

/// Change an item's behavior mode and apply the mode's participation rules.
/// \param item Initialized item object to configure.
/// \param mode Supported item behavior mode from zero through six.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8011443C (0x8011443C).
void itemSetMode(u8 *item, s32 mode);

#endif
