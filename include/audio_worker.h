/// \file
/// Defines audio rendering worker startup, retained resources and shutdown contracts.

#ifndef AUDIO_WORKER_H
#define AUDIO_WORKER_H

#include "scheduler_types.h"

/// Render synthesis commands and report their count for one stereo sample buffer.
/// \param commands Writable command buffer with the configured capacity.
/// \param count Receives the command count; zero ends playback.
/// \param samples Physical output-buffer address supplied to synthesis commands.
/// \param frames Number of stereo sample frames to render.
/// \returns Command cursor after the generated commands.
typedef Acmd *(*AudioRenderCallback)(Acmd *commands, s32 *count, s16 *samples, s32 frames);

/// Start the shared audio worker after scheduler and synthesizer initialization.
/// Only one worker may be active; retained resources must remain valid until shutdown.
/// \param scheduler Initialized scheduler receiving audio tasks.
/// \param heap Audio heap owning command and output buffers through shutdown.
/// \param id Operating-system thread identifier.
/// \param pri Worker thread priority.
/// \param render Retained synthesis callback; its count must not exceed maxCommands.
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
