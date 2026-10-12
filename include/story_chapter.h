/// \file
/// Declares current story-chapter queries and visit-driven progression.

#ifndef STORY_CHAPTER_H
#define STORY_CHAPTER_H

#include "ultra64.h"

/// Gets the current story chapter.
/// \returns The current chapter number.
/// \note Original decompilation identifier: \c osAfterPreNMI (0x8002ACE4).
s32 progressGetCurrentChapter(void);

/// Updates the story chapter after a visit according to completed objectives and unlocks its milestones.
/// \returns The resulting chapter number, which may remain unchanged or return to an earlier chapter.
/// \note Original decompilation identifier: \c func_8002AD9C (0x8002AD9C).
s32 progressUpdateChapterAfterVisit(void);

#endif
