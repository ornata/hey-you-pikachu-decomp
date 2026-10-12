/// \file
/// Declares item-owned tracking of spatial sound playback handles.

#ifndef ITEM_AUDIO_H
#define ITEM_AUDIO_H

#include "item_types.h"

/// Play a spatial sound and retain its playback handle in item state.
/// \pre With an active camera matrix, the item camera-space position has nonzero length.
/// \param item Live item providing the world-space sound position.
/// \param state Initialized state belonging to item; receives the playback handle.
/// \param soundId Valid sound identifier whose resources remain loaded during playback.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_801132F8 (0x801132F8).
void itemPlayTrackedSound(u8 *item, ItemStatePrefix *state, s32 soundId);

#endif
