/// \file
/// Defines door/sign destination state and supported passage-access categories.

#ifndef PASSAGE_TYPES_H
#define PASSAGE_TYPES_H

#include "ultra64.h"

/// Access categories shared by passage interaction and description text.
typedef enum PassageAccessState {
    PASSAGE_ACCESS_ALLOWED = 0,            ///< Passage may be entered.
    PASSAGE_ACCESS_LOCKED = 1,             ///< Passage is locked.
    PASSAGE_ACCESS_UNAVAILABLE = 2,        ///< Passage is not yet available.
    PASSAGE_ACCESS_REQUIRES_PIKACHU = 3,   ///< Passage is waiting for nearby Pikachu.
    PASSAGE_ACCESS_BLOCKED_BY_SQUIRTLE = 4 ///< Squirtle blocks the passage.
} PassageAccessState;

/// Destination and access state owned by a door or sign object.
typedef struct PassageState {
    s32 kind;        ///< Description and destination category, zero through three.
    s32 destination; ///< Destination area or chase-stage identifier, depending on kind.
    s32 entrance;    ///< Entrance selected in the destination area.
    s32 accessState; ///< PassageAccessState stored as a signed target word.
} PassageState;

typedef char PassageStateSizeCheck[(sizeof(PassageState) == 0x10) ? 1 : -1];
typedef char PassageKindOffsetCheck[(((u32) & ((PassageState *)0)->kind) == 0) ? 1 : -1];
typedef char PassageDestinationOffsetCheck[(((u32) & ((PassageState *)0)->destination) == 4) ? 1 : -1];
typedef char PassageEntranceOffsetCheck[(((u32) & ((PassageState *)0)->entrance) == 8) ? 1 : -1];
typedef char PassageAccessStateOffsetCheck[(((u32) & ((PassageState *)0)->accessState) == 0xC) ? 1 : -1];

#endif
