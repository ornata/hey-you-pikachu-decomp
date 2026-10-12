/// \file
/// Defines weighted resident choices shared by the fishing-scene population tables.

#ifndef FISHING_POPULATION_H
#define FISHING_POPULATION_H

#include <ultra64.h>

/// One resident kind and its cumulative threshold in the scene's random pool.
/// Each scene preserves its own inclusive or exclusive threshold comparison.
typedef struct FishingResidentChoice {
    s32 kind;      ///< Actor kind to spawn when this choice is selected.
    s32 threshold; ///< Cumulative percentage threshold compared with the random roll.
} FishingResidentChoice;

typedef char FishingResidentChoiceSizeCheck[(sizeof(FishingResidentChoice) == 8) ? 1 : -1];

#endif
