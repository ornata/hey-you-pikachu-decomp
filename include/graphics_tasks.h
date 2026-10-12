/// \file
/// Declares graphics-task initialization and command-list submission to the scheduler.

#ifndef GRAPHICS_TASKS_H
#define GRAPHICS_TASKS_H

#include "scheduler_types.h"

/// Initialize graphics task submission and its framebuffer pair.
/// \pre The game heap can supply the task pool and graphics working buffers.
/// \param schedulerQueue Borrowed graphics submission queue, valid while tasks can be submitted.
/// \param firstFramebuffer Borrowed first framebuffer, valid throughout drawing and presentation.
/// \param secondFramebuffer Borrowed second framebuffer, valid throughout drawing and presentation.
/// \param taskCount Positive reusable-task count whose allocation size fits in unsigned 32 bits.
/// \param outputCapacity Positive output-buffer capacity in 64-bit words whose byte size fits in signed 32 bits.
/// \returns Borrowed pointer to the globally owned task pool.
/// \note Original decompilation identifier: \c func_8003C500 (0x8003C500).
SchedulerTask *graphicsTaskInitialize(OSMesgQueue *schedulerQueue, void *firstFramebuffer, void *secondFramebuffer,
                                      s32 taskCount, s32 outputCapacity);

/// Finish and submit a graphics command list to the scheduler.
/// \pre Submission is initialized and the next reusable task has completed its previous use.
/// \param microcode Valid graphics microcode table index.
/// \param flags Scheduler flags controlling completion and framebuffer presentation.
/// \param start Command-list start, advanced to the next list after submission; command storage remains valid until completion.
/// \param end Write cursor with room for two additional commands, advanced past the list terminator.
/// \param reply Borrowed completion queue, valid until task completion.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_8003C5EC (0x8003C5EC).
void graphicsTaskSubmit(s32 microcode, s32 flags, Gfx **start, Gfx **end, OSMesgQueue *reply);

#endif
