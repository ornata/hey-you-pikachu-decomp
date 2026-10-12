/// \file
/// Declares story-chapter assignment and its milestone unlock contract.

#ifndef STORY_MILESTONES_H
#define STORY_MILESTONES_H

#include "ultra64.h"

/// Sets the current story chapter and unlocks the milestones available at that chapter.
/// \param chapter Story chapter to store and use for milestone unlocks.
/// \returns Nothing.
///
/// \note Portability: The empty debug range check preserves IDO register allocation; the original bounds are not recoverable.
/// \note Original decompilation identifier: \c func_8002AD04 (0x8002AD04).
void progressSetChapterAndUnlockMilestones(s32 chapter);

#endif
