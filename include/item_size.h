/// \file
/// Declares item sizing, bounds refresh and gradual shrinking contracts.

#ifndef ITEM_SIZE_H
#define ITEM_SIZE_H

#include "ultra64.h"

/// Set an item uniform size relative to its kind and area scale, updating its bounds.
/// \param item Initialized item object to resize.
/// \param scale Finite nonnegative size multiplier relative to the normal item size.
/// \returns Nothing.
///
/// \note Requires a loaded area configuration and valid item-kind scale and bounds.
/// \note Original decompilation identifier: \c func_80113348 (0x80113348).
void itemSetScale(u8 *item, f32 scale);

/// Shrink an item toward thirty percent of its normal size.
/// \param item Initialized item object to resize.
/// \param rate Nonnegative reduction in relative size per second.
/// \returns Nothing.
///
/// \note Requires positive area and item-kind scales and a finite nonnegative frame duration.
/// \note Original decompilation identifier: \c func_801133C0 (0x801133C0).
void itemShrink(u8 *item, f32 rate);

/// Enable gradual shrinking of an item during subsequent updates.
/// \param item Initialized item object to shrink.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80113330 (0x80113330).
void itemEnableShrinking(u8 *item);

#endif
