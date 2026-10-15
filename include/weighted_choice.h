/// \file
/// Defines mutable weighted-key tables and their random-selection interface.

#ifndef WEIGHTED_CHOICE_H
#define WEIGHTED_CHOICE_H

#include "ultra64.h"

/// One selectable key and its mutable selection weight.
typedef struct WeightedChoiceEntry {
    s32 key;    ///< Nonzero choice key, or zero to terminate the entry array.
    s32 weight; ///< Initial weight; cumulative threshold after table preparation.
} WeightedChoiceEntry;

/// Borrowed writable entries retained by their caller across repeated choices.
/// Tables require at least two choices followed by a zero-key terminator.
typedef struct WeightedChoiceTable {
    WeightedChoiceEntry *entries; ///< Choices and their writable terminator.
    s32 lastChoiceIndex;          ///< Last selectable entry index; zero marks an unprepared table.
} WeightedChoiceTable;

typedef char WeightedChoiceEntrySizeCheck[(sizeof(WeightedChoiceEntry) == 8) ? 1 : -1];
typedef char WeightedChoiceWeightOffsetCheck[(((u32) & ((WeightedChoiceEntry *)0)->weight) == 4) ? 1 : -1];
typedef char WeightedChoiceTableSizeCheck[(sizeof(WeightedChoiceTable) == 8) ? 1 : -1];
typedef char WeightedChoiceIndexOffsetCheck[(((u32) & ((WeightedChoiceTable *)0)->lastChoiceIndex) == 4) ? 1 : -1];

/// Prepares a weighted selection table for repeated random choices.
/// \param table Writable table with nonnegative weights, a positive total fitting s32, and a zero-key terminator; prepared tables remain unchanged.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80032490 (0x80032490).
void weightedChoicePrepareTable(WeightedChoiceTable *table);

/// Chooses a key from a weighted selection table, preparing the table when needed.
/// \param table Writable table satisfying the preparation contract.
/// \returns The selected nonzero key, or zero if the table yields no selection.
/// \note Original decompilation identifier: \c func_800324D8 (0x800324D8).
s32 weightedChoiceSelectKey(WeightedChoiceTable *table);

#endif
