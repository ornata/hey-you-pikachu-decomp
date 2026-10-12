/// \file
/// Declares weighted item placement over borrowed scene position arrays.

#ifndef MAZE_SCATTER_H
#define MAZE_SCATTER_H

#include "weighted_choice.h"

/// Scatter weighted item choices across a random selection of candidate positions, optionally adding contact
/// effects at unused positions.
/// \param positions Readable array of count consecutive world positions in X, Y, Z order.
/// \param count Nonnegative number of candidate positions; array size and requested quota must fit in a
/// signed integer.
/// \param table Initialized weighted-choice table containing valid item kinds.
/// \param fraction Fraction of candidate positions to fill, from zero through one; the requested count is
/// rounded down.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8010E768 (0x8010E768).
void mazeScatterItems(f32 *positions, s32 count, WeightedChoiceTable *table, f32 fraction);

#endif
