/// \file
/// Defines ROM-backed sequence playback, rate controls and asynchronous fades.

#ifndef AUDIO_SEQUENCE_H
#define AUDIO_SEQUENCE_H

#include <ultra64.h>

/// Start a sequence at full volume using an available initialized playback slot.
/// \param data ROM address of sequence data, available throughout playback.
/// \returns Playback handle, or zero if all four sequence slots are busy.
/// \note Original decompilation identifier: \c func_8001FB5C (0x8001FB5C).
s32 audioPlaySequence(s32 data);

/// Start a sequence with a transition from thirty percent to full volume.
/// \param data ROM address of sequence data, available throughout playback.
/// \param time Nonnegative fade duration in seconds; zero selects full volume immediately.
/// \returns Playback handle, or zero when no sequence slot is available.
/// \note Original decompilation identifier: \c func_800200E0 (0x800200E0).
s32 audioPlaySequenceWithFade(s32 data, f64 time);

/// Request a volume fade and stop for a sequence handle.
/// \param id Sequence playback handle; zero or an unknown handle has no effect.
/// \param fadeSeconds Nonnegative fade duration in seconds; zero stops immediately.
/// \returns Zero; a timed fade may remain active after return.
/// \note Original decompilation identifier: \c func_800201B8 (0x800201B8).
s32 audioFadeOutSequence(s32 id, f64 fadeSeconds);

/// Change the rate associated with a retained sequence playback handle.
/// \param id Sequence playback handle; zero selects no sequence.
/// \param rate Positive playback multiplier; one selects the original rate.
/// \returns One if the handle belongs to a sequence slot, otherwise zero.
/// \note Original decompilation identifier: \c func_8001FF84 (0x8001FF84).
s32 audioSetSequenceRate(s32 id, f64 rate);

/// Read the rate associated with a retained sequence playback handle.
/// \param id Sequence playback handle; zero selects no sequence.
/// \returns Playback multiplier, or minus one if no sequence slot holds the handle.
/// \note Original decompilation identifier: \c func_8001FFF4 (0x8001FFF4).
f64 audioGetSequenceRate(s32 id);

#endif
