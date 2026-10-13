/// \file
/// Defines game audio initialization and shutdown resource-lifetime contracts.

#ifndef AUDIO_INITIALIZE_H
#define AUDIO_INITIALIZE_H

#include "scheduler_types.h"

/// Initialize game audio playback services and start their worker.
/// \param scheduler Initialized scheduler, valid until audioShutdown completes.
/// \param threadId Operating-system audio worker thread identifier.
/// \param priority Audio worker scheduling priority.
/// \param frequency Requested output frequency in hertz, accepted by osAiSetFrequency.
/// \returns Resident audio heap bytes used, or zero when already initialized.
/// \note Original decompilation identifier: \c func_8001F680 (0x8001F680).
s32 audioInitialize(GameScheduler *scheduler, OSId threadId, OSPri priority, u32 frequency);

/// Stop the audio worker and close the synthesizer before replacing audio resources.
/// Playback handles and borrowed playback state cease to be usable after shutdown.
/// \returns Minus one when audio was shut down, or zero when already inactive.
/// \note Original decompilation identifier: \c func_8001F9DC (0x8001F9DC).
s32 audioShutdown(void);

#endif
