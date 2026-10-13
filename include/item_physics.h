/// \file
/// Declares item impact audio and held-cap visibility updates.

#ifndef ITEM_PHYSICS_H
#define ITEM_PHYSICS_H

#include <ultra64.h>

/// Play the surface and item impact sounds appropriate to a contact.
/// \param object Item producing the impact.
/// \param contactFlags Collision flags controlling surface sound playback.
/// \param surface Surface properties used to select the impact sound.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801220B8 (0x801220B8).
void itemPlayImpactSounds(u8 *object, s32 contactFlags, s32 surface);

/// Update baseball-cap visibility for the primary actor held-item appearance.
/// \param object Baseball-cap item to update.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80123000 (0x80123000).
void baseballCapUpdateHeldVisibility(u8 *object);

#endif
