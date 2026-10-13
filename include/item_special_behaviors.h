/// \file
/// Declares held-item effects and released-item collision and flight behavior.

#ifndef ITEM_SPECIAL_BEHAVIORS_H
#define ITEM_SPECIAL_BEHAVIORS_H

#include <ultra64.h>

/// Restore the baseball cap normal size when released by an actor.
/// \param object Item to update.
/// \param unused Unused release callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123060 (0x80123060).
void baseballCapResetReleasedScale(u8 *object, s32 unused);

/// Animate a light bulb held by an actor and consume the primary actor action resource while it glows.
/// \param object Item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123098 (0x80123098).
void lightBulbUpdateHeldGlow(u8 *object);

/// Restore the light bulb unlit appearance when released by an actor.
/// \param object Item to update.
/// \param unused Unused release callback argument.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123140 (0x80123140).
void lightBulbResetReleasedGlow(u8 *object, s32 unused);

/// Inflate an actor-held balloon and update its inflated state.
/// \param object Item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123178 (0x80123178).
void balloonUpdateHeldInflation(u8 *object);

/// Begin upward flight for a sufficiently inflated balloon.
/// \param object Item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123220 (0x80123220).
void balloonBeginReleasedFlight(u8 *object);

/// Resolve item contact with the map and play the appropriate impact effects.
/// \param object Item to update.
/// \param contactFlags Contact flags supplied by the item collision system.
/// \returns One when map contact was processed, or zero when the item was replaced before processing.
/// \note Original decompilation identifier: \c func_80123294 (0x80123294).
s32 itemResolveMapCollision(u8 *object, u16 contactFlags);

/// Initialize spinning flight for a launched flying item.
/// \param object Item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123374 (0x80123374).
void itemInitializeFlightSpin(u8 *object);

#endif
