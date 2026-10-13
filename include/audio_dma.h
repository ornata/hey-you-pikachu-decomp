/// \file
/// Defines audio sample-cache initialization, callback creation and read completion.

#ifndef AUDIO_DMA_H
#define AUDIO_DMA_H

#include <ultra64.h>

/// Initialize the sample cache before creating synthesizer DMA callbacks.
/// \param heap Audio heap owning cache buffers and request storage for the cache lifetime.
/// \param slots Positive cache-entry count, at most 255.
/// \param shift Base-two logarithm of the sample page size in bytes, from zero through 24.
/// \returns One.
/// \note Original decompilation identifier: \c func_80049234 (0x80049234).
s32 audioDmaInitialize(ALHeap *heap, s32 slots, s32 shift);

/// Provide the shared sample-cache callback to the synthesizer.
/// \param state Writable callback-state pointer slot, receiving null.
/// \returns Callback supplying physical sample addresses; new reads complete at audioDmaFinishFrame.
/// Requests within a frame must fit the cache capacity, and each sample span must
/// fit its cache page plus configured read-ahead storage.
/// \note Original decompilation identifier: \c func_80049160 (0x80049160).
ALDMAproc audioDmaCreateCallback(void *state);

/// Finish this frame's sample reads before submitting its synthesis task.
/// \returns Nothing; advances the sample-cache frame lifetime.
/// \note Original decompilation identifier: \c func_80049170 (0x80049170).
void audioDmaFinishFrame(void);

#endif
