/// \file
/// Declares cue-bank selection and current sequence playback controls.
/// Bank selection and cue lookup share retained reader state; callers must serialize them.

#ifndef AUDIO_CUE_H
#define AUDIO_CUE_H

#include <ultra64.h>

/// Gets the selected audio cue and its requested playback rate.
/// \param cue Receives the selected cue ID, or minus one when no cue is selected.
/// \param rate Receives the requested playback rate multiplier.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_80023130 (0x80023130).
s32 audioGetCurrentCueState(s32 *cue, f64 *rate);

/// Transitions from the current audio cue to a new cue or silence.
/// \param cue Valid cue ID in the selected bank, or a negative value to stop the current cue.
/// \param fadeIn Nonnegative new cue fade-in duration in seconds.
/// \param fadeOut Nonnegative previous cue fade-out duration in seconds.
/// \returns The new playback identifier, or zero for silence or unavailable playback capacity.
/// \note Original decompilation identifier: \c func_80023150 (0x80023150).
s32 audioChangeCurrentCue(s32 cue, f64 fadeIn, f64 fadeOut);

/// Records the requested playback rate and applies it to the current cue when that cue permits rate changes.
/// \param rate Positive playback rate multiplier; one uses the original rate.
/// \returns One if the playback rate was applied, or zero if no playback was updated.
/// \note Original decompilation identifier: \c func_800231F4 (0x800231F4).
s32 audioSetCurrentCueRate(f64 rate);

/// Starts an audio cue at full volume.
/// \param cue Valid cue ID in the initialized current audio cue bank.
/// \returns The playback identifier, or zero if no sequence slot is available.
/// \note Original decompilation identifier: \c func_80023244 (0x80023244).
s32 audioPlayCue(s32 cue);

/// Selects the audio cue bank and prepares its ROM reader.
/// \param bank ROM address of the audio cue bank.
/// \returns Zero.
/// \note Original decompilation identifier: \c func_8002327C (0x8002327C).
s32 audioSelectCueBank(s32 bank);

/// Gets the ROM address of a cue’s sequence data. Requires the cue-bank ROM reader to be initialized.
/// \param base ROM address of the cue bank.
/// \param index Valid cue index within the bank.
/// \returns The ROM address of the selected cue’s sequence data.
/// \note Original decompilation identifier: \c func_800232AC (0x800232AC).
s32 audioGetCueDataRomAddress(s32 base, s32 index);

#endif
