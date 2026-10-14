/// \file
/// Defines audio rendering worker startup, retained resources and shutdown contracts.

#ifndef AUDIO_WORKER_H
#define AUDIO_WORKER_H

#include "scheduler_types.h"

/// Render synthesis commands and report their count for one stereo sample buffer.
/// \param commands Borrowed writable command buffer with the configured capacity.
/// \param count Writable count slot, receiving a value from zero through the configured capacity;
/// zero terminates the audio worker.
/// \param samples Physical address of storage for frames interleaved stereo pairs of signed 16-bit samples.
/// This target address is for synthesis commands and must not be dereferenced as a CPU pointer.
/// \param frames Positive number of stereo sample frames to render.
/// \returns Command cursor after the generated commands.
typedef Acmd *(*AudioRenderCallback)(Acmd *commands, s32 *count, s16 *samples, s32 frames);

/// Start the shared audio worker after scheduler and synthesizer initialization.
/// Only one worker may be active; retained resources must remain valid until shutdown.
/// \param scheduler Initialized scheduler receiving audio tasks.
/// \param heap Borrowed allocator descriptor used during startup; its backing command and output
/// storage must remain allocated until shutdown acknowledgment.
/// \param id Operating-system thread identifier.
/// \param pri Worker thread priority.
/// \param render Non-null synthesis callback retained until shutdown; its count must not exceed maxCommands.
/// \param ucode Retained audio microcode text.
/// \param ucodeData Retained audio microcode data.
/// \param samples Positive stereo sample-frame count per output buffer.
/// \param maxCommands Positive command-buffer capacity in audio commands.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_800496B8 (0x800496B8).
void audioWorkerStart(GameScheduler *scheduler, ALHeap *heap, OSId id, OSPri pri, AudioRenderCallback render,
                      u64 *ucode, u64 *ucodeData, s32 samples, s32 maxCommands);

/// Request shutdown of the started worker and wait for its acknowledgment.
/// Call once per startup; retained rendering resources may then be released.
/// \returns Nothing.
/// \note Original decompilation identifier: \c func_80049680 (0x80049680).
void audioWorkerStopAndWait(void);

#endif
